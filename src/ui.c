

#include "bubble.h"
#include "rlgl.h"

static void player_hud(const BBAssets *assets, const BBPlayer *player, int index, float width, float height)
{
    float energy = fmaxf(0, fminf(1, player->energy));
    float x = 8.0f;
    if (index != 0) {
        x = width - 120.0f;
    }
    int fill = (int)(energy * 34 + 0.5f);
    Color color = player_colors[index];
    char label[32];
    char value[16];

    snprintf(label, sizeof label, "P%d", index + 1);
    text(assets, label, x, 2, color);
    int score = player->score;
    if (score < 0) {
        score = 0;
    }
    snprintf(value, sizeof value, "%06d", score);
    text(assets, value, x + 112 - MeasureTextEx(assets->font, value, 8, 0).x, 2, color);
    if (player->state == BB_PLAYER_BUBBLE) {

        snprintf(label, sizeof label, "HELP %dS", rescue_seconds(player));
        text(assets, label, x, 13, color);
    } else if (player->state == BB_PLAYER_WAIT_ROUND) {
        text(assets, "NEXT ROUND", x, 13, GRAY);
    } else {

        if (player->state == BB_PLAYER_OUT) {
            color = GRAY;
            snprintf(value, sizeof value, "OUT");
        }
        else if (player->boosting) {
            color = YELLOW;
            snprintf(value, sizeof value, "X2 %dS", (int)ceil((double)energy * BB_ENERGY_BOOST_SECONDS));
        }
        else {
            snprintf(value, sizeof value, "%d", (int)(energy * 100));
        }
        text(assets, "MANA", x, 13, color);
        DrawRectangleRec((Rectangle){x + 34, 14, 36, 7}, color);
        DrawRectangleRec((Rectangle){x + 35, 15, 34, 5}, (Color){32, 32, 32, 255});
        if (fill > 0) {
            DrawRectangleRec((Rectangle){x + 35, 15, (float)fill, 5}, color);
        }
        text(assets, value, x + 112 - MeasureTextEx(assets->font, value, 8, 0).x, 13, color);
    }
    if (player->state == BB_PLAYER_BUBBLE) {
        snprintf(label, sizeof label, "P%d HELP", index + 1);
    }
    else if (player->state == BB_PLAYER_WAIT_ROUND) {
        snprintf(label, sizeof label, "P%d WAIT", index + 1);
    }
    else if (player->state == BB_PLAYER_OUT) {
        snprintf(label, sizeof label, "P%d OUT", index + 1);
    }
    else {
        int lives = player->lives;
        if (lives < 0) {
            lives = 0;
        }
        snprintf(label, sizeof label, "P%d LIFE %d", index + 1, lives);
    }
    x = 8;
    if (index != 0) {
        x = width - 8 - MeasureTextEx(assets->font, label, 8, 0).x;
    }
    text(assets, label, x, height - 10, player_colors[index]);
}

static void leaderboard(const BBAssets *assets, const BBUI *ui, float top, int maximum)
{
    int limit = ui->score_count;
    if (limit > maximum) {
        limit = maximum;
    }
    char value[24];
    text(assets, "NO", 8, top, YELLOW);
    text(assets, "NAME", 32, top, YELLOW);
    text(assets, "SCORE", 132, top, YELLOW);
    text(assets, "ROUND", 196, top, YELLOW);
    for (int i = 0; i < limit; ++i) {
        float y = top + 14 + (float)i * 12;
        snprintf(value, sizeof value, "%u", (unsigned)i + 1);
        text(assets, value, 8, y, WHITE);
        text(assets, ui->scores[i].name, 32, y, WHITE);
        snprintf(value, sizeof value, "%u", ui->scores[i].score);
        text(assets, value, 172 - MeasureTextEx(assets->font, value, 8, 0).x, y, WHITE);
        snprintf(value, sizeof value, "%d", ui->scores[i].round);
        text(assets, value, 216, y, WHITE);
    }
    if (!limit) {
        centered(assets, "NO SCORES YET", top + 20, WHITE);
    }
}

