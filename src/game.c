

#include "bubble.h"

float clamp(float value, float low, float high)
{
    if(value < low)
        return low;
    if(value > high)
        return high;
    return value;
}

float decrease(float value, float dt)
{
    value -= dt;
    if(value < 0.0f)
        return 0.0f;
    return value;
}

float random_from_state(unsigned int *state)
{

    *state = *state * 1664525u + 1013904223u;
    return (float)(*state % 10000u) / 10000.0f;
}

float random_unit(BBGame *game)
{
    return random_from_state(&game->rng);
}

static void enter_level(BBGame *game, int level)
{
    game->level = level;
    game->level_time = 0.0f;
    reset_platforms(game);
    game->state_time = 0.0f;
    game->state = BB_STATE_PLAY;
    game->events.level = true;
    memset(game->enemies, 0, sizeof game->enemies);
    memset(game->bubbles, 0, sizeof game->bubbles);
    memset(game->boulders, 0, sizeof game->boulders);
    memset(game->pickups, 0, sizeof game->pickups);
    for(int i = 0; i < BB_MAX_PLAYERS; ++i)
    {
        BBPlayer *player = &game->players[i];

        if(game->player_count == 2 && player->active && player->state == BB_PLAYER_WAIT_ROUND)
            player->lives = 1;
        if(player->active && player->lives > 0)
        {
            place_player(game, player, i);
            player->invulnerable = 3.0f;
        }
    }
    game->round_enemy_quota = 0;
    game->round_pickups_collected = 0;
    game->round_pickup_bonus_awarded = false;
    game->bonus_timer = 0.0f;
    add_round_enemies(game, false);
}

void shuffle_pickups(BBGame *game)
{
    for(int i = 0; i < BB_PICKUP_TYPE_COUNT; ++i)
    {
        game->pickup_order[i] = (BBPickupType)i;
    }

    for(int i = BB_PICKUP_TYPE_COUNT - 1; i > 0; --i)
    {
        int j = (int)(random_unit(game) * (float)(i + 1));
        BBPickupType saved = game->pickup_order[i];
        game->pickup_order[i] = game->pickup_order[j];
        game->pickup_order[j] = saved;
    }
    game->pickup_cursor = 0;
}

void bb_game_init(BBGame *game, const unsigned char *tiles, unsigned int seed)
{
    memset(game, 0, sizeof *game);
    game->world_width = BB_MAP_WIDTH;
    game->world_height = BB_MAP_HEIGHT;
    if(tiles != NULL)
    {
        memcpy(game->maps, tiles, sizeof game->maps);
    }
    game->rng = seed;
    game->player_count = 1;
    game->state = BB_STATE_MENU;
}

void bb_game_start(BBGame *game, int player_count)
{
    if(player_count < 1)
    {
        player_count = 1;
    }
    if(player_count > BB_MAX_PLAYERS)
    {
        player_count = BB_MAX_PLAYERS;
    }
    game->player_count = player_count;
    memset(game->players, 0, sizeof game->players);
    memset(game->enemies, 0, sizeof game->enemies);
    memset(game->bubbles, 0, sizeof game->bubbles);
    memset(game->boulders, 0, sizeof game->boulders);
    memset(game->pickups, 0, sizeof game->pickups);
    game->state = BB_STATE_INTRO;
    game->state_time = 0.0f;
    game->level_time = 0.0f;
    memset(game->platforms, 0, sizeof game->platforms);
    game->platform_count = 0;
    game->ticks = 0;
    game->platform_warning_cooldown = 0.0;
    game->platform_last_choice = 0;
    memset(&game->events, 0, sizeof game->events);
    game->level = 0;
    game->round_enemy_quota = 0;
    game->round_pickups_collected = 0;
    game->round_pickup_bonus_awarded = false;
    game->bonus_timer = 0.0f;
    game->won = false;
    shuffle_pickups(game);
    for(int i = 0; i < BB_MAX_PLAYERS; ++i)
    {
        BBPlayer *player = &game->players[i];

        place_player(game, player, i);
        player->active = i < player_count;
        if(player->active)
        {
            player->lives = BB_STARTING_LIVES;
        }
    }
}

