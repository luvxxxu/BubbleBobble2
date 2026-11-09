/* 버블보블 메인 파일
 * 창 열고 입력 받고 게임 돌리고 그리는 반복문
 * 게임 규칙은 game.c 등에 있음 */
#include "bubble.h"
#include <time.h>

// 이번 프레임에 눌린 키 표
static bool pressed_keys[512];

// 키 입력 받기
// 키가 하나라도 들어왔으면 true (키가 들어온다는 건 창이 활성 상태라는 뜻)
static bool poll_key_edges(void)
{
    bool any = false;
    memset(pressed_keys, 0, sizeof pressed_keys);
    while (true)
    {
        int key = GetKeyPressed(); // 0이면 끝
        if (key == 0)
            break;
        any = true;
        if (key > 0 && key < 512)
            pressed_keys[key] = true;
    }
    return any;
}

static bool key_pressed(int key)
{
    return (key >= 0 && key < 512 && pressed_keys[key]) || IsKeyPressed(key);
}

static bool pad_pressed(int index, int button)
{
    return IsGamepadAvailable(index) && IsGamepadButtonPressed(index, button);
}

// 플레이어 입력 합치기 (0이 1P, 1이 2P)
static BBInput player_input(int player, int player_count)
{
    BBInput input = {0};
    int direction = 0;

    if (player == 0)
    {
        if (IsKeyDown(KEY_D))
            direction++;
        if (IsKeyDown(KEY_A))
            direction--;
        input.jump = key_pressed(KEY_W);
        input.fire = key_pressed(KEY_E);
    }
    // 2P (혼자 할 땐 방향키도 가능)
    if (player == 1 || player_count == 1)
    {
        if (IsKeyDown(KEY_RIGHT))
            direction++;
        if (IsKeyDown(KEY_LEFT))
            direction--;
        input.jump = input.jump || key_pressed(KEY_UP);
    }
    if (player == 1)
        input.fire = key_pressed(KEY_SPACE);
    if (player_count == 1)
    {
        input.jump = input.jump || key_pressed(KEY_X);
        input.fire = input.fire || key_pressed(KEY_SPACE) || key_pressed(KEY_SLASH) || key_pressed(KEY_RIGHT_CONTROL);
    }
    input.move = (float)direction;

    if (IsGamepadAvailable(player))
    {
        float stick = GetGamepadAxisMovement(player, GAMEPAD_AXIS_LEFT_X);
        if (fabsf(stick) > 0.2f) // 살짝 흔들림 무시
            input.move += stick;
        if (IsGamepadButtonDown(player, GAMEPAD_BUTTON_LEFT_FACE_RIGHT))
            input.move += 1;
        if (IsGamepadButtonDown(player, GAMEPAD_BUTTON_LEFT_FACE_LEFT))
            input.move -= 1;
        input.jump = input.jump || pad_pressed(player, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)
                     || pad_pressed(player, GAMEPAD_BUTTON_LEFT_FACE_UP);
        input.fire = input.fire || pad_pressed(player, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
    }
    input.move = clamp(input.move, -1.0f, 1.0f);
    return input;
}

// 다음에 이름 입력할 플레이어 찾기
static void score_entry_next(BBUI *ui, const BBGame *game)
{
    ui->entering_initials = false;
    for (int i = 0; i < game->player_count; ++i)
    {
        if (game->players[i].active && !ui->score_recorded[i])
        {
            ui->score_player = i;
            ui->entering_initials = true;
            break;
        }
    }
    ui->initial_cursor = 0;
    strcpy(ui->initials, "AAA"); // AAA부터
}

static void score_entry_begin(BBUI *ui, const BBGame *game)
{
    memset(ui->score_recorded, 0, sizeof ui->score_recorded);
    score_entry_next(ui, game);
    // 남은 글자 입력 비우기
    while (GetCharPressed() != 0)
    {
    }
}

// 이름이랑 점수 저장
static void score_submit(BBUI *ui, const BBGame *game, const char *path)
{
    int player = ui->score_player;
    BbScore record = {0};
    BbScore updated[BB_SCORE_COUNT];
    int count = ui->score_count;

    if (game->players[player].score > 0)
        record.score = (unsigned int)game->players[player].score;
    record.round = game->level + 1;
    strcpy(record.name, ui->initials);
    // 복사본에 먼저 넣어보고 저장되면 반영
    memcpy(updated, ui->scores, sizeof updated);
    bb_scores_insert(updated, &count, record);
    if (!bb_scores_save(path, updated, count))
    {
        ui->save_failed = true;
        ui->initial_cursor = 2;
        return;
    }
    memcpy(ui->scores, updated, sizeof updated);
    ui->score_count = count;
    ui->score_recorded[player] = true;
    ui->save_failed = false;
    score_entry_next(ui, game);
}

// 점수 화면 입력 처리
static void score_input(BBUI *ui, BBGame *game, bool up, bool down, bool confirm, const char *path)
{
    // 이름 입력 끝났으면 메뉴로
    if (!ui->entering_initials)
    {
        if (confirm)
            bb_game_menu(game);
        return;
    }
    if (key_pressed(KEY_LEFT) && ui->initial_cursor > 0)
        ui->initial_cursor--;
    if (key_pressed(KEY_RIGHT) && ui->initial_cursor < 2)
        ui->initial_cursor++;

    char *letter = &ui->initials[ui->initial_cursor]; // 지금 고치는 글자
    if (up)
    {
        if (*letter >= 'Z')
            *letter = 'A';
        else
            *letter = *letter + 1;
    }
    if (down)
    {
        if (*letter <= 'A')
            *letter = 'Z';
        else
            *letter = *letter - 1;
    }
    // 글자 키 직접 입력 (소문자는 대문자로)
    while (true)
    {
        int key = GetCharPressed();
        if (key == 0)
            break;
        if (key >= 'a' && key <= 'z')
            key = key - 'a' + 'A';
        if (key >= 'A' && key <= 'Z')
            *letter = (char)key;
    }
    if (confirm)
    {
        ui->initial_cursor++;
        if (ui->initial_cursor >= 3)
            score_submit(ui, game, path);
    }
}

static void resize_canvas(RenderTexture2D *canvas, int width, int height)
{
    if (canvas->texture.width == width && canvas->texture.height == height)
        return;
    UnloadRenderTexture(*canvas);
    *canvas = LoadRenderTexture(width, height);
    SetTextureFilter(canvas->texture, TEXTURE_FILTER_POINT); // 도트 안 흐리게
}

static void apply_mute(bool muted, bool audio_ready)
{
    if (audio_ready)
        SetMasterVolume(muted ? 0.0f : 1.0f);
}

int main(void)
{
    unsigned char maps[BB_LEVEL_COUNT * BB_MAP_WIDTH * BB_MAP_HEIGHT];
    char asset_directory[512];
    char score_path[512];
    BBUI ui = {0};
    BBGame game;
    BBAssets assets;

    // 에셋 폴더 찾기
    snprintf(asset_directory, sizeof asset_directory, "%sassets", GetApplicationDirectory());
    if (!DirectoryExists(asset_directory))
        snprintf(asset_directory, sizeof asset_directory, "Assets");
    snprintf(score_path, sizeof score_path, "%sScores.txt", GetApplicationDirectory());

    SetTraceLogLevel(LOG_WARNING);
    if (!bb_assets_validate(asset_directory))
        return 1;
    bb_levels_build(maps);
    ui.selected_players = 1;
    bb_scores_load(score_path, ui.scores, &ui.score_count);

    // 창 열기
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(BB_SCREEN_WIDTH * 3, BB_SCREEN_HEIGHT * 3, "Bubble Bobble"); // 768x672
    SetWindowMinSize(BB_SCREEN_WIDTH, BB_SCREEN_HEIGHT + BB_HUD_TOP + BB_HUD_BOTTOM);
    SetExitKey(KEY_NULL); // Esc는 메뉴로
    SetTargetFPS(60);
    InitAudioDevice();
    bool audio_ready = IsAudioDeviceReady();
    if (!bb_assets_load(&assets, asset_directory, audio_ready))
    {
        printf("Could not load game resources.\n");
        return 1;
    }
    RenderTexture2D canvas = {0};
    resize_canvas(&canvas, GetRenderWidth(), GetRenderHeight());
    bb_game_init(&game, maps, (unsigned int)time(NULL));

    BBInput pending[BB_MAX_PLAYERS] = {0}; // 아직 안 보낸 입력
    double accumulator = 0; // 남은 시간
    bool was_simulating = false;

    while (!WindowShouldClose())
    {
        bool got_key = poll_key_edges();
        BBState previous_state = game.state;
        // 전체화면 전환 직후 포커스 판정이 틀려도 키가 들어오면 활성으로 봄
        bool focused = IsWindowFocused() || got_key;
        bool clear_actions = false;
        bool confirm = focused && (key_pressed(KEY_ENTER) || key_pressed(KEY_KP_ENTER) || pad_pressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)
                                   || pad_pressed(1, GAMEPAD_BUTTON_RIGHT_FACE_DOWN));
        bool up = focused && (key_pressed(KEY_UP) || key_pressed(KEY_W)
                              || pad_pressed(0, GAMEPAD_BUTTON_LEFT_FACE_UP) || pad_pressed(1, GAMEPAD_BUTTON_LEFT_FACE_UP));
        bool down = focused && (key_pressed(KEY_DOWN) || key_pressed(KEY_S)
                                || pad_pressed(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN) || pad_pressed(1, GAMEPAD_BUTTON_LEFT_FACE_DOWN));

        // F11 전체화면, M 음소거
        if (focused && key_pressed(KEY_F11))
            ToggleBorderlessWindowed();
        if (focused && key_pressed(KEY_M))
        {
            ui.muted = !ui.muted;
            apply_mute(ui.muted, audio_ready);
        }

        // 창이 최소화됐으면 건너뛰기
        int framebuffer_width = GetRenderWidth();
        int framebuffer_height = GetRenderHeight();
        int window_width = GetScreenWidth();
        int window_height = GetScreenHeight();
        if (framebuffer_width <= 0 || framebuffer_height <= 0 || window_width <= 0 || window_height <= 0)
        {
            accumulator = 0;
            was_simulating = false;
            BeginDrawing();
            EndDrawing();
            continue;
        }
        resize_canvas(&canvas, framebuffer_width, framebuffer_height);
        // 화면 크기 맞추기
        float pixel_ratio = fminf((float)framebuffer_width / window_width, (float)framebuffer_height / window_height);
        BBViewport viewport = bb_viewport_fit_hud(framebuffer_width, framebuffer_height, pixel_ratio);
        bb_game_resize(&game, viewport.columns, viewport.rows);

        if (focused && key_pressed(KEY_ESCAPE))
        {
            // Esc: 메뉴면 종료, 아니면 메뉴로
            if (game.state == BB_STATE_MENU)
                break;
            ui.entering_initials = false;
            bb_game_menu(&game);
            ui.paused = false;
            accumulator = 0;
            memset(pending, 0, sizeof pending);
        }
        else if (focused && game.state == BB_STATE_MENU)
        {
            // 위/아래로 메뉴 고르기
            if (up)
                ui.menu_selection = (ui.menu_selection + BB_MENU_COUNT - 1) % BB_MENU_COUNT;
            if (down)
                ui.menu_selection = (ui.menu_selection + 1) % BB_MENU_COUNT;
            if (key_pressed(KEY_ONE))
                ui.menu_selection = BB_MENU_ONE_PLAYER;
            if (key_pressed(KEY_TWO))
                ui.menu_selection = BB_MENU_TWO_PLAYERS;
            if (ui.menu_selection != BB_MENU_LEADERBOARD)
                ui.selected_players = ui.menu_selection + 1;
            if (confirm)
            {
                if (ui.menu_selection == BB_MENU_LEADERBOARD)
                {
                    // 순위표 보기만
                    game.state = BB_STATE_SCORE;
                    game.player_count = 0;
                    memset(game.players, 0, sizeof game.players);
                    game.won = false;
                    ui.entering_initials = false;
                }
                else
                    bb_game_start(&game, ui.selected_players);
                accumulator = 0;
                clear_actions = true; // 확인 키가 게임으로 안 넘어가게
            }
        }
        else if (focused && game.state == BB_STATE_SCORE)
        {
            score_input(&ui, &game, up, down, confirm, score_path);
        }
        else if (focused)
        {
            if (game.state == BB_STATE_INTRO && confirm)
            {
                bb_game_skip_intro(&game);
                clear_actions = true;
            }
            if (key_pressed(KEY_P) || pad_pressed(0, GAMEPAD_BUTTON_MIDDLE_RIGHT)
                || (game.player_count == 2 && pad_pressed(1, GAMEPAD_BUTTON_MIDDLE_RIGHT)))
            {
                ui.paused = !ui.paused;
                clear_actions = true;
                accumulator = 0;
            }
        }

        if (assets.audio)
            UpdateMusicStream(assets.music);

        bool simulate = !ui.paused && focused && game.state != BB_STATE_MENU && game.state != BB_STATE_SCORE;
        if (simulate)
        {
            // 시간 모아서 1/120초씩 갱신
            float elapsed = GetFrameTime();
            if (elapsed > 0.25f)
                elapsed = 0.25f; // 너무 긴 프레임은 줄임
            accumulator += elapsed;
            if (clear_actions || !was_simulating)
                memset(pending, 0, sizeof pending);
            bool discard_actions = clear_actions || !was_simulating ||
                                   (game.state != BB_STATE_PLAY && game.state != BB_STATE_CLEAR);
            for (int i = 0; i < game.player_count; ++i)
            {
                BBInput input = player_input(i, game.player_count);
                pending[i].move = input.move;
                // 점프/발사는 다음 갱신까지 기억
                if (discard_actions)
                {
                    pending[i].jump = false;
                    pending[i].fire = false;
                }
                else
                {
                    pending[i].jump = pending[i].jump || input.jump;
                    pending[i].fire = pending[i].fire || input.fire;
                }
            }
            while (accumulator >= BB_FIXED_DT)
            {
                bb_game_update(&game, pending, BB_FIXED_DT);
                bb_audio_events(&assets, game.events); // 소리 내기
                // 점프/발사는 한 번만 전달
                for (int i = 0; i < game.player_count; ++i)
                {
                    pending[i].jump = false;
                    pending[i].fire = false;
                }
                accumulator -= BB_FIXED_DT;
                if (game.state == BB_STATE_SCORE)
                {
                    accumulator = 0;
                    break;
                }
            }
        }
        else
        {
            accumulator = 0;
            memset(pending, 0, sizeof pending);
        }
        was_simulating = simulate;

        // 점수 화면 되면 이름 입력 시작
        if (game.state == BB_STATE_SCORE && previous_state != BB_STATE_SCORE && previous_state != BB_STATE_MENU)
            score_entry_begin(&ui, &game);

        BeginTextureMode(canvas);
        bb_draw_game(&assets, &game, &ui, &viewport, framebuffer_width, framebuffer_height);
        EndTextureMode();
        BeginDrawing();
        ClearBackground(BLACK);
        bb_present_canvas(canvas, framebuffer_width, framebuffer_height);
        EndDrawing();
    }

    UnloadRenderTexture(canvas);
    bb_assets_unload(&assets);
    if (audio_ready)
        CloseAudioDevice();
    CloseWindow();
    return 0;
}
