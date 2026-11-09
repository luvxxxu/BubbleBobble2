#ifndef BUBBLE_H
#define BUBBLE_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "raylib.h"

#define BB_CLEAR_WAIT 5.0f
#define BB_SCROLL_SECONDS 1.5f
#define BB_CLEAR_DURATION (BB_CLEAR_WAIT + BB_SCROLL_SECONDS)
#define BB_MAP_WIDTH 32
#define BB_MAP_HEIGHT 28
#define BB_WORLD_MAX_WIDTH 256
#define BB_WORLD_MAX_HEIGHT 256
#define BB_LEVEL_COUNT 5
#define BB_STARTING_LIVES 5
#define BB_MAX_PLAYERS 2
#define BB_MAX_ENEMIES ((BB_LEVEL_COUNT * BB_WORLD_MAX_WIDTH + BB_MAP_WIDTH - 1) / BB_MAP_WIDTH)
#define BB_MAX_BOULDERS 32
#define BB_MAX_PICKUPS BB_MAX_ENEMIES
#define BB_PICKUP_SCORE 300
#define BB_ALL_PICKUPS_BONUS 500
#define BB_BONUS_TEXT_SECONDS 3.0f
#define BB_MAX_BUBBLES 64
#define BB_RESCUE_BUBBLE_RADIUS 1.75f
#define BB_RESCUE_BUBBLE_SECONDS 10.0
#define BB_FIXED_DT (1.0f / 120.0f)
#define BB_PLAYER_GROUND_SPEED 6.0f
#define BB_PLAYER_AIR_SPEED 3.0f
#define BB_ENERGY_CHARGE_SECONDS 15.0f
#define BB_ENERGY_BOOST_SECONDS 5.0f
#define BB_PLATFORM_TRAVEL 1.0f
#define BB_PLATFORM_MIN_SPEED 1.5f
#define BB_PLATFORM_MAX_SPEED 3.0f
#define BB_PLATFORM_FIRST_SOLID_SECONDS 20.0f
#define BB_PLATFORM_ROUND_ACCELERATION 3.5f
#define BB_PLATFORM_SOLID_VARIATION 2.0f
#define BB_PLATFORM_FIRST_WARNING_GAP_SECONDS 8.0
#define BB_PLATFORM_WARNING_GAP_REDUCTION 1.5
#define BB_PLATFORM_BLINK_SECONDS 0.2
#define BB_PLATFORM_BLINK_COUNT 3
#define BB_PLATFORM_WARNING_SECONDS (BB_PLATFORM_BLINK_SECONDS * BB_PLATFORM_BLINK_COUNT * 2)
#define BB_PLATFORM_ABSENT_SECONDS 3.0
#define BB_MAX_PLATFORMS (((BB_MAP_WIDTH + 1) / 2) * (BB_MAP_HEIGHT - 2))

