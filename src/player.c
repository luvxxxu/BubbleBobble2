

#include "bubble.h"

void place_player(const BBGame *game, BBPlayer *player, int number)
{

    memset(&player->body, 0, sizeof player->body);
    player->body.x = 4.0f;
    player->facing = 1;
    if(number != 0)
    {
        player->body.x = (float)game->world_width - 4.0f;
        player->facing = -1;
    }
    player->body.y = (float)game->world_height - 4.0f;
    player->state = BB_PLAYER_NORMAL;
    player->death_timer = 0.0f;
    player->rescue_pop_timer = 0.0f;
    player->rescue_time_left = 0.0;
    player->rescue_turn_timer = 0.0;
    player->rescue_drift_vx = 0.0f;
    player->rescue_drift_vy = 0.0f;
    player->rescue_rng = 0;
    player->falling = true;
    player->air_control = true;
    player->jump_start_y = player->body.y;
    player->attack_timer = 0.0f;
    player->fire_cooldown = 0.0f;
}

void clamp_rescue_bubble(const BBGame *game, BBPlayer *player)
{
    float margin = 2.0f + BB_RESCUE_BUBBLE_RADIUS;
    float x = clamp(player->body.x, margin, (float)game->world_width - margin);
    float y = clamp(player->body.y, 5.5f, (float)game->world_height - 3.5f);
    if((player->body.x - x) * player->body.vx > 0.0f)
    {
        player->body.vx = -player->body.vx;
        player->rescue_drift_vx = fabsf(player->rescue_drift_vx);
        if(player->body.x > x)
        {
            player->rescue_drift_vx = -player->rescue_drift_vx;
        }
    }
    if((player->body.y - y) * player->body.vy > 0.0f)
    {
        player->body.vy = -player->body.vy;
        player->rescue_drift_vy = fabsf(player->rescue_drift_vy);
        if(player->body.y > y)
        {
            player->rescue_drift_vy = -player->rescue_drift_vy;
        }
    }
    player->body.x = x;
    player->body.y = y;
}

static void choose_rescue_drift(BBPlayer *player)
{
    float angle = random_from_state(&player->rescue_rng) * 6.28318531f;
    float speed = 1.8f + random_from_state(&player->rescue_rng) * 1.4f;

    player->rescue_drift_vx = cosf(angle) * speed;
    player->rescue_drift_vy = sinf(angle) * speed;
    player->rescue_turn_timer += 1.0 + (double)random_from_state(&player->rescue_rng) * 0.8;
}

static void begin_rescue_bubble(BBGame *game, BBPlayer *player)
{
    player->state = BB_PLAYER_BUBBLE;
    player->body.vx = 0.0f;
    player->body.vy = 0.0f;
    player->body.grounded = false;
    player->invulnerable = 0.0f;
    player->attack_timer = 0.0f;
    player->fire_cooldown = 0.0f;
    player->rescue_pop_timer = 0.0f;
    player->rescue_time_left = BB_RESCUE_BUBBLE_SECONDS;
    player->rescue_turn_timer = 0.0;
    random_unit(game);
    player->rescue_rng = game->rng;
    choose_rescue_drift(player);
    clamp_rescue_bubble(game, player);
}

