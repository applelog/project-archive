#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <raylib.h>

#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 360
#define TARGET_FPS    60
#define TILE_SIZE     24

#define MAX_MEMORIES  8
#define MAX_NPCS      12
#define MAX_TRAIN_CARS 3

typedef enum {
    DIR_DOWN = 0,
    DIR_UP,
    DIR_LEFT,
    DIR_RIGHT
} Direction;

typedef enum {
    // 1. Deep Multi-phase Cinematic Intro
    STATE_INTRO_DARK_PROLOGUE = 0,   // Dark screen with typing monologue & music box chime
    STATE_INTRO_EYE_AWAKENING,       // 1st person blurred eye opening & focus shift
    STATE_INTRO_MIRROR_TALK,         // Looking at 20s reflection & touching glass
    STATE_INTRO_ZOOM_OUT,            // Train roars in & smooth 3D zoom out
    
    // 2. Interactive Tutorial
    STATE_TUTORIAL_MOVE,             // Movement tutorial
    STATE_TUTORIAL_TALK,             // NPC dialogue tutorial
    STATE_TUTORIAL_SHARD,            // Clue inspection & color bloom tutorial
    STATE_TUTORIAL_BOARD,            // Train boarding tutorial
    
    // 3. Gameplay & Subway Transit
    STATE_GAMEPLAY,                  // Free roaming across areas
    STATE_SUBWAY_MAP_SELECT,         // Station selection menu
    STATE_SUBWAY_RIDING_CUTSCENE,    // Animated train transit cutscene
    
    // 4. Multi-Endings
    STATE_ENDING_A_TRUE,             // True realization ending
    STATE_ENDING_B_FAMILY,           // Family guide home ending
    STATE_ENDING_C_HOSPITAL          // Hospitalization ending
} GameState;

typedef enum {
    AREA_SUBWAY_PLATFORM = 0,
    AREA_MARKET_ALLEY,
    AREA_SCHOOL_PLAYGROUND,
    AREA_OLD_HOUSE,
    AREA_COUNT
} AreaId;

typedef struct {
    float x, y;
    float vx, vy;
    Direction dir;
    int anim_frame;
    float anim_timer;
    bool is_moving;
    
    // Core attributes
    float memory_clarity; // 0.0 (Monochrome/Confused) to 1.0 (Full Color)
    int collected_shards;
    float wandering_time; // Accumulating time towards night
    AreaId current_area;
} Player;

typedef struct {
    int id;
    AreaId area;
    float x, y;
    Direction dir;
    char name[32];
    char dialogue_initial[128];
    char dialogue_hesitant[128];
    bool is_speaking;
} NPC;

typedef struct {
    int id;
    AreaId area;
    float x, y;
    char name[48];
    char memory_text[160];
    bool is_collected;
} MemoryShard;

typedef struct {
    float x;
    float target_x;
    float speed;
    bool is_stopped;
    bool doors_open;
    float door_open_ratio; // 0.0 (closed) to 1.0 (fully open)
    float stop_timer;
} SubwayTrain;

#endif // TYPES_H
