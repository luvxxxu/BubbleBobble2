

#include "bubble.h"

const Color player_colors[BB_MAX_PLAYERS] = {
    {92, 230, 52, 255}, {80, 184, 255, 255}
};

typedef struct BBImageSize {
    const char *name;
    int width;
    int height;
} BBImageSize;

bool bb_assets_validate(const char *directory)
{
    static const BBImageSize images[] = {
        {"BubbleCharacter.png", 96, 64}, {"BobbleCharacter.png", 96, 64},
        {"Enemys.png", 256, 192}, {"Items.png", 576, 64},
        {"LevelTiles.png", 32, 80}, {"AttackBubble.png", 112, 16},
        {"BubbleLarge.png", 288, 32}, {"IntroBackground.png", 256, 224},
        {"PlatformTiles.png", 8, 40}, {"Backgrounds.png", 256, 1120},
        {"Celestial.png", 144, 432}, {"IntroSky.png", 256, 224}
    };
    for (size_t i = 0; i < sizeof images / sizeof images[0]; ++i) {
        const char *path = TextFormat("%s/%s", directory, images[i].name);
        Image image = LoadImage(path);
        if (!image.data || image.width != images[i].width || image.height != images[i].height) {
            printf("Missing or invalid asset: %s (expected %dx%d)\n", path, images[i].width, images[i].height);
            if (image.data) {
                UnloadImage(image);
            }
            return false;
        }
        UnloadImage(image);
    }
    return true;
}

static Texture2D texture(const char *directory, const char *name)
{
    Texture2D result = LoadTexture(TextFormat("%s/%s", directory, name));
    if (result.id) {
        SetTextureFilter(result, TEXTURE_FILTER_POINT);
    }
    return result;
}

static bool bubble_green(Color color)
{
    return color.a != 0 && color.r == 92 && color.g == 230 && color.b == 52;
}

static void blue_capture_ring(Color *pixels, int image_width, int column, int row)
{
    bool ring[16][16] = {{false}};
    int left = column * 16;
    int top = row * 16;
    for (int x = 0; x < 16; ++x) {
        ring[0][x] = bubble_green(pixels[top * image_width + left + x]);
    }

    bool changed = true;
    while (changed) {
        changed = false;
        for (int y = 0; y < 16; ++y) {
            for (int x = 0; x < 16; ++x) {
                int position = (top + y) * image_width + left + x;
                if (ring[y][x] || !bubble_green(pixels[position])) {
                    continue;
                }
                // 대각선도 이어진 걸로 봄
                bool connected = false;
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        int nx = x + dx;
                        int ny = y + dy;
                        if (nx >= 0 && nx < 16 && ny >= 0 && ny < 16 && ring[ny][nx]) {
                            connected = true;
                        }
                    }
                }
                if (connected) {
                    ring[y][x] = true;
                    changed = true;
                }
            }
        }
    }
    for (int y = 0; y < 16; ++y) {
        for (int x = 0; x < 16; ++x) {
            if (ring[y][x]) {
                pixels[(top + y) * image_width + left + x] = player_colors[1];
            }
        }
    }
}

