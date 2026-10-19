#ifndef SCENARIO_H
#define SCENARIO_H

#include "types.h"
#include "palette.h"
#include "scenario_data.h"

typedef struct {
    bool active;
    NPC* current_npc;
    char inspection_title[32];
    char inspection_desc[96];
    bool is_inspecting_prop;
    int selected_choice;
    float typewriter_timer;
    bool awaiting_choice;
    MemoryLog memory_log;
} ScenarioManager;

void Scenario_Init(ScenarioManager* mgr, PlayerState* player);
void Scenario_Update(ScenarioManager* mgr, PlayerState* player, float dt);
void Scenario_Render(const ScenarioManager* mgr, float saturation);
bool Scenario_TryInteract(ScenarioManager* mgr, PlayerState* player);
void Scenario_ShowMessage(ScenarioManager* mgr, PlayerState* player, const char* title, const char* msg);
const MemoryLog* Scenario_GetMemoryLog(const ScenarioManager* mgr);

#endif // SCENARIO_H
