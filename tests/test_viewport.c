/* 화면 크기를 게임의 타일 수와 그리기 배율로 바꾸는 계산을 검사한다.
 * 실제 창을 열지 않고 여러 픽셀 크기를 함수에 넣어 캐릭터 비율과 맵 확장이
 * 유지되는지 확인한다. assert는 잘못된 결과를 만나면 즉시 시험을 멈춘다. */
#include "bubble.h"
#include <stdint.h>

#include <assert.h>
#include <math.h>
#include <stdio.h>

/* 실제 픽셀 수에서 계산한 표시 범위가 화면 전체를 덮고, 바깥 벽의
 * 반 타일 미만만 잘라내는지 확인한다. 이동 가능한 내부 영역은 보존한다. */
static void test_resolutions(void)
{
    const int resolutions[][2] = {
        {256, 224}, {768, 672}, {1000, 700}, {1920, 1080},
        {2560, 1440}, {3840, 2160}, {3420, 1920}, {3440, 1440},
        {1080, 1920}, {700, 1000}, {160, 120}
    };
    for(unsigned i = 0; i < sizeof resolutions / sizeof resolutions[0]; ++i)
    {
        int width = resolutions[i][0], height = resolutions[i][1];
        BBViewport view = bb_viewport_fit(width, height, 1.0f);
        float tile = view.scale * 8.0f;
        assert(isfinite(view.scale) && view.scale > 0.0f);
        if(width >= 256 && height >= 224)
            assert(view.scale == floorf(view.scale));
        assert(view.columns >= BB_MAP_WIDTH && view.columns <= BB_WORLD_MAX_WIDTH);
        assert(view.rows >= BB_MAP_HEIGHT && view.rows <= BB_WORLD_MAX_HEIGHT);
        assert(view.offset_x <= 0.001f && view.offset_y <= 0.001f);
        assert(view.offset_x > -tile * 0.5f - 0.001f);
        assert(view.offset_y > -tile * 0.5f - 0.001f);
        assert(view.offset_x + (float)view.columns * tile >= (float)width - 0.001f);
        assert(view.offset_y + (float)view.rows * tile >= (float)height - 0.001f);
        assert(view.offset_x + 2.0f * tile > 0.0f);
        assert(view.offset_x + ((float)view.columns - 2.0f) * tile < (float)width);
    }
}

/* 16:9에서는 기존 32열을 늘리는 대신 이동 가능한 열 수가 늘어나야 한다.
 * 같은 물리 크기를 두 배로 그려도 배율 외의 월드 배치는 같아야 한다. */
static void test_expansion_and_retina(void)
{
    BBViewport base = bb_viewport_fit(768, 672, 1.0f);
    assert(base.columns == 32 && base.rows == 28 && base.scale == 3.0f);
    BBViewport wide = bb_viewport_fit(1920, 1080, 1.0f);
    BBViewport retina = bb_viewport_fit(3840, 2160, 2.0f);
    assert(wide.columns == 60 && wide.rows == 34 && wide.scale == 4.0f);
    assert(retina.columns == wide.columns && retina.rows == wide.rows);
    assert(retina.scale == wide.scale * 2.0f);
    assert(retina.offset_x == wide.offset_x * 2.0f);
    assert(retina.offset_y == wide.offset_y * 2.0f);
    BBViewport portrait = bb_viewport_fit(700, 1000, 1.0f);
    assert(portrait.rows > BB_MAP_HEIGHT);
    BBViewport invalid = bb_viewport_fit(0, -1, 1.0f);
    assert(invalid.columns == BB_MAP_WIDTH && invalid.rows == BB_MAP_HEIGHT);
    assert(invalid.scale == 1.0f);
    BBViewport fraction = bb_viewport_fit(2400, 1350, 1.25f);
    assert(fraction.columns == wide.columns && fraction.rows == wide.rows);
    assert(fraction.scale == wide.scale * 1.25f);
}

