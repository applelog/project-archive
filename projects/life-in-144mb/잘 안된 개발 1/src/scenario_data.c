#include "scenario_data.h"
#include "audio_synth.h"
#include "render_world.h"
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

NPC g_stage_npcs[5][MAX_NPCS];
int g_stage_npc_counts[5] = {0, 0, 0, 0, 0};

void ScenarioData_Init(PlayerState* player) {
    // 1. 부모님 관계 확률 설정 (Random Family Dynamics)
    int roll = GetRandomValue(1, 100);
    if (roll <= 45) {
        player->parent_status = PARENTS_HARMONIOUS; // 45% 화목
    } else if (roll <= 70) {
        player->parent_status = PARENTS_CONFLICT;   // 25% 다툼
    } else if (roll <= 90) {
        player->parent_status = PARENTS_DIVORCED;   // 20% 이혼/홀어머니
    } else {
        player->parent_status = PARENTS_RECONCILED; // 10% 재결합
    }

    // 2. 평생의 절친 유형 설정
    int f_roll = GetRandomValue(0, 2);
    player->best_friend_type = (BestFriendType)f_roll;
    if (player->best_friend_type == FRIEND_NEIGHBOR) {
        strncpy(player->best_friend_name, "소꿉친구 민준", sizeof(player->best_friend_name) - 1);
    } else if (player->best_friend_type == FRIEND_STATIONERY) {
        strncpy(player->best_friend_name, "문방구집 태호", sizeof(player->best_friend_name) - 1);
    } else {
        strncpy(player->best_friend_name, "의리파 동현", sizeof(player->best_friend_name) - 1);
    }

    // ==========================================
    // STAGE 0 (ACT 1: 유년기 실내 & 야외)
    // ==========================================
    g_stage_npc_counts[0] = 5;

    // NPC 0: 어머니 (우리 집 실내 안방)
    g_stage_npcs[0][0] = (NPC){
        .id = 101,
        .map_id = MAP_MY_HOME,
        .x = 6 * 24.0f,
        .y = 6 * 24.0f,
        .dir = DIR_DOWN,
        .name = "어머니",
        .role_title = "우리 집 안방",
        .sprite_id = 0,
        .affection = 50,
        .is_romanceable = false,
        .is_married = false,
        .is_best_friend = false,
        .talked_in_current_act = false
    };

    // NPC 1: 아버지 (우리 집 실내 거실)
    g_stage_npcs[0][1] = (NPC){
        .id = 102,
        .map_id = MAP_MY_HOME,
        .x = 10 * 24.0f,
        .y = 6 * 24.0f,
        .dir = DIR_DOWN,
        .name = "아버지",
        .role_title = "",
        .sprite_id = 1,
        .affection = 40,
        .is_romanceable = false,
        .is_married = false,
        .is_best_friend = false,
        .talked_in_current_act = false
    };
    strncpy(g_stage_npcs[0][1].role_title, (player->parent_status == PARENTS_DIVORCED) ? "멀리서 온 편지" : "거실 소파", sizeof(g_stage_npcs[0][1].role_title) - 1);

    // NPC 2: ★ 평생의 절친 (문방구 앞 평상 / 야외)
    g_stage_npcs[0][2] = (NPC){
        .id = 1,
        .map_id = MAP_OVERWORLD,
        .x = 24 * 24.0f,
        .y = 25 * 24.0f,
        .dir = DIR_LEFT,
        .name = "",
        .role_title = "문방구 앞 평상",
        .sprite_id = 2,
        .affection = 30,
        .is_romanceable = false,
        .is_married = false,
        .is_best_friend = true,
        .talked_in_current_act = false
    };
    strncpy(g_stage_npcs[0][2].name, player->best_friend_name, sizeof(g_stage_npcs[0][2].name) - 1);

    // NPC 3: 초등학교 담임선생님 (1학년 2반 교실 실내 교탁)
    g_stage_npcs[0][3] = (NPC){
        .id = 103,
        .map_id = MAP_CLASSROOM,
        .x = 9 * 24.0f,
        .y = 3 * 24.0f,
        .dir = DIR_DOWN,
        .name = "선생님",
        .role_title = "교실 교탁 앞",
        .sprite_id = 0,
        .affection = 20,
        .is_romanceable = false,
        .is_married = false,
        .is_best_friend = false,
        .talked_in_current_act = false
    };

    // NPC 4: 짝꿍 수아 (1학년 2반 교실 실내 옆 짝꿍 자리)
    g_stage_npcs[0][4] = (NPC){
        .id = 2,
        .map_id = MAP_CLASSROOM,
        .x = 8 * 24.0f,
        .y = 6 * 24.0f,
        .dir = DIR_UP,
        .name = "짝꿍 수아",
        .role_title = "옆 짝꿍 자리",
        .sprite_id = 3,
        .affection = 15,
        .is_romanceable = true,
        .is_married = false,
        .is_best_friend = false,
        .talked_in_current_act = false
    };

    // ==========================================
    // STAGE 1 (ACT 2: 청년기 캠퍼스, 한강 둔치, 포차 실내)
    // ==========================================
    g_stage_npcs[1][0] = (NPC){ // ★ 절친 (포장마차 실내)
        .id = 1,
        .map_id = MAP_POJANGMACHA_INTERIOR,
        .x = 6 * 24.0f,
        .y = 4 * 24.0f,
        .dir = DIR_RIGHT,
        .name = "",
        .role_title = "포장마차 안",
        .sprite_id = 2,
        .affection = 30,
        .is_romanceable = false,
        .is_married = false,
        .is_best_friend = true,
        .talked_in_current_act = false
    };
    strncpy(g_stage_npcs[1][0].name, player->best_friend_name, sizeof(g_stage_npcs[1][0].name) - 1);

    // NPC 1: 첫사랑 수아 (야외 한강 둔치 벤치)
    g_stage_npcs[1][1] = (NPC){
        .id = 2,
        .map_id = MAP_OVERWORLD,
        .x = 38 * 24.0f,
        .y = 49 * 24.0f,
        .dir = DIR_RIGHT,
        .name = "수아",
        .role_title = "한강 둔치 벤치",
        .sprite_id = 3,
        .affection = 15,
        .is_romanceable = true,
        .is_married = false,
        .is_best_friend = false,
        .talked_in_current_act = false
    };

    // NPC 2: 동아리 후배 지은 (야외 캠퍼스 광장)
    g_stage_npcs[1][2] = (NPC){
        .id = 3,
        .map_id = MAP_OVERWORLD,
        .x = 34 * 24.0f,
        .y = 22 * 24.0f,
        .dir = DIR_DOWN,
        .name = "후배 지은",
        .role_title = "캠퍼스 분수대 앞",
        .sprite_id = 4,
        .affection = 10,
        .is_romanceable = true,
        .is_married = false,
        .is_best_friend = false,
        .talked_in_current_act = false
    };

    // NPC 3: 학회 동기 혜진 (야외 도서관 앞)
    g_stage_npcs[1][3] = (NPC){
        .id = 4,
        .map_id = MAP_OVERWORLD,
        .x = 44 * 24.0f,
        .y = 10 * 24.0f,
        .dir = DIR_DOWN,
        .name = "동기 혜진",
        .role_title = "중앙도서관 앞",
        .sprite_id = 5,
        .affection = 10,
        .is_romanceable = true,
        .is_married = false,
        .is_best_friend = false,
        .talked_in_current_act = false
    };
    g_stage_npc_counts[1] = 4;

    // ==========================================
    // STAGE 2 (ACT 3: 성인기 사무실 & 아파트 302호)
    // ==========================================
    // NPC 0: 배우자 (우리 집 아파트 302호 실내 안방)
    g_stage_npcs[2][0] = (NPC){
        .id = 200,
        .map_id = MAP_APARTMENT_302,
        .x = 11 * 24.0f,
        .y = 6 * 24.0f,
        .dir = DIR_DOWN,
        .name = "배우자",
        .role_title = "우리 집 안방",
        .sprite_id = 3,
        .affection = 100,
        .is_romanceable = false,
        .is_married = true,
        .is_best_friend = false,
        .talked_in_current_act = false
    };

    // NPC 1: ★ 절친 (야외 퇴근길)
    g_stage_npcs[2][1] = (NPC){
        .id = 1,
        .map_id = MAP_OVERWORLD,
        .x = 20 * 24.0f,
        .y = 28 * 24.0f,
        .dir = DIR_LEFT,
        .name = "",
        .role_title = "퇴근길 골목",
        .sprite_id = 2,
        .affection = 60,
        .is_romanceable = false,
        .is_married = false,
        .is_best_friend = true,
        .talked_in_current_act = false
    };
    strncpy(g_stage_npcs[2][1].name, player->best_friend_name, sizeof(g_stage_npcs[2][1].name) - 1);

    // NPC 2: 박 과장님 (회사 사무실 실내)
    g_stage_npcs[2][2] = (NPC){
        .id = 201,
        .map_id = MAP_OFFICE_INTERIOR,
        .x = 12 * 24.0f,
        .y = 5 * 24.0f,
        .dir = DIR_DOWN,
        .name = "박 과장님",
        .role_title = "회사 사무실",
        .sprite_id = 1,
        .affection = 20,
        .is_romanceable = false,
        .is_married = false,
        .is_best_friend = false,
        .talked_in_current_act = false
    };
    g_stage_npc_counts[2] = 3;

    // ==========================================
    // STAGE 3 (ACT 4: 노년기 고향마을 & 경로당 실내)
    // ==========================================
    g_stage_npcs[3][0] = (NPC){ // ★ 노년의 절친 (야외 느티나무 평상)
        .id = 1,
        .map_id = MAP_OVERWORLD,
        .x = 24 * 24.0f,
        .y = 16 * 24.0f,
        .dir = DIR_RIGHT,
        .name = "",
        .role_title = "느티나무 평상",
        .sprite_id = 6,
        .affection = 100,
        .is_romanceable = false,
        .is_married = false,
        .is_best_friend = true,
        .talked_in_current_act = false
    };
    strncpy(g_stage_npcs[3][0].name, player->best_friend_name, sizeof(g_stage_npcs[3][0].name) - 1);

    g_stage_npcs[3][1] = (NPC){ // 노년의 동반자 (경로당/고향집 실내)
        .id = 200,
        .map_id = MAP_SENIOR_HALL,
        .x = 8 * 24.0f,
        .y = 5 * 24.0f,
        .dir = DIR_UP,
        .name = "동반자",
        .role_title = "따스한 온돌방",
        .sprite_id = 0,
        .affection = 100,
        .is_romanceable = false,
        .is_married = true,
        .is_best_friend = false,
        .talked_in_current_act = false
    };

    g_stage_npcs[3][2] = (NPC){ // 장성한 자녀 (야외 집 앞)
        .id = 301,
        .map_id = MAP_OVERWORLD,
        .x = 8 * 24.0f,
        .y = 12 * 24.0f,
        .dir = DIR_LEFT,
        .name = "성장한 자녀",
        .role_title = "본가 앞마당",
        .sprite_id = 1,
        .affection = 90,
        .is_romanceable = false,
        .is_married = false,
        .is_best_friend = false,
        .talked_in_current_act = false
    };
    g_stage_npc_counts[3] = 3;

    // ==========================================
    // STAGE 4 (ACT 5: 영원의 회상 통로)
    // ==========================================
    g_stage_npcs[4][0] = (NPC){
        .id = 999,
        .map_id = MAP_OVERWORLD,
        .x = 20 * 24.0f,
        .y = 10 * 24.0f,
        .dir = DIR_DOWN,
        .name = "빛의 인도자",
        .role_title = "기억의 문",
        .sprite_id = 6,
        .affection = 100,
        .is_romanceable = false,
        .is_married = false,
        .is_best_friend = false,
        .talked_in_current_act = false
    };
    g_stage_npc_counts[4] = 1;
}

