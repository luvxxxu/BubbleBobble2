#ifndef BUBBLE_H
#define BUBBLE_H

/* 게임 전체에서 쓰는 헤더 (상수, 구조체, 함수 선언)
 * main.c 입력/메인 반복문, game.c 게임 진행, player.c 플레이어, enemy.c 적/버블/바위/음식
 * level.c 맵/발판, world.c 충돌, score.c 순위표 파일, render.c 불러오기/그리기 도구
 * scene.c 맵/캐릭터 그리기, ui.c 메뉴/HUD */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "raylib.h"

#define BB_CLEAR_WAIT 5.0f // 과일이 떨어진 뒤 기다리는 시간(초)
#define BB_SCROLL_SECONDS 1.5f // 화면 올라가는 시간
#define BB_CLEAR_DURATION (BB_CLEAR_WAIT + BB_SCROLL_SECONDS) // 올라가는 시간까지
#define BB_MAP_WIDTH 32 // 맵 가로 (타일)
#define BB_MAP_HEIGHT 28 // 맵 세로
#define BB_WORLD_MAX_WIDTH 256 // 늘어난 맵 최대 가로
#define BB_WORLD_MAX_HEIGHT 256
#define BB_LEVEL_COUNT 5 // 라운드 수
#define BB_STARTING_LIVES 5 // 시작 목숨
#define BB_MAX_PLAYERS 2
#define BB_MAX_ENEMIES ((BB_LEVEL_COUNT * BB_WORLD_MAX_WIDTH + BB_MAP_WIDTH - 1) / BB_MAP_WIDTH) // 적 최대 수
#define BB_MAX_BOULDERS 32
#define BB_MAX_PICKUPS BB_MAX_ENEMIES
#define BB_PICKUP_SCORE 300 // 음식 하나 점수
#define BB_ALL_PICKUPS_BONUS 500 // 다 먹으면 보너스
#define BB_BONUS_TEXT_SECONDS 3.0f // 보너스 글자 시간(초)
#define BB_MAX_BUBBLES 64
#define BB_RESCUE_BUBBLE_RADIUS 1.75f // 부활 버블 크기
#define BB_RESCUE_BUBBLE_SECONDS 10.0 // 구조 제한 시간(초)
#define BB_FIXED_DT (1.0f / 120.0f) // 한 번 갱신하는 시간(초)
#define BB_PLAYER_GROUND_SPEED 6.0f // 타일/초
#define BB_PLAYER_AIR_SPEED 3.0f
#define BB_ENERGY_CHARGE_SECONDS 15.0f // 마나 채우는 시간
#define BB_ENERGY_BOOST_SECONDS 5.0f // 두 배 빨라지는 시간
#define BB_PLATFORM_TRAVEL 1.0f // 발판 이동 거리
#define BB_PLATFORM_MIN_SPEED 1.5f
#define BB_PLATFORM_MAX_SPEED 3.0f
#define BB_PLATFORM_FIRST_SOLID_SECONDS 20.0f // 첫 라운드 발판 유지 시간
#define BB_PLATFORM_ROUND_ACCELERATION 3.5f // 라운드마다 줄어드는 양
#define BB_PLATFORM_SOLID_VARIATION 2.0f
#define BB_PLATFORM_FIRST_WARNING_GAP_SECONDS 8.0
#define BB_PLATFORM_WARNING_GAP_REDUCTION 1.5
#define BB_PLATFORM_BLINK_SECONDS 0.2 // 깜빡임 한 번
#define BB_PLATFORM_BLINK_COUNT 3 // 깜빡이는 횟수
#define BB_PLATFORM_WARNING_SECONDS (BB_PLATFORM_BLINK_SECONDS * BB_PLATFORM_BLINK_COUNT * 2)
#define BB_PLATFORM_ABSENT_SECONDS 3.0 // 사라져 있는 시간
#define BB_MAX_PLATFORMS (((BB_MAP_WIDTH + 1) / 2) * (BB_MAP_HEIGHT - 2)) // 발판 최대 개수

