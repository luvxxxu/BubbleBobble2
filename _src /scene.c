/* 게임 화면 안쪽(배경, 맵, 발판, 적, 거품, 음식, 플레이어) 그리기
 * 게임 구조체의 값을 읽어서 알맞은 그림을 고른다 */
#include "bubble.h"

// 스프라이트 시트에서 한 칸 번호
typedef struct BBSpriteCell {
    int column;
    int row;
} BBSpriteCell;

// 적 종류로 시트 행 번호 구하기
static int enemy_row(BBEnemyType type)
{
    switch (type) {
        case BB_ENEMY_MAITA: return 1;
        case BB_ENEMY_MONSTA: return 3;
        default: return 0;
    }
}

// 음식 그림 골라서 그리기
static void pickup_sprite(const BBAssets *assets, const BBPickup *pickup)
{
    // 순서는 BBPickupType이랑 같음
    static const BBSpriteCell cells[BB_PICKUP_TYPE_COUNT] = {
        {19, 0}, // 수박
        {25, 0}, // 감자튀김
        {30, 1}, // 체리
        {32, 1}, // 딸기
        {16, 0}, // 복숭아
        {13, 0}, // 오렌지
        {34, 1}, // 포도
        {17, 0}, // 바나나
        {35, 1}, // 파인애플
        {14, 0}, // 레몬
        {4, 0},  // 사과
        {18, 0}, // 배
        {5, 0},  // 무
        {10, 0}, // 옥수수
        {2, 0},  // 당근
        {1, 0},  // 가지
        {3, 1},  // 아이스크림
        {28, 0}, // 케이크
        {24, 0}, // 도넛
        {27, 0}  // 햄버거
    };
    // unsigned로 바꾸면 음수도 범위 검사에 걸림
    unsigned int type = (unsigned int)pickup->type;
    if (type >= BB_PICKUP_TYPE_COUNT) {
        return;
    }
    sprite(assets->items, cells[type].column, cells[type].row, 16,
           pickup->body.x, pickup->body.y, false);
}

// 발판 테두리 점선 그리기
static void platform_outline(float x, float y, int width)
{
    for (int offset = 0; offset < width; offset += 4) {
        int dash = width - offset;
        if (dash > 2) {
            dash = 2;
        }
        DrawRectangleRec((Rectangle){x + (float)offset, y, (float)dash, 1}, WHITE);
        DrawRectangleRec((Rectangle){x + (float)offset, y + 7, (float)dash, 1}, WHITE);
    }
    for (int offset = 0; offset < 8; offset += 4) {
        DrawRectangleRec((Rectangle){x, y + (float)offset, 1, 2}, WHITE);
        DrawRectangleRec((Rectangle){x + (float)width - 1, y + (float)offset, 1, 2}, WHITE);
    }
}

// 해, 달, 행성 그리기 (배경 가운데 그림 기준 위치에 딱 한 번만)
static void draw_celestial(const BBAssets *assets, int level, float origin_x, float origin_y)
{
    // 라운드별 그림 번호와 중심 위치 (1라운드 해, 4라운드 달, 5라운드 행성)
    static const struct { int cell, x, y; } place[BB_LEVEL_COUNT] = {
        {0, 212, 34}, {-1, 0, 0}, {-1, 0, 0}, {1, 194, 58}, {2, 58, 63}
    };
    if (level < 0 || level >= BB_LEVEL_COUNT || place[level].cell < 0) {
        return;
    }
    DrawTextureRec(assets->celestial, (Rectangle){0, (float)(place[level].cell * 144), 144, 144},
                   (Vector2){origin_x + (float)place[level].x - 72, origin_y + (float)place[level].y - 72}, WHITE);
}

// 배경 한 장을 월드 안에 들어오는 만큼만 그리기 (flip이면 뒤집음)
static void draw_background_copy(const BBAssets *assets, int level, float dest_x, float dest_y,
                                 bool flip_x, bool flip_y, float world_w, float world_h)
{
    float left = fmaxf(dest_x, 0.0f), right = fminf(dest_x + (float)BB_SCREEN_WIDTH, world_w);
    float top = fmaxf(dest_y, 0.0f), bottom = fminf(dest_y + (float)BB_SCREEN_HEIGHT, world_h);
    if (right <= left || bottom <= top) {
        return;
    }
    // 그림에서 가져올 구간
    float u0 = left - dest_x, u1 = right - dest_x;
    float v0 = top - dest_y, v1 = bottom - dest_y;
    float source_x = flip_x ? (float)BB_SCREEN_WIDTH - u1 : u0;
    float source_y = flip_y ? (float)BB_SCREEN_HEIGHT - v1 : v0;
    float source_w = (u1 - u0) * (flip_x ? -1.0f : 1.0f);
    float source_h = (v1 - v0) * (flip_y ? -1.0f : 1.0f);
    DrawTexturePro(assets->backgrounds,
                   (Rectangle){source_x, (float)(level * BB_SCREEN_HEIGHT) + source_y, source_w, source_h},
                   (Rectangle){left, top, right - left, bottom - top},
                   (Vector2){0, 0}, 0, WHITE);
}

