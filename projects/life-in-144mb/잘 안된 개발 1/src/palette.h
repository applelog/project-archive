#ifndef PALETTE_H
#define PALETTE_H

#include <raylib.h>
#include "types.h"

typedef struct {
    Color sky;
    Color sky_horizon;
    Color cloud;
    Color ground;
    Color path;
    Color foliage_dark;
    Color foliage_light;
    Color structure_wall;
    Color structure_roof;
    Color accent;
    Color shadow;
    Color light;
} MasterPalette;

void Palette_Init(void);
MasterPalette Palette_GetForAct(LifeAct act);
Color Palette_Lerp(Color c1, Color c2, float t);
MasterPalette Palette_Interpolate(const MasterPalette* a, const MasterPalette* b, float t);
Color Palette_ApplySaturation(Color c, float saturation);

#endif // PALETTE_H