static void score_screen(const BBAssets *assets, const BBGame *game, const BBUI *ui)
{
    char label[48];

    const char *title = "GAME OVER";
    if (game->player_count == 0) {
        title = "LEADERBOARD";
    } else if (game->won) {
        title = "ALL ROUNDS CLEAR!";
    }
    ui_panel(4, 8, 248, game->player_count != 0 && ui->entering_initials ? 214 : 206, 120);
    centered(assets, title, 16, YELLOW);
    for (int i = 0; i < game->player_count; ++i) {
        int score = game->players[i].score;
        if (score < 0) {
            score = 0;
        }
        snprintf(label, sizeof label, "P%d SCORE %d", i + 1, score);
        centered(assets, label, 34.0f + (float)i * 12, player_colors[i]);
    }
    if (ui->entering_initials) {
        snprintf(label, sizeof label, "P%d ENTER INITIALS", ui->score_player + 1);
        centered(assets, label, 66, player_colors[ui->score_player]);
        centered(assets, ui->initials, 80, WHITE);

        float initial_x = floorf((BB_SCREEN_WIDTH - MeasureTextEx(assets->font, ui->initials, 8, 0).x) * 0.5f);
        DrawRectangle((int)initial_x + ui->initial_cursor * 8, 89, 7, 1, player_colors[ui->score_player]);
        leaderboard(assets, ui, 104, 6);
        if (ui->save_failed) {
            centered(assets, "ENTER RETRY OR ESC CANCEL", 198, GRAY);
        } else {
            centered(assets, "ARROWS LETTER  ENTER NEXT", 198, GRAY);
        }
        centered(assets, "ESC CANCEL ENTRY", 214, GRAY);
    } else {
        if (game->player_count == 0) {
            leaderboard(assets, ui, 42.0f, 10);
        } else {
            leaderboard(assets, ui, 66.0f, 9);
        }
        centered(assets, "ENTER TO RETURN", 202, GRAY);
    }
    if (ui->save_failed) {
        ui_panel(0, 212, BB_SCREEN_WIDTH, 12, 235);
        if (ui->entering_initials) {
            centered(assets, "SAVE FAILED  ENTER RETRY", 214, RED);
        } else {
            centered(assets, "SCORES UNAVAILABLE", 214, RED);
        }
    }
}

static void menu(const BBAssets *assets, const BBUI *ui)
{
    const char *entries[BB_MENU_COUNT] = {"1 PLAYER", "2 PLAYERS", "LEADERBOARD"};

    ui_panel(64, 121, 128, 42, 150);
    ui_panel(4, 169, 248, 46, 150);
    for (int i = 0; i < BB_MENU_COUNT; ++i) {
        Color color = GRAY;
        if (i == ui->menu_selection) {
            color = YELLOW;
            if (i < 2) {
                color = player_colors[i];
            }
            DrawRectangle(68, 127 + i * 13, 4, 4, color);
        }
        text(assets, entries[i], 80, (float)(125 + i * 13), color);
    }
    text(assets, "MOVE", 32, 174, GRAY);
    text(assets, "JUMP", 104, 174, GRAY);
    text(assets, "FIRE", 184, 174, GRAY);
    text(assets, "P1", 8, 188, player_colors[0]);
    text(assets, "A D", 32, 188, player_colors[0]);
    text(assets, "W", 104, 188, player_colors[0]);
    text(assets, "E", 184, 188, player_colors[0]);
    text(assets, "P2", 8, 201, player_colors[1]);
    text(assets, "ARROWS", 32, 201, player_colors[1]);
    text(assets, "UP", 104, 201, player_colors[1]);
    text(assets, "SPACE", 184, 201, player_colors[1]);
}

void bb_present_canvas(RenderTexture2D canvas, int framebuffer_width, int framebuffer_height)
{
    if(framebuffer_width <= 0 || framebuffer_height <= 0 || !IsRenderTextureValid(canvas))
    {
        return;
    }

    rlDrawRenderBatchActive();
    rlViewport(0, 0, framebuffer_width, framebuffer_height);
    rlMatrixMode(RL_PROJECTION);
    rlLoadIdentity();
    rlOrtho(0, framebuffer_width, framebuffer_height, 0, 0.0, 1.0);
    rlMatrixMode(RL_MODELVIEW);
    rlLoadIdentity();

    DrawTexturePro(canvas.texture,
                   (Rectangle){0, 0, (float)canvas.texture.width, -(float)canvas.texture.height},
                   (Rectangle){0, 0, (float)framebuffer_width, (float)framebuffer_height},
                   (Vector2){0, 0}, 0, WHITE);
    rlDrawRenderBatchActive();
}