void bb_game_menu(BBGame *game)
{
    game->state = BB_STATE_MENU;
    game->state_time = 0.0f;
    memset(&game->events, 0, sizeof game->events);
}

void bb_game_skip_intro(BBGame *game)
{
    if(game->state == BB_STATE_INTRO)
    {
        enter_level(game, 0);
    }
}

int bb_game_enemies_left(const BBGame *game)
{
    int count = 0;
    for(int i = 0; i < BB_MAX_ENEMIES; ++i)
        if(game->enemies[i].active && game->enemies[i].state != BB_ENEMY_DEAD)
        {
            ++count;
        }
    return count;
}

int bb_game_pickup_score(BBPickupType type)
{
    if(type < 0 || type >= BB_PICKUP_TYPE_COUNT)
        return 0;
    return BB_PICKUP_SCORE;
}

static bool enemy_flying(const BBGame *game)
{
    for(int i = 0; i < BB_MAX_ENEMIES; ++i)
        if(game->enemies[i].active && game->enemies[i].state == BB_ENEMY_DEAD)
            return true;
    return false;
}

static void next_round_or_win(BBGame *game)
{
    if(game->level + 1 < BB_LEVEL_COUNT)
    {
        enter_level(game, game->level + 1);
    }
    else
    {
        game->state = BB_STATE_SCORE;
        game->state_time = 0.0f;
        game->won = true;
    }
}

static void check_collisions_and_progress(BBGame *game)
{
    check_rescue_bump(game);
    check_bubble_bump(game);
    for(int e = 0; e < BB_MAX_ENEMIES; ++e)
    {
        BBEnemy *enemy = &game->enemies[e];
        if(!enemy->active || enemy->state == BB_ENEMY_SPAWNING || enemy->state == BB_ENEMY_CAPTURED || enemy->state == BB_ENEMY_DEAD)
            continue;
        for(int p = 0; p < BB_MAX_PLAYERS; ++p)
            if(rects_overlap(&enemy->body, 0.9f, 0.95f,
                             &game->players[p].body, 0.9f, 0.975f))
                damage_player(game, &game->players[p]);
    }
    check_boulder_bump(game);
    check_pickup_bump(game);

    bool living_player = false;
    bool has_lives = false;

    for(int i = 0; i < game->player_count; ++i) {
        if(!game->players[i].active)
            continue;
        if(game->players[i].state == BB_PLAYER_NORMAL || game->players[i].state == BB_PLAYER_DEAD)
        {
            living_player = true;
        }
        if(game->players[i].lives > 0)
        {
            has_lives = true;
        }
    }
    if(!living_player)
    {
        for(int i = 0; i < game->player_count; ++i)
            if(game->players[i].state == BB_PLAYER_BUBBLE || game->players[i].state == BB_PLAYER_WAIT_ROUND)
            {
                game->players[i].state = BB_PLAYER_OUT;
            }
        game->state = BB_STATE_SCORE;
        game->state_time = 0.0f;
        game->won = false;
        return;
    }

    if(!has_lives)
        return;

    if(game->state == BB_STATE_PLAY && bb_game_enemies_left(game) == 0)
    {
        game->state = BB_STATE_CLEAR;
        game->state_time = 0.0f;
        }
    else if(game->state == BB_STATE_CLEAR)
    {

        float end = BB_CLEAR_DURATION;
        if(game->level + 1 >= BB_LEVEL_COUNT)
            end = BB_CLEAR_WAIT;
        if(enemy_flying(game))
            game->state_time = 0.0f;
        else if(game->state_time >= end)
            next_round_or_win(game);
    }
}