// 배경 그리기 (넓어진 부분은 뒤집어서 이어 붙임)
static void draw_backdrop(const BBAssets *assets, int level, int world_columns, int world_rows)
{
    float world_w = (float)(world_columns * 8);
    float world_h = (float)(world_rows * 8);
    float origin_x = (float)(((world_columns - BB_MAP_WIDTH) / 2) * 8);
    float origin_y = world_h - (float)BB_SCREEN_HEIGHT;
    int first_column = (int)floorf(-origin_x / (float)BB_SCREEN_WIDTH);
    int last_column = (int)floorf((world_w - 1.0f - origin_x) / (float)BB_SCREEN_WIDTH);
    for (int row = 0; origin_y - (float)(row * BB_SCREEN_HEIGHT) > -(float)BB_SCREEN_HEIGHT; ++row) {
        for (int column = first_column; column <= last_column; ++column) {
            draw_background_copy(assets, level, origin_x + (float)(column * BB_SCREEN_WIDTH),
                                 origin_y - (float)(row * BB_SCREEN_HEIGHT), (column & 1) != 0, (row & 1) != 0,
                                 world_w, world_h);
        }
    }
    draw_celestial(assets, level, origin_x, origin_y);
}

// 벽, 바닥 한 칸 그리기 (블록 그림 16x16을 8x8 네 조각으로 나눠 씀)
static void draw_block_tile(const BBAssets *assets, const BBGame *game, int level, int tile, int x, int y)
{
    // 가로 조각: 왼쪽 절반은 왼쪽 끝부터, 오른쪽 절반은 오른쪽 끝부터 세서 짝을 맞춤
    int quarter_x = x & 1;
    if (x * 2 >= game->world_width) {
        quarter_x = 1 - ((game->world_width - 1 - x) & 1);
    }
    int floor_top = game->world_height - 2;
    int block = 0; // 0 벽 몸통, 1 윗면 블록
    int quarter_y;
    if (y >= floor_top) {
        quarter_y = y - floor_top;
        if (tile != 3) {
            block = 1;
        }
    } else {
        // 벽은 맨 위(2번째 줄)부터 두 칸씩 묶음, 맨 위 블록은 윗면 그림
        quarter_y = (y - 2) & 1;
        if (y - 2 < 2) {
            block = 1;
        }
    }
    DrawTextureRec(assets->tiles,
                   (Rectangle){(float)(block * 16 + quarter_x * 8), (float)(level * 16 + quarter_y * 8), 8, 8},
                   (Vector2){(float)(x * 8), (float)(y * 8)}, WHITE);
}

// 인트로 배경: 로고 그림은 가운데 한 장만 두고, 창이 넓으면 좌우는 로고·해를 뺀 하늘 그림을 거울처럼 이어 붙임
// 위아래가 남으면 맨 윗줄/맨 아랫줄을 늘려서 채움
void draw_intro_backdrop(const BBAssets *assets, const Camera2D *panel, int framebuffer_width, int framebuffer_height)
{
    float left = -panel->offset.x / panel->zoom;
    float right = ((float)framebuffer_width - panel->offset.x) / panel->zoom;
    float top = -panel->offset.y / panel->zoom;
    float bottom = ((float)framebuffer_height - panel->offset.y) / panel->zoom;
    int first_column = (int)floorf(left / (float)BB_SCREEN_WIDTH);
    int last_column = (int)floorf((right - 0.01f) / (float)BB_SCREEN_WIDTH);
    for (int column = first_column; column <= last_column; ++column) {
        float flip = (column & 1) != 0 ? -1.0f : 1.0f;
        // 가운데(0번)만 로고가 있는 그림, 나머지는 하늘만 있는 그림
        Texture2D picture = column == 0 ? assets->intro_background : assets->intro_sky;
        float x = (float)(column * BB_SCREEN_WIDTH);
        float width = (float)BB_SCREEN_WIDTH * flip;
        DrawTexturePro(picture, (Rectangle){0, 0, width, (float)BB_SCREEN_HEIGHT},
                       (Rectangle){x, 0, (float)BB_SCREEN_WIDTH, (float)BB_SCREEN_HEIGHT}, (Vector2){0, 0}, 0, WHITE);
        if (top < 0.0f) {
            DrawTexturePro(assets->intro_sky, (Rectangle){0, 0, width, 1},
                           (Rectangle){x, top, (float)BB_SCREEN_WIDTH, -top}, (Vector2){0, 0}, 0, WHITE);
        }
        if (bottom > (float)BB_SCREEN_HEIGHT) {
            DrawTexturePro(assets->intro_sky, (Rectangle){0, (float)(BB_SCREEN_HEIGHT - 1), width, 1},
                           (Rectangle){x, (float)BB_SCREEN_HEIGHT, (float)BB_SCREEN_WIDTH, bottom - (float)BB_SCREEN_HEIGHT},
                           (Vector2){0, 0}, 0, WHITE);
        }
    }
}

