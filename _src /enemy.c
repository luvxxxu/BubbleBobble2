/* 적, 버블, 바위, 음식 움직임
 * 적 종류는 젠짱, 마이타, 몬스타 */
#include "bubble.h"

// 적 하나를 등장 대기 상태로 초기화
static void spawn_enemy(BBGame *game, int index, BBEnemyType type, float x, float y, float delay)
{
    BBEnemy *enemy = &game->enemies[index];
    memset(enemy, 0, sizeof *enemy);
    enemy->body.x = x;
    enemy->body.y = (float)(game->world_height - BB_MAP_HEIGHT);
    enemy->type = type;
    enemy->state = BB_ENEMY_SPAWNING;
    enemy->active = true;
    enemy->facing = 1;
    if(random_unit(game) < 0.5f)
    {
        enemy->facing = -1;
    }
    enemy->spawn_delay = delay;
    enemy->spawn_y = y;
}

// 이번 라운드 적 수
static int round_enemy_target(const BBGame *game)
{
    return ((game->level + 1) * game->world_width + BB_MAP_WIDTH - 1) / BB_MAP_WIDTH;
}

// 라운드 시작할 때 적 추가
void add_round_enemies(BBGame *game, bool expanding)
{
    int target = round_enemy_target(game);
    int first = game->round_enemy_quota;
    int base_count = game->level + 1;
    for(int i = first; i < target; ++i)
    {
        // 추가된 적은 좌우로 번갈아 배치
        float offset = ((float)i - (float)(base_count - 1) * 0.5f) * 3.0f;
        if(i >= base_count)
        {
            int extra = i - base_count;
            float distance = ((float)(base_count + 1) * 0.5f + (float)(extra / 2)) * 3.0f;
            offset = distance;
            if(extra % 2 == 0)
            {
                offset = -distance;
            }
        }
        float x = clamp((float)game->world_width * 0.5f + offset, 3.0f, (float)game->world_width - 3.0f);
        float spawn_height = 4.0f;
        if(i % 2 == 0)
        {
            spawn_height = 8.0f;
        }
        float y = (float)(game->world_height - BB_MAP_HEIGHT) + spawn_height;
        BBEnemyType type = (BBEnemyType)((i + game->level) % BB_ENEMY_TYPE_COUNT);
        // 5마리씩 시간을 묶음
        float delay;
        if(expanding)
        {
            delay = 0.45f + (float)((i - first) % 5) * 0.15f;
        }
        else
        {
            delay = (float)(i % 5) * 0.45f;
        }
        spawn_enemy(game, i, type, x, y, delay);
    }
    if(target > game->round_enemy_quota)
    {
        game->round_enemy_quota = target;
    }
}

// 가장 가까운 플레이어 찾기
static const BBPlayer *closest_player(const BBGame *game, const BBBody *body)
{
    const BBPlayer *target = NULL;
    float nearest = 1.0e9f;
    for(int i = 0; i < BB_MAX_PLAYERS; ++i)
    {
        const BBPlayer *player = &game->players[i];
        if(!player->active || player->state != BB_PLAYER_NORMAL)
            continue;
        float dx = player->body.x - body->x;
        float dy = player->body.y - body->y;
        float distance = dx * dx + dy * dy;
        if(distance < nearest)
        {
            target = player;
            nearest = distance;
        }
    }
    return target;
}

// 위쪽에 올라갈 발판이 있는지 확인
static bool platform_above(const BBGame *game, const BBBody *body)
{
    int start = (int)floorf(body->y - 1.0f);
    for(int y = start; y > start - 5; --y)
    {
        float tile_left = 0.0f;
        if(solid_tile_at(game, body->x, y, &tile_left) != 0)
            return true;
    }
    return false;
}