// 게임 화면 상태
typedef enum BBState {
    BB_STATE_MENU,   /* 메뉴 */
    BB_STATE_INTRO,  /* READY 화면 */
    BB_STATE_PLAY,   /* 플레이 중 */
    BB_STATE_CLEAR,  /* 라운드 클리어 */
    BB_STATE_SCORE   /* 이름 입력 */
} BBState;
typedef enum BBPlayerState {
    BB_PLAYER_NORMAL,     /* 평소 */
    BB_PLAYER_DEAD,       /* 죽는 중 */
    BB_PLAYER_OUT,        /* 탈락 */
    BB_PLAYER_BUBBLE,     /* 버블에 갇힘 */
    BB_PLAYER_WAIT_ROUND  /* 다음 라운드 대기 */
} BBPlayerState;
typedef enum BBEnemyType {
    BB_ENEMY_ZENCHAN,
    BB_ENEMY_MAITA,
    BB_ENEMY_MONSTA,
    BB_ENEMY_TYPE_COUNT /* 종류 개수 */
} BBEnemyType;
typedef enum BBEnemyState {
    BB_ENEMY_SPAWNING, /* 등장 대기 */
    BB_ENEMY_WALKING,  /* 걷는 중 */
    BB_ENEMY_FALLING,  /* 떨어지는 중 */
    BB_ENEMY_JUMPING,  /* 점프 중 */
    BB_ENEMY_CAPTURED, /* 버블에 잡힘 */
    BB_ENEMY_DEAD      /* 죽음 */
} BBEnemyState;
typedef enum BBPickupType {
    BB_PICKUP_WATERMELON, BB_PICKUP_FRIES, BB_PICKUP_CHERRY,
    BB_PICKUP_STRAWBERRY, BB_PICKUP_PEACH, BB_PICKUP_ORANGE,
    BB_PICKUP_GRAPES, BB_PICKUP_BANANA, BB_PICKUP_PINEAPPLE,
    BB_PICKUP_LEMON, BB_PICKUP_APPLE, BB_PICKUP_PEAR,
    BB_PICKUP_RADISH, BB_PICKUP_CORN, BB_PICKUP_CARROT,
    BB_PICKUP_EGGPLANT, BB_PICKUP_ICE_CREAM, BB_PICKUP_CAKE,
    BB_PICKUP_DONUT, BB_PICKUP_BURGER, BB_PICKUP_TYPE_COUNT
} BBPickupType;

// 소리 재생용 이벤트
typedef struct BBEvents {
    bool fire;   /* 발사 */
    bool jump;   /* 점프 */
    bool pop;    /* 터짐 */
    bool death;  /* 맞음 */
    bool pickup; /* 음식 먹음 */
    bool level;  /* 새 라운드 */
} BBEvents;

// 위치는 타일 단위, 시간은 초
typedef struct BBBody {
    float x, y;      /* 위치 */
    float vx, vy;    /* 속도 */
    bool grounded;  /* 땅에 닿았는지 */
} BBBody;

typedef struct BBInput {
    float move;     /* 왼쪽 -1, 오른쪽 1 */
    bool jump;      /* 점프 눌림 */
    bool fire;      /* 발사 눌림 */
} BBInput;

// 플레이어 한 명의 정보
typedef struct BBPlayer {
    BBBody body;         /* 위치, 속도 */
    BBPlayerState state; /* 현재 상태 */
    int lives;           /* 남은 목숨 */
    int score;           /* 점수 */
    float invulnerable; /* 무적 남은 시간 */
    float death_timer;  /* 죽는 연출 시간 */
    float rescue_pop_timer; /* 구조 후 연출 시간 */
    double rescue_time_left; /* 구조 남은 시간 */
    double rescue_turn_timer; /* 방향 바꿀 때까지 */
    float rescue_drift_vx, rescue_drift_vy; /* 떠다니는 속도 */
    unsigned int rescue_rng; /* 떠다니는 방향 난수 */
    bool active;      /* 참가 여부 */
    bool air_control; /* 공중에서 움직일 수 있는지 */
    bool falling;     /* 느리게 떨어지는 중 */
    bool boosting;    /* 가속 중 */
    int facing; /* 왼쪽 -1, 오른쪽 1 */
    float fire_cooldown; /* 발사 대기 시간 */
    float attack_timer;  /* 공격 그림 시간 */
    float jump_start_y; /* 점프 시작 높이 */
    float energy; /* 마나 0~1 */
    double energy_elapsed; /* 충전 지난 시간 */
} BBPlayer;
// 발사한 버블
typedef struct BBBubble {
    BBBody body;   /* 위치, 속도 */
    bool active;  /* 사용 중인지 */
    bool popping; /* 터지는 중 */
    bool releasing; /* true면 적을 풀어줌 */
    int owner;    /* 발사한 플레이어 번호 */
    int captured_enemy; /* 잡은 적 번호, 없으면 -1 */
    float age;       /* 나온 지 지난 시간 */
    float pop_timer; /* 터지는 시간 */
} BBBubble;
typedef struct BBEnemy {
    BBBody body;         /* 위치, 속도 */
    BBEnemyType type;     /* 종류 */
    BBEnemyState state;   /* 상태 */
    bool active;         /* 사용 중인지 */
    bool angry;          /* 화났는지 */
    int facing;          /* 왼쪽 -1, 오른쪽 1 */
    float spawn_delay;   /* 등장 전 대기 시간 */
    float spawn_y;       /* 등장 높이 */
    float age;           /* 지난 시간 */
    int bounces;         /* 죽은 뒤 튕긴 횟수 */
    float bounce_timer;  /* 마지막 튕김 후 지난 시간 */
    float jump_timer;    /* 점프 후 지난 시간 */
    float turn_timer;    /* 방향 정한 후 지난 시간 */
    float throw_timer;   /* 바위 던진 후 지난 시간 */
} BBEnemy;
typedef struct BBBoulder {
    BBBody body; /* 위치, 속도 */
    bool active; /* 날아가는 중 */
    float age;   /* 지난 시간 */
} BBBoulder;
// 떨어진 음식
typedef struct BBPickup {
    BBBody body;        /* 위치, 속도 */
    BBPickupType type;  /* 음식 종류 */
    bool active;       /* 화면에 있는지 */
    float age;         /* 지난 시간 */
} BBPickup;