void bb_draw_game(const BBAssets *assets, const BBGame *game, const BBUI *ui,
                  const BBViewport *viewport, int framebuffer_width, int framebuffer_height)
{
    ClearBackground(BLACK);
    if (viewport->scale <= 0.0f || framebuffer_width <= 0 || framebuffer_height <= 0)
        return;
    float scale = viewport->scale;
    // 메뉴/점수 화면은 창에 꽉 차게 소수 배율로 키움 (게임 화면은 정수 배율 유지)
    float panel_scale = fminf((float)framebuffer_width / BB_SCREEN_WIDTH,
                              (float)framebuffer_height / BB_SCREEN_HEIGHT);
    if (game->state != BB_STATE_MENU && game->state != BB_STATE_SCORE) {
        panel_scale = fminf(scale, panel_scale);
    }
    Camera2D panel = {0};
    panel.zoom = panel_scale;
    panel.offset = (Vector2){((float)framebuffer_width - BB_SCREEN_WIDTH * panel_scale) * 0.5f, ((float)framebuffer_height - BB_SCREEN_HEIGHT * panel_scale) * 0.5f};
    if (game->state == BB_STATE_MENU || game->state == BB_STATE_SCORE) {
        BeginMode2D(panel);
        if (game->state == BB_STATE_MENU) {
            draw_intro_backdrop(assets, &panel, framebuffer_width, framebuffer_height);
            menu(assets, ui);
        }
        else {
            draw_title_backdrop(assets, &panel, framebuffer_width, framebuffer_height);
            score_screen(assets, game, ui);
        }
        EndMode2D();
        return;
    }
    if (game->state == BB_STATE_INTRO) {
        BeginMode2D(panel);
        draw_title_backdrop(assets, &panel, framebuffer_width, framebuffer_height);
        EndMode2D();
    }
    Camera2D world = {0};
    world.zoom = scale;
    world.offset = (Vector2){viewport->offset_x, viewport->offset_y};

    float scroll = 0.0f;
    float top_bar = (float)BB_HUD_TOP * scale;
    if (viewport->offset_x <= 0.0f && viewport->offset_y <= top_bar) {
        scroll = round_scroll(game);
    }
    if (scroll > 0.0f) {
        float world_pixels = (float)(game->world_height * 8);
        Camera2D incoming = world;
        incoming.offset.y += (scroll - 1.0f) * world_pixels * scale;
        BeginMode2D(incoming);
        draw_scenery(assets, game, game->level + 1);
        EndMode2D();
        world.offset.y += scroll * world_pixels * scale;
    }
    BeginMode2D(world);

    if ((viewport->offset_x > 0.0f || viewport->offset_y > top_bar) && game->state != BB_STATE_INTRO) {
        float tile_pixels = 8.0f * scale;
        int first_x = (int)floorf(-viewport->offset_x / tile_pixels);
        int first_y = (int)floorf(-viewport->offset_y / tile_pixels);
        int last_x = (int)ceilf(((float)framebuffer_width - viewport->offset_x) / tile_pixels);
        int last_y = (int)ceilf(((float)framebuffer_height - viewport->offset_y) / tile_pixels);
        for (int y = first_y; y < last_y; ++y) {
            for (int x = first_x; x < last_x; ++x) {

                DrawTextureRec(assets->tiles, (Rectangle){(float)((x & 1) * 8), (float)(game->level * 16 + (y & 1) * 8), 8, 8},
                               (Vector2){(float)x * 8.0f, (float)y * 8.0f}, WHITE);
            }
        }
        DrawRectangle(0, 0, game->world_width * 8, game->world_height * 8, BLACK);
    }
    draw_world(assets, game);
    EndMode2D();

    BeginMode2D(world);
    rescue_labels(assets, game, viewport, framebuffer_width, framebuffer_height, panel_scale);
    EndMode2D();

    Camera2D hud = {0};
    hud.zoom = scale;
    float width = (float)framebuffer_width / hud.zoom;
    float height = (float)framebuffer_height / hud.zoom;
    BeginMode2D(hud);
    DrawRectangleRec((Rectangle){0, 0, width, BB_HUD_TOP}, BLACK);
    DrawRectangleRec((Rectangle){0, height - BB_HUD_BOTTOM, width, BB_HUD_BOTTOM}, BLACK);
    for (int i = 0; i < game->player_count; ++i) {
        if (game->players[i].active) {
            player_hud(assets, &game->players[i], i, width, height);
        }
    }
    char label[32];
    snprintf(label, sizeof label, "ROUND %d", game->level + 1);
    text(assets, label, floorf((width - MeasureTextEx(assets->font, label, 8, 0).x) * 0.5f), height - 10, WHITE);
    EndMode2D();

    BeginMode2D(panel);
    if (game->state == BB_STATE_INTRO) {
        ui_panel(40, 82, 176, 70, 160);
        centered(assets, "READY!", 88, YELLOW);
        centered(assets, "ENTER TO START", 110, WHITE);
        centered(assets, "CHARGE 15S  BOOST 5S", 130, GRAY);
        centered(assets, "BOOST: MOVE SPEED X2", 142, GRAY);
    }
    if (game->state == BB_STATE_CLEAR) {
        centered(assets, "ROUND CLEAR!", 100, YELLOW);
    }
    if (game->bonus_timer > 0.0f) {
        char bonus[32];
        Color blink = WHITE;
        if (((int)(game->bonus_timer * 5.0f)) % 2 == 0) {
            blink = YELLOW;
        }
        ui_panel(36, 116, 184, 38, 200);
        centered(assets, "ALL FOOD BONUS!", 122, blink);
        snprintf(bonus, sizeof bonus, "P%d  +%d", game->bonus_player + 1, BB_ALL_PICKUPS_BONUS);
        centered(assets, bonus, 138, player_colors[game->bonus_player]);
    }
    if (ui->paused) {
        ui_panel(48, 86, 160, 58, 225);
        centered(assets, "PAUSED", 94, YELLOW);
        centered(assets, "P TO RESUME", 108, WHITE);
        centered(assets, "M SOUND  F11 FULL", 124, GRAY);
    }
    EndMode2D();
}