static void update_rescue_bubble(const BBGame *game, BBPlayer *player, int index, float dt)
{
    player->rescue_time_left = fmax(0.0, player->rescue_time_left - (double)dt);
    if(player->rescue_time_left == 0.0)
    {
        player->state = BB_PLAYER_WAIT_ROUND;
        player->body.vx = 0.0f;
        player->body.vy = 0.0f;
        player->body.grounded = false;
        player->rescue_turn_timer = 0.0;
        player->rescue_drift_vx = 0.0f;
        player->rescue_drift_vy = 0.0f;
        return;
    }
    player->rescue_turn_timer -= (double)dt;
    if(player->rescue_turn_timer <= 0.0)
    {
        choose_rescue_drift(player);
    }

    float age = (float)(BB_RESCUE_BUBBLE_SECONDS - player->rescue_time_left);
    float phase = (float)index * 3.14159265f;
    float vx = player->rescue_drift_vx + sinf(age * 2.0f + phase) * 0.55f;
    float vy = player->rescue_drift_vy + cosf(age * 1.7f + phase) * 0.8f;

    float blend = 1.0f - expf(-3.0f * dt);
    player->body.vx += (vx - player->body.vx) * blend;
    player->body.vy += (vy - player->body.vy) * blend;
    player->body.x += player->body.vx * dt;
    player->body.y += player->body.vy * dt;
    player->body.grounded = false;
    clamp_rescue_bubble(game, player);
}

void damage_player(BBGame *game, BBPlayer *player)
{
    if(!player->active || player->state != BB_PLAYER_NORMAL || player->invulnerable > 0.0f)
        return;
    --player->lives;
    player->state = BB_PLAYER_DEAD;
    player->death_timer = 1.5f;
    player->body.vx = 0.0f;
    player->body.vy = 0.0f;
    player->rescue_pop_timer = 0.0f;
    game->events.death = true;
}

static void fire_bubble(BBGame *game, BBPlayer *player, int owner)
{
    if(player->fire_cooldown > 0.0f)
        return;
    for(int i = 0; i < BB_MAX_BUBBLES; ++i)
    {
        BBBubble *bubble = &game->bubbles[i];
        if(bubble->active)
            continue;
        float x = player->body.x + (float)player->facing * 2.4f;
        float y = player->body.y;
        float speed = 20.0f * (float)player->facing;

        for(float distance = 0.1f; distance <= 3.15f; distance += 0.1f)
        {
            float cast_x = player->body.x + (float)player->facing * distance;
            int row = (int)floorf(y);
            float tile_left = 0.0f;
            int tile = solid_tile_at(game, cast_x, row, &tile_left);
            if(tile == 2 || tile == 3)
            {
                float side = 1.0f;
                if(player->facing > 0)
                {
                    side = 0.0f;
                }
                float boundary = tile_left + side;
                x = boundary - (float)player->facing * 0.751f;
                speed = 0.0f;
                break;
            }
        }
        memset(bubble, 0, sizeof *bubble);
        bubble->body.x = clamp(x, 0.75f, (float)game->world_width - 0.75f);
        bubble->body.y = y;
        bubble->body.vx = speed;
        bubble->active = true;
        bubble->owner = owner;
        bubble->captured_enemy = -1;

        player->fire_cooldown = 0.4f;
        player->attack_timer = 0.2f;
        game->events.fire = true;
        break;
    }
}

static void update_mana(BBPlayer *player, float dt)
{
    float duration = BB_ENERGY_CHARGE_SECONDS;
    if(player->boosting)
    {
        duration = BB_ENERGY_BOOST_SECONDS;
    }
    player->energy_elapsed += (double)dt;
    if(player->energy_elapsed >= duration) {
        player->energy_elapsed -= duration;
        player->boosting = !player->boosting;
        if(player->boosting)
        {
            player->body.vx *= 2.0f;
        }
        else
        {
            player->body.vx *= 0.5f;
        }
    }
    if(player->boosting) {
        player->energy = 1.0f - (float)(player->energy_elapsed / BB_ENERGY_BOOST_SECONDS);
    } else {
        player->energy = (float)(player->energy_elapsed / BB_ENERGY_CHARGE_SECONDS);
    }
    player->energy = clamp(player->energy, 0.0f, 1.0f);
}

