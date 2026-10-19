#ifndef REMINISCENCE_H
#define REMINISCENCE_H

#include "types.h"
#include "palette.h"
#include <raylib.h>

typedef struct {
    bool is_active;
    int current_memory_idx;
    float slide_timer;
    float slide_duration;
    float fade_alpha;
    bool is_finished;
} ReminiscenceEngine;

void Reminiscence_Init(ReminiscenceEngine* rem);
void Reminiscence_Start(ReminiscenceEngine* rem);
void Reminiscence_Update(ReminiscenceEngine* rem, const MemoryLog* log, float dt);
void Reminiscence_Render(const ReminiscenceEngine* rem, const MemoryLog* log);

#endif // REMINISCENCE_H
