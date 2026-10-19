#ifndef FONT_KOREAN_H
#define FONT_KOREAN_H

#include <raylib.h>
#include <stdint.h>

void FontKorean_Init(void);
Font FontKorean_GetFont(void);
void FontKorean_DrawText(const char* text, int x, int y, int size, Color color);
int  FontKorean_MeasureTextWidth(const char* text, int size);
void FontKorean_Close(void);

#endif // FONT_KOREAN_H
