#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stdbool.h>

#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 360
#define TARGET_FPS    60
#define MAX_MEMORIES  48
#define MAX_NPCS      16
#define MAX_WARPS     16
#define MAX_PROPS     24

typedef enum {
    DIR_DOWN = 0,
    DIR_UP,
    DIR_LEFT,
    DIR_RIGHT
} Direction;

typedef enum {
    ACT_0_INFANT = 0,   // 1인칭 아기 시점 (모빌, 안방, 부모님 얼굴 내려다봄)
    ACT_1_CHILDHOOD,    // 유년기 (봄): 골목길, 등굣길, [문 진입: 우리집, 문방구, 교실]
    ACT_2_YOUTH,        // 청년기 (여름): 대학 캠퍼스, 한강, [문 진입: 동아리방, 도서관, 포장마차]
    ACT_3_ADULTHOOD,    // 성인기 (가을): 회사, [문 진입: 사무실, 아파트 계단실, 302호 안방]
    ACT_4_ELDERLY,      // 노년기 (겨울): 황혼의 마을, [문 진입: 경로당, 고향집]
    ACT_5_DEATH_REPLAY, // 임종: 전 생애가 [찬란한 한국의 사계절 풀컬러]로 시네마틱 파노라마 회상
    ACT_ENDING          // 에필로그 & 인생 기록 앨범
} LifeAct;

// 맵 ID (야외 & 실내 건물 분리)
typedef enum {
    MAP_OVERWORLD = 0,      // 야외 메인 맵
    MAP_MY_HOME,            // 우리 집 실내
    MAP_STATIONERY_STORE,   // 문방구 실내
    MAP_CLASSROOM,          // 초등학교 교실 실내
    MAP_POJANGMACHA_INTERIOR,// 포장마차 실내
    MAP_APARTMENT_STAIRS,   // 아파트 계단실 (1F->2F->3F)
    MAP_APARTMENT_302,      // 우리 집 아파트 302호 실내
    MAP_OFFICE_INTERIOR,    // 회사 사무실 실내
    MAP_SENIOR_HALL,        // 경로당 실내
    MAP_COUNT
} MapId;

// 부모님 관계 상태 (확률적 결정)
typedef enum {
    PARENTS_HARMONIOUS = 0, // 화목하고 다정한 부모님
    PARENTS_CONFLICT,       // 성격 차이로 다투시는 부모님
    PARENTS_DIVORCED,       // 이혼 후 홀어머니/홀아버지와 함께
    PARENTS_RECONCILED      // 갈등 끝에 늦게나마 서로 화해한 부모님
} ParentStatus;

// 절친 유형
typedef enum {
    FRIEND_NEIGHBOR = 0,    // 옆집 소꿉친구 (민준)
    FRIEND_STATIONERY,      // 동네 문방구집 개구쟁이 (태호)
    FRIEND_CLASSMATE        // 초중고 평생 의리 동창 (동현)
} BestFriendType;

// 고백/결혼 상대 NPC ID
typedef enum {
    ROMANCE_NONE = 0,
    ROMANCE_SUA = 1,        // 소꿉친구 수아 (따뜻한 추억과 순정)
    ROMANCE_JIEUN = 2,      // 동아리 후배 지은 (발랄하고 사랑스러운 후배)
    ROMANCE_HYEJIN = 3      // 직장 동료 혜진 (지적이고 든든한 파트너)
} RomanceTarget;

// 포켓몬식 문/워프 정의
typedef struct {
    int from_map;
    int trigger_x, trigger_y;   // 워프 타일 좌표
    int target_map;
    int spawn_x, spawn_y;       // 도착 타일 좌표
    Direction spawn_dir;
    bool is_locked;             // 남의 집 등 잠금 여부
    char locked_msg[64];        // "남의 집이다. 문이 굳게 잠겨 있다."
} MapWarp;

// 포켓몬식 조사 가능한 오브젝트 (TV, 냉장고, 책장, 칠판 등)
typedef struct {
    int map_id;
    int x, y;
    char title[32];
    char message[96];
} InspectableProp;

typedef struct {
    float x, y;
    float vx, vy;
    Direction dir;
    float age;
    float progress;
    bool is_moving;
    float anim_timer;
    bool in_dialogue;
    int current_act;
    MapId current_map;          // 현재 머무는 맵
    
    // 사랑 & 색감 시스템
    float color_saturation;
    float color_spark_timer;
    float color_bloom_radius;
    bool true_love_awakened;
    bool is_married;
    RomanceTarget married_target;
    char spouse_name[32];
    int career_score;
    int family_score;

    // 인생 배경 설정 (RNG)
    ParentStatus parent_status;
    BestFriendType best_friend_type;
    char best_friend_name[32];

    // 아파트 계단실 센서등
    float stair_sensor_light;
    
    // 맵 전환 페이드
    float warp_fade_timer;      // 0.0 ~ 1.0 (페이드 인/아웃)
    int pending_warp_target_map;
    float pending_warp_spawn_x;
    float pending_warp_spawn_y;
    Direction pending_warp_spawn_dir;
    bool is_warping;
} PlayerState;

typedef struct {
    int id;
    MapId map_id;               // 어느 맵에 존재하는지
    float x, y;
    Direction dir;
    char name[32];
    char role_title[32];
    int sprite_id;              // 0: 어머니, 1: 아버지, 2: 절친, 3: 수아, 4: 지은, 5: 혜진, 6: 노년어르신, 7: 자녀
    int affection;              // 0 ~ 100
    int dialogue_stage;
    bool is_romanceable;
    bool is_married;
    bool is_best_friend;
    bool talked_in_current_act;
} NPC;

typedef struct {
    int act_id;
    float age_at_time;
    char title[48];
    char description[144];
    int choice_index;
    int scene_snapshot_id;
    bool was_in_color;
} MemoryShard;

typedef struct {
    MemoryShard items[MAX_MEMORIES];
    int count;
} MemoryLog;

#endif // TYPES_H
