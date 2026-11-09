

#include "bubble.h"

BBViewport bb_viewport_fit(int framebuffer_width, int framebuffer_height, float pixel_ratio)
{
    BBViewport view = {0};
    view.columns = BB_MAP_WIDTH;
    view.rows = BB_MAP_HEIGHT;
    view.scale = 1.0f;
    if(framebuffer_width <= 0 || framebuffer_height <= 0)
    {
        return view;
    }
    if(pixel_ratio <= 0.0f)
    {
        pixel_ratio = 1.0f;
    }

    float width = (float)framebuffer_width;
    float height = (float)framebuffer_height;

    double width_scale = (double)framebuffer_width / (BB_MAP_WIDTH * 8.0);
    double height_scale = (double)framebuffer_height / (BB_MAP_HEIGHT * 8.0);
    double fit = fmin(width_scale, height_scale) / (double)pixel_ratio;
    double drawing_scale = fit;
    if(fit >= 1.0)
    {

        drawing_scale = floor(fit);
    }
    view.scale = (float)(drawing_scale * (double)pixel_ratio);
    float tile_pixels = 8.0f * view.scale;

    view.columns = (int)fminf(ceilf(width / tile_pixels), BB_WORLD_MAX_WIDTH);
    view.rows = (int)fminf(ceilf(height / tile_pixels), BB_WORLD_MAX_HEIGHT);
    if(view.columns < BB_MAP_WIDTH)
    {
        view.columns = BB_MAP_WIDTH;
    }
    if(view.rows < BB_MAP_HEIGHT)
    {
        view.rows = BB_MAP_HEIGHT;
    }
    view.offset_x = (width - (float)view.columns * tile_pixels) * 0.5f;
    view.offset_y = (height - (float)view.rows * tile_pixels) * 0.5f;
    return view;
}

BBViewport bb_viewport_fit_hud(int framebuffer_width, int framebuffer_height, float pixel_ratio)
{
    int hud = BB_HUD_TOP + BB_HUD_BOTTOM;
    if(framebuffer_width <= 0 || framebuffer_height <= 0)
    {
        return bb_viewport_fit(framebuffer_width, framebuffer_height, pixel_ratio);
    }

    int scaled_height = framebuffer_height * BB_SCREEN_HEIGHT / (BB_SCREEN_HEIGHT + hud);
    BBViewport view = bb_viewport_fit(framebuffer_width, scaled_height, pixel_ratio);
    if(pixel_ratio <= 0.0f)
    {
        pixel_ratio = 1.0f;
    }
    // 정수 배율이 창을 85%도 못 채우면(예: 768x672) 소수 배율로 꽉 채움
    double exact = fmin((double)framebuffer_width / BB_SCREEN_WIDTH,
                        (double)framebuffer_height / (BB_SCREEN_HEIGHT + hud)) / (double)pixel_ratio;
    if(exact >= 1.0 && (double)view.scale / (double)pixel_ratio < exact * 0.85)
    {
        view.scale = (float)(exact * (double)pixel_ratio);
        view.columns = (int)fminf(ceilf((float)framebuffer_width / (8.0f * view.scale) - 0.001f), BB_WORLD_MAX_WIDTH);
        if(view.columns < BB_MAP_WIDTH)
        {
            view.columns = BB_MAP_WIDTH;
        }
        view.offset_x = ((float)framebuffer_width - (float)view.columns * 8.0f * view.scale) * 0.5f;
    }
    float tile_pixels = 8.0f * view.scale;
    float top = (float)BB_HUD_TOP * view.scale;
    float area = (float)framebuffer_height - top - (float)BB_HUD_BOTTOM * view.scale;
    // 소수 배율에서 오차 때문에 한 줄 더 잡히지 않게 조금 뺌
    view.rows = (int)fminf(ceilf(area / tile_pixels - 0.001f), BB_WORLD_MAX_HEIGHT);
    if(view.rows < BB_MAP_HEIGHT)
    {
        view.rows = BB_MAP_HEIGHT;
    }
    view.offset_y = top + (area - (float)view.rows * tile_pixels) * 0.5f;
    return view;
}

int solid_tile_at(const BBGame *game, float x, int y, float *left)
{
    int column = (int)floorf(x);
    int tile = tile_at(game, column, y);
    if(tile == 3 || (tile == 2 && (y >= game->world_height - 2 ||
                                  platform_for_tile(game, column, y) == NULL)))
    {
        *left = (float)column;
        return tile;
    }
    for(int i = 0; i < game->platform_count; ++i)
    {
        const BBMovingPlatform *platform = &game->platforms[i];
        if(platform->y != y || !bb_game_platform_solid(platform))
            continue;
        float platform_left = (float)platform->x + platform->offset;
        if(x >= platform_left && x < platform_left + (float)platform->width)
        {
            *left = platform_left + floorf(x - platform_left);
            return 2;
        }
    }
    return 0;
}

bool rects_overlap(const BBBody *a, float a_half_width, float a_half_height,
                          const BBBody *b, float b_half_width, float b_half_height)
{
    return fabsf(a->x - b->x) < a_half_width + b_half_width && fabsf(a->y - b->y) < a_half_height + b_half_height;
}

