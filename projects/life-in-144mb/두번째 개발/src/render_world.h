#ifndef RENDER_WORLD_H
#define RENDER_WORLD_H

#include "types.h"
#include "palette.h"

void Render_Init(void);
void Render_DrawFake3DShadow(float x, float y, float rx, float ry);
void Render_DrawCharacter(float x, float y, Direction dir, int anim_frame, bool is_player, float saturation);
void Render_DrawMovingSubway(const SubwayTrain* train, float saturation);
void Render_DrawAreaMap(AreaId area, float saturation);
void Render_DrawOpeningEyeBlink(float eye_open_ratio);
void Render_DrawMirrorReflection(float zoom, float alpha);
void Render_DrawIntroDarkPrologue(float timer, const char* monologue_line);
void Render_DrawIntroMirrorAwakening(float blur_ratio, float mirror_zoom, const char* monologue_line);
void Render_DrawSubwayInteriorTransit(float transit_timer, AreaId dest_area, float saturation);
void Render_DrawStationSelectMenu(int selected_index, AreaId current_area);
void Render_DrawTutorialBanner(const char* prompt, int step, int total_steps);
void Render_DrawDialogueBox(const char* speaker, const char* text, float saturation);
void Render_DrawColorBloom(float x, float y, float radius);

#endif // RENDER_WORLD_H
