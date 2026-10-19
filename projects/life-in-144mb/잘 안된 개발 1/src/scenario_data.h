#ifndef SCENARIO_DATA_H
#define SCENARIO_DATA_H

#include "types.h"
#include <stddef.h>

extern NPC g_stage_npcs[5][MAX_NPCS];
extern int g_stage_npc_counts[5];

void ScenarioData_Init(PlayerState* player);
void ScenarioData_GetDialogue(NPC* npc, const PlayerState* player,
                              char* out_line1, size_t sz1,
                              char* out_line2, size_t sz2,
                              char* out_choice_a, size_t sza,
                              char* out_choice_b, size_t szb);
void ScenarioData_ApplyChoice(NPC* npc, PlayerState* player, int choice, MemoryLog* mem_log);

bool ScenarioData_CheckInspectableProp(MapId map_id, int x, int y, char* out_title, size_t sz_t, char* out_desc, size_t sz_d);

#endif // SCENARIO_DATA_H
