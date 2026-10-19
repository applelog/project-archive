#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/scenario.h"
#include "../src/audio_synth.h"

int main(void) {
    printf("=== Running Task 6: Narrative Engine & Multi-Endings Tests ===\n");
    
    // Set up mock windowless Raylib audio
    SetTraceLogLevel(LOG_NONE);
    InitAudioDevice();
    AudioSynth_Init();
    
    // 1. Scenario_Init
    ScenarioManager sm;
    Scenario_Init(&sm);
    assert(sm.shard_count == 4);
    assert(sm.npc_count == 4);
    assert(!sm.dialogue_active);
    assert(sm.current_dialogue_npc == -1);
    printf("[TEST] 1. Scenario_Init assertions passed.\n");
    
    // 2. Memory Shard collection
    Player player;
    player.x = 320.0f;
    player.y = 265.0f;
    player.collected_shards = 0;
    player.memory_clarity = 0.15f;
    player.current_area = AREA_SUBWAY_PLATFORM;
    
    bool collected = Scenario_TryInteract(&sm, &player);
    assert(collected);
    assert(sm.shards[0].is_collected);
    assert(player.collected_shards == 1);
    assert(player.memory_clarity > 0.15f);
    printf("[TEST] 2. Shard collection passed.\n");
    
    // 3. NPC Interaction & Dialogue Dismissal
    player.x = 420.0f;
    player.y = 220.0f;
    bool talked = Scenario_TryInteract(&sm, &player);
    assert(talked);
    assert(sm.dialogue_active);
    assert(sm.current_dialogue_npc == 0);
    
    // Pressing interact again while dialogue is active MUST dismiss it
    bool dismissed = Scenario_TryInteract(&sm, &player);
    assert(dismissed);
    assert(!sm.dialogue_active);
    assert(sm.current_dialogue_npc == -1);
    printf("[TEST] 3. NPC interaction and instant dialogue dismissal passed.\n");
    
    // 4. Multi-ending logic
    player.collected_shards = 4;
    assert(Scenario_EvaluateEnding(&sm, &player) == STATE_ENDING_A_TRUE);
    
    player.collected_shards = 2;
    sm.alert_gauge = 100.0f;
    assert(Scenario_EvaluateEnding(&sm, &player) == STATE_ENDING_B_FAMILY);
    
    player.collected_shards = 0;
    assert(Scenario_EvaluateEnding(&sm, &player) == STATE_ENDING_C_HOSPITAL);
    printf("[TEST] 4. Multi-ending branching logic passed.\n");
    
    AudioSynth_Close();
    CloseAudioDevice();
    printf("[PASS] All Narrative Engine & Scenario tests passed successfully!\n");
    return 0;
}
