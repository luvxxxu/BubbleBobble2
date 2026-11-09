/* 맵 타일 만들기, 타일 읽기, 움직이는 발판 처리
 * game은 게임 전체 구조체의 주소 */
#include "bubble.h"

// 발판 한 줄 (왼쪽 칸, 윗면 칸, 타일 개수)
typedef struct BBPlatformSpan {
    int x;     // 왼쪽 끝 칸
    int y;     // 윗면 칸
    int width; // 타일 개수
} BBPlatformSpan;

// 라운드별 발판 배치, 낮은 발판부터
static const BBPlatformSpan layouts[BB_LEVEL_COUNT][BB_LEVEL_PLATFORM_COUNT] = {
    {{3, 22, 10}, {9, 18, 10}, {17, 14, 10}, {10, 10, 10}, {4, 6, 10}},
    {{19, 22, 10}, {13, 18, 10}, {5, 14, 10}, {12, 10, 10}, {18, 6, 10}},
    {{10, 22, 12}, {4, 18, 10}, {11, 14, 10}, {18, 10, 10}, {10, 6, 12}},
    {{4, 22, 10}, {11, 18, 10}, {18, 14, 10}, {11, 10, 10}, {4, 6, 10}},
    {{18, 22, 10}, {11, 18, 10}, {4, 14, 10}, {11, 10, 10}, {18, 6, 10}}
};

// 모든 라운드 맵 채우기 (0 빈칸, 1 장식, 2 발판, 3 벽)
void bb_levels_build(unsigned char *maps)
{
    // 일단 전부 0으로
    memset(maps, 0, BB_LEVEL_COUNT * BB_MAP_HEIGHT * BB_MAP_WIDTH);
    for (int level = 0; level < BB_LEVEL_COUNT; ++level) {
        // 이번 라운드 맵의 시작 주소
        unsigned char *map = maps + level * BB_MAP_HEIGHT * BB_MAP_WIDTH;
        // 양옆 벽
        for (int y = 2; y < BB_MAP_HEIGHT; ++y) {
            map[y * BB_MAP_WIDTH] = 3;
            map[y * BB_MAP_WIDTH + 1] = 3;
            map[y * BB_MAP_WIDTH + 30] = 3;
            map[y * BB_MAP_WIDTH + 31] = 3;
        }
        // 바닥 (가운데 네 칸은 구멍)
        for (int x = 2; x < BB_MAP_WIDTH - 2; ++x) {
            if (x >= 14 && x < 18) {
                continue;
            }
            map[26 * BB_MAP_WIDTH + x] = 2;
            map[27 * BB_MAP_WIDTH + x] = 1;
        }
        for (int i = 0; i < BB_LEVEL_PLATFORM_COUNT; ++i) {
            const BBPlatformSpan *span = &layouts[level][i];
            for (int x = span->x; x < span->x + span->width; ++x) {
                map[span->y * BB_MAP_WIDTH + x] = 2;
            }
        }
    }
}

// 라운드 번호를 받아서 타일 읽기
int bb_game_tile_for_level(const BBGame *game, int level, int x, int y)
{
    if(level < 0 || level >= BB_LEVEL_COUNT)
        return 0;
    // 맵 밖은 빈칸
    if(x < 0 || x >= game->world_width || y < 0 || y >= game->world_height)
        return 0;
    if(game->world_width == BB_MAP_WIDTH && game->world_height == BB_MAP_HEIGHT)
        return game->maps[level][y][x];

    int offset_x = (game->world_width - BB_MAP_WIDTH) / 2;
    int offset_y = game->world_height - BB_MAP_HEIGHT;
    int source_x = x - offset_x;
    int source_y = y - offset_y;
    if(x < 2 || x >= game->world_width - 2)
    {
        // 화면이 넓어졌을 때 옆 벽
        int wall_x = x;
        if(x >= 2)
        {
            wall_x = BB_MAP_WIDTH - (game->world_width - x);
        }
        int wall_y = y;
        if(y >= 2)
        {
            wall_y = source_y;
            if(source_y < 2)
            {
                wall_y = 2;
            }
        }
        return game->maps[level][wall_y][wall_x];
    }
    if(source_y < 0)
        return 0;
    if(source_x >= 2 && source_x < BB_MAP_WIDTH - 2)
        return game->maps[level][source_y][source_x];
    if(y >= game->world_height - 2)
    {
        int floor_x = 2;
        if(source_x >= 2)
        {
            floor_x = BB_MAP_WIDTH - 3;
        }
        return game->maps[level][source_y][floor_x];
    }
    return 0;
}