bool ScenarioData_CheckInspectableProp(MapId map_id, int x, int y, char* out_title, size_t sz_t, char* out_desc, size_t sz_d) {
    TileType t = RenderWorld_GetTile(map_id, x, y);

    if (t == TILE_TV) {
        snprintf(out_title, sz_t, "브라운관 TV");
        snprintf(out_desc, sz_d, "화면에서 옛날 한국 만화영화 주제가가 정겹게 흘러나오고 있다.");
        return true;
    } else if (t == TILE_FRIDGE) {
        snprintf(out_title, sz_t, "냉장고");
        snprintf(out_desc, sz_d, "열어보니 시원하고 구수한 보리차 유리병과 김치가 들어있다.");
        return true;
    } else if (t == TILE_BOOKSHELF) {
        snprintf(out_title, sz_t, "책장");
        snprintf(out_desc, sz_d, "손때 묻은 초등학교 교과서와 추억의 만화책들이 꽂혀있다.");
        return true;
    } else if (t == TILE_BLACKBOARD) {
        snprintf(out_title, sz_t, "교실 칠판");
        snprintf(out_desc, sz_d, "분필로 '바르고 씩씩한 1학년 2반' 급훈이 정갈하게 적혀있다.");
        return true;
    } else if (t == TILE_ARCADE_MACHINE) {
        snprintf(out_title, sz_t, "문방구 미니 오락기");
        snprintf(out_desc, sz_d, "100원 동전을 넣고 쪼그려 앉아 비행기 슈팅 게임을 하던 추억의 기계다.");
        return true;
    } else if (t == TILE_BED && map_id == MAP_APARTMENT_302) {
        snprintf(out_title, sz_t, "아기 요람");
        snprintf(out_desc, sz_d, "색색의 모빌 아래 우리 아기가 색색 숨소리를 내며 곤히 자고 있다.");
        return true;
    } else if (t == TILE_DOOR_LOCKED) {
        snprintf(out_title, sz_t, "이웃집 현관문");
        snprintf(out_desc, sz_d, "남의 집이다. 도어락이 굳게 잠겨 있어 들어갈 수 없다.");
        return true;
    }
    return false;
}