// READY, 점수 화면 배경: 1라운드 배경을 가운데 두고 넓은 곳은 뒤집어 이어 붙임
// 맨 아래 20줄(바닥 구멍 자리)은 빼고 위 204줄만 씀
void draw_title_backdrop(const BBAssets *assets, const Camera2D *panel,
                                int framebuffer_width, int framebuffer_height)
{
    const float tile_height = 204.0f;
    float left = -panel->offset.x / panel->zoom;
    float right = ((float)framebuffer_width - panel->offset.x) / panel->zoom;
    float top = -panel->offset.y / panel->zoom;
    float bottom = ((float)framebuffer_height - panel->offset.y) / panel->zoom;
    int first_column = (int)floorf(left / (float)BB_SCREEN_WIDTH);
    int last_column = (int)floorf((right - 0.01f) / (float)BB_SCREEN_WIDTH);
    int first_row = (int)floorf(top / tile_height);
    int last_row = (int)floorf((bottom - 0.01f) / tile_height);
    for (int row = first_row; row <= last_row; ++row) {
        for (int column = first_column; column <= last_column; ++column) {
            float flip_x = (column & 1) != 0 ? -1.0f : 1.0f;
            float flip_y = (row & 1) != 0 ? -1.0f : 1.0f;
            DrawTexturePro(assets->backgrounds,
                           (Rectangle){0, 0, (float)BB_SCREEN_WIDTH * flip_x, tile_height * flip_y},
                           (Rectangle){(float)(column * BB_SCREEN_WIDTH), (float)row * tile_height, (float)BB_SCREEN_WIDTH, tile_height},
                           (Vector2){0, 0}, 0, WHITE);
        }
    }
    // 해는 가운데 그림 자리에 한 번만
    draw_celestial(assets, 0, 0, 0);
}

// 움직이는 것 빼고 풍경만 그리기 (다음 라운드 미리보기용)
void draw_scenery(const BBAssets *assets, const BBGame *game, int level)
{
    draw_backdrop(assets, level, game->world_width, game->world_height);
    for (int y = 0; y < game->world_height; ++y) {
        for (int x = 0; x < game->world_width; ++x) {
            int tile = bb_game_tile_for_level(game, level, x, y);
            if (!tile) {
                continue;
            }
            bool air_platform = tile == 2 && y < game->world_height - 2;
            if (air_platform) {
                DrawTextureRec(assets->platform_tiles, (Rectangle){0, (float)(level * 8), 8, 8},
                               (Vector2){(float)(x * 8), (float)(y * 8)}, WHITE);
            } else {
                draw_block_tile(assets, game, level, tile, x, y);
            }
        }
    }
}

// 라운드 넘어갈 때 스크롤 진행도 (0에서 1)
float round_scroll(const BBGame *game)
{
    if (game->state != BB_STATE_CLEAR || game->level + 1 >= BB_LEVEL_COUNT) {
        return 0.0f;
    }
    float t = (game->state_time - (BB_CLEAR_DURATION - BB_SCROLL_SECONDS)) / BB_SCROLL_SECONDS;
    if (t <= 0.0f) {
        return 0.0f;
    }
    if (t > 1.0f) {
        t = 1.0f;
    }
    return t * t * (3.0f - 2.0f * t);
}