// 지금 라운드 타일 읽기
int bb_game_tile(const BBGame *game, int x, int y)
{
    return bb_game_tile_for_level(game, game->level, x, y);
}

// 이 칸에 해당하는 발판 찾기
const BBMovingPlatform *platform_for_tile(const BBGame *game, int x, int y)
{
    for(int i = 0; i < game->platform_count; ++i)
    {
        const BBMovingPlatform *platform = &game->platforms[i];
        if(platform->y == y && x >= platform->x && x < platform->x + platform->width)
            return platform;
    }
    return NULL;
}

// 밟을 수 있는 발판인가
bool bb_game_platform_solid(const BBMovingPlatform *platform)
{
    return platform != NULL &&
           (platform->state == BB_PLATFORM_SOLID || platform->state == BB_PLATFORM_DISAPPEARING);
}

// 깜빡일 때 지금 보이는 차례인가
bool bb_game_platform_blink_visible(const BBMovingPlatform *platform)
{
    if(platform == NULL || (platform->state != BB_PLATFORM_DISAPPEARING &&
                            platform->state != BB_PLATFORM_APPEARING) ||
       platform->state_time >= BB_PLATFORM_WARNING_SECONDS)
        return false;
    int interval = (int)(platform->state_time / BB_PLATFORM_BLINK_SECONDS);
    return interval % 2 == 0;
}

// 타일 읽기, 사라진 발판은 빈칸 취급
int tile_at(const BBGame *game, int x, int y)
{
    int tile = bb_game_tile(game, x, y);
    if(tile == 2 && y < game->world_height - 2)
    {
        const BBMovingPlatform *platform = platform_for_tile(game, x, y);
        if(platform != NULL && !bb_game_platform_solid(platform))
            return 0;
    }
    return tile;
}

// 맵에서 이어진 발판을 찾아서 등록
void reset_platforms(BBGame *game)
{
    memset(game->platforms, 0, sizeof game->platforms);
    game->platform_count = 0;
    game->platform_warning_cooldown = 0.0;
    game->platform_last_choice = 0;
    for(int y = 0; y < BB_MAP_HEIGHT - 2; ++y)
    {
        for(int x = 0; x < BB_MAP_WIDTH; ++x)
        {
            if(game->maps[game->level][y][x] != 2)
                continue;
            int first = x;
            while(x + 1 < BB_MAP_WIDTH && game->maps[game->level][y][x + 1] == 2)
            {
                ++x;
            }
            // 속도는 랜덤
            float speed = BB_PLATFORM_MIN_SPEED + random_unit(game) *
                          (BB_PLATFORM_MAX_SPEED - BB_PLATFORM_MIN_SPEED);
            // 라운드가 갈수록 빨리 사라짐
            float duration = BB_PLATFORM_FIRST_SOLID_SECONDS -
                             BB_PLATFORM_ROUND_ACCELERATION * (float)game->level +
                             random_unit(game) * BB_PLATFORM_SOLID_VARIATION;
            // 발판 정보 적기
            BBMovingPlatform *platform = &game->platforms[game->platform_count];
            platform->x = first + (game->world_width - BB_MAP_WIDTH) / 2;
            platform->y = y + game->world_height - BB_MAP_HEIGHT;
            platform->width = x - first + 1;
            platform->speed = speed;
            // 시작 위치와 방향도 랜덤 (처음부터 제각각 움직이게)
            float minimum, maximum;
            platform_bounds(game, platform->x, platform->width, &minimum, &maximum);
            platform->offset = minimum + random_unit(game) * (maximum - minimum);
            platform->direction = 1;
            if(random_unit(game) < 0.5f)
            {
                platform->direction = -1;
            }
            platform->state = BB_PLATFORM_SOLID;
            platform->solid_duration = duration;
            game->platform_count++;
        }
    }
}