void update_player(BBGame *game, BBPlayer *player, int index, BBInput input, float dt)
{
    if(!player->active || player->state == BB_PLAYER_OUT || player->state == BB_PLAYER_WAIT_ROUND)
        return;
    if(player->state == BB_PLAYER_BUBBLE)
    {
        update_rescue_bubble(game, player, index, dt);
        return;
    }
    player->rescue_pop_timer = decrease(player->rescue_pop_timer, dt);
    player->invulnerable = decrease(player->invulnerable, dt);
    player->attack_timer = decrease(player->attack_timer, dt);
    player->fire_cooldown = decrease(player->fire_cooldown, dt);
    if(player->state == BB_PLAYER_DEAD)
    {

        player->death_timer = decrease(player->death_timer, dt);
        if(player->death_timer <= 0.0f)
        {
            if(player->lives > 0)
            {
                place_player(game, player, index);
                player->invulnerable = 3.0f;
            }
            else if(game->player_count == 2)
            {
                begin_rescue_bubble(game, player);
            }
            else
                player->state = BB_PLAYER_OUT;
        }
        return;
    }
    update_mana(player, dt);
    float boost_multiplier = 1.0f;
    if(player->boosting)
    {
        boost_multiplier = 2.0f;
    }
    float speed_multiplier = boost_multiplier * (float)game->world_width / (float)BB_MAP_WIDTH;
    float move = clamp(input.move, -1.0f, 1.0f);
    if(move < 0.0f)
    {
        player->facing = -1;
    }
    else if(move > 0.0f)
    {
        player->facing = 1;
    }
    if(player->body.grounded)
    {

        player->body.vx = move * BB_PLAYER_GROUND_SPEED * speed_multiplier;
        player->body.vy = 30.0f * dt;
        player->falling = true;
        if(input.jump)
        {
            // y는 위로 갈수록 작아서 점프는 음수
            player->body.vy = -18.0f;
            player->body.grounded = false;
            player->jump_start_y = player->body.y;
            player->falling = false;
            player->air_control = false;
            game->events.jump = true;
        }
    }
    if(!player->body.grounded)
    {
        if(player->body.y > player->jump_start_y && player->body.vy > 0.0f)
        {
            player->falling = true;
        }
        if(player->falling)
        {

            player->body.vy = 6.0f;
        }
        else
        {

            if(player->body.vx == 0.0f || player->body.vx * move < 0.0f)
            {
                player->air_control = true;
            }
            player->body.vy += 30.0f * dt;
        }
        if(player->falling || player->air_control)
        {
            player->body.vx = move * BB_PLAYER_AIR_SPEED * speed_multiplier;
        }
    }
    move_body(game, &player->body, 0.9f, 0.975f, dt, true, true);
    if(input.fire)
    {
        fire_bubble(game, player, index);
    }
}

static void revive_player(BBGame *game, int index)
{
    BBPlayer *player = &game->players[index];
    BBBody position = player->body;
    place_player(game, player, index);
    player->body.x = position.x;
    player->body.y = position.y;
    player->jump_start_y = position.y;
    player->lives = 1;
    player->invulnerable = 3.0f;
    player->rescue_pop_timer = 0.25f;
    game->events.pop = true;
}

static bool rescue_with_bubble(BBGame *game, const BBPlayer *waiting, int teammate)
{
    for(int b = 0; b < BB_MAX_BUBBLES; ++b)
    {
        BBBubble *bubble = &game->bubbles[b];
        if(!bubble->active || bubble->popping || bubble->captured_enemy >= 0 || bubble->owner != teammate)
            continue;
        if(circles_overlap(&waiting->body, BB_RESCUE_BUBBLE_RADIUS, &bubble->body, 0.75f))
        {
            pop_bubble(game, bubble, false);
            return true;
        }
    }
    return false;
}