static Texture2D blue_texture(const char *directory, const char *name, bool captured)
{
    Texture2D result = {0};
    Image original = LoadImage(TextFormat("%s/%s", directory, name));
    if (!original.data) {
        return result;
    }

    int expected_width = captured ? 256 : 112;
    int expected_height = captured ? 192 : 16;
    if (original.width != expected_width || original.height != expected_height) {
        UnloadImage(original);
        return result;
    }
    Color *pixels = LoadImageColors(original);
    if (!pixels) {
        UnloadImage(original);
        return result;
    }
    if (captured) {
        // 0 젠짱, 1 마이타, 3 몬스타 (2는 바위)
        for (int row = 0; row < 4; ++row) {
            if (row == 2) {
                continue;
            }
            blue_capture_ring(pixels, original.width, 6, row);
            blue_capture_ring(pixels, original.width, 7, row);
        }
    } else {
        for (int i = 0; i < original.width * original.height; ++i) {
            Color color = pixels[i];
            bool highlight = color.r >= 240 && color.g >= 240 && color.b >= 240;

            if (color.a != 0 && !highlight) {
                pixels[i] = player_colors[1];
                pixels[i].a = color.a;
            }
        }
    }

    Image recolored = {pixels, original.width, original.height, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
    result = LoadTextureFromImage(recolored);
    if (result.id) {
        SetTextureFilter(result, TEXTURE_FILTER_POINT);
    }
    UnloadImageColors(pixels);
    UnloadImage(original);
    return result;
}

static Sound sound(const char *directory, const char *name)
{
    return LoadSound(TextFormat("%s/%s", directory, name));
}

bool bb_assets_load(BBAssets *assets, const char *directory, bool audio)
{
    memset(assets, 0, sizeof *assets);
    assets->players[0] = texture(directory, "BubbleCharacter.png");
    assets->players[1] = texture(directory, "BobbleCharacter.png");
    assets->enemy = texture(directory, "Enemys.png");
    assets->items = texture(directory, "Items.png");
    assets->tiles = texture(directory, "LevelTiles.png");
    assets->platform_tiles = texture(directory, "PlatformTiles.png");
    assets->backgrounds = texture(directory, "Backgrounds.png");
    assets->celestial = texture(directory, "Celestial.png");
    assets->intro_background = texture(directory, "IntroBackground.png");
    assets->intro_sky = texture(directory, "IntroSky.png");
    assets->bubble = texture(directory, "AttackBubble.png");
    assets->large_bubble = texture(directory, "BubbleLarge.png");
    assets->blue_bubble = blue_texture(directory, "AttackBubble.png", false);
    assets->blue_captured_enemy = blue_texture(directory, "Enemys.png", true);
    assets->font = LoadFontEx(TextFormat("%s/NES_Font.ttf", directory), 8, NULL, 95);
    if (!assets->players[0].id || !assets->players[1].id || !assets->enemy.id || !assets->items.id || !assets->tiles.id ||
        !assets->platform_tiles.id || !assets->backgrounds.id || !assets->celestial.id || !assets->intro_background.id || !assets->intro_sky.id ||
        !assets->bubble.id || !assets->large_bubble.id || !assets->blue_bubble.id ||
        !assets->blue_captured_enemy.id || !IsFontValid(assets->font) ||
        assets->font.texture.id == GetFontDefault().texture.id) {
        return false;
    }
    SetTextureFilter(assets->font.texture, TEXTURE_FILTER_POINT);
    assets->audio = audio && IsAudioDeviceReady();
    if (assets->audio) {
        assets->fire = sound(directory, "SFX/Bubble Bobble SFX (2).wav");
        assets->death = sound(directory, "SFX/Bubble Bobble SFX (3).wav");
        assets->jump = sound(directory, "SFX/Jump.wav");
        assets->music = LoadMusicStream(TextFormat("%s/SFX/The Quest Begins.ogg", directory));
        if (!IsSoundValid(assets->fire) || !IsSoundValid(assets->death) || !IsSoundValid(assets->jump) || !IsMusicValid(assets->music)) {
            return false;
        }

        assets->music.looping = true;
        SetMusicVolume(assets->music, 0.45f);
        PlayMusicStream(assets->music);
    }
    return true;
}

void bb_assets_unload(BBAssets *assets)
{
    Texture2D textures[] = {
        assets->players[0], assets->players[1], assets->tiles, assets->bubble,
        assets->large_bubble, assets->enemy, assets->items,
        assets->blue_bubble, assets->blue_captured_enemy,
        assets->platform_tiles, assets->backgrounds, assets->celestial, assets->intro_background, assets->intro_sky
    };
    for (int i = 0; i < (int)(sizeof textures / sizeof textures[0]); ++i) {
        if (textures[i].id) {
            UnloadTexture(textures[i]);
        }
    }
    if (assets->font.texture.id && assets->font.texture.id != GetFontDefault().texture.id) {
        UnloadFont(assets->font);
    }
    Sound sounds[] = {assets->fire, assets->jump, assets->death};
    for (int i = 0; i < (int)(sizeof sounds / sizeof sounds[0]); ++i) {
        if (IsSoundValid(sounds[i])) {
            UnloadSound(sounds[i]);
        }
    }
    if (IsMusicValid(assets->music)) {
        UnloadMusicStream(assets->music);
    }
    memset(assets, 0, sizeof *assets);
}

void bb_audio_events(const BBAssets *assets, BBEvents events)
{
    if (!assets->audio) {
        return;
    }
    if (events.fire) {
        PlaySound(assets->fire);
    }
    if (events.jump) {
        PlaySound(assets->jump);
    }
    if (events.death) {
        PlaySound(assets->death);
    }
}

void text(const BBAssets *assets, const char *value, float x, float y, Color color)
{
    DrawTextEx(assets->font, value, (Vector2){x + 1, y + 1}, 8, 0, (Color){8, 12, 48, color.a});
    DrawTextEx(assets->font, value, (Vector2){x, y}, 8, 0, color);
}

void ui_panel(int x, int y, int width, int height, unsigned char alpha)
{
    DrawRectangle(x, y, width, height, (Color){4, 8, 40, alpha});
}

void centered(const BBAssets *assets, const char *value, float y, Color color)
{
    float width = MeasureTextEx(assets->font, value, 8, 0).x;
    text(assets, value, floorf((BB_SCREEN_WIDTH - width) * 0.5f), y, color);
}

static void colored_sprite(Texture2D sheet, int column, int row, int size,
                           float x, float y, bool flip, Color color)
{
    // 음수 너비면 좌우 반전
    Rectangle source = (Rectangle){(float)(column * size), (float)(row * size), (float)size, (float)size};
    if (flip) {
        source.width = -(float)size;
    }
    float left = roundf(x * 8 - (float)size / 2);
    float top = roundf(y * 8 - (float)size / 2);
    Rectangle destination = (Rectangle){left, top, (float)size, (float)size};
    DrawTexturePro(sheet, source, destination, (Vector2){0, 0}, 0, color);
}

void sprite(Texture2D sheet, int column, int row, int size, float x, float y, bool flip)
{
    colored_sprite(sheet, column, row, size, x, y, flip, WHITE);
}

void large_bubble_sprite(const BBAssets *assets, int player, int frame, float x, float y)
{
    Color color = player == 1 ? player_colors[1] : WHITE;
    colored_sprite(assets->large_bubble, frame, 0, 32, x, y, false, color);
}