// 그 칸 발판이 얼마나 움직였는지
float bb_game_tile_offset(const BBGame *game, int x, int y)
{
    if(y < 0 || y >= game->world_height - 2 || bb_game_tile(game, x, y) != 2)
        return 0.0f;
    const BBMovingPlatform *platform = platform_for_tile(game, x, y);
    if(platform == NULL)
        return 0.0f;
    return platform->offset;
}

// 발판이 왔다갔다하는 거리
float bb_game_platform_travel(const BBGame *game)
{
    float extra_travel = fmaxf(0.0f, (float)(game->world_width - BB_MAP_WIDTH)) * 0.5f;
    return BB_PLATFORM_TRAVEL + extra_travel;
}

// 발판이 벽 안에서 움직일 범위 구하기
void platform_bounds(const BBGame *game, int x, int width, float *minimum, float *maximum)
{
    float travel = bb_game_platform_travel(game);
    *minimum = fmaxf(-travel, 2.0f - (float)x);
    *maximum = fminf(travel, (float)(game->world_width - 2 - x - width));
    if(*minimum > *maximum)
    {
        *minimum = 0.0f;
        *maximum = 0.0f;
    }
}

// 발판 상태 시간 갱신 (깜빡임 > 사라짐 > 다시 생김)
static void update_platform_state(BBMovingPlatform *platform, float dt)
{
    bool was_solid = bb_game_platform_solid(platform);
    platform->state_time += (double)dt;
    while(platform->state != BB_PLATFORM_SOLID)
    {
        double duration = BB_PLATFORM_WARNING_SECONDS;
        if(platform->state == BB_PLATFORM_ABSENT)
        {
            duration = BB_PLATFORM_ABSENT_SECONDS;
        }
        if(platform->state_time < duration)
            break;
        platform->state_time -= duration;
        switch(platform->state)
        {
            case BB_PLATFORM_SOLID:
                break;
            case BB_PLATFORM_DISAPPEARING:
                platform->state = BB_PLATFORM_ABSENT;
                break;
            case BB_PLATFORM_ABSENT:
                platform->state = BB_PLATFORM_APPEARING;
                break;
            case BB_PLATFORM_APPEARING:
                platform->state = BB_PLATFORM_SOLID;
                break;
        }
    }
    platform->vanished = was_solid && !bb_game_platform_solid(platform);
}

static double platform_solid_duration(const BBMovingPlatform *platform)
{
    return platform->solid_duration > 0.0f ? platform->solid_duration : BB_PLATFORM_FIRST_SOLID_SECONDS;
}

// 다음에 사라질 발판 고르기
static void schedule_platform_disappearance(BBGame *game, float dt)
{
    if(game->platform_count == 0)
        return;
    // 동시에 사라질 수 있는 개수
    int limit = 1 + game->level / 2;
    int available = 1;
    if(game->platform_count > 2)
    {
        available = game->platform_count - 2;
    }
    if(limit > available)
    {
        limit = available;
    }
    int cycling = 0;
    for(int i = 0; i < game->platform_count; ++i)
        if(game->platforms[i].state != BB_PLATFORM_SOLID)
        {
            ++cycling;
        }
    if(cycling >= limit || game->platform_warning_cooldown > 0.0)
        return;
    // 사라질 때가 된 발판 모으기
    int candidates[BB_MAX_PLATFORMS];
    int candidate_count = 0;
    for(int index = 0; index < game->platform_count; ++index)
    {
        const BBMovingPlatform *platform = &game->platforms[index];
        if(platform->state == BB_PLATFORM_SOLID && platform->state_time >= platform_solid_duration(platform))
        {
            candidates[candidate_count++] = index;
        }
    }
    if(candidate_count == 0)
        return;
    // 그중 하나 랜덤
    int selected = (int)(random_unit(game) * (float)candidate_count);
    int index = candidates[selected];
    BBMovingPlatform *platform = &game->platforms[index];
    double overdue = platform->state_time - platform_solid_duration(platform);
    platform->state = BB_PLATFORM_DISAPPEARING;
    platform->state_time = 0.0;
    if(overdue < (double)dt)
    {
        platform->state_time = overdue;
    }
    // 다음 발판이 사라질 때까지 간격
    game->platform_warning_cooldown = BB_PLATFORM_FIRST_WARNING_GAP_SECONDS -
                                      BB_PLATFORM_WARNING_GAP_REDUCTION * (double)game->level;
    game->platform_last_choice = index;
}