/* 매우 가늘거나 긴 창에서는 맵 최대 크기를 넘지 않으면서도 기존 플레이
 * 영역을 확대해 잘라내지 않는다. 남는 공간은 화면 쪽에서 외벽을 연장한다. */
static void test_extreme_aspect_ratios(void)
{
    BBViewport wide = bb_viewport_fit(4096, 224, 1.0f);
    assert(wide.columns == BB_WORLD_MAX_WIDTH && wide.rows == BB_MAP_HEIGHT);
    assert(wide.scale == 1.0f && wide.offset_x > 0.0f && wide.offset_y == 0.0f);
    BBViewport tall = bb_viewport_fit(256, 4096, 1.0f);
    assert(tall.rows == BB_WORLD_MAX_HEIGHT && tall.columns == BB_MAP_WIDTH);
    assert(tall.scale == 1.0f && tall.offset_y > 0.0f && tall.offset_x == 0.0f);
}

/* 검은 띠(위 24, 아래 12)를 뺀 자리에 맵이 들어가고, 맵이 띠와 겹치지 않는지 확인한다.
 * 맵 그림은 띠 안쪽 경계에서만 잘린다. 정수 배율이 창을 85%도 못 채우면 소수 배율로 맞춘다.
 * 768x780은 3배에 32x28칸, 기본 창 768x672는 약 2.58배에 38x28칸이다. */
static void test_hud_bars(void)
{
    BBViewport base = bb_viewport_fit_hud(768, 780, 1.0f);
    assert(base.scale == 3.0f && base.columns == 32 && base.rows == 28);
    assert(base.offset_y == (float)BB_HUD_TOP * 3.0f);
    BBViewport window = bb_viewport_fit_hud(768, 672, 1.0f);
    assert(fabsf(window.scale - 672.0f / 260.0f) < 0.0001f && window.rows == 28 && window.columns == 38);
    assert(fabsf(window.offset_y - (float)BB_HUD_TOP * window.scale) < 0.01f);
    BBViewport nearly = bb_viewport_fit_hud(1920, 1080, 1.0f); /* 4.15배 -> 4배(96%)는 정수 유지 */
    assert(nearly.scale == 4.0f);
    const int sizes[][2] = {{768, 672}, {1366, 768}, {768, 780}, {1920, 1080}, {2560, 1440}, {3840, 2160}, {700, 1000}, {256, 260}, {4096, 300}};
    for(unsigned i = 0; i < sizeof sizes / sizeof sizes[0]; ++i)
    {
        int width = sizes[i][0], height = sizes[i][1];
        BBViewport view = bb_viewport_fit_hud(width, height, 1.0f);
        float tile = view.scale * 8.0f;
        float top = (float)BB_HUD_TOP * view.scale;
        float bottom = (float)height - (float)BB_HUD_BOTTOM * view.scale;
        /* 띠 사이 칸을 맵이 빈틈없이 채우고, 넘치는 건 반 칸 미만이다. */
        assert(view.rows >= BB_MAP_HEIGHT && view.columns >= BB_MAP_WIDTH);
        assert(view.offset_y <= top + 0.001f);
        assert(view.offset_y + (float)view.rows * tile >= bottom - 0.001f);
        assert(view.offset_y > top - tile * 0.5f - 0.001f);
        /* 띠 두 개와 기본 맵 높이가 창 안에 들어간다 */
        assert(top + 224.0f * view.scale + (float)BB_HUD_BOTTOM * view.scale <= (float)height + 0.001f);
    }
    BBViewport invalid = bb_viewport_fit_hud(0, -1, 1.0f);
    assert(invalid.columns == BB_MAP_WIDTH && invalid.rows == BB_MAP_HEIGHT);
}

/* 일반 해상도, 고밀도 화면, 극단적인 화면비를 모두 통과해야 성공으로 끝낸다. */
int main(void)
{
    test_resolutions();
    test_expansion_and_retina();
    test_extreme_aspect_ratios();
    test_hud_bars();
    puts("Native-resolution viewport, world expansion, square pixels, and Retina scaling passed.");
    return 0;
}