void check_rescue_bump(BBGame *game)
{
    if(game->player_count != 2)
        return;
    for(int target = 0; target < game->player_count; ++target)
    {
        BBPlayer *waiting = &game->players[target];
        if(!waiting->active || waiting->state != BB_PLAYER_BUBBLE)
            continue;

        int teammate = 1 - target;
        const BBPlayer *rescuer = &game->players[teammate];
        if(!rescuer->active || rescuer->state != BB_PLAYER_NORMAL || rescuer->lives <= 0)
            continue;
        if(circles_overlap(&waiting->body, BB_RESCUE_BUBBLE_RADIUS, &rescuer->body, 0.95f) ||
           rescue_with_bubble(game, waiting, teammate))
        {
            revive_player(game, target);
        }
    }
}

void check_bubble_bump(BBGame *game)
{
    for(int b = 0; b < BB_MAX_BUBBLES; ++b)
    {
        BBBubble *bubble = &game->bubbles[b];
        if(!bubble->active || bubble->popping)
            continue;
        if(bubble->captured_enemy < 0)
        {

            for(int e = 0; e < BB_MAX_ENEMIES; ++e)
            {
                BBEnemy *enemy = &game->enemies[e];
                if(!enemy->active || enemy->state == BB_ENEMY_SPAWNING ||
                   enemy->state == BB_ENEMY_CAPTURED || enemy->state == BB_ENEMY_DEAD)
                    continue;
                if(circles_overlap(&bubble->body, 0.75f, &enemy->body, 0.95f))
                {
                    bubble->captured_enemy = e;
                    enemy->state = BB_ENEMY_CAPTURED;
                    break;
                }
            }
        }

        if(bubble->age < 1.0f)
            continue;
        for(int p = 0; p < BB_MAX_PLAYERS; ++p)
        {
            BBPlayer *player = &game->players[p];
            if(!player->active || player->state != BB_PLAYER_NORMAL ||
               !circles_overlap(&bubble->body, 0.75f, &player->body, 0.95f))
                continue;
            float dx = player->body.vx - bubble->body.vx;
            float dy = player->body.vy - bubble->body.vy;
            // 상대 속도랑 거품 나이로 터질지 정함
            if(sqrtf(dx * dx + dy * dy) + bubble->age >= 15.0f)
            {
                pop_bubble(game, bubble, false);
                break;
            }
            if(player->body.vy >= bubble->body.vy && player->body.y < bubble->body.y - 0.5f)
            {
                player->body.y = bubble->body.y - 1.725f;
                player->body.vy = bubble->body.vy;
                player->body.grounded = true;
            }
        }
    }
}

void check_boulder_bump(BBGame *game)
{
    for(int b = 0; b < BB_MAX_BOULDERS; ++b)
    {
        BBBoulder *boulder = &game->boulders[b];
        if(!boulder->active)
            continue;
        for(int p = 0; p < BB_MAX_PLAYERS; ++p)
            if(circles_overlap(&boulder->body, 0.75f, &game->players[p].body, 0.95f))
            {
                damage_player(game, &game->players[p]);
            }
    }
}

void check_pickup_bump(BBGame *game)
{
    for(int i = 0; i < BB_MAX_PICKUPS; ++i)
    {
        BBPickup *pickup = &game->pickups[i];
        if(!pickup->active)
            continue;
        for(int p = 0; p < BB_MAX_PLAYERS; ++p)
        {
            BBPlayer *player = &game->players[p];
            if(player->active && player->state == BB_PLAYER_NORMAL &&
               circles_overlap(&pickup->body, 0.95f, &player->body, 0.95f))
            {
                player->score += bb_game_pickup_score(pickup->type);
                pickup->active = false;
                game->events.pickup = true;
                game->round_pickups_collected++;

                if(!game->round_pickup_bonus_awarded && game->round_enemy_quota > 0 &&
                   game->round_pickups_collected == game->round_enemy_quota &&
                   bb_game_enemies_left(game) == 0)
                {
                    player->score += BB_ALL_PICKUPS_BONUS;
                    game->round_pickup_bonus_awarded = true;
                    game->bonus_timer = BB_BONUS_TEXT_SECONDS;
                    game->bonus_player = p;
                }
                break;
            }
        }
    }
}