void ScenarioData_GetDialogue(NPC* npc, const PlayerState* player,
                              char* out_line1, size_t sz1,
                              char* out_line2, size_t sz2,
                              char* out_choice_a, size_t sza,
                              char* out_choice_b, size_t szb) {
    out_line1[0] = '\0';
    out_line2[0] = '\0';
    out_choice_a[0] = '\0';
    out_choice_b[0] = '\0';

    // ACT 1 유년기
    if (player->current_act == ACT_1_CHILDHOOD) {
        if (strcmp(npc->name, "어머니") == 0) {
            snprintf(out_line1, sz1, "우리 아이, 오늘 학교 첫 등교네! 가방 잘 챙겼니?");
            snprintf(out_line2, sz2, "선생님 말씀 잘 듣고, 친구들이랑 사이좋게 지내렴.");
            snprintf(out_choice_a, sza, "네 엄마! 씩씩하게 다녀올게요! (호감도 +30)");
            snprintf(out_choice_b, szb, "엄마 손 꼭 잡으며 웃어준다 (호감도 +50)");
        }
        else if (strcmp(npc->name, "선생님") == 0) {
            snprintf(out_line1, sz1, "모두 자리에 앉으세요! 오늘부터 신나는 1학년 수업을 시작합니다.");
            snprintf(out_line2, sz2, "칠판에 적힌 글씨를 공책에 바르게 써볼까요?");
            snprintf(out_choice_a, sza, "연필을 꼭 쥐고 열심히 필기한다 (학습 +30)");
            snprintf(out_choice_b, szb, "손을 번쩍 들고 씩씩하게 발표한다 (자신감 +40)");
        }
        else if (strcmp(npc->name, "짝꿍 수아") == 0) {
            snprintf(out_line1, sz1, "안녕! 우리 같은 반 짝꿍 됐네! 반가워!");
            snprintf(out_line2, sz2, "내가 아끼는 알록달록 지우개 빌려줄까?");
            snprintf(out_choice_a, sza, "고마워! 내 크레파스도 같이 쓰자! (호감도 +40)");
            snprintf(out_choice_b, szb, "쉬는 시간에 같이 그네 타러 가자! (호감도 +30)");
        }
        else if (npc->is_best_friend) {
            snprintf(out_line1, sz1, "야! 학교 끝나고 문방구 들렀다가 떡볶이 먹으러 가자!");
            snprintf(out_line2, sz2, "오늘 새로 산 메탈 팽이로 배틀 한 판 붙자!");
            snprintf(out_choice_a, sza, "좋아! 팽이 배틀 승부다! (우정 +40)");
            snprintf(out_choice_b, szb, "내가 떡꼬치 쏠게, 같이 가자! (우정 +50)");
        }
    }
    // ACT 2 청년기
    else if (player->current_act == ACT_2_YOUTH) {
        if (npc->is_best_friend) {
            snprintf(out_line1, sz1, "야, 너 좋아하는 사람한테 고백은 했냐?");
            snprintf(out_line2, sz2, "수아든, 지은이든, 혜진이든... 네 마음에 솔직해져 봐!");
            snprintf(out_choice_a, sza, "용기 내서 내 마음을 전할 거야! (용기 +40)");
            snprintf(out_choice_b, szb, "야, 일단 소주 한잔하자! (우정 +40)");
        }
        else if (strcmp(npc->name, "수아") == 0) {
            if (npc->affection < 45) {
                snprintf(out_line1, sz1, "한강 노을이 참 예쁘다... 어릴 적 초등학교 짝꿍 하던 때 기억나?");
                snprintf(out_line2, sz2, "너랑 있으면 어릴 적 따뜻했던 기억들이 전부 떠올라.");
                snprintf(out_choice_a, sza, "나한테 넌 항상 가장 소중한 사람이야. (호감도 +40)");
                snprintf(out_choice_b, szb, "따뜻한 캔커피를 건넨다. (호감도 +30)");
            } else {
                snprintf(out_line1, sz1, "너와 함께 걸어온 수많은 계절들... 이제 마음을 알 것 같아.");
                snprintf(out_line2, sz2, "앞으로 남은 내 모든 봄과 겨울에도, 내 곁에 있어줄래?");
                snprintf(out_choice_a, sza, "[수아에게 청혼] 평생 널 지켜줄게! [소꿉친구 결혼/컬러개화!]");
                snprintf(out_choice_b, szb, "소중한 친구로 남아줘서 고마워.");
            }
        }
        else if (strcmp(npc->name, "후배 지은") == 0) {
            if (npc->affection < 45) {
                snprintf(out_line1, sz1, "선배! 도서관에서 공부하다가 선배 생각나서 커피 사 왔어요.");
                snprintf(out_line2, sz2, "선배는 다정하고 든든해서... 같이 있으면 참 힘이 돼요.");
                snprintf(out_choice_a, sza, "지은아, 항상 날 챙겨줘서 고마워. (호감도 +40)");
                snprintf(out_choice_b, szb, "우리 이번 축제 공연 같이 보러 갈래? (호감도 +35)");
            } else {
                snprintf(out_line1, sz1, "선배... 사실 오랫동안 선배만 바라보고 있었어요.");
                snprintf(out_line2, sz2, "선배의 평생 반려자가 되고 싶어요... 제 손 잡아주실래요?");
                snprintf(out_choice_a, sza, "[지은에게 청혼] 널 영원히 사랑해! [후배 지은과 결혼/컬러개화!]");
                snprintf(out_choice_b, szb, "고마운 후배로 아끼겠다고 말한다.");
            }
        }
        else if (strcmp(npc->name, "동기 혜진") == 0) {
            if (npc->affection < 45) {
                snprintf(out_line1, sz1, "우리가 함께 공모전 준비하고 밤새 토론하던 시간들이 참 뜻깊었어.");
                snprintf(out_line2, sz2, "너처럼 지혜롭고 깊이 있는 파트너는 처음이야.");
                snprintf(out_choice_a, sza, "너와 함께라면 어떤 도전도 두렵지 않아. (호감도 +40)");
                snprintf(out_choice_b, szb, "서로를 응원하는 든든한 동료가 되자. (호감도 +30)");
            } else {
                snprintf(out_line1, sz1, "우리가 함께 인생이라는 큰 연구를 함께 완성해 보면 어떨까?");
                snprintf(out_line2, sz2, "서로의 꿈을 가장 가까이서 지지해 주는 평생의 짝이 되어줄래?");
                snprintf(out_choice_a, sza, "[혜진에게 청혼] 나의 동반자가 되어줘! [혜진과 결혼/컬러개화!]");
                snprintf(out_choice_b, szb, "최고의 지적 동료로 남자고 한다.");
            }
        }
    }
    // ACT 3 성인기
    else if (player->current_act == ACT_3_ADULTHOOD) {
        if (npc->is_best_friend) {
            snprintf(out_line1, sz1, "야, 일하느라 고생 많았다! 오늘 맥주 한잔 마시고 털어내자.");
            snprintf(out_line2, sz2, "너 아파트 계단 올라갈 때 센서등 켜지면 진짜 퇴근한 실감 난다며?");
            snprintf(out_choice_a, sza, "시원하게 맥주잔을 부딪힌다! (평생우정 +50)");
            snprintf(out_choice_b, szb, "주말에 우리 아파트에 밥 먹으러 와! (가족우정 +50)");
        }
        else if (strcmp(npc->name, "배우자") == 0) {
            if (player->is_married) {
                snprintf(out_line1, sz1, "여보, 도어락 열리는 소리 듣고 나왔어요! 오늘 고생 많았죠?");
                snprintf(out_line2, sz2, "우리 아기가 오늘 처음으로 아장아장 걸음마를 뗐어요!");
                snprintf(out_choice_a, sza, "아이와 배우자를 꼭 안아준다 [가족의 영원한 사랑]");
                snprintf(out_choice_b, szb, "가족의 든든한 울타리가 되겠다고 약속한다.");
            } else {
                snprintf(out_line1, sz1, "독신으로 살아가며 온 열정을 내 학문과 예술에 바친 시간들...");
                snprintf(out_line2, sz2, "고독 속에서 피워낸 성취들이 찬란하게 빛을 발한다.");
                snprintf(out_choice_a, sza, "내 모든 혼이 담긴 역작을 완성한다. (성취 +50)");
                snprintf(out_choice_b, szb, "조용히 따뜻한 차 한 잔을 마신다.");
            }
        }
        else if (strcmp(npc->name, "박 과장님") == 0) {
            snprintf(out_line1, sz1, "김 대리, 이번 프로젝트 깔끔하게 마무리해 줘서 고맙네.");
            snprintf(out_line2, sz2, "아파트에서 기다리는 가족들 생각해서 오늘은 어서 퇴근하게!");
            snprintf(out_choice_a, sza, "감사합니다 과장님, 먼저 퇴근하겠습니다!");
            snprintf(out_choice_b, szb, "남은 업무만 정리하고 가겠습니다.");
        }
    }
    // ACT 4 노년기
    else if (player->current_act == ACT_4_ELDERLY) {
        if (npc->is_best_friend) {
            snprintf(out_line1, sz1, "야... 우리 초등학교 때 등굣길 걷던 게 엊그제 같은데...");
            snprintf(out_line2, sz2, "벌써 머리가 하얗게 세었구려. 평생 내 친구여줘서 정말 고맙네.");
            snprintf(out_choice_a, sza, "주름진 손을 꼭 맞잡는다. [평생의 지기]");
            snprintf(out_choice_b, szb, "장기판에 차(車)를 두며 껄껄 웃는다. [영원한 우정]");
        }
        else if (strcmp(npc->name, "동반자") == 0) {
            snprintf(out_line1, sz1, "여보, 돌아보니 우리 함께한 인생도 참 봄날의 벚꽃처럼 지나갔구려.");
            snprintf(out_line2, sz2, "당신과 함께 울고 웃었던 모든 날이 내겐 기적이었소.");
            snprintf(out_choice_a, sza, "함께 걸어와 줘서 고맙다고 손을 쥔다. [영원한 감사]");
            snprintf(out_choice_b, szb, "조용히 눈을 맞추며 미소 짓는다.");
        }
        else {
            snprintf(out_line1, sz1, "아버지, 저를 사랑으로 훌륭하게 키워주셔서 감사해요.");
            snprintf(out_line2, sz2, "남겨주신 따스한 사랑, 제 아이에게도 온전히 물려줄게요.");
            snprintf(out_choice_a, sza, "네가 내 삶의 가장 찬란한 보물이란다.");
            snprintf(out_choice_b, szb, "언제나 행복하게 살아가렴.");
        }
    }
    // ACT 5 임종 회상
    else {
        snprintf(out_line1, sz1, "수고 많으셨습니다. 당신의 일생은 참으로 따뜻하고 아름다웠습니다.");
        snprintf(out_line2, sz2, "이제 당신이 겪어온 모든 흑백의 기억들이 영원한 풀컬러로 피어납니다...");
        snprintf(out_choice_a, sza, "내 삶의 모든 순간들을 바라본다. [풀컬러 파노라마]");
        snprintf(out_choice_b, szb, "평온한 미소로 눈을 감는다. [풀컬러 파노라마]");
    }
}

