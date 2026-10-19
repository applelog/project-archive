#include "palette.h"
#include <math.h>

static MasterPalette s_palettes[7];

static inline float Clamp01(float v) {
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

void Palette_Init(void) {
    // ACT 0: 흑백 모노톤 유년기 꿈 (Monochrome Tint)
    s_palettes[ACT_0_INFANT] = (MasterPalette){
        .sky            = (Color){ 20, 20, 20, 255 },
        .sky_horizon    = (Color){ 45, 45, 45, 255 },
        .cloud          = (Color){ 180, 180, 180, 255 },
        .ground         = (Color){ 220, 220, 215, 255 },
        .path           = (Color){ 190, 190, 185, 255 },
        .foliage_dark   = (Color){ 60, 60, 60, 255 },
        .foliage_light  = (Color){ 120, 120, 120, 255 },
        .structure_wall = (Color){ 230, 230, 225, 255 },
        .structure_roof = (Color){ 70, 70, 70, 255 },
        .accent         = (Color){ 140, 140, 140, 255 },
        .shadow         = (Color){ 30, 30, 30, 255 },
        .light          = (Color){ 250, 250, 250, 255 }
    };

    // ACT 1: 봄 (Childhood Spring - Vibrant but initially viewed through mono filter)
    s_palettes[ACT_1_CHILDHOOD] = (MasterPalette){
        .sky            = (Color){ 118, 196, 240, 255 },
        .sky_horizon    = (Color){ 200, 235, 250, 255 },
        .cloud          = (Color){ 255, 248, 250, 255 },
        .ground         = (Color){ 142, 202, 98, 255 },
        .path           = (Color){ 228, 204, 156, 255 },
        .foliage_dark   = (Color){ 58, 138, 62, 255 },
        .foliage_light  = (Color){ 245, 172, 198, 255 }, // Cherry blossom pink
        .structure_wall = (Color){ 245, 238, 222, 255 },
        .structure_roof = (Color){ 220, 88, 72, 255 },
        .accent         = (Color){ 248, 168, 48, 255 },
        .shadow         = (Color){ 32, 42, 28, 255 },
        .light          = (Color){ 255, 252, 232, 255 }
    };

    // ACT 2: 여름 (Youth Summer - Vivid Energy)
    s_palettes[ACT_2_YOUTH] = (MasterPalette){
        .sky            = (Color){ 48, 142, 238, 255 },
        .sky_horizon    = (Color){ 132, 214, 252, 255 },
        .cloud          = (Color){ 255, 255, 255, 255 },
        .ground         = (Color){ 78, 186, 88, 255 },
        .path           = (Color){ 218, 192, 142, 255 },
        .foliage_dark   = (Color){ 26, 112, 54, 255 },
        .foliage_light  = (Color){ 118, 220, 110, 255 },
        .structure_wall = (Color){ 238, 232, 216, 255 },
        .structure_roof = (Color){ 38, 98, 188, 255 },
        .accent         = (Color){ 246, 110, 42, 255 },
        .shadow         = (Color){ 20, 36, 44, 255 },
        .light          = (Color){ 255, 246, 206, 255 }
    };

    // ACT 3: 가을 & 사랑의 결실 (Adulthood & Family - Rich Autumn Gold)
    s_palettes[ACT_3_ADULTHOOD] = (MasterPalette){
        .sky            = (Color){ 242, 158, 86, 255 },
        .sky_horizon    = (Color){ 252, 216, 142, 255 },
        .cloud          = (Color){ 252, 238, 224, 255 },
        .ground         = (Color){ 198, 148, 76, 255 },
        .path           = (Color){ 178, 126, 78, 255 },
        .foliage_dark   = (Color){ 158, 48, 38, 255 }, // Crimson maple
        .foliage_light  = (Color){ 238, 138, 42, 255 }, // Warm amber
        .structure_wall = (Color){ 212, 194, 168, 255 },
        .structure_roof = (Color){ 124, 52, 42, 255 },
        .accent         = (Color){ 248, 78, 102, 255 }, // Heart Pink / Warm Love
        .shadow         = (Color){ 44, 28, 24, 255 },
        .light          = (Color){ 255, 236, 182, 255 }
    };

    // ACT 4: 겨울 & 황혼 (Elderly Winter - Serene Twilight)
    s_palettes[ACT_4_ELDERLY] = (MasterPalette){
        .sky            = (Color){ 72, 82, 118, 255 },
        .sky_horizon    = (Color){ 146, 154, 188, 255 },
        .cloud          = (Color){ 218, 222, 236, 255 },
        .ground         = (Color){ 224, 230, 242, 255 }, // Soft snow
        .path           = (Color){ 176, 182, 198, 255 },
        .foliage_dark   = (Color){ 48, 54, 76, 255 },
        .foliage_light  = (Color){ 188, 198, 216, 255 },
        .structure_wall = (Color){ 168, 164, 172, 255 },
        .structure_roof = (Color){ 62, 66, 88, 255 },
        .accent         = (Color){ 238, 186, 88, 255 }, // Warm lamplight
        .shadow         = (Color){ 28, 32, 48, 255 },
        .light          = (Color){ 244, 246, 255, 255 }
    };

    // ACT 5 & ENDING: 회상과 영원 (Reminiscence Golden Glow)
    s_palettes[ACT_5_DEATH_REPLAY] = (MasterPalette){
        .sky            = (Color){ 42, 28, 56, 255 },
        .sky_horizon    = (Color){ 168, 112, 146, 255 },
        .cloud          = (Color){ 252, 224, 210, 255 },
        .ground         = (Color){ 186, 152, 112, 255 },
        .path           = (Color){ 228, 196, 142, 255 },
        .foliage_dark   = (Color){ 112, 64, 98, 255 },
        .foliage_light  = (Color){ 242, 196, 126, 255 },
        .structure_wall = (Color){ 238, 226, 208, 255 },
        .structure_roof = (Color){ 158, 88, 108, 255 },
        .accent         = (Color){ 255, 215, 64, 255 }, // Golden Light
        .shadow         = (Color){ 36, 20, 42, 255 },
        .light          = (Color){ 255, 248, 228, 255 }
    };
    s_palettes[ACT_ENDING] = s_palettes[ACT_5_DEATH_REPLAY];
}

MasterPalette Palette_GetForAct(LifeAct act) {
    if (act < 0 || act > ACT_ENDING) return s_palettes[ACT_0_INFANT];
    return s_palettes[act];
}

Color Palette_Lerp(Color c1, Color c2, float t) {
    t = Clamp01(t);
    return (Color){
        (unsigned char)(c1.r + (c2.r - c1.r) * t),
        (unsigned char)(c1.g + (c2.g - c1.g) * t),
        (unsigned char)(c1.b + (c2.b - c1.b) * t),
        (unsigned char)(c1.a + (c2.a - c1.a) * t)
    };
}

Color Palette_ApplySaturation(Color c, float saturation) {
    saturation = Clamp01(saturation);
    // Grayscale luminance via standard ITU-R BT.601
    float gray = 0.299f * c.r + 0.587f * c.g + 0.114f * c.b;
    unsigned char g = (unsigned char)gray;
    
    return (Color){
        (unsigned char)(g + (c.r - g) * saturation),
        (unsigned char)(g + (c.g - g) * saturation),
        (unsigned char)(g + (c.b - g) * saturation),
        c.a
    };
}

MasterPalette Palette_Interpolate(const MasterPalette* a, const MasterPalette* b, float t) {
    t = Clamp01(t);
    MasterPalette out;
    out.sky            = Palette_Lerp(a->sky, b->sky, t);
    out.sky_horizon    = Palette_Lerp(a->sky_horizon, b->sky_horizon, t);
    out.cloud          = Palette_Lerp(a->cloud, b->cloud, t);
    out.ground         = Palette_Lerp(a->ground, b->ground, t);
    out.path           = Palette_Lerp(a->path, b->path, t);
    out.foliage_dark   = Palette_Lerp(a->foliage_dark, b->foliage_dark, t);
    out.foliage_light  = Palette_Lerp(a->foliage_light, b->foliage_light, t);
    out.structure_wall = Palette_Lerp(a->structure_wall, b->structure_wall, t);
    out.structure_roof = Palette_Lerp(a->structure_roof, b->structure_roof, t);
    out.accent         = Palette_Lerp(a->accent, b->accent, t);
    out.shadow         = Palette_Lerp(a->shadow, b->shadow, t);
    out.light          = Palette_Lerp(a->light, b->light, t);
    return out;
}
