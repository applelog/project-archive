#include "reminiscence.h"
#include "audio_synth.h"
#include "render_world.h"
#include <stdio.h>
#include <math.h>

void Reminiscence_Init(ReminiscenceEngine* rem) {
    rem->is_active = false;
    rem->current_memory_idx = 0;
    rem->slide_timer = 0.0f;
    rem->slide_duration = 6.0f; // 6 seconds per scenic memory snapshot
    rem->fade_alpha = 1.0f;
    rem->is_finished = false;
}

void Reminiscence_Start(ReminiscenceEngine* rem) {
    rem->is_active = true;
    rem->current_memory_idx = 0;
    rem->slide_timer = 0.0f;
    rem->fade_alpha = 1.0f;
    rem->is_finished = false;
    AudioSynth_PlayBGM(ACT_5_DEATH_REPLAY);
}

void Reminiscence_Update(ReminiscenceEngine* rem, const MemoryLog* log, float dt) {
    if (!rem->is_active) return;

    rem->slide_timer += dt;

    // Fade-in and fade-out effect per memory slide
    if (rem->slide_timer < 1.0f) {
        rem->fade_alpha = 1.0f - rem->slide_timer;
    } else if (rem->slide_timer > (rem->slide_duration - 1.0f)) {
        rem->fade_alpha = rem->slide_timer - (rem->slide_duration - 1.0f);
    } else {
        rem->fade_alpha = 0.0f;
    }

    // Skip / Advance slide with SPACE / Z
    if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_RIGHT)) {
        rem->slide_timer = rem->slide_duration;
    }

    if (rem->slide_timer >= rem->slide_duration) {
        rem->slide_timer = 0.0f;
        rem->current_memory_idx++;
        AudioSynth_PlaySFX(SFX_MEMORY_DISCOVER);

        if (log && rem->current_memory_idx >= log->count) {
            rem->is_finished = true;
            AudioSynth_PlayBGM(ACT_ENDING);
        }
    }
}

void Reminiscence_Render(const ReminiscenceEngine* rem, const MemoryLog* log) {
    if (!rem->is_active) return;

    // Warm golden nostalgic background
    ClearBackground((Color){ 24, 20, 32, 255 });

    // Starlight / Floating golden memory dust particles
    float t = GetTime();
    for (int i = 0; i < 30; i++) {
        int px = (int)((i * 37 + (int)(t * 15.0f)) % SCREEN_WIDTH);
        int py = (int)((i * 29 + (int)(sinf(t + i) * 20.0f) + 180) % SCREEN_HEIGHT);
        DrawCircle(px, py, 2, (Color){ 255, 230, 140, 180 });
    }

    // If all memories replayed: Show Final Life Album & Ending
    if (rem->is_finished || !log || log->count == 0) {
        DrawRectangle(40, 30, SCREEN_WIDTH - 80, SCREEN_HEIGHT - 60, (Color){ 30, 26, 40, 240 });
        DrawRectangleLines(40, 30, SCREEN_WIDTH - 80, SCREEN_HEIGHT - 60, (Color){ 255, 215, 64, 255 });
        DrawRectangleLines(44, 34, SCREEN_WIDTH - 88, SCREEN_HEIGHT - 68, (Color){ 240, 107, 50, 255 });

        RenderWorld_DrawPixelText("《 당신이 걸어온 인생의 앨범 》", SCREEN_WIDTH / 2 - 140, 50, 18, (Color){ 255, 220, 100, 255 });
        RenderWorld_DrawPixelText("어릴 적 흑백 같았던 세상도, 사랑을 품었던 모든 날들도...", SCREEN_WIDTH / 2 - 190, 80, 13, (Color){ 230, 230, 230, 255 });
        RenderWorld_DrawPixelText("돌아보니 찬란한 하나의 빛나는 봄날이었습니다.", SCREEN_WIDTH / 2 - 170, 100, 13, (Color){ 255, 180, 200, 255 });

        // List recorded highlights
        int start_y = 135;
        for (int i = 0; i < log->count && i < 4; i++) {
            const MemoryShard* s = &log->items[i];
            char buf[128];
            snprintf(buf, sizeof(buf), "♥ %s : %s", s->title, s->description);
            RenderWorld_DrawPixelText(buf, 60, start_y + i * 32, 12, (Color){ 255, 245, 220, 255 });
        }

        RenderWorld_DrawPixelText("[ R : 새로운 인생 다시 시작하기 ]", SCREEN_WIDTH / 2 - 130, SCREEN_HEIGHT - 55, 14, (Color){ 255, 215, 64, 255 });
        return;
    }

    // Current Memory Snapshot Display
    const MemoryShard* shard = &log->items[rem->current_memory_idx];

    // Frame Card
    int card_x = 50;
    int card_y = 35;
    int card_w = SCREEN_WIDTH - 100;
    int card_h = SCREEN_HEIGHT - 70;

    DrawRectangle(card_x, card_y, card_w, card_h, (Color){ 36, 32, 46, 245 });
    DrawRectangleLines(card_x, card_y, card_w, card_h, (Color){ 255, 215, 64, 255 });

    // Title Tag
    char tag_buf[64];
    snprintf(tag_buf, sizeof(tag_buf), "기억의 순간 (%d / %d)", rem->current_memory_idx + 1, log->count);
    RenderWorld_DrawPixelText(tag_buf, card_x + 20, card_y + 18, 12, (Color){ 240, 140, 80, 255 });

    RenderWorld_DrawPixelText(shard->title, card_x + 20, card_y + 36, 18, (Color){ 255, 235, 120, 255 });

    // Scene Vignette Window (Recreating the setting in FULL GLORIOUS COLOR!)
    int win_x = card_x + 20;
    int win_y = card_y + 68;
    int win_w = card_w - 40;
    int win_h = 110;

    // Full Vibrant Seasonal Palette based on the memory's act
    MasterPalette mem_pal = Palette_GetForAct((LifeAct)shard->act_id);
    DrawRectangle(win_x, win_y, win_w, win_h, mem_pal.sky);
    DrawRectangle(win_x, win_y + 60, win_w, 50, mem_pal.ground);
    DrawRectangleLines(win_x, win_y, win_w, win_h, (Color){ 255, 255, 255, 200 });

    // Full color iconic scene illustration
    DrawCircle(win_x + 50, win_y + 35, 20, mem_pal.sky_horizon);
    DrawCircle(win_x + win_w - 60, win_y + 65, 28, mem_pal.foliage_dark);
    DrawCircle(win_x + win_w - 70, win_y + 55, 20, mem_pal.foliage_light);
    DrawRectangle(win_x + 120, win_y + 40, 40, 35, mem_pal.structure_wall);
    DrawTriangle((Vector2){ win_x + 115, win_y + 40 }, (Vector2){ win_x + 165, win_y + 40 }, (Vector2){ win_x + 140, win_y + 20 }, mem_pal.structure_roof);

    // Warm Hearts
    RenderWorld_DrawPixelText("♥", win_x + win_w / 2 - 8, win_y + 30, 24, (Color){ 255, 100, 140, 240 });

    // Description text
    RenderWorld_DrawPixelText(shard->description, card_x + 20, card_y + 195, 13, (Color){ 245, 245, 245, 255 });

    RenderWorld_DrawPixelText("▶ [SPACE / Z : 다음 기억으로]", card_x + card_w - 190, card_y + card_h - 22, 12, (Color){ 200, 200, 200, 200 });

    // Transition Fade Overlay
    if (rem->fade_alpha > 0.01f) {
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){ 10, 8, 14, (unsigned char)(255 * rem->fade_alpha) });
    }
}