void ScenarioData_ApplyChoice(NPC* npc, PlayerState* player, int choice, MemoryLog* mem_log) {
    npc->talked_in_current_act = true;

    if (mem_log && mem_log->count < MAX_MEMORIES) {
        MemoryShard shard;
        shard.act_id = player->current_act;
        shard.age_at_time = player->age;
        shard.choice_index = choice;
        shard.scene_snapshot_id = player->current_act;
        shard.was_in_color = true;

        char title[48];
        char desc[144];
        snprintf(title, sizeof(title), "%s와의 기억", npc->name);

        if (player->current_act == ACT_1_CHILDHOOD) {
            if (strcmp(npc->name, "어머니") == 0) {
                npc->affection += 40;
                snprintf(desc, sizeof(desc), "7세, 가방을 메고 첫 등굣길을 나설 때 따뜻하게 손을 쥐어주던 어머니의 배웅.");
            } else if (strcmp(npc->name, "선생님") == 0) {
                snprintf(desc, sizeof(desc), "8세, 초등학교 교실 칠판 앞에서 또박또박 글씨를 배우며 넓은 세상을 알아가던 날.");
            } else if (strcmp(npc->name, "짝꿍 수아") == 0) {
                npc->affection += 40;
                snprintf(desc, sizeof(desc), "8세, 교실 짝꿍 자리에서 수아와 알록달록 지우개를 나누며 수줍게 웃었던 기억.");
            } else if (npc->is_best_friend) {
                npc->affection += 45;
                snprintf(desc, sizeof(desc), "8세, 방과 후 골목길에서 %s와(과) 딱지를 치고 떡볶이를 나눠 먹으며 나눈 평생의 우정.", npc->name);
            }
        }
        else if (player->current_act == ACT_2_YOUTH) {
            if (npc->is_best_friend) {
                npc->affection += 40;
                snprintf(desc, sizeof(desc), "20세, 포장마차에서 %s와(과) 술잔을 기울이며 청춘의 꿈과 연애를 함께 고민하던 밤.", npc->name);
            }
            else if (strcmp(npc->name, "수아") == 0) {
                if (choice == 0 && npc->affection >= 35) {
                    player->is_married = true;
                    player->married_target = ROMANCE_SUA;
                    strncpy(player->spouse_name, "수아", sizeof(player->spouse_name) - 1);
                    player->true_love_awakened = true;
                    player->color_spark_timer = 6.0f;
                    player->color_saturation = 1.0f;
                    player->color_bloom_radius = 1.0f;
                    AudioSynth_PlaySFX(SFX_MEMORY_DISCOVER);
                    snprintf(desc, sizeof(desc), "21세, 소꿉친구 수아의 손을 잡으며 청혼하던 순간, 흑백이었던 한강과 온 세상이 찬란한 빛으로 물들었다.");
                } else {
                    npc->affection += 35;
                    snprintf(desc, sizeof(desc), "21세, 한강 둔치 벤치에서 수아와 소중한 추억을 나눈 따뜻한 청춘의 시간.");
                }
            }
            else if (strcmp(npc->name, "후배 지은") == 0) {
                if (choice == 0 && npc->affection >= 35) {
                    player->is_married = true;
                    player->married_target = ROMANCE_JIEUN;
                    strncpy(player->spouse_name, "지은", sizeof(player->spouse_name) - 1);
                    player->true_love_awakened = true;
                    player->color_spark_timer = 6.0f;
                    player->color_saturation = 1.0f;
                    player->color_bloom_radius = 1.0f;
                    AudioSynth_PlaySFX(SFX_MEMORY_DISCOVER);
                    snprintf(desc, sizeof(desc), "21세, 후배 지은의 고백을 받아안고 사랑을 약속하던 순간, 캠퍼스 전체가 눈부신 오색 빛깔로 피어났다.");
                } else {
                    npc->affection += 35;
                    snprintf(desc, sizeof(desc), "21세, 동아리방 앞에서 지은과 커피를 나누며 설레는 미래를 꿈꾸던 날.");
                }
            }
            else if (strcmp(npc->name, "동기 혜진") == 0) {
                if (choice == 0 && npc->affection >= 35) {
                    player->is_married = true;
                    player->married_target = ROMANCE_HYEJIN;
                    strncpy(player->spouse_name, "혜진", sizeof(player->spouse_name) - 1);
                    player->true_love_awakened = true;
                    player->color_spark_timer = 6.0f;
                    player->color_saturation = 1.0f;
                    player->color_bloom_radius = 1.0f;
                    AudioSynth_PlaySFX(SFX_MEMORY_DISCOVER);
                    snprintf(desc, sizeof(desc), "22세, 지적인 동반자 혜진과 서로의 삶을 함께 완성하기로 맹세하던 순간, 세상이 환한 빛으로 개화했다.");
                } else {
                    npc->affection += 35;
                    snprintf(desc, sizeof(desc), "22세, 도서관 앞에서 혜진과 학문과 이상에 대해 깊이 있는 토론을 나누던 청춘.");
                }
            }
        }
        else if (player->current_act == ACT_3_ADULTHOOD) {
            if (npc->is_best_friend) {
                snprintf(desc, sizeof(desc), "35세, 지친 퇴근길마다 %s와(과) 만나 서로의 어깨를 토닥여주던 든든한 평생의 지기.", npc->name);
            } else if (player->is_married) {
                player->family_score += 50;
                snprintf(desc, sizeof(desc), "34세, 아파트 계단을 올라 센서등이 켜질 때 문을 열어준 %s와(과) 내 품에 안긴 작은 아기의 숭고한 기적.", player->spouse_name);
            } else {
                player->career_score += 50;
                snprintf(desc, sizeof(desc), "38세, 내 모든 열정과 밤낮을 바쳐 마침내 사회에서 인정받는 위대한 결실을 맺었다.");
            }
        }
        else if (player->current_act == ACT_4_ELDERLY) {
            if (npc->is_best_friend) {
                snprintf(desc, sizeof(desc), "74세, 느티나무 평상에 마주 앉아 백발이 된 %s와(과) 지난 70년 세월을 껄껄 웃으며 나눈 우정.", npc->name);
            } else if (strcmp(npc->name, "동반자") == 0) {
                snprintf(desc, sizeof(desc), "75세, 주름진 손을 맞잡고 함께 걸어와 준 %s에게 진심으로 감사하며 맞이한 평온한 황혼.", player->spouse_name);
            } else {
                snprintf(desc, sizeof(desc), "76세, 잘 자라준 자녀를 바라보며 내가 받은 사랑을 온전히 물려주었다는 벅찬 안도감.");
            }
        }
        else {
            snprintf(desc, sizeof(desc), "80세, 한 점 후회 없이 사랑하고 치열하게 살다 간 아름다운 한 사람의 일생.");
        }

        strncpy(shard.title, title, sizeof(shard.title) - 1);
        strncpy(shard.description, desc, sizeof(shard.description) - 1);
        mem_log->items[mem_log->count++] = shard;
    }
}