// 마이타 바위 던지기
static void throw_boulder(BBGame *game, BBEnemy *enemy)
{
    for(int i = 0; i < BB_MAX_BOULDERS; ++i)
    {
        BBBoulder *boulder = &game->boulders[i];
        if(boulder->active)
            continue;
        float angle = random_unit(game) - 0.5f;
        memset(boulder, 0, sizeof *boulder);
        boulder->body.x = enemy->body.x;
        boulder->body.y = enemy->body.y;
        boulder->body.vx = cosf(angle) * 15.0f * (float)enemy->facing;
        boulder->body.vy = sinf(angle) * 15.0f;
        boulder->active = true;
        enemy->throw_timer = 0.0f;
        return;
    }
}

// 죽은 자리에 음식 만들기
static void spawn_pickup(BBGame *game, const BBEnemy *enemy)
{
    for(int i = 0; i < BB_MAX_PICKUPS; ++i)
    {
        BBPickup *pickup = &game->pickups[i];
        if(pickup->active)
            continue;
        if(game->pickup_cursor >= BB_PICKUP_TYPE_COUNT)
        {
            shuffle_pickups(game);
        }
        memset(pickup, 0, sizeof *pickup);
        pickup->body = enemy->body;
        pickup->active = true;
        pickup->type = game->pickup_order[game->pickup_cursor];
        game->pickup_cursor++;
        pickup->body.vx = 0.0f;
        pickup->body.vy = 9.0f;
        return;
    }
}

