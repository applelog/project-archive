#include <assert.h>
#include <stdio.h>
#include <math.h>
#include "../src/types.h"
#include "../src/palette.h"

int main(void) {
    // 1. Verify types.h structures & constants
    assert(SCREEN_WIDTH == 640);
    assert(SCREEN_HEIGHT == 360);
    assert(TARGET_FPS == 60);
    assert(TILE_SIZE == 24);

    Player player = {0};
    player.dir = DIR_DOWN;
    player.memory_clarity = 1.0f;
    assert(player.memory_clarity == 1.0f);

    NPC npc = {0};
    npc.area = AREA_SUBWAY_PLATFORM;
    assert(npc.area == AREA_SUBWAY_PLATFORM);

    MemoryShard shard = {0};
    shard.id = 1;
    assert(shard.id == 1);

    SubwayTrain train = {0};
    train.doors_open = false;
    assert(!train.doors_open);

    // 2. Verify Palette_GetDefault()
    GBCPalette p = Palette_GetDefault();
    assert(p.bg_dark.a == 255);
    assert(p.bg_mid.a == 255);
    assert(p.bg_light.a == 255);
    assert(p.fg_highlight.a == 255);
    assert(p.subway_line.a == 255);
    assert(p.skin_tone.a == 255);
    assert(p.dress_white.a == 255);
    assert(p.shadow_color.a == 90);

    // 3. Verify Palette_ApplyMonochrome()
    Color red = (Color){ 255, 0, 0, 255 };
    Color desat_0 = Palette_ApplyMonochrome(red, 0.0f);
    assert(desat_0.r == desat_0.g && desat_0.g == desat_0.b);
    assert(desat_0.a == 255);
    
    // Saturation = 1.0 (unchanged)
    Color desat_1 = Palette_ApplyMonochrome(red, 1.0f);
    assert(desat_1.r == 255 && desat_1.g == 0 && desat_1.b == 0 && desat_1.a == 255);

    // Saturation = 0.5
    Color desat_half = Palette_ApplyMonochrome(red, 0.5f);
    assert(desat_half.r > desat_half.g);
    assert(desat_half.g == desat_half.b);

    // 4. Verify Palette_LerpColor()
    Color c1 = (Color){ 0, 100, 200, 255 };
    Color c2 = (Color){ 100, 200, 0, 128 };
    
    Color lerp_0 = Palette_LerpColor(c1, c2, 0.0f);
    assert(lerp_0.r == c1.r && lerp_0.g == c1.g && lerp_0.b == c1.b && lerp_0.a == c1.a);

    Color lerp_1 = Palette_LerpColor(c1, c2, 1.0f);
    assert(lerp_1.r == c2.r && lerp_1.g == c2.g && lerp_1.b == c2.b && lerp_1.a == c2.a);

    Color lerp_half = Palette_LerpColor(c1, c2, 0.5f);
    assert(lerp_half.r == 50);
    assert(lerp_half.g == 150);
    assert(lerp_half.b == 100);
    assert(lerp_half.a == 191);

    printf("[TEST] Palette tests passed successfully!\n");
    return 0;
}
