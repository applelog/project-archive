#include <assert.h>
#include <stdio.h>
#include <math.h>
#include "../src/life_engine.h"
#include "../src/audio_synth.h"
#include "../src/font_korean.h"

int main(void) {
    printf("=== Running Task 7: Game Life Loop & Engine Integration Tests ===\n");
    
    // 1. Test Windowless/Mock Audio & Font init
    SetTraceLogLevel(LOG_NONE);
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(100, 100, "LifeEngineTest");
    
    AudioSynth_Init();
    FontKorean_Init();
    
    // 2. Test LifeEngine_Init()
    LifeEngine engine;
    LifeEngine_Init(&engine);
    
    assert(engine.state == STATE_INTRO_DARK_PROLOGUE);
    assert(engine.intro_timer == 0.0f);
    assert(engine.prologue_line_index == 0);
    assert(engine.player.collected_shards == 0);
    assert(engine.scenario.shard_count == 4);
    assert(engine.scenario.npc_count == 4);
    printf("[PASS] LifeEngine_Init verified.\n");
    
    // 3. Test Dark Prologue Transition to Eye Awakening
    for (int i = 0; i < 3; i++) {
        LifeEngine_Update(&engine, 4.0f);
    }
    assert(engine.state == STATE_INTRO_EYE_AWAKENING);
    printf("[PASS] STATE_INTRO_DARK_PROLOGUE -> STATE_INTRO_EYE_AWAKENING verified.\n");
    
    // 4. Test Eye Awakening to Mirror Talk
    LifeEngine_Update(&engine, 5.0f);
    assert(engine.state == STATE_INTRO_MIRROR_TALK);
    printf("[PASS] STATE_INTRO_EYE_AWAKENING -> STATE_INTRO_MIRROR_TALK verified.\n");
    
    // 5. Test Mirror Talk to Zoom Out
    LifeEngine_Update(&engine, 5.0f);
    assert(engine.state == STATE_INTRO_ZOOM_OUT);
    printf("[PASS] STATE_INTRO_MIRROR_TALK -> STATE_INTRO_ZOOM_OUT verified.\n");
    
    // 6. Test Zoom Out to Tutorial
    LifeEngine_Update(&engine, 3.0f);
    assert(engine.state == STATE_TUTORIAL_MOVE);
    printf("[PASS] STATE_INTRO_ZOOM_OUT -> STATE_TUTORIAL_MOVE verified.\n");
    
    // 7. Test Ending Transition & Reset
    engine.state = STATE_GAMEPLAY;
    engine.player.collected_shards = 4;
    LifeEngine_Update(&engine, 0.1f);
    assert(engine.state == STATE_ENDING_A_TRUE);
    
    // Re-init
    LifeEngine_Init(&engine);
    assert(engine.state == STATE_INTRO_DARK_PROLOGUE);
    assert(engine.player.collected_shards == 0);
    printf("[PASS] Multi-ending evaluation and full game restart verified.\n");
    
    // 8. Test Render Frame Safety
    BeginDrawing();
    LifeEngine_Render(&engine);
    EndDrawing();
    
    engine.state = STATE_SUBWAY_RIDING_CUTSCENE;
    BeginDrawing();
    LifeEngine_Render(&engine);
    EndDrawing();
    
    engine.state = STATE_SUBWAY_MAP_SELECT;
    BeginDrawing();
    LifeEngine_Render(&engine);
    EndDrawing();
    
    engine.state = STATE_GAMEPLAY;
    BeginDrawing();
    LifeEngine_Render(&engine);
    EndDrawing();
    
    engine.state = STATE_ENDING_A_TRUE;
    BeginDrawing();
    LifeEngine_Render(&engine);
    EndDrawing();
    
    AudioSynth_Close();
    CloseWindow();
    
    printf("[PASS] All LifeEngine integration & render tests passed successfully!\n");
    return 0;
}