typedef enum BBState {
    BB_STATE_MENU,
    BB_STATE_INTRO,
    BB_STATE_PLAY,
    BB_STATE_CLEAR,
    BB_STATE_SCORE
} BBState;
typedef enum BBPlayerState {
    BB_PLAYER_NORMAL,
    BB_PLAYER_DEAD,
    BB_PLAYER_OUT,
    BB_PLAYER_BUBBLE,
    BB_PLAYER_WAIT_ROUND
} BBPlayerState;
typedef enum BBEnemyType {
    BB_ENEMY_ZENCHAN,
    BB_ENEMY_MAITA,
    BB_ENEMY_MONSTA,
    BB_ENEMY_TYPE_COUNT
} BBEnemyType;
typedef enum BBEnemyState {
    BB_ENEMY_SPAWNING,
    BB_ENEMY_WALKING,
    BB_ENEMY_FALLING,
    BB_ENEMY_JUMPING,
    BB_ENEMY_CAPTURED,
    BB_ENEMY_DEAD
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

typedef struct BBEvents {
    bool fire;
    bool jump;
    bool pop;
    bool death;
    bool pickup;
    bool level;
} BBEvents;

// 위치는 타일 단위, 시간은 초
typedef struct BBBody {
    float x, y;
    float vx, vy;
    bool grounded;
} BBBody;

typedef struct BBInput {
    float move;     /* 왼쪽 -1, 오른쪽 1 */
    bool jump;
    bool fire;
} BBInput;

typedef struct BBPlayer {
    BBBody body;
    BBPlayerState state;
    int lives;
    int score;
    float invulnerable;
    float death_timer;
    float rescue_pop_timer;
    double rescue_time_left;
    double rescue_turn_timer;
    float rescue_drift_vx, rescue_drift_vy;
    unsigned int rescue_rng;
    bool active;
    bool air_control;
    bool falling;
    bool boosting;
    int facing; /* 왼쪽 -1, 오른쪽 1 */
    float fire_cooldown;
    float attack_timer;
    float jump_start_y;
    float energy; /* 마나 0~1 */
    double energy_elapsed;
} BBPlayer;

typedef struct BBBubble {
    BBBody body;
    bool active;
    bool popping;
    bool releasing;
    int owner;
    int captured_enemy; /* 잡은 적 번호, 없으면 -1 */
    float age;
    float pop_timer;
} BBBubble;
typedef struct BBEnemy {
    BBBody body;
    BBEnemyType type;
    BBEnemyState state;
    bool active;
    bool angry;
    int facing;          /* 왼쪽 -1, 오른쪽 1 */
    float spawn_delay;
    float spawn_y;
    float age;
    int bounces;
    float bounce_timer;
    float jump_timer;
    float turn_timer;
    float throw_timer;
} BBEnemy;
typedef struct BBBoulder {
    BBBody body;
    bool active;
    float age;
} BBBoulder;

typedef struct BBPickup {
    BBBody body;
    BBPickupType type;
    bool active;
    float age;
} BBPickup;

typedef enum BBPlatformState {
    BB_PLATFORM_SOLID,        /* 밟을 수 있음 */
    BB_PLATFORM_DISAPPEARING, /* 깜빡이는 중, 밟을 수 있음 */
    BB_PLATFORM_ABSENT,
    BB_PLATFORM_APPEARING    /* 깜빡이는 중, 통과함 */
} BBPlatformState;

typedef struct BBMovingPlatform {
    int x, y, width;
    float offset;
    float step;
    float speed;
    int direction;           /* 왼쪽 -1, 오른쪽 1 */
    BBPlatformState state;
    double state_time;
    float solid_duration;
    bool vanished;
} BBMovingPlatform;

typedef struct BBGame {
    unsigned char maps[BB_LEVEL_COUNT][BB_MAP_HEIGHT][BB_MAP_WIDTH];
    int world_width, world_height;
    BBPlayer players[BB_MAX_PLAYERS];
    BBBubble bubbles[BB_MAX_BUBBLES];
    BBEnemy enemies[BB_MAX_ENEMIES];
    BBBoulder boulders[BB_MAX_BOULDERS];
    BBPickup pickups[BB_MAX_PICKUPS];
    BBPickupType pickup_order[BB_PICKUP_TYPE_COUNT];
    int pickup_cursor;
    int level;
    int round_enemy_quota;
    int round_pickups_collected;
    bool round_pickup_bonus_awarded;
    float bonus_timer;
    int bonus_player;
    bool won;
    int player_count;
    BBState state;
    float state_time;
    float level_time;
    BBMovingPlatform platforms[BB_MAX_PLATFORMS];
    int platform_count;
    double platform_warning_cooldown;
    int platform_last_choice;
    unsigned long long ticks;
    unsigned int rng;
    BBEvents events;
} BBGame;

// tiles가 NULL이면 빈 맵
void bb_game_init(BBGame *game, const unsigned char *tiles, unsigned int seed);

void bb_game_resize(BBGame *game, int width, int height);

int bb_game_tile(const BBGame *game, int x, int y);

int bb_game_tile_for_level(const BBGame *game, int level, int x, int y);

void bb_game_start(BBGame *game, int player_count);

void bb_game_menu(BBGame *game);
void bb_game_skip_intro(BBGame *game);
// 한 번 갱신, inputs는 NULL 가능
void bb_game_update(BBGame *game, const BBInput inputs[BB_MAX_PLAYERS], float dt);

float bb_game_tile_offset(const BBGame *game, int x, int y);
float bb_game_platform_travel(const BBGame *game);

bool bb_game_platform_solid(const BBMovingPlatform *platform);
bool bb_game_platform_blink_visible(const BBMovingPlatform *platform);

int bb_game_enemies_left(const BBGame *game);
int bb_game_pickup_score(BBPickupType type);

typedef struct BBHits {
    bool x;
    bool y;
} BBHits;

#define BB_LEVEL_PLATFORM_COUNT 5

void bb_levels_build(unsigned char *maps);

typedef struct BBViewport {
    int columns, rows;
    float scale;
    float offset_x, offset_y;
} BBViewport;

BBViewport bb_viewport_fit(int framebuffer_width, int framebuffer_height, float pixel_ratio);

#define BB_HUD_TOP 24
#define BB_HUD_BOTTOM 12

BBViewport bb_viewport_fit_hud(int framebuffer_width, int framebuffer_height, float pixel_ratio);

#define BB_SCORE_COUNT 10
#define BB_SCORE_MAX_ROUND 5

typedef struct {
    unsigned int score;
    int round;
    char name[4];
} BbScore;

bool bb_scores_load(const char *path, BbScore scores[BB_SCORE_COUNT], int *count);
void bb_scores_insert(BbScore scores[BB_SCORE_COUNT], int *count, BbScore score);
bool bb_scores_save(const char *path, const BbScore *scores, int count);

enum { BB_SCREEN_WIDTH = 256, BB_SCREEN_HEIGHT = 224 };
typedef enum BBMenu {
    BB_MENU_ONE_PLAYER, BB_MENU_TWO_PLAYERS, BB_MENU_LEADERBOARD, BB_MENU_COUNT
} BBMenu;
typedef struct BBAssets {
    Texture2D players[BB_MAX_PLAYERS];
    Texture2D enemy, tiles, bubble, large_bubble, items;
    Texture2D platform_tiles;
    Texture2D backgrounds;
    Texture2D celestial;
    Texture2D intro_background;
    Texture2D intro_sky;
    Texture2D blue_bubble;
    Texture2D blue_captured_enemy;
    Font font;
    Sound fire, jump, death;
    Music music;
    bool audio;
} BBAssets;
typedef struct BBUI {
    bool muted, paused;
    int selected_players;
    int menu_selection;
    bool save_failed, entering_initials;
    bool score_recorded[BB_MAX_PLAYERS];
    int score_player, initial_cursor;
    char initials[4];
    BbScore scores[BB_SCORE_COUNT];
    int score_count;
} BBUI;

bool bb_assets_validate(const char *directory);
bool bb_assets_load(BBAssets *assets, const char *directory, bool audio);
void bb_assets_unload(BBAssets *assets);
void bb_draw_game(const BBAssets *assets, const BBGame *game, const BBUI *ui,
                  const BBViewport *viewport, int framebuffer_width, int framebuffer_height);
void bb_present_canvas(RenderTexture2D canvas, int framebuffer_width, int framebuffer_height);
void bb_audio_events(const BBAssets *assets, BBEvents events);

extern const Color player_colors[BB_MAX_PLAYERS];

float clamp(float value, float low, float high);
float decrease(float value, float dt);
float random_from_state(unsigned int *state);
float random_unit(BBGame *game);
void shuffle_pickups(BBGame *game);

void place_player(const BBGame *game, BBPlayer *player, int number);
void clamp_rescue_bubble(const BBGame *game, BBPlayer *player);
void damage_player(BBGame *game, BBPlayer *player);
void update_player(BBGame *game, BBPlayer *player, int index, BBInput input, float dt);
void check_rescue_bump(BBGame *game);
void check_bubble_bump(BBGame *game);
void check_boulder_bump(BBGame *game);
void check_pickup_bump(BBGame *game);

void add_round_enemies(BBGame *game, bool expanding);
void update_enemy(BBGame *game, BBEnemy *enemy, int index, float dt);
void pop_bubble(BBGame *game, BBBubble *bubble, bool release);
void update_bubble(BBGame *game, BBBubble *bubble, float dt, float center_x, float center_y);
void update_boulder(BBGame *game, BBBoulder *boulder, float dt);
void update_pickup(BBGame *game, BBPickup *pickup, float dt);

const BBMovingPlatform *platform_for_tile(const BBGame *game, int x, int y);
int tile_at(const BBGame *game, int x, int y);
void reset_platforms(BBGame *game);
void platform_bounds(const BBGame *game, int x, int width, float *minimum, float *maximum);
void update_platforms(BBGame *game, float dt);
float platform_carry(const BBGame *game, const BBBody *body, float half_width, float half_height);
void release_platform_rider(const BBGame *game, BBBody *body, float half_width, float half_height);

int solid_tile_at(const BBGame *game, float x, int y, float *left);
bool rects_overlap(const BBBody *a, float a_half_width, float a_half_height, const BBBody *b, float b_half_width, float b_half_height);
bool circles_overlap(const BBBody *a, float a_radius, const BBBody *b, float b_radius);
BBHits move_body(const BBGame *game, BBBody *body, float half_width, float half_height, float dt, bool one_way, bool wrap);

void text(const BBAssets *assets, const char *value, float x, float y, Color color);
void ui_panel(int x, int y, int width, int height, unsigned char alpha);
void centered(const BBAssets *assets, const char *value, float y, Color color);
void sprite(Texture2D sheet, int column, int row, int size, float x, float y, bool flip);
void large_bubble_sprite(const BBAssets *assets, int player, int frame, float x, float y);

int rescue_seconds(const BBPlayer *player);
void draw_intro_backdrop(const BBAssets *assets, const Camera2D *panel, int framebuffer_width, int framebuffer_height);
void draw_title_backdrop(const BBAssets *assets, const Camera2D *panel, int framebuffer_width, int framebuffer_height);
void draw_scenery(const BBAssets *assets, const BBGame *game, int level);
float round_scroll(const BBGame *game);
void draw_world(const BBAssets *assets, const BBGame *game);
void rescue_labels(const BBAssets *assets, const BBGame *game, const BBViewport *viewport, int framebuffer_width, int framebuffer_height, float hud_scale);

#endif