// 벽, 바닥, 발판 그리기
static void draw_tiles(const BBAssets *assets, const BBGame *game)
{
    // 인트로에서는 맵 안 그림
    if (game->state == BB_STATE_INTRO) {
        return;
    }
    draw_backdrop(assets, game->level, game->world_width, game->world_height);
    for (int y = 0; y < game->world_height; ++y) {
        for (int x = 0; x < game->world_width; ++x) {
            int tile = bb_game_tile(game, x, y);
            if (!tile) {
                continue;
            }
            // 공중 발판은 아래에서 따로 그림
            if (tile == 2 && y < game->world_height - 2) {
                continue;
            }
            draw_block_tile(assets, game, game->level, tile, x, y);
        }
    }
    for (int i = 0; i < game->platform_count; ++i) {
        const BBMovingPlatform *platform = &game->platforms[i];
        float x = roundf(((float)platform->x + platform->offset) * 8.0f);
        float y = (float)(platform->y * 8);
        if (platform->state == BB_PLATFORM_SOLID) {
            for (int tile = 0; tile < platform->width; ++tile) {
                DrawTextureRec(assets->platform_tiles, (Rectangle){0, (float)(game->level * 8), 8, 8},
                               (Vector2){x + (float)(tile * 8), y}, WHITE);
            }
        } else if (bb_game_platform_blink_visible(platform)) {
            // 발판이 사라지기 전에 깜빡임
            platform_outline(x, y, platform->width * 8);
        }
    }
}

// 적 그리기 (잡힌 적은 제외)
static void draw_enemies(const BBAssets *assets, const BBGame *game)
{
    int enemy_frame = (int)(game->level_time * 4) % 2;
    for (int i = 0; i < BB_MAX_ENEMIES; ++i) {
        const BBEnemy *enemy = &game->enemies[i];
        // 잡힌 적은 거품에서 그림
        if (!enemy->active || enemy->state == BB_ENEMY_CAPTURED ||
            (enemy->state == BB_ENEMY_SPAWNING && enemy->age <= enemy->spawn_delay)) {
            continue;
        }
        // 화난 적은 +2, 죽으면 12~15번이 돌아감
        int frame = enemy_frame;
        if (enemy->angry) {
            frame += 2;
        }
        if (enemy->state == BB_ENEMY_DEAD) {
            frame = 12 + (int)(enemy->age * 4) % 4;
        }
        sprite(assets->enemy, frame, enemy_row(enemy->type), 16, enemy->body.x, enemy->body.y, enemy->facing > 0);
    }
}

// 거품 그리기 (1P 2P 색 다름)
static void draw_bubbles(const BBAssets *assets, const BBGame *game)
{
    int enemy_frame = (int)(game->level_time * 4) % 2;
    for (int i = 0; i < BB_MAX_BUBBLES; ++i) {
        const BBBubble *bubble = &game->bubbles[i];
        if (!bubble->active) {
            continue;
        }
        int frame;
        // 터지는 중 / 커지는 중 / 떠다니는 중
        if (bubble->popping) {
            frame = 5 + (int)(fmaxf(0, 0.25f - bubble->pop_timer) * 8) % 2;
        }
        else if (bubble->age < 0.333f) {
            frame = (int)(bubble->age * 12);
        }
        else {
            frame = 3 + (int)(bubble->age * 12) % 2;
        }
        // 적이 들어있으면 적 그림
        if (bubble->captured_enemy < 0 || bubble->popping) {
            Texture2D sheet = bubble->owner == 1 ? assets->blue_bubble : assets->bubble;
            sprite(sheet, frame, 0, 16, bubble->body.x, bubble->body.y, false);
        } else if (bubble->captured_enemy < BB_MAX_ENEMIES) {
            Texture2D sheet = bubble->owner == 1 ? assets->blue_captured_enemy : assets->enemy;
            int row = enemy_row(game->enemies[bubble->captured_enemy].type);
            sprite(sheet, 6 + enemy_frame, row, 16, bubble->body.x, bubble->body.y, false);
        }
    }
}

// 음식이랑 바위 그리기
static void draw_pickups_and_boulders(const BBAssets *assets, const BBGame *game)
{
    for (int i = 0; i < BB_MAX_PICKUPS; ++i) {
        if (game->pickups[i].active) {
            pickup_sprite(assets, &game->pickups[i]);
        }
    }
    for (int i = 0; i < BB_MAX_BOULDERS; ++i) {
        if (game->boulders[i].active) {
            sprite(assets->enemy, 5 - (int)(game->boulders[i].age * 4) % 6, 2, 16,
                   game->boulders[i].body.x, game->boulders[i].body.y, false);
        }
    }
}