typedef enum BBPlatformState {
    BB_PLATFORM_SOLID,        /* 밟을 수 있음 */
    BB_PLATFORM_DISAPPEARING, /* 깜빡이는 중, 밟을 수 있음 */
    BB_PLATFORM_ABSENT,       /* 없음 */
    BB_PLATFORM_APPEARING    /* 깜빡이는 중, 통과함 */
} BBPlatformState;

typedef struct BBMovingPlatform {
    int x, y, width;          /* 기준 위치, 타일 수 */
    float offset;            /* 기준에서 움직인 거리 */
    float step;              /* 이번에 움직인 거리 */
    float speed;             /* 초당 이동 거리 */
    int direction;           /* 왼쪽 -1, 오른쪽 1 */
    BBPlatformState state;   /* 현재 단계 */
    double state_time;       /* 단계 지난 시간 */
    float solid_duration;    /* 유지 시간 */
    bool vanished;           /* 방금 사라졌는지 */
} BBMovingPlatform;

// 게임 데이터 전부
typedef struct BBGame {
    unsigned char maps[BB_LEVEL_COUNT][BB_MAP_HEIGHT][BB_MAP_WIDTH]; /* [라운드][행][열] */
    int world_width, world_height; /* 늘어난 맵 크기 */
    BBPlayer players[BB_MAX_PLAYERS];   /* 플레이어 */
    BBBubble bubbles[BB_MAX_BUBBLES];   /* 버블 */
    BBEnemy enemies[BB_MAX_ENEMIES];   /* 적 */
    BBBoulder boulders[BB_MAX_BOULDERS]; /* 바위 */
    BBPickup pickups[BB_MAX_PICKUPS];  /* 음식 */
    BBPickupType pickup_order[BB_PICKUP_TYPE_COUNT]; /* 섞은 음식 순서 */
    int pickup_cursor; /* 순서에서 현재 위치 */
    int level; /* 0부터 시작 */
    int round_enemy_quota; /* 이번 라운드 나온 적 수 */
    int round_pickups_collected; /* 먹은 음식 수 */
    bool round_pickup_bonus_awarded; /* 보너스 받았는지 */
    float bonus_timer; /* 보너스 글자 시간, 0이면 꺼짐 */
    int bonus_player;  /* 보너스 받은 플레이어 */
    bool won; /* 끝까지 깼는지 */
    int player_count; /* 1 또는 2 */
    BBState state;          /* 화면 상태 */
    float state_time;       /* 상태 지난 시간 */
    float level_time;       /* 라운드 지난 시간 */
    BBMovingPlatform platforms[BB_MAX_PLATFORMS]; /* 공중 발판 */
    int platform_count;    /* 쓰는 발판 수 */
    double platform_warning_cooldown; /* 다음 발판 사라질 때까지 */
    int platform_last_choice; /* 마지막 선택 번호 */
    unsigned long long ticks;   /* 갱신 횟수 */
    unsigned int rng;     /* 난수 값 */
    BBEvents events;  /* 이번에 생긴 이벤트 */
} BBGame;