// 적 이동
void update_enemy(BBGame *game, BBEnemy *enemy, int index, float dt)
{
    if(!enemy->active || enemy->state == BB_ENEMY_CAPTURED)
        return;
    enemy->age += dt;
    if(enemy->state == BB_ENEMY_SPAWNING)
    {
        if(enemy->age <= enemy->spawn_delay)
            return;
        float progress = clamp((enemy->age - enemy->spawn_delay) / 2.0f, 0.0f, 1.0f);
        // 위에서 2초 동안 내려옴
        float origin_y = (float)(game->world_height - BB_MAP_HEIGHT);
        enemy->body.y = origin_y + (enemy->spawn_y - origin_y) * progress;
        if(progress >= 1.0f)
        {
            enemy->state = BB_ENEMY_FALLING;
        }
        return;
    }
    if(enemy->state == BB_ENEMY_DEAD)
    {
        // 날아가다가 벽이나 바닥에 부딪히면 튕김
        enemy->body.vy += 9.81f * dt;
        float vx = enemy->body.vx;
        float vy = enemy->body.vy;
        enemy->bounce_timer += dt;
        BBHits hit = move_body(game, &enemy->body, 0.95f, 0.95f, dt, true, true);
        // move_body가 속도를 0으로 만들기 전 값으로 반사
        if(hit.x)
            enemy->body.vx = -vx;
        if(hit.y)
            enemy->body.vy = -vy;
        // 0.5초 안에 또 닿은 건 같은 튕김으로 셈
        if((hit.x || hit.y) && enemy->bounce_timer >= 0.5f)
        {
            ++enemy->bounces;
            enemy->bounce_timer = 0.0f;
        }
        // 네 번 튕기거나 4초가 지나면 음식
        if(enemy->bounces > 3 || enemy->age > 4.0f)
        {
            spawn_pickup(game, enemy);
            enemy->active = false;
        }
        return;
    }
    // 종류별 능력치
    float speed = 4.5f;
    float jump_speed = -12.0f;
    float fall_speed = 8.0f;
    float jump_horizontal = 0.5f;
    float turn_delay = 2.0f + (float)(index % 4) * 0.5f;
    switch(enemy->type) {
        case BB_ENEMY_ZENCHAN:
            speed = 6.0f;
            jump_speed = -10.0f;
            fall_speed = 5.0f;
            jump_horizontal = 0.2f;
            break;
        case BB_ENEMY_MONSTA:
            jump_speed = -9.0f;
            fall_speed = 5.5f;
            jump_horizontal = 1.0f;
            turn_delay = 1.0f;
            break;
        case BB_ENEMY_MAITA:
        default:
            break;
    }
    enemy->jump_timer += dt;
    enemy->turn_timer += dt;
    enemy->throw_timer += dt;
    bool jump = false;
    const BBPlayer *target = closest_player(game, &enemy->body);
    if(target != NULL)
    {
        float dx = target->body.x - enemy->body.x;
        // 일정 시간마다 플레이어 쪽으로 방향 바꿈
        if(enemy->turn_timer >= turn_delay && fabsf(dx) > 5.0f)
        {
            enemy->facing = 1;
            if(dx < 0.0f)
            {
                enemy->facing = -1;
            }
            enemy->turn_timer = 0.0f;
        }
        jump = target->body.y < enemy->body.y - 1.0f && dx * (float)enemy->facing >= 0.0f &&
               enemy->jump_timer > 1.5f && platform_above(game, &enemy->body);
    }
    // 몬스타는 주기적으로 점프
    if(enemy->type == BB_ENEMY_MONSTA && enemy->jump_timer >= 1.1f)
    {
        jump = true;
    }
    float move = (float)enemy->facing;
    if(enemy->type == BB_ENEMY_MAITA && enemy->throw_timer >= 5.0f)
    {
        throw_boulder(game, enemy);
    }
    float multiplier = 1.0f;
    if(enemy->angry && enemy->type == BB_ENEMY_ZENCHAN)
    {
        multiplier = 1.8f;
    }
    // 땅에서는 걷고 공중에서는 중력
    if(enemy->body.grounded)
    {
        enemy->state = BB_ENEMY_WALKING;
        enemy->body.vx = move * speed * multiplier;
        enemy->body.vy = 9.81f * dt;
        if(jump)
        {
            enemy->body.vx *= jump_horizontal;
            enemy->body.vy = jump_speed;
            enemy->state = BB_ENEMY_JUMPING;
            enemy->body.grounded = false;
            enemy->jump_timer = 0.0f;
        }
    }
    else if(enemy->state == BB_ENEMY_JUMPING)
    {
        enemy->body.vy += 9.81f * dt;
    }
    else
    {
        enemy->state = BB_ENEMY_FALLING;
        enemy->body.vx = 0.0f;
        if(enemy->type == BB_ENEMY_MONSTA)
        {
            enemy->body.vx = move * speed;
        }
        enemy->body.vy = fall_speed * multiplier;
    }
    BBHits hit = move_body(game, &enemy->body, 0.9f, 0.95f, dt, true, true);
    if(hit.x)
    {
        // 벽에 부딪히면 방향 바꿈
        enemy->facing = -enemy->facing;
        enemy->turn_timer = 0.0f;
    }
}

// 거품 터뜨리기 (release면 적이 탈출)
void pop_bubble(BBGame *game, BBBubble *bubble, bool release)
{
    if(bubble->popping)
        return;
    bubble->popping = true;
    bubble->releasing = release;
    bubble->pop_timer = 0.25f;
    bubble->body.vx = 0.0f;
    bubble->body.vy = 0.0f;
    game->events.pop = true;
}

// 거품이 다 터지면 적 돌려놓기
static void finish_pop(BBGame *game, BBBubble *bubble)
{
    if(bubble->captured_enemy >= 0 && bubble->captured_enemy < BB_MAX_ENEMIES)
    {
        BBEnemy *enemy = &game->enemies[bubble->captured_enemy];
        enemy->body = bubble->body;
        enemy->body.grounded = false;
        enemy->age = 0.0f;
        // release면 적 풀어주고 아니면 죽음
        if(bubble->releasing)
        {
            enemy->state = BB_ENEMY_FALLING;
            if(enemy->type == BB_ENEMY_ZENCHAN)
            {
                enemy->angry = true;
            }
        }
        else
        {
            // 50~70도로 좌우 아무쪽이나 날아감
            float angle = (50.0f + 20.0f * random_unit(game)) * 0.01745329252f;
            float speed = 15.0f + 5.0f * random_unit(game);
            float direction = 1.0f;
            if(random_unit(game) < 0.5f)
                direction = -1.0f;
            enemy->state = BB_ENEMY_DEAD;
            enemy->body.vx = cosf(angle) * speed * direction;
            enemy->body.vy = -sinf(angle) * speed;
            enemy->bounce_timer = 0.0f;
            enemy->bounces = 0;
        }
    }
    bubble->active = false;
}

