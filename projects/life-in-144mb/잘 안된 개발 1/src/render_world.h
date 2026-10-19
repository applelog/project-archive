#ifndef RENDER_WORLD_H
#define RENDER_WORLD_H

#include <raylib.h>
#include "types.h"
#include "palette.h"

#define MAX_MAP_W 60
#define MAX_MAP_H 50
#define TILE_SIZE 24

typedef enum {
    TILE_VOID = 0,          // 맵 외곽 빈 공간
    TILE_GRASS,             // 잔디
    TILE_TALL_GRASS,        // 풀숲
    TILE_PATH_DIRT,         // 흙길
    TILE_PATH_STONE,        // 보도블록
    TILE_PATH_ASPHALT,      // 도로 아스팔트
    TILE_CROSSWALK,         // 횡단보도
    TILE_WATER,             // 물
    TILE_WATER_SHORE,       // 물가
    TILE_WALL_BRICK,        // 붉은 벽돌벽
    TILE_WALL_CONCRETE,     // 시멘트벽
    TILE_WALL_WOOD,         // 나무벽
    TILE_WALL_INTERIOR,     // 실내 도배 벽면
    TILE_FLOOR_WOOD,        // 실내 장판/원목 마루
    TILE_FLOOR_TILE,        // 타일 바닥
    TILE_FLOOR_CARPET,      // 카펫 바닥
    TILE_WINDOW,            // 창문
    TILE_DOOR_HOME,         // 우리 집 문
    TILE_DOOR_LOCKED,       // 남의 집 문 (잠김)
    TILE_DOOR_SHOP,         // 문방구 문
    TILE_DOOR_SCHOOL,       // 학교 교실 문
    TILE_DOOR_POCHA,        // 포장마차 입구
    TILE_DOOR_APT,          // 아파트 현관문
    TILE_DOOR_OFFICE,       // 회사 빌딩 문
    TILE_DOORMAT,           // 실내 출구 워프 매트
    TILE_TREE_BIG,          // 큰 나무
    TILE_TREE_CHERRY,       // 벚꽃나무
    TILE_TREE_PINE,         // 소나무
    TILE_FLOWER_GARDEN,     // 화단
    TILE_FENCE_WOOD,        // 나무 울타리
    TILE_FENCE_IRON,        // 철제 펜스
    TILE_BED,               // 침대 / 요람
    TILE_TABLE,             // 식탁 / 테이블
    TILE_TV,                // TV 가전
    TILE_FRIDGE,            // 냉장고
    TILE_BOOKSHELF,         // 책장
    TILE_SINK,              // 싱크대
    TILE_BENCH,             // 벤치
    TILE_PYEONGSANG,        // 평상
    TILE_STATIONERY_SHELF,  // 문방구 선반
    TILE_ARCADE_MACHINE,    // 미니 오락기
    TILE_VENDING_MACHINE,   // 자판기
    TILE_POJANGMACHA_POT,   // 포차 솥
    TILE_GAZEBO,            // 전통 정자
    TILE_POLE,              // 전신주
    TILE_STREETLAMP,        // 가로등
    TILE_BLACKBOARD,        // 학교 칠판
    TILE_DESK,              // 책걸상
    TILE_SOCCER_GOAL,       // 축구골대
    TILE_STAIRS_UP,         // 올라가는 계단
    TILE_STAIRS_DOWN,       // 내려가는 계단
    TILE_MAILBOX,           // 우편함
    TILE_DOORBELL,          // 도어락
    TILE_COUNT
} TileType;

void RenderWorld_Init(void);
void RenderWorld_InitFont(void);
Font RenderWorld_GetKoreanFont(void);
void RenderWorld_GetMapDimensions(MapId map_id, int* out_w, int* out_h);
TileType RenderWorld_GetTile(MapId map_id, int x, int y);

void RenderWorld_DrawMap(MapId map_id, const MasterPalette* pal, float saturation, LifeAct act, Camera2D camera, float sensor_light);
void RenderWorld_DrawNPC(const NPC* npc, const MasterPalette* pal, float saturation, float anim_time);
void RenderWorld_DrawPlayerTopDown(const PlayerState* player, const MasterPalette* pal);
void RenderWorld_DrawInfant1stPerson(float blink_alpha, float clarity, int milestone, float baby_hand_y, float mobile_angle);
void RenderWorld_DrawColorBloomWave(float center_x, float center_y, float radius, float max_radius);
void RenderWorld_DrawWarpFade(float fade_alpha);
void RenderWorld_DrawDialogueBox(const char* speaker, const char* role_title,
                                 const char* line1, const char* line2, 
                                 const char* choice_a, const char* choice_b, int selected_choice, 
                                 float saturation);
void RenderWorld_DrawHUD(const PlayerState* player, float lived_years, float saturation);
void RenderWorld_DrawLocationBanner(const char* location_name, float timer);
void RenderWorld_DrawPixelText(const char* text, int x, int y, int size, Color color);

#endif // RENDER_WORLD_H