bool circles_overlap(const BBBody *a, float a_radius,
                            const BBBody *b, float b_radius)
{
    float dx = a->x - b->x;
    float dy = a->y - b->y;
    float radius = a_radius + b_radius;
    return dx * dx + dy * dy < radius * radius;
}

// one_way: 일방 통과 발판, wrap: 아래로 떨어지면 위로 순환
BBHits move_body(const BBGame *game, BBBody *body, float half_width, float half_height,
                          float dt, bool one_way, bool wrap)
{
    BBHits hit = {false, false};
    float old_x = body->x;
    float old_y = body->y;

    float horizontal_step = body->vx * dt;
    if(one_way)
    {
        horizontal_step += platform_carry(game, body, half_width, half_height);
    }
    body->x += horizontal_step;

    int first_column = (int)floorf(fminf(old_x, body->x) - half_width + 0.0001f);
    int last_column = (int)floorf(fmaxf(old_x, body->x) + half_width - 0.0001f);
    for(int y = (int)floorf(body->y - half_height + 0.0001f); y <= (int)floorf(body->y + half_height - 0.0001f); ++y)
    {
        for(int x = first_column; x <= last_column; ++x)
        {
            if(tile_at(game, x, y) != 3)
                continue;
            // 벽 바깥에서 들어온 경우만 처리
            if(horizontal_step > 0.0f && old_x + half_width <= (float)x + 0.001f)
            {
                body->x = fminf(body->x, (float)x - half_width);
                hit.x = true;
            }
            else if(horizontal_step < 0.0f && old_x - half_width >= (float)(x + 1) - 0.001f)
            {
                body->x = fmaxf(body->x, (float)(x + 1) + half_width);
                hit.x = true;
            }
        }
    }
    if(body->x < half_width || body->x > (float)game->world_width - half_width)
    {
        body->x = clamp(body->x, half_width, (float)game->world_width - half_width);
        hit.x = true;
    }
    if(hit.x)
    {
        body->vx = 0.0f;
    }

    body->y += body->vy * dt;

    body->grounded = false;
    for(int y = (int)floorf(body->y - half_height + 0.0001f); y <= (int)floorf(body->y + half_height + 0.0001f); ++y)
    {

        for(int x = (int)floorf(body->x - half_width + 0.0001f);
            x <= (int)floorf(body->x + half_width - 0.0001f); ++x)
        {
            int tile = tile_at(game, x, y);
            if(tile != 3 && !(one_way && tile == 2))
                continue;
            if(tile == 2 && y < game->world_height - 2 && platform_for_tile(game, x, y) != NULL)
                continue;
            float tile_left = (float)x;
            if(body->x + half_width <= tile_left + 0.0001f || body->x - half_width >= tile_left + 1.0f - 0.0001f)
                continue;
            if(body->vy >= 0.0f && old_y + half_height <= (float)y + 0.001f && body->y + half_height >= (float)y)
            {

                body->y = fminf(body->y, (float)y - half_height);
                body->grounded = true;
                hit.y = true;
            }
            else if(tile == 3 && body->vy < 0.0f && old_y - half_height >= (float)(y + 1) - 0.001f)
            {

                body->y = fmaxf(body->y, (float)(y + 1) + half_height);
                hit.y = true;
            }
        }
        if(one_way && body->vy >= 0.0f && old_y + half_height <= (float)y + 0.001f && body->y + half_height >= (float)y)
        {
            for(int i = 0; i < game->platform_count; ++i)
            {
                const BBMovingPlatform *platform = &game->platforms[i];
                if(platform->y != y || !bb_game_platform_solid(platform))
                    continue;
                float left = (float)platform->x + platform->offset;
                float right = left + (float)platform->width;
                if(body->x + half_width <= left + 0.0001f || body->x - half_width >= right - 0.0001f)
                    continue;
                body->y = fminf(body->y, (float)y - half_height);
                body->grounded = true;
                hit.y = true;
            }
        }
    }
    if(hit.y)
    {
        body->vy = 0.0f;
    }

    if(wrap && body->y > (float)game->world_height + 1.0f)
    {
        body->y -= (float)game->world_height + 2.0f;
    }
    return hit;
}

static float platform_resize_shift(const BBGame *game, const BBBody *body, float half_width, float half_height, int dx)
{
    if(!body->grounded || body->vy < 0.0f)
        return 0.0f;
    float feet = body->y + half_height;
    int row = (int)floorf(feet + 0.001f);
    if(fabsf(feet - (float)row) > 0.001f)
        return 0.0f;
    float shift = 0.0f;
    float best_overlap = 0.0001f;
    for(int i = 0; i < game->platform_count; ++i)
    {
        const BBMovingPlatform *platform = &game->platforms[i];
        if(platform->y != row || !bb_game_platform_solid(platform))
            continue;
        float left = (float)platform->x + platform->offset;
        float right = left + (float)platform->width;
        float overlap = fminf(body->x + half_width, right) - fmaxf(body->x - half_width, left);
        if(overlap > best_overlap)
        {
            float minimum, maximum;
            platform_bounds(game, platform->x + dx, platform->width, &minimum, &maximum);
            best_overlap = overlap;
            shift = clamp(platform->offset, minimum, maximum) - platform->offset;
        }
    }
    return shift;
}