// 거품 갱신
void update_bubble(BBGame *game, BBBubble *bubble, float dt, float center_x, float center_y)
{
    if(!bubble->active)
        return;
    if(bubble->popping)
    {
        bubble->pop_timer -= dt;
        if(bubble->pop_timer <= 0.0f)
        {
            finish_pop(game, bubble);
        }
        return;
    }
    bubble->age += dt;
    // 적이 잡혀 있으면 수명 13초
    float lifetime = 8.0f;
    if(bubble->captured_enemy >= 0)
    {
        lifetime = 13.0f;
    }
    if(bubble->age > lifetime)
    {
        pop_bubble(game, bubble, true);
        return;
    }
    // 랜덤으로 흔들림
    bubble->body.vx += ((random_unit(game) * 2.0f - 1.0f) * 20.0f - bubble->body.vx) * dt;
    bubble->body.vy += ((random_unit(game) * 2.0f - 1.0f) * 20.0f - bubble->body.vy) * dt;
    if(bubble->age > 1.0f)
    {
        // 처음 1초 뒤부터 가운데로 모임
        bubble->body.vx += (center_x - bubble->body.x) * 0.3f * dt;
        bubble->body.vy += (center_y - bubble->body.y) * 0.3f * dt;
        if(bubble->body.y > (float)(game->world_height - BB_MAP_HEIGHT) + 11.0f)
        {
            bubble->body.vy -= 3.0f * dt;
        }
    }
    float vx = bubble->body.vx;
    float vy = bubble->body.vy;
    // 벽에 부딪히면 튕김
    BBHits hit = move_body(game, &bubble->body, 0.75f, 0.75f, dt, false, false);
    if(hit.x)
    {
        bubble->body.vx = -vx * 0.8f;
    }
    if(hit.y)
    {
        bubble->body.vy = -vy * 0.8f;
    }
    if(bubble->body.y < 0.75f)
    {
        // 위쪽 경계
        bubble->body.y = 0.75f;
        bubble->body.vy = fabsf(bubble->body.vy) * 0.8f;
    }
    if(bubble->body.y > (float)game->world_height + 1.0f)
    {
        bubble->body.y -= (float)game->world_height + 2.0f;
    }
}

// 바위 이동 (3초 뒤 사라짐)
void update_boulder(BBGame *game, BBBoulder *boulder, float dt)
{
    if(!boulder->active)
        return;
    boulder->age += dt;
    if(boulder->age >= 3.0f)
    {
        boulder->active = false;
        return;
    }
    boulder->body.vy += 9.81f * dt;
    float vx = boulder->body.vx;
    float vy = boulder->body.vy;
    BBHits hit = move_body(game, &boulder->body, 0.75f, 0.75f, dt, true, true);
    if(hit.x)
    {
        boulder->body.vx = -vx * 0.9f;
    }
    if(hit.y)
    {
        boulder->body.vy = -vy * 0.9f;
    }
}

// 음식 이동 (10초 뒤 사라짐)
void update_pickup(BBGame *game, BBPickup *pickup, float dt)
{
    if(!pickup->active)
        return;
    pickup->age += dt;
    if(pickup->age >= 10.0f)
    {
        pickup->active = false;
        return;
    }
    pickup->body.vx = 0.0f;
    pickup->body.vy = 9.0f;
    move_body(game, &pickup->body, 0.95f, 0.95f, dt, true, true);
}