// 플레이어 그리기
static void draw_players(const BBAssets *assets, const BBGame *game)
{
    for (int i = 0; i < game->player_count; ++i) {
        const BBPlayer *player = &game->players[i];
        if (!player->active || player->state == BB_PLAYER_OUT || player->state == BB_PLAYER_WAIT_ROUND) {
            continue;
        }
        if (player->state == BB_PLAYER_BUBBLE) {
            // 거품에 갇힌 상태
            sprite(assets->players[i], 2 + (int)(game->level_time * 4) % 2, 1, 16,
                   player->body.x, player->body.y, player->facing < 0);
            int frame = 3 + (int)(game->level_time * 4) % 2;
            large_bubble_sprite(assets, i, frame, player->body.x, player->body.y);
            continue;
        }
        // 구조되면서 거품 터지는 모습
        if (player->state == BB_PLAYER_NORMAL && player->rescue_pop_timer > 0.0f) {
            int frame = 7;
            if (player->rescue_pop_timer > 0.125f) {
                frame = 6;
            }
            large_bubble_sprite(assets, i, frame, player->body.x, player->body.y);
        }
        // 무적일 때 깜빡임
        if (player->invulnerable > 0 && player->state != BB_PLAYER_DEAD && (int)(game->level_time * 12) % 2) {
            continue;
        }
        // 죽었을 때도 깜빡임
        if (player->state == BB_PLAYER_DEAD && (int)(player->death_timer * 8) % 2) {
            continue;
        }
        // 플레이어 그림 고르기 (뒤에 오는 조건이 덮어씀)
        int row = 0;
        int column = 0;
        if (fabsf(player->body.vx) > 0.1f) {
            column = (int)(game->level_time * 7) % 2;
        }
        if (!player->body.grounded) {
            row = 1;
            column = 2;
        }
        if (player->attack_timer > 0) {
            row = 2;
            column = 0;
        }
        if (player->state == BB_PLAYER_DEAD) {
            row = 0;
            column = 0;
        }
        if (game->state == BB_STATE_INTRO) {
            row = 2;
            column = 1 + (int)(game->state_time * 4) % 2;
        }
        sprite(assets->players[i], column, row, 16, player->body.x, player->body.y, player->facing < 0);
        // 인트로에서는 큰 거품도 같이
        if (game->state == BB_STATE_INTRO) {
            int frame = 3 + (int)(game->state_time * 4) % 2;
            large_bubble_sprite(assets, i, frame, player->body.x, player->body.y);
        }
    }
}

// 게임 화면 전체 그리기 (그리는 순서가 앞뒤 순서)
void draw_world(const BBAssets *assets, const BBGame *game)
{
    draw_tiles(assets, game);
    draw_enemies(assets, game);
    draw_bubbles(assets, game);
    draw_pickups_and_boulders(assets, game);
    draw_players(assets, game);
}

// 구조 남은 초 (1에서 10)
int rescue_seconds(const BBPlayer *player)
{
    double seconds = ceil(player->rescue_time_left);
    seconds = fmin(BB_RESCUE_BUBBLE_SECONDS, seconds);
    seconds = fmax(1.0, seconds);
    return (int)seconds;
}

// 거품에 갇힌 플레이어 위에 HELP 글자 그리기
void rescue_labels(const BBAssets *assets, const BBGame *game, const BBViewport *viewport,
                          int framebuffer_width, int framebuffer_height, float hud_scale)
{
    float left = -viewport->offset_x / viewport->scale;
    float right = ((float)framebuffer_width - viewport->offset_x) / viewport->scale;
    float top = (24.0f * hud_scale - viewport->offset_y) / viewport->scale;
    float bottom = ((float)framebuffer_height - 12.0f * hud_scale - viewport->offset_y) / viewport->scale;
    for (int i = 0; i < game->player_count; ++i) {
        const BBPlayer *player = &game->players[i];
        if (!player->active || player->state != BB_PLAYER_BUBBLE) {
            continue;
        }
        char label[16];
        snprintf(label, sizeof label, "P%d HELP %dS", i + 1, rescue_seconds(player));
        float width = MeasureTextEx(assets->font, label, 8, 0).x;
        float x = roundf(player->body.x * 8.0f - width * 0.5f);
        float y = roundf(player->body.y * 8.0f - 27.0f);
        // 위가 좁으면 아래로
        if (y < top + 2.0f) {
            y = roundf(player->body.y * 8.0f + 19.0f);
        }
        x = fmaxf(left + 2.0f, fminf(x, right - width - 2.0f));
        y = fmaxf(top + 2.0f, fminf(y, bottom - 10.0f));
        DrawRectangleRec((Rectangle){x - 1, y - 1, width + 2, 10}, BLACK);
        text(assets, label, x, y, player_colors[i]);
    }
}