static void step_game(BBGame *game, const BBInput inputs[BB_MAX_PLAYERS], float dt)
{
    game->state_time += dt;
    if(game->state == BB_STATE_INTRO)
    {

        if(game->state_time >= 7.0f)
        {
            enter_level(game, 0);
        }
        return;
    }
    game->level_time += dt;
    game->bonus_timer = decrease(game->bonus_timer, dt);

    update_platforms(game, dt);

    for(int i = 0; i < game->player_count; ++i)
        if(game->players[i].active)
        {
            release_platform_rider(game, &game->players[i].body, 0.9f, 0.975f);
        }
    for(int i = 0; i < BB_MAX_ENEMIES; ++i)
    {
        if(game->enemies[i].active)
        {
            float half_width = 0.9f;
            if(game->enemies[i].state == BB_ENEMY_DEAD)
            {
                half_width = 0.95f;
            }
            release_platform_rider(game, &game->enemies[i].body, half_width, 0.95f);
        }
    }
    for(int i = 0; i < BB_MAX_BOULDERS; ++i)
        if(game->boulders[i].active)
        {
            release_platform_rider(game, &game->boulders[i].body, 0.75f, 0.75f);
        }
    for(int i = 0; i < BB_MAX_PICKUPS; ++i)
        if(game->pickups[i].active)
        {
            release_platform_rider(game, &game->pickups[i].body, 0.95f, 0.95f);
        }
    for(int i = 0; i < game->player_count; ++i)
    {
        BBInput input = {0};
        if(inputs != NULL)
        {
            input = inputs[i];
        }
        update_player(game, &game->players[i], i, input, dt);
    }
    for(int i = 0; i < BB_MAX_ENEMIES; ++i)
    {
        update_enemy(game, &game->enemies[i], i, dt);
    }
    float center_x = 0.0f;
    float center_y = 0.0f;
    int bubbles = 0;

    for(int i = 0; i < BB_MAX_BUBBLES; ++i)
    {
        if(game->bubbles[i].active && !game->bubbles[i].popping)
        {
            center_x += game->bubbles[i].body.x;
            center_y += game->bubbles[i].body.y;
            ++bubbles;
        }
    }
    if(bubbles > 0)
    {
        center_x /= (float)bubbles;
        center_y /= (float)bubbles;
    }
    for(int i = 0; i < BB_MAX_BUBBLES; ++i)
    {
        update_bubble(game, &game->bubbles[i], dt, center_x, center_y);
    }
    for(int i = 0; i < BB_MAX_BOULDERS; ++i)
    {
        update_boulder(game, &game->boulders[i], dt);
    }
    for(int i = 0; i < BB_MAX_PICKUPS; ++i)
    {
        update_pickup(game, &game->pickups[i], dt);
    }

    check_collisions_and_progress(game);
}

void bb_game_update(BBGame *game, const BBInput inputs[BB_MAX_PLAYERS], float dt)
{
    memset(&game->events, 0, sizeof game->events);
    if(dt <= 0.0f || dt > (1.0f / 30.0f) ||
       game->state == BB_STATE_MENU || game->state == BB_STATE_SCORE)
        return;
    ++game->ticks;
    int steps = 1;
    if(game->world_width > BB_MAP_WIDTH && dt > BB_FIXED_DT)
    {
        steps = (int)ceilf(dt / BB_FIXED_DT);
    }
    float step_dt = dt / (float)steps;
    BBInput held_inputs[BB_MAX_PLAYERS] = {0};
    if(inputs != NULL)
        for(int i = 0; i < BB_MAX_PLAYERS; ++i)
        {
            held_inputs[i].move = inputs[i].move;
        }
    for(int step = 0; step < steps; ++step)
    {
        // 점프, 발사는 첫 번째에만
        float elapsed = step_dt;
        if(step + 1 == steps)
        {
            elapsed = dt - step_dt * (float)step;
        }
        step_game(game, step == 0 ? inputs : held_inputs, elapsed);
        if(game->state == BB_STATE_MENU || game->state == BB_STATE_SCORE)
            break;
    }
}
