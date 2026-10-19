#ifndef SCENARIO_H
#define SCENARIO_H

#include "types.h"

typedef struct {
    MemoryShard shards[MAX_MEMORIES];
    int shard_count;
    NPC npcs[MAX_NPCS];
    int npc_count;
    int current_dialogue_npc;
    bool dialogue_active;
    float bloom_timer;
    float bloom_x, bloom_y;
    float alert_gauge; // 0.0 to 100.0 (Wandering / Family searching)
    
    // Tutorial flags
    bool tut_moved;
    bool tut_talked;
    bool tut_shard_collected;
    bool tut_boarded;
} ScenarioManager;

void Scenario_Init(ScenarioManager* sm);
void Scenario_Update(ScenarioManager* sm, Player* player, float dt);
bool Scenario_TryInteract(ScenarioManager* sm, Player* player);
GameState Scenario_EvaluateEnding(const ScenarioManager* sm, const Player* player);

#endif // SCENARIO_H