static void resize_body(BBBody *body, float half_width, int dx, int dy, float platform_shift,
                        int width, bool narrower, bool shorter)
{
    body->x += (float)dx + platform_shift;
    body->y += (float)dy;
    if(narrower)
    {
        body->x = clamp(body->x, 2.0f + half_width, (float)width - 2.0f - half_width);
    }

    if(shorter && body->y < -1.0f)
    {
        body->y = -1.0f;
        body->grounded = false;
    }
}

void bb_game_resize(BBGame *game, int width, int height)
{
    if(width < BB_MAP_WIDTH)
    {
        width = BB_MAP_WIDTH;
    }
    if(width > BB_WORLD_MAX_WIDTH)
    {
        width = BB_WORLD_MAX_WIDTH;
    }
    if(height < BB_MAP_HEIGHT)
    {
        height = BB_MAP_HEIGHT;
    }
    if(height > BB_WORLD_MAX_HEIGHT)
    {
        height = BB_WORLD_MAX_HEIGHT;
    }
    if(width == game->world_width && height == game->world_height)
        return;
    int dx = (width - BB_MAP_WIDTH) / 2 - (game->world_width - BB_MAP_WIDTH) / 2;
    int dy = height - game->world_height;
    float player_speed_ratio = (float)width / (float)game->world_width;
    bool narrower = width < game->world_width;
    bool wider = width > game->world_width;
    bool shorter = height < game->world_height;
    game->world_width = width;
    game->world_height = height;
    for(int i = 0; i < BB_MAX_PLAYERS; ++i)
    {
        BBPlayer *player = &game->players[i];
        float translated_y = player->body.y + (float)dy;
        float half_width = player->state == BB_PLAYER_BUBBLE ? BB_RESCUE_BUBBLE_RADIUS : 0.9f;
        float shift = 0.0f;
        if(narrower && player->active && player->state != BB_PLAYER_BUBBLE)
        {
            shift = platform_resize_shift(game, &player->body, 0.9f, 0.975f, dx);
        }
        resize_body(&player->body, half_width, dx, dy, shift, width, narrower, shorter);

        if(player->state == BB_PLAYER_BUBBLE)
        {
            clamp_rescue_bubble(game, player);
        }
        else
        {
            player->body.vx *= player_speed_ratio;
        }
        player->jump_start_y += (float)dy;
        if(player->body.y != translated_y)
        {
            player->jump_start_y += player->body.y - translated_y;
        }
    }
    for(int i = 0; i < BB_MAX_ENEMIES; ++i)
    {
        BBEnemy *enemy = &game->enemies[i];
        float half_width = enemy->state == BB_ENEMY_DEAD ? 0.95f : 0.9f;
        float shift = 0.0f;
        if(narrower && enemy->active && enemy->state != BB_ENEMY_CAPTURED &&
           enemy->state != BB_ENEMY_SPAWNING)
        {
            shift = platform_resize_shift(game, &enemy->body, half_width, 0.95f, dx);
        }
        resize_body(&enemy->body, half_width, dx, dy, shift, width, narrower, shorter);
        enemy->spawn_y += (float)dy;
    }
    for(int i = 0; i < BB_MAX_BUBBLES; ++i)
    {
        resize_body(&game->bubbles[i].body, 0.75f, dx, dy, 0.0f, width, narrower, shorter);
    }
    for(int i = 0; i < BB_MAX_BOULDERS; ++i)
    {
        BBBoulder *boulder = &game->boulders[i];
        float shift = 0.0f;
        if(narrower && boulder->active)
        {
            shift = platform_resize_shift(game, &boulder->body, 0.75f, 0.75f, dx);
        }
        resize_body(&boulder->body, 0.75f, dx, dy, shift, width, narrower, shorter);
    }
    for(int i = 0; i < BB_MAX_PICKUPS; ++i)
    {
        BBPickup *pickup = &game->pickups[i];
        float shift = 0.0f;
        if(narrower && pickup->active)
        {
            shift = platform_resize_shift(game, &pickup->body, 0.95f, 0.95f, dx);
        }
        resize_body(&pickup->body, 0.95f, dx, dy, shift, width, narrower, shorter);
    }
    for(int i = 0; i < game->platform_count; ++i)
    {
        game->platforms[i].x += dx;
        game->platforms[i].y += dy;
        if(narrower)
        {
            BBMovingPlatform *platform = &game->platforms[i];
            float minimum, maximum;
            platform_bounds(game, platform->x, platform->width, &minimum, &maximum);
            float offset = clamp(platform->offset, minimum, maximum);
            if(offset != platform->offset)
            {
                platform->offset = offset;
                platform->step = 0.0f;
                platform->direction = -1;
                if(offset <= minimum)
                {
                    platform->direction = 1;
                }
            }
        }
    }
    if(wider && game->state == BB_STATE_PLAY)
    {
        add_round_enemies(game, true);
    }
}
