#ifndef PALETTE_H
#define PALETTE_H

#include <raylib.h>

typedef struct {
    Color bg_dark;          // Darkest outline ink (Almost Black #181A17)
    Color bg_mid;           // Mid tone olive/slate (#48544C)
    Color bg_light;         // Soft pistachio/cream (#A8B898)
    Color fg_highlight;     // Warm vintage off-white (#E8F0D8)
    Color subway_line;      // Seoul Line 2 Green (#2E8B57)
    Color skin_tone;        // Peach skin (#F0C4A0)
    Color dress_white;      // Dress white (#F5F5F0)
    Color shadow_color;     // Drop shadow (#0A0C0A80)
    
    // Pokemon Gold/Silver authentic colors
    Color poke_gold;        // #E8A838 (Gold Version title & badges)
    Color poke_silver;      // #A8B8D0 (Silver Version metallic blue)
    Color poke_red;         // #D84038 (Classic Pokemon HP Red / Accents)
    Color poke_blue;        // #3878D8 (Classic Water / Menu Blue)
    Color poke_wood;        // #B07038 (Hanok / Floorboard Warm Wood)
    Color poke_brick;       // #B85040 (Retro Brick Wall)
    Color poke_leaf_dark;   // #286028 (Pokemon GBC Tree Shadow)
    Color poke_leaf_light;  // #58A848 (Pokemon GBC Tree Highlight)
} GBCPalette;

GBCPalette Palette_GetDefault(void);
Color Palette_LerpColor(Color a, Color b, float t);
Color Palette_ApplyMonochrome(Color c, float saturation);

#endif // PALETTE_H