// 발판 위치 갱신
void update_platforms(BBGame *game, float dt)
{
    game->platform_warning_cooldown = fmax(0.0, game->platform_warning_cooldown - (double)dt);
    for(int i = 0; i < game->platform_count; ++i)
    {
        BBMovingPlatform *platform = &game->platforms[i];
        float old_offset = platform->offset;
        float minimum, maximum;
        platform_bounds(game, platform->x, platform->width, &minimum, &maximum);
        platform->offset = clamp(platform->offset, minimum, maximum);
        if(minimum < maximum)
        {
            platform->offset += (float)platform->direction * platform->speed * dt;
        }
        // 끝에 닿으면 방향 바꿈
        if(platform->offset >= maximum)
        {
            platform->offset = 2.0f * maximum - platform->offset;
            platform->direction = -1;
        }
        else if(platform->offset <= minimum)
        {
            platform->offset = 2.0f * minimum - platform->offset;
            platform->direction = 1;
        }
        platform->step = platform->offset - old_offset;
        update_platform_state(platform, dt);
    }
    schedule_platform_disappearance(game, dt);
}

// 발판 위에 서 있으면 발판이 움직인 만큼 같이 옮기기
float platform_carry(const BBGame *game, const BBBody *body, float half_width, float half_height)
{
    if(!body->grounded || body->vy < 0.0f)
        return 0.0f;
    float feet = body->y + half_height;
    int row = (int)floorf(feet + 0.001f);
    if(row < 0 || row >= game->world_height - 2 || fabsf(feet - (float)row) > 0.001f)
        return 0.0f;
    // 발판 두 개에 걸치면 더 많이 겹친 쪽을 따라감
    float carry = 0.0f;
    float best_overlap = 0.0001f;
    for(int i = 0; i < game->platform_count; ++i)
    {
        const BBMovingPlatform *platform = &game->platforms[i];
        if(platform->y != row || !bb_game_platform_solid(platform))
            continue;
        float left = (float)platform->x + platform->offset - platform->step;
        float right = left + (float)platform->width;
        float overlap = fminf(body->x + half_width, right) - fmaxf(body->x - half_width, left);
        if(overlap > best_overlap)
        {
            best_overlap = overlap;
            carry = platform->step;
        }
    }
    return carry;
}

// 발판이 막 사라졌으면 그 위에 있던 몸을 떨어지게 하기
void release_platform_rider(const BBGame *game, BBBody *body, float half_width, float half_height)
{
    if(!body->grounded || body->vy < 0.0f)
        return;
    float feet = body->y + half_height;
    int row = (int)floorf(feet + 0.001f);
    if(fabsf(feet - (float)row) > 0.001f)
        return;
    bool lost_support = false;
    for(int i = 0; i < game->platform_count; ++i)
    {
        const BBMovingPlatform *platform = &game->platforms[i];
        if(platform->y != row)
            continue;
        float left = (float)platform->x + platform->offset - platform->step;
        float right = left + (float)platform->width;
        if(body->x + half_width <= left + 0.0001f || body->x - half_width >= right - 0.0001f)
            continue;
        if(bb_game_platform_solid(platform))
            return;
        lost_support = lost_support || platform->vanished;
    }
    if(!lost_support)
        return;
    // 벽 위에 서 있으면 그대로
    for(int x = (int)floorf(body->x - half_width + 0.0001f);
        x <= (int)floorf(body->x + half_width - 0.0001f); ++x)
        if(tile_at(game, x, row) == 3)
            return;
    body->grounded = false;
}