// tiles가 NULL이면 빈 맵
void bb_game_init(BBGame *game, const unsigned char *tiles, unsigned int seed);
// 맵 크기 바꾸기 (32~256, 28~256)
void bb_game_resize(BBGame *game, int width, int height);
// 타일 값 가져오기, 맵 밖은 0
int bb_game_tile(const BBGame *game, int x, int y);
// 라운드 번호를 지정해서 가져오기
int bb_game_tile_for_level(const BBGame *game, int level, int x, int y);
// 새 게임 시작
void bb_game_start(BBGame *game, int player_count);
// 메뉴로 돌아감
void bb_game_menu(BBGame *game);
void bb_game_skip_intro(BBGame *game);
// 한 번 갱신, inputs는 NULL 가능
void bb_game_update(BBGame *game, const BBInput inputs[BB_MAX_PLAYERS], float dt);

// 발판이 움직인 양
float bb_game_tile_offset(const BBGame *game, int x, int y);
float bb_game_platform_travel(const BBGame *game);
// 밟을 수 있는지
bool bb_game_platform_solid(const BBMovingPlatform *platform);
bool bb_game_platform_blink_visible(const BBMovingPlatform *platform);

// 남은 적 수
int bb_game_enemies_left(const BBGame *game);
int bb_game_pickup_score(BBPickupType type);


// 부딪혔는지
typedef struct BBHits {
    bool x;
    bool y;
} BBHits;

/* ---- 맵 (level.c) ---- */
#define BB_LEVEL_PLATFORM_COUNT 5 // 라운드당 공중 발판 수

void bb_levels_build(unsigned char *maps);


/* ---- 화면 크기 맞춤 (level.c) ---- */
typedef struct BBViewport {
    int columns, rows; /* 타일 가로, 세로 수 */
    float scale; /* 확대 배율 */
    float offset_x, offset_y; /* 가운데 맞추는 위치 */
} BBViewport;

BBViewport bb_viewport_fit(int framebuffer_width, int framebuffer_height, float pixel_ratio);

/* 점수 줄(위)과 목숨 줄(아래)은 맵 밖의 검은 띠에 그림 (칸이 아니라 256x224 기준 픽셀 수) */
#define BB_HUD_TOP 24
#define BB_HUD_BOTTOM 12

// 위아래 검은 띠를 뺀 자리에 맵을 맞춤 (offset_y는 창 맨 위부터의 위치)
BBViewport bb_viewport_fit_hud(int framebuffer_width, int framebuffer_height, float pixel_ratio);


/* ---- 순위표 (score.c) ---- */
#define BB_SCORE_COUNT 10     /* 기록 개수 */
#define BB_SCORE_MAX_ROUND 5  /* 최고 라운드 */

typedef struct {
    unsigned int score; /* 점수 */
    int round;          /* 도달한 라운드 */
    char name[4];       /* 이름 3글자 + '\0' */
} BbScore;

// 파일 없으면 빈 순위표
bool bb_scores_load(const char *path, BbScore scores[BB_SCORE_COUNT], int *count);
void bb_scores_insert(BbScore scores[BB_SCORE_COUNT], int *count, BbScore score);
bool bb_scores_save(const char *path, const BbScore *scores, int count);

/* ---- 그리기와 소리 (render.c) ---- */
enum { BB_SCREEN_WIDTH = 256, BB_SCREEN_HEIGHT = 224 };
typedef enum BBMenu {
    BB_MENU_ONE_PLAYER, BB_MENU_TWO_PLAYERS, BB_MENU_LEADERBOARD, BB_MENU_COUNT
} BBMenu;
typedef struct BBAssets {
    Texture2D players[BB_MAX_PLAYERS]; /* 1P 초록, 2P 파랑 */
    Texture2D enemy, tiles, bubble, large_bubble, items;
    Texture2D platform_tiles; /* 발판 타일 */
    Texture2D backgrounds; /* 라운드별 배경 */
    Texture2D celestial; /* 해, 달, 행성 */
    Texture2D intro_background; /* 인트로(메뉴) 배경, 로고 포함 */
    Texture2D intro_sky; /* 로고, 해를 뺀 인트로 배경 (양옆 이어 붙이기용) */
    Texture2D blue_bubble; /* 2P 버블 */
    Texture2D blue_captured_enemy; /* 파란 포획 버블 */
    Font font; /* 글꼴 */
    Sound fire, jump, death; /* 효과음 */
    Music music; /* 배경음 */
    bool audio; /* 소리 준비됐는지 */
} BBAssets;
typedef struct BBUI {
    bool muted, paused; /* 음소거 / 일시정지 */
    int selected_players; /* 1 또는 2 */
    int menu_selection; /* 선택한 메뉴 */
    bool save_failed, entering_initials; /* 저장 실패 / 이름 입력 중 */
    bool score_recorded[BB_MAX_PLAYERS]; /* 점수 처리했는지 */
    int score_player, initial_cursor; /* 입력할 플레이어 / 글자 칸 */
    char initials[4]; /* 이름 3글자 + '\0' */
    BbScore scores[BB_SCORE_COUNT]; /* 순위표 */
    int score_count; /* 기록 개수 */
} BBUI;

