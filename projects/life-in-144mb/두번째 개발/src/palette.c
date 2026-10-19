#include "palette.h"

GBCPalette Palette_GetDefault(void) {
    GBCPalette p;
    // GBC Warm Vintage Pokemon Gold Palette
    p.bg_dark          = (Color){ 24, 26, 23, 255 };    // Deep ink slate #181A17
    p.bg_mid           = (Color){ 72, 84, 76, 255 };    // GBC Olive Green #48544C
    p.bg_light         = (Color){ 168, 184, 152, 255 }; // Light pistachio paper #A8B898
    p.fg_highlight     = (Color){ 246, 246, 236, 255 }; // Warm Pokemon Dialog White #F6F6EC
    p.subway_line      = (Color){ 46, 145, 80, 255 };   // Line 2 Forest Green #2E9150
    p.skin_tone        = (Color){ 248, 204, 168, 255 }; // Soft Peach Skin #F8CCA8
    p.dress_white      = (Color){ 250, 250, 246, 255 }; // Retro White Dress #FAFAF6
    p.shadow_color     = (Color){ 10, 15, 10, 110 };    // Translucent drop shadow
    
    // Pokemon Gold/Silver authentic tones
    p.poke_gold        = (Color){ 232, 168, 56, 255 };  // Gold Accent #E8A838
    p.poke_silver      = (Color){ 168, 184, 208, 255 }; // Silver Slate #A8B8D0
    p.poke_red         = (Color){ 216, 64, 56, 255 };   // Pokemon Red #D84038
    p.poke_blue        = (Color){ 56, 120, 216, 255 };  // Pokemon Blue #3878D8
    p.poke_wood        = (Color){ 176, 112, 56, 255 };  // Wood Plank #B07038
    p.poke_brick       = (Color){ 184, 80, 64, 255 };   // Terracotta Brick #B85040
    p.poke_leaf_dark   = (Color){ 40, 96, 40, 255 };    // GBC Tree Shadow #286028
    p.poke_leaf_light  = (Color){ 88, 168, 72, 255 };   // GBC Tree Highlight #58A848
    return p;
}

Color Palette_LerpColor(Color a, Color b, float t) {
    if (t <= 0.0f) return a;
    if (t >= 1.0f) return b;
    unsigned char r = (unsigned char)(a.r + (b.r - a.r) * t);
    unsigned char g = (unsigned char)(a.g + (b.g - a.g) * t);
    unsigned char b_val = (unsigned char)(a.b + (b.b - a.b) * t);
    unsigned char alpha = (unsigned char)(a.a + (b.a - a.a) * t);
    return (Color){ r, g, b_val, alpha };
}

Color Palette_ApplyMonochrome(Color c, float saturation) {
    if (saturation >= 0.99f) return c;
    if (saturation < 0.0f) saturation = 0.0f;
    // BT.601 perceptual luminance calculation
    float gray = (c.r * 0.299f + c.g * 0.587f + c.b * 0.114f);
    Color grayColor = (Color){ (unsigned char)gray, (unsigned char)gray, (unsigned char)gray, c.a };
    
    unsigned char r = (unsigned char)(grayColor.r + (c.r - grayColor.r) * saturation);
    unsigned char g = (unsigned char)(grayColor.g + (c.g - grayColor.g) * saturation);
    unsigned char b = (unsigned char)(grayColor.b + (c.b - grayColor.b) * saturation);
    return (Color){ r, g, b, c.a };
}