// 그림 파일 다 있는지 확인
bool bb_assets_validate(const char *directory);
bool bb_assets_load(BBAssets *assets, const char *directory, bool audio);
void bb_assets_unload(BBAssets *assets);
void bb_draw_game(const BBAssets *assets, const BBGame *game, const BBUI *ui,
                  const BBViewport *viewport, int framebuffer_width, int framebuffer_height);
void bb_present_canvas(RenderTexture2D canvas, int framebuffer_width, int framebuffer_height);
void bb_audio_events(const BBAssets *assets, BBEvents events);


// 파일끼리 같이 쓰는 함수들
extern const Color player_colors[BB_MAX_PLAYERS]; /* 1P 초록, 2P 파랑 (render.c) */

/* game.c */
float clamp(float value, float low, float high);
float decrease(float value, float dt);
float random_from_state(unsigned int *state);
float random_unit(BBGame *game);
void shuffle_pickups(BBGame *game);

/* player.c */
void place_player(const BBGame *game, BBPlayer *player, int number);
void clamp_rescue_bubble(const BBGame *game, BBPlayer *player);
void damage_player(BBGame *game, BBPlayer *player);
void update_player(BBGame *game, BBPlayer *player, int index, BBInput input, float dt);
void check_rescue_bump(BBGame *game);
void check_bubble_bump(BBGame *game);
void check_boulder_bump(BBGame *game);
void check_pickup_bump(BBGame *game);

/* enemy.c */
void add_round_enemies(BBGame *game, bool expanding);
void update_enemy(BBGame *game, BBEnemy *enemy, int index, float dt);
void pop_bubble(BBGame *game, BBBubble *bubble, bool release);
void update_bubble(BBGame *game, BBBubble *bubble, float dt, float center_x, float center_y);
void update_boulder(BBGame *game, BBBoulder *boulder, float dt);
void update_pickup(BBGame *game, BBPickup *pickup, float dt);

/* level.c */
const BBMovingPlatform *platform_for_tile(const BBGame *game, int x, int y);
int tile_at(const BBGame *game, int x, int y);
void reset_platforms(BBGame *game);
void platform_bounds(const BBGame *game, int x, int width, float *minimum, float *maximum);
void update_platforms(BBGame *game, float dt);
float platform_carry(const BBGame *game, const BBBody *body, float half_width, float half_height);
void release_platform_rider(const BBGame *game, BBBody *body, float half_width, float half_height);

/* world.c */
int solid_tile_at(const BBGame *game, float x, int y, float *left);
bool rects_overlap(const BBBody *a, float a_half_width, float a_half_height, const BBBody *b, float b_half_width, float b_half_height);
bool circles_overlap(const BBBody *a, float a_radius, const BBBody *b, float b_radius);
BBHits move_body(const BBGame *game, BBBody *body, float half_width, float half_height, float dt, bool one_way, bool wrap);

/* render.c */
void text(const BBAssets *assets, const char *value, float x, float y, Color color);
void ui_panel(int x, int y, int width, int height, unsigned char alpha);
void centered(const BBAssets *assets, const char *value, float y, Color color);
void sprite(Texture2D sheet, int column, int row, int size, float x, float y, bool flip);
void large_bubble_sprite(const BBAssets *assets, int player, int frame, float x, float y);

/* scene.c */
int rescue_seconds(const BBPlayer *player);
void draw_intro_backdrop(const BBAssets *assets, const Camera2D *panel, int framebuffer_width, int framebuffer_height);
void draw_title_backdrop(const BBAssets *assets, const Camera2D *panel, int framebuffer_width, int framebuffer_height);
void draw_scenery(const BBAssets *assets, const BBGame *game, int level);
float round_scroll(const BBGame *game);
void draw_world(const BBAssets *assets, const BBGame *game);
void rescue_labels(const BBAssets *assets, const BBGame *game, const BBViewport *viewport, int framebuffer_width, int framebuffer_height, float hud_scale);

#endif
