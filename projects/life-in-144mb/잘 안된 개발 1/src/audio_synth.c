#include "audio_synth.h"
#include <raylib.h>
#include <math.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define SAMPLE_RATE 44100
#define MAX_STEPS   64
#define NUM_SONGS   7
#define MAX_SFX_VOICES 6

#define ___     255 // Note hold / sustain
#define OFF     0   // Note off / rest

// MIDI Note Definitions
#define C_2 36
#define Cs2 37
#define Db2 37
#define D_2 38
#define Ds2 39
#define Eb2 39
#define E_2 40
#define F_2 41
#define Fs2 42
#define Gb2 42
#define G_2 43
#define Gs2 44
#define Ab2 44
#define A_2 45
#define As2 46
#define Bb2 46
#define B_2 47

#define C_3 48
#define Cs3 49
#define Db3 49
#define D_3 50
#define Ds3 51
#define Eb3 51
#define E_3 52
#define F_3 53
#define Fs3 54
#define Gb3 54
#define G_3 55
#define Gs3 56
#define Ab3 56
#define A_3 57
#define As3 58
#define Bb3 58
#define B_3 59

#define C_4 60
#define Cs4 61
#define Db4 61
#define D_4 62
#define Ds4 63
#define Eb4 63
#define E_4 64
#define F_4 65
#define Fs4 66
#define Gb4 66
#define G_4 67
#define Gs4 68
#define Ab4 68
#define A_4 69
#define As4 70
#define Bb4 70
#define B_4 71

#define C_5 72
#define Cs5 73
#define Db5 73
#define D_5 74
#define Ds5 75
#define Eb5 75
#define E_5 76
#define F_5 77
#define Fs5 78
#define Gb5 78
#define G_5 79
#define Gs5 80
#define Ab5 80
#define A_5 81
#define As5 82
#define Bb5 82
#define B_5 83

#define C_6 84
#define Cs6 85
#define Db6 85
#define D_6 86
#define Ds6 87
#define Eb6 87
#define E_6 88
#define F_6 89
#define Fs6 90
#define Gb6 90
#define G_6 91
#define Gs6 92
#define Ab6 92
#define A_6 93
#define As6 94
#define Bb6 94
#define B_6 95
#define C_7 96

// Envelope States
typedef enum {
    ENV_IDLE = 0,
    ENV_ATTACK,
    ENV_DECAY,
    ENV_SUSTAIN,
    ENV_RELEASE
} EnvState;

typedef struct {
    float attack_time;
    float decay_time;
    float sustain_level;
    float release_time;
    EnvState state;
    float level;
} Envelope;

typedef struct {
    float phase;
    float freq;
    float target_freq;
    float duty;
    Envelope env;
    float lfo_phase;
    float lfo_rate;
    float lfo_depth;
    int8_t arp_offsets[4];
    int arp_count;
    int arp_step;
    float arp_timer;
    float arp_speed;
    float filter_lp;
    float volume;
} PulseVoice;

typedef struct {
    float phase;
    float freq;
    Envelope env;
    float filter_lp;
    float volume;
} TriangleVoice;

typedef struct {
    float phase;
    float freq;
    uint16_t lfsr;
    bool short_mode;
    Envelope env;
    float filter_lp;
    float filter_hp;
    float cutoff;
    float volume;
} NoiseVoice;

typedef struct {
    bool active;
    SFXType type;
    float time;
    float duration;
    float phase;
    float freq;
    float freq_start;
    float freq_end;
    float volume;
    uint16_t lfsr;
    float filter_lp;
    int stage;
} SFXVoice;

typedef struct {
    int bpm;
    int ticks_per_beat;
    int total_steps;
    
    float ch1_duty;
    Envelope ch1_env;
    float ch1_vibrato_rate;
    float ch1_vibrato_depth;
    uint8_t ch1[MAX_STEPS];

    float ch2_duty;
    Envelope ch2_env;
    int ch2_arp_mode; // 0=none, 1=maj, 2=min, 3=oct, 4=5th
    uint8_t ch2[MAX_STEPS];

    Envelope ch3_env;
    uint8_t ch3[MAX_STEPS];

    Envelope ch4_env;
    uint8_t ch4[MAX_STEPS]; // Drum trigger: 0=none, 1=kick, 2=snare, 3=hihat, 4=openhat, 5=wind
} SongTrack;

typedef struct {
    AudioStream stream;
    bool initialized;
    float master_volume;
    
    // Playback state
    int current_act;
    int target_act;
    float song_tick_timer;
    int current_step;
    
    // APU Channels
    PulseVoice pulse1;
    PulseVoice pulse2;
    TriangleVoice triangle;
    NoiseVoice noise;
    
    // SFX Voices
    SFXVoice sfx_voices[MAX_SFX_VOICES];
    
    // Frequency lookup table
    float note_freq[128];
} SynthState;

static SynthState g_synth;

// Pre-composed Songs for all 7 Acts
static const SongTrack g_songs[NUM_SONGS] = {
    // -------------------------------------------------------------
    // Act 0: Infant (Lullaby Music Box - 3/4 time, 76 BPM)
    // -------------------------------------------------------------
    {
        .bpm = 76,
        .ticks_per_beat = 4,
        .total_steps = 48,
        .ch1_duty = 0.25f,
        .ch1_env = { 0.005f, 0.45f, 0.0f, 0.15f, ENV_IDLE, 0.0f },
        .ch1_vibrato_rate = 4.5f,
        .ch1_vibrato_depth = 0.015f,
        .ch1 = {
            E_5, ___, G_5, ___, E_5, ___, D_5, ___, C_5, ___, D_5, ___,
            E_5, ___, G_5, ___, A_5, ___, G_5, ___, E_5, ___, D_5, ___,
            E_5, ___, D_5, ___, C_5, ___, D_5, ___, E_5, ___, G_5, ___,
            E_5, ___, C_5, ___, D_5, ___, E_5, ___, D_5, ___, C_5, ___
        },
        .ch2_duty = 0.125f,
        .ch2_env = { 0.005f, 0.35f, 0.0f, 0.10f, ENV_IDLE, 0.0f },
        .ch2_arp_mode = 0,
        .ch2 = {
            OFF, OFF, C_6, ___, OFF, OFF, G_5, ___, OFF, OFF, E_5, ___,
            OFF, OFF, C_6, ___, OFF, OFF, C_6, ___, OFF, OFF, G_5, ___,
            OFF, OFF, C_6, ___, OFF, OFF, G_5, ___, OFF, OFF, C_6, ___,
            OFF, OFF, G_5, ___, OFF, OFF, G_5, ___, OFF, OFF, E_6, ___
        },
        .ch3_env = { 0.04f, 0.2f, 0.7f, 0.3f, ENV_IDLE, 0.0f },
        .ch3 = {
            C_3, ___, ___, ___, ___, ___, G_3, ___, ___, ___, ___, ___,
            F_3, ___, ___, ___, ___, ___, G_3, ___, ___, ___, ___, ___,
            A_2, ___, ___, ___, ___, ___, F_3, ___, ___, ___, ___, ___,
            G_3, ___, ___, ___, ___, ___, C_3, ___, ___, ___, ___, ___
        },
        .ch4_env = { 0.005f, 0.05f, 0.0f, 0.05f, ENV_IDLE, 0.0f },
        .ch4 = { 0 } // Whisper quiet for infant sleep
    },

    // -------------------------------------------------------------
    // Act 1: Childhood (Playground Joy & Spring Blooms - 4/4 time, 126 BPM)
    // -------------------------------------------------------------
    {
        .bpm = 126,
        .ticks_per_beat = 4,
        .total_steps = 64,
        .ch1_duty = 0.50f,
        .ch1_env = { 0.01f, 0.12f, 0.55f, 0.10f, ENV_IDLE, 0.0f },
        .ch1_vibrato_rate = 5.5f,
        .ch1_vibrato_depth = 0.025f,
        .ch1 = {
            G_4, ___, B_4, ___, D_5, ___, ___, ___, B_4, ___, C_5, ___, D_5, ___, ___, ___,
            E_5, ___, D_5, ___, B_4, ___, G_4, ___, A_4, ___, ___, ___, ___, ___, ___, ___,
            G_4, ___, B_4, ___, D_5, ___, ___, ___, E_5, ___, D_5, ___, B_4, ___, ___, ___,
            A_4, ___, B_4, ___, C_5, ___, D_5, ___, G_4, ___, ___, ___, ___, ___, ___, ___
        },
        .ch2_duty = 0.25f,
        .ch2_env = { 0.005f, 0.08f, 0.35f, 0.06f, ENV_IDLE, 0.0f },
        .ch2_arp_mode = 1, // Major arpeggio
        .ch2 = {
            G_4, G_4, G_4, G_4, G_4, G_4, G_4, G_4, C_4, C_4, C_4, C_4, D_4, D_4, D_4, D_4,
            C_4, C_4, C_4, C_4, G_4, G_4, G_4, G_4, D_4, D_4, D_4, D_4, D_4, D_4, D_4, D_4,
            G_4, G_4, G_4, G_4, B_4, B_4, B_4, B_4, C_4, C_4, C_4, C_4, G_4, G_4, G_4, G_4,
            C_4, C_4, C_4, C_4, D_4, D_4, D_4, D_4, G_4, G_4, G_4, G_4, G_4, G_4, G_4, G_4
        },
        .ch3_env = { 0.01f, 0.10f, 0.6f, 0.08f, ENV_IDLE, 0.0f },
        .ch3 = {
            G_2, ___, D_3, ___, G_2, ___, B_2, ___, C_3, ___, G_3, ___, C_3, ___, E_3, ___,
            C_3, ___, G_3, ___, G_2, ___, D_3, ___, D_3, ___, A_3, ___, D_3, ___, Fs3, ___,
            G_2, ___, D_3, ___, B_2, ___, Fs3, ___, C_3, ___, G_3, ___, G_2, ___, D_3, ___,
            C_3, ___, G_3, ___, D_3, ___, A_3, ___, G_2, ___, D_3, ___, G_2, ___, G_3, ___
        },
        .ch4_env = { 0.005f, 0.06f, 0.0f, 0.04f, ENV_IDLE, 0.0f },
        .ch4 = {
            1, 3, 3, 3, 2, 3, 3, 3, 1, 3, 3, 3, 2, 3, 3, 4,
            1, 3, 3, 3, 2, 3, 3, 3, 1, 3, 1, 3, 2, 3, 4, 3,
            1, 3, 3, 3, 2, 3, 3, 3, 1, 3, 3, 3, 2, 3, 3, 4,
            1, 3, 3, 3, 2, 3, 3, 3, 1, 1, 2, 2, 4, 3, 2, 3
        }
    },

    // -------------------------------------------------------------
    // Act 2: Youth (Summer Passion & Fast Forward - 4/4 time, 146 BPM)
    // -------------------------------------------------------------
    {
        .bpm = 146,
        .ticks_per_beat = 4,
        .total_steps = 64,
        .ch1_duty = 0.50f,
        .ch1_env = { 0.008f, 0.15f, 0.65f, 0.10f, ENV_IDLE, 0.0f },
        .ch1_vibrato_rate = 6.0f,
        .ch1_vibrato_depth = 0.03f,
        .ch1 = {
            D_4, ___, F_4, ___, A_4, ___, D_5, ___, C_5, ___, ___, Bb4, A_4, ___, G_4, ___,
            F_4, ___, A_4, ___, C_5, ___, F_5, ___, E_5, ___, D_5, ___, C_5, ___, ___, ___,
            D_5, ___, ___, D_5, C_5, ___, Bb4, ___, A_4, ___, G_4, ___, F_4, ___, G_4, ___,
            A_4, ___, ___, ___, Cs5, ___, ___, ___, E_5, ___, ___, ___, D_5, ___, ___, ___
        },
        .ch2_duty = 0.25f,
        .ch2_env = { 0.005f, 0.08f, 0.40f, 0.05f, ENV_IDLE, 0.0f },
        .ch2_arp_mode = 2, // Minor arpeggio
        .ch2 = {
            D_4, OFF, D_4, D_4, OFF, D_4, OFF, D_4, G_4, OFF, G_4, G_4, OFF, G_4, OFF, G_4,
            F_4, OFF, F_4, F_4, OFF, F_4, OFF, F_4, C_4, OFF, C_4, C_4, OFF, C_4, OFF, C_4,
            Bb3, OFF, Bb3, Bb3, OFF, Bb3, OFF, Bb3, F_4, OFF, F_4, F_4, OFF, F_4, OFF, F_4,
            A_3, OFF, A_3, A_3, OFF, A_3, OFF, A_3, A_4, A_4, A_4, A_4, D_4, D_4, D_4, D_4
        },
        .ch3_env = { 0.008f, 0.08f, 0.75f, 0.06f, ENV_IDLE, 0.0f },
        .ch3 = {
            D_2, D_2, D_3, D_2, D_2, D_3, D_2, D_2, G_2, G_2, G_3, G_2, G_2, G_3, G_2, G_2,
            F_2, F_2, F_3, F_2, F_2, F_3, F_2, F_2, C_3, C_3, C_4, C_3, C_3, C_4, C_3, C_3,
            Bb2, Bb2, Bb3, Bb2, Bb2, Bb3, Bb2, Bb2, F_2, F_2, F_3, F_2, F_2, F_3, F_2, F_2,
            A_2, A_2, A_3, A_2, Cs3, Cs3, Cs4, Cs3, E_3, E_3, E_4, E_3, D_3, D_3, D_2, D_2
        },
        .ch4_env = { 0.004f, 0.05f, 0.0f, 0.03f, ENV_IDLE, 0.0f },
        .ch4 = {
            1, 3, 1, 3, 2, 3, 1, 3, 1, 3, 1, 3, 2, 3, 4, 3,
            1, 3, 1, 3, 2, 3, 1, 3, 1, 3, 1, 3, 2, 3, 4, 3,
            1, 3, 1, 3, 2, 3, 1, 3, 1, 3, 1, 3, 2, 3, 4, 3,
            1, 1, 1, 3, 2, 2, 2, 3, 1, 1, 2, 2, 4, 4, 2, 4
        }
    },

    // -------------------------------------------------------------
    // Act 3: Adulthood (Autumn Twilight & Steady Waltz - 3/4 time, 104 BPM)
    // -------------------------------------------------------------
    {
        .bpm = 104,
        .ticks_per_beat = 4,
        .total_steps = 48,
        .ch1_duty = 0.25f,
        .ch1_env = { 0.015f, 0.25f, 0.5f, 0.2f, ENV_IDLE, 0.0f },
        .ch1_vibrato_rate = 5.0f,
        .ch1_vibrato_depth = 0.025f,
        .ch1 = {
            A_4, ___, ___, B_4, ___, ___, C_5, ___, ___, ___, ___, ___,
            B_4, ___, ___, A_4, ___, ___, F_4, ___, ___, ___, ___, ___,
            G_4, ___, ___, A_4, ___, ___, B_4, ___, ___, C_5, ___, ___,
            B_4, ___, ___, Gs4, ___, ___, A_4, ___, ___, ___, ___, ___
        },
        .ch2_duty = 0.125f,
        .ch2_env = { 0.01f, 0.15f, 0.35f, 0.15f, ENV_IDLE, 0.0f },
        .ch2_arp_mode = 0,
        .ch2 = {
            OFF, OFF, OFF, OFF, C_5, E_5, OFF, OFF, C_5, E_5, OFF, OFF,
            OFF, OFF, OFF, OFF, D_5, F_5, OFF, OFF, D_5, F_5, OFF, OFF,
            OFF, OFF, OFF, OFF, B_4, D_5, OFF, OFF, C_5, E_5, OFF, OFF,
            OFF, OFF, OFF, OFF, B_4, E_5, OFF, OFF, A_4, C_5, OFF, OFF
        },
        .ch3_env = { 0.02f, 0.2f, 0.7f, 0.2f, ENV_IDLE, 0.0f },
        .ch3 = {
            A_2, ___, ___, ___, E_3, ___, ___, ___, A_3, ___, ___, ___,
            D_3, ___, ___, ___, A_3, ___, ___, ___, D_4, ___, ___, ___,
            G_2, ___, ___, ___, D_3, ___, ___, ___, C_3, ___, G_3, ___,
            E_2, ___, ___, ___, B_2, ___, ___, ___, A_2, ___, ___, ___
        },
        .ch4_env = { 0.005f, 0.08f, 0.0f, 0.06f, ENV_IDLE, 0.0f },
        .ch4 = {
            1, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0,
            1, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0,
            1, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0,
            1, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0
        }
    },

    // -------------------------------------------------------------
    // Act 4: Elderly (Winter Solitude & Snowfall - 4/4 time, 58 BPM)
    // -------------------------------------------------------------
    {
        .bpm = 58,
        .ticks_per_beat = 4,
        .total_steps = 64,
        .ch1_duty = 0.125f,
        .ch1_env = { 0.05f, 0.5f, 0.4f, 0.4f, ENV_IDLE, 0.0f },
        .ch1_vibrato_rate = 3.8f,
        .ch1_vibrato_depth = 0.035f,
        .ch1 = {
            E_4, ___, ___, ___, G_4, ___, ___, ___, B_4, ___, ___, ___, ___, ___, ___, ___,
            A_4, ___, ___, ___, G_4, ___, ___, ___, Fs4, ___, ___, ___, ___, ___, ___, ___,
            E_4, ___, ___, ___, D_4, ___, ___, ___, E_4, ___, ___, ___, G_4, ___, ___, ___,
            Fs4, ___, ___, ___, D_4, ___, ___, ___, E_4, ___, ___, ___, ___, ___, ___, ___
        },
        .ch2_duty = 0.125f,
        .ch2_env = { 0.02f, 0.4f, 0.1f, 0.3f, ENV_IDLE, 0.0f },
        .ch2_arp_mode = 0,
        .ch2 = {
            OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, B_5, ___, OFF, OFF, E_6, ___, OFF, OFF,
            OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, D_6, ___, OFF, OFF, B_5, ___, OFF, OFF,
            OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, G_5, ___, OFF, OFF, B_5, ___, OFF, OFF,
            OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, E_6, ___, OFF, OFF, OFF, OFF, OFF, OFF
        },
        .ch3_env = { 0.08f, 0.4f, 0.8f, 0.6f, ENV_IDLE, 0.0f },
        .ch3 = {
            E_2, ___, ___, ___, ___, ___, ___, ___, ___, ___, ___, ___, ___, ___, ___, ___,
            C_2, ___, ___, ___, ___, ___, ___, ___, ___, ___, ___, ___, ___, ___, ___, ___,
            D_2, ___, ___, ___, ___, ___, ___, ___, ___, ___, ___, ___, ___, ___, ___, ___,
            E_2, ___, ___, ___, ___, ___, ___, ___, ___, ___, ___, ___, ___, ___, ___, ___
        },
        .ch4_env = { 0.2f, 0.8f, 0.1f, 0.5f, ENV_IDLE, 0.0f },
        .ch4 = {
            5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        } // Soft wind gusts
    },

    // -------------------------------------------------------------
    // Act 5: Death & Scenic Reminiscence (Transcendence & Golden Symphony - 4/4 time, 88 BPM)
    // -------------------------------------------------------------
    {
        .bpm = 88,
        .ticks_per_beat = 4,
        .total_steps = 64,
        .ch1_duty = 0.50f,
        .ch1_env = { 0.02f, 0.20f, 0.70f, 0.25f, ENV_IDLE, 0.0f },
        .ch1_vibrato_rate = 5.2f,
        .ch1_vibrato_depth = 0.028f,
        .ch1 = {
            D_4, ___, Fs4, ___, A_4, ___, D_5, ___, Cs5, ___, B_4, ___, A_4, ___, ___, ___,
            B_4, ___, D_5, ___, G_5, ___, Fs5, ___, E_5, ___, D_5, ___, B_4, ___, ___, ___,
            Cs5, ___, E_5, ___, A_5, ___, G_5, ___, Fs5, ___, E_5, ___, D_5, ___, ___, ___,
            Fs4, ___, A_4, ___, D_5, ___, ___, ___, D_5, ___, ___, ___, ___, ___, ___, ___
        },
        .ch2_duty = 0.25f,
        .ch2_env = { 0.01f, 0.12f, 0.40f, 0.10f, ENV_IDLE, 0.0f },
        .ch2_arp_mode = 1, // Major cascading harp
        .ch2 = {
            D_4, D_4, Fs4, Fs4, A_4, A_4, D_5, D_5, A_4, A_4, Fs4, Fs4, D_4, D_4, A_3, A_3,
            G_4, G_4, B_4, B_4, D_5, D_5, G_5, G_5, D_5, D_5, B_4, B_4, G_4, G_4, D_4, D_4,
            A_4, A_4, Cs5, Cs5, E_5, E_5, A_5, A_5, E_5, E_5, Cs5, Cs5, A_4, A_4, E_4, E_4,
            D_4, D_4, Fs4, Fs4, A_4, A_4, D_5, D_5, D_5, D_5, A_4, A_4, Fs4, Fs4, D_4, D_4
        },
        .ch3_env = { 0.03f, 0.15f, 0.75f, 0.25f, ENV_IDLE, 0.0f },
        .ch3 = {
            D_2, ___, A_2, ___, D_3, ___, Fs3, ___, G_2, ___, D_3, ___, G_3, ___, B_3, ___,
            A_2, ___, E_3, ___, A_3, ___, Cs4, ___, D_2, ___, A_2, ___, D_3, ___, D_2, ___,
            G_2, ___, D_3, ___, G_3, ___, B_3, ___, A_2, ___, E_3, ___, A_3, ___, Cs4, ___,
            D_2, ___, A_2, ___, D_3, ___, Fs3, ___, D_2, ___, ___, ___, ___, ___, ___, ___
        },
        .ch4_env = { 0.008f, 0.12f, 0.0f, 0.08f, ENV_IDLE, 0.0f },
        .ch4 = {
            1, 0, 3, 0, 2, 0, 3, 0, 1, 0, 3, 0, 2, 0, 4, 0,
            1, 0, 3, 0, 2, 0, 3, 0, 1, 0, 3, 0, 2, 0, 4, 0,
            1, 0, 3, 0, 2, 0, 3, 0, 1, 0, 3, 0, 2, 0, 4, 0,
            1, 0, 1, 0, 2, 0, 2, 0, 4, 0, 0, 0, 0, 0, 0, 0
        }
    },

    // -------------------------------------------------------------
    // Act Ending: Epilogue (Music Box of Life - 3/4 time, 66 BPM)
    // -------------------------------------------------------------
    {
        .bpm = 66,
        .ticks_per_beat = 4,
        .total_steps = 48,
        .ch1_duty = 0.25f,
        .ch1_env = { 0.005f, 0.5f, 0.0f, 0.2f, ENV_IDLE, 0.0f },
        .ch1_vibrato_rate = 4.0f,
        .ch1_vibrato_depth = 0.015f,
        .ch1 = {
            C_5, ___, E_5, ___, G_5, ___, E_5, ___, C_5, ___, D_5, ___,
            E_5, ___, G_5, ___, C_6, ___, ___, ___, ___, ___, ___, ___,
            B_5, ___, A_5, ___, G_5, ___, E_5, ___, F_5, ___, D_5, ___,
            C_5, ___, ___, ___, ___, ___, ___, ___, ___, ___, ___, ___
        },
        .ch2_duty = 0.125f,
        .ch2_env = { 0.005f, 0.3f, 0.0f, 0.15f, ENV_IDLE, 0.0f },
        .ch2_arp_mode = 0,
        .ch2 = {
            OFF, OFF, C_6, ___, OFF, OFF, G_5, ___, OFF, OFF, E_5, ___,
            OFF, OFF, G_5, ___, OFF, OFF, E_6, ___, OFF, OFF, C_6, ___,
            OFF, OFF, D_6, ___, OFF, OFF, C_6, ___, OFF, OFF, G_5, ___,
            OFF, OFF, E_5, ___, OFF, OFF, G_5, ___, OFF, OFF, C_6, ___
        },
        .ch3_env = { 0.05f, 0.3f, 0.7f, 0.4f, ENV_IDLE, 0.0f },
        .ch3 = {
            C_3, ___, ___, ___, ___, ___, G_3, ___, ___, ___, ___, ___,
            F_3, ___, ___, ___, ___, ___, C_3, ___, ___, ___, ___, ___,
            G_3, ___, ___, ___, ___, ___, F_3, ___, ___, ___, ___, ___,
            C_3, ___, ___, ___, ___, ___, ___, ___, ___, ___, ___, ___
        },
        .ch4_env = { 0.005f, 0.05f, 0.0f, 0.05f, ENV_IDLE, 0.0f },
        .ch4 = { 0 }
    }
};

// -----------------------------------------------------------------
// Helper DSP Functions
// -----------------------------------------------------------------

static void Envelope_Init(Envelope* env, float a, float d, float s, float r) {
    env->attack_time = a > 0.001f ? a : 0.001f;
    env->decay_time = d > 0.001f ? d : 0.001f;
    env->sustain_level = s;
    env->release_time = r > 0.001f ? r : 0.001f;
    env->state = ENV_IDLE;
    env->level = 0.0f;
}

static void Envelope_Gate(Envelope* env, bool on) {
    if (on) {
        env->state = ENV_ATTACK;
    } else {
        if (env->state != ENV_IDLE) {
            env->state = ENV_RELEASE;
        }
    }
}

static inline float Envelope_Process(Envelope* env, float dt) {
    switch (env->state) {
        case ENV_IDLE:
            env->level = 0.0f;
            break;
        case ENV_ATTACK:
            env->level += dt / env->attack_time;
            if (env->level >= 1.0f) {
                env->level = 1.0f;
                env->state = ENV_DECAY;
            }
            break;
        case ENV_DECAY:
            env->level -= dt / env->decay_time * (1.0f - env->sustain_level);
            if (env->level <= env->sustain_level) {
                env->level = env->sustain_level;
                env->state = ENV_SUSTAIN;
            }
            break;
        case ENV_SUSTAIN:
            env->level = env->sustain_level;
            break;
        case ENV_RELEASE:
            env->level -= dt / env->release_time;
            if (env->level <= 0.0f) {
                env->level = 0.0f;
                env->state = ENV_IDLE;
            }
            break;
    }
    return env->level;
}

static inline float Fast_SoftClip(float x) {
    return x / (1.0f + fabsf(x) * 0.45f);
}

// -----------------------------------------------------------------
// Channel Synthesizers
// -----------------------------------------------------------------

static inline float Process_PulseVoice(PulseVoice* v, float dt) {
    float env_val = Envelope_Process(&v->env, dt);
    if (env_val <= 0.0f || v->freq <= 0.0f) return 0.0f;

    // Arpeggio
    float note_freq = v->freq;
    if (v->arp_count > 1) {
        v->arp_timer += dt;
        if (v->arp_timer >= v->arp_speed) {
            v->arp_timer = 0.0f;
            v->arp_step = (v->arp_step + 1) % v->arp_count;
        }
        int8_t semitone = v->arp_offsets[v->arp_step];
        if (semitone != 0) {
            note_freq *= powf(2.0f, (float)semitone / 12.0f);
        }
    }

    // Vibrato
    if (v->lfo_depth > 0.0001f) {
        v->lfo_phase += v->lfo_rate * dt;
        if (v->lfo_phase >= 1.0f) v->lfo_phase -= 1.0f;
        float vib = sinf(v->lfo_phase * 2.0f * (float)M_PI) * v->lfo_depth;
        note_freq *= (1.0f + vib);
    }

    // Phase advance
    v->phase += note_freq * dt;
    if (v->phase >= 1.0f) v->phase -= 1.0f;

    // Raw Pulse
    float raw = (v->phase < v->duty) ? 0.75f : -0.75f;

    // Gentle 1-pole Lowpass Filter for warm analog feel (cutoff ~7kHz)
    float alpha = 0.65f;
    v->filter_lp += alpha * (raw - v->filter_lp);

    return v->filter_lp * env_val * v->volume;
}

static inline float Process_TriangleVoice(TriangleVoice* v, float dt) {
    float env_val = Envelope_Process(&v->env, dt);
    if (env_val <= 0.0f || v->freq <= 0.0f) return 0.0f;

    v->phase += v->freq * dt;
    if (v->phase >= 1.0f) v->phase -= 1.0f;

    // Triangle Wave: -1.0 to +1.0
    float raw = 4.0f * fabsf(v->phase - 0.5f) - 1.0f;

    // Lowpass filter
    float alpha = 0.55f;
    v->filter_lp += alpha * (raw - v->filter_lp);

    return v->filter_lp * env_val * v->volume;
}

static inline float Process_NoiseVoice(NoiseVoice* v, float dt) {
    float env_val = Envelope_Process(&v->env, dt);
    if (env_val <= 0.0f) return 0.0f;

    // Clock LFSR
    v->phase += v->freq * dt;
    while (v->phase >= 1.0f) {
        v->phase -= 1.0f;
        uint16_t b0 = v->lfsr & 1;
        uint16_t b1 = v->short_mode ? ((v->lfsr >> 6) & 1) : ((v->lfsr >> 1) & 1);
        uint16_t feedback = b0 ^ b1;
        v->lfsr = (v->lfsr >> 1) | (feedback << 14);
    }

    float raw = (v->lfsr & 1) ? 0.7f : -0.7f;

    // Filter
    float alpha = v->cutoff > 0.01f ? v->cutoff : 0.4f;
    v->filter_lp += alpha * (raw - v->filter_lp);

    return v->filter_lp * env_val * v->volume;
}

// -----------------------------------------------------------------
// SFX Processing
// -----------------------------------------------------------------

static inline float Process_SFXVoice(SFXVoice* v, float dt) {
    if (!v->active) return 0.0f;

    v->time += dt;
    if (v->time >= v->duration) {
        v->active = false;
        return 0.0f;
    }

    float sample = 0.0f;
    float progress = v->time / v->duration;

    switch (v->type) {
        case SFX_BLINK: {
            // Subtle downward chirp (160 Hz -> 55 Hz)
            v->freq = v->freq_start + (v->freq_end - v->freq_start) * progress;
            v->phase += v->freq * dt;
            if (v->phase >= 1.0f) v->phase -= 1.0f;
            float env = sinf(progress * (float)M_PI);
            sample = sinf(v->phase * 2.0f * (float)M_PI) * env * v->volume;
            break;
        }

        case SFX_STEP: {
            // Soft lowpass-filtered white noise burst
            uint16_t b0 = v->lfsr & 1;
            uint16_t b1 = (v->lfsr >> 1) & 1;
            v->lfsr = (v->lfsr >> 1) | ((b0 ^ b1) << 14);
            float raw = (v->lfsr & 1) ? 0.8f : -0.8f;
            v->filter_lp += 0.12f * (raw - v->filter_lp);
            float env = (1.0f - progress) * (1.0f - progress);
            sample = v->filter_lp * env * v->volume;
            break;
        }

        case SFX_BABY_COO: {
            // Cute pitch-shifted pulse with soft vibrato (420 -> 580 -> 510 Hz)
            float t = progress;
            if (t < 0.5f) {
                v->freq = 420.0f + (580.0f - 420.0f) * (t / 0.5f);
            } else {
                v->freq = 580.0f + (510.0f - 580.0f) * ((t - 0.5f) / 0.5f);
            }
            float vib = sinf(v->time * 38.0f) * 16.0f;
            v->phase += (v->freq + vib) * dt;
            if (v->phase >= 1.0f) v->phase -= 1.0f;
            float raw = (v->phase < 0.25f) ? 0.7f : -0.7f;
            float env = sinf(t * (float)M_PI);
            sample = raw * env * v->volume;
            break;
        }

        case SFX_CHOICE_SELECT: {
            // Crisp two-note bell chime (C6 -> G6)
            if (v->time < 0.06f) {
                v->freq = 1046.5f; // C6
            } else {
                v->freq = 1567.98f; // G6
            }
            v->phase += v->freq * dt;
            if (v->phase >= 1.0f) v->phase -= 1.0f;
            float env = expf(-progress * 5.0f);
            float raw = (v->phase < 0.25f) ? 0.75f : -0.75f;
            sample = raw * env * v->volume;
            break;
        }

        case SFX_MEMORY_DISCOVER: {
            // Sparkling multi-tone harmonic chime (E5 -> G#5 -> B5 -> E6 -> G#6)
            float step_time = 0.055f;
            int step = (int)(v->time / step_time);
            if (step > 4) step = 4;
            const float freqs[5] = { 659.25f, 830.61f, 987.77f, 1318.51f, 1661.22f };
            v->freq = freqs[step];
            v->phase += v->freq * dt;
            if (v->phase >= 1.0f) v->phase -= 1.0f;
            float env = expf(-progress * 3.5f);
            float pulse = (v->phase < 0.20f) ? 0.8f : -0.8f;
            float chime = sinf(v->phase * 4.0f * (float)M_PI) * 0.3f;
            sample = (pulse + chime) * env * v->volume;
            break;
        }

        case SFX_WIND_AMBIENT: {
            // Filtered ambient noise gust
            uint16_t b0 = v->lfsr & 1;
            uint16_t b1 = (v->lfsr >> 1) & 1;
            v->lfsr = (v->lfsr >> 1) | ((b0 ^ b1) << 14);
            float raw = (v->lfsr & 1) ? 0.9f : -0.9f;
            float cutoff = 0.04f + 0.12f * sinf(progress * (float)M_PI);
            v->filter_lp += cutoff * (raw - v->filter_lp);
            float env = sinf(progress * (float)M_PI);
            sample = v->filter_lp * env * v->volume;
            break;
        }

        case SFX_HEARTBEAT: {
            // Deep rhythmic sub thump (lub-dub)
            float t = progress;
            float sub_freq = 55.0f;
            float sub_env = 0.0f;
            if (t < 0.35f) {
                float p1 = t / 0.35f;
                sub_freq = 60.0f - p1 * 18.0f;
                sub_env = sinf(p1 * (float)M_PI);
            } else if (t >= 0.45f && t < 0.90f) {
                float p2 = (t - 0.45f) / 0.45f;
                sub_freq = 68.0f - p2 * 22.0f;
                sub_env = sinf(p2 * (float)M_PI) * 1.15f;
            }
            v->phase += sub_freq * dt;
            if (v->phase >= 1.0f) v->phase -= 1.0f;
            sample = sinf(v->phase * 2.0f * (float)M_PI) * sub_env * v->volume;
            break;
        }

        default:
            break;
    }

    return sample;
}

// -----------------------------------------------------------------
// Sequencer Tick & Step Processing
// -----------------------------------------------------------------

static void Trigger_Drum(int drum_type) {
    if (drum_type == 0) return;

    switch (drum_type) {
        case 1: // Kick
            g_synth.noise.freq = 120.0f;
            g_synth.noise.cutoff = 0.8f;
            g_synth.noise.short_mode = false;
            g_synth.noise.volume = 0.65f;
            Envelope_Init(&g_synth.noise.env, 0.002f, 0.06f, 0.0f, 0.03f);
            Envelope_Gate(&g_synth.noise.env, true);
            break;

        case 2: // Snare
            g_synth.noise.freq = 4200.0f;
            g_synth.noise.cutoff = 0.65f;
            g_synth.noise.short_mode = false;
            g_synth.noise.volume = 0.50f;
            Envelope_Init(&g_synth.noise.env, 0.003f, 0.10f, 0.0f, 0.05f);
            Envelope_Gate(&g_synth.noise.env, true);
            break;

        case 3: // Closed Hi-Hat
            g_synth.noise.freq = 9500.0f;
            g_synth.noise.cutoff = 0.9f;
            g_synth.noise.short_mode = true;
            g_synth.noise.volume = 0.35f;
            Envelope_Init(&g_synth.noise.env, 0.001f, 0.035f, 0.0f, 0.02f);
            Envelope_Gate(&g_synth.noise.env, true);
            break;

        case 4: // Open Hi-Hat / Crash
            g_synth.noise.freq = 8200.0f;
            g_synth.noise.cutoff = 0.85f;
            g_synth.noise.short_mode = false;
            g_synth.noise.volume = 0.45f;
            Envelope_Init(&g_synth.noise.env, 0.002f, 0.22f, 0.0f, 0.12f);
            Envelope_Gate(&g_synth.noise.env, true);
            break;

        case 5: // Ambient Wind
            g_synth.noise.freq = 800.0f;
            g_synth.noise.cutoff = 0.15f;
            g_synth.noise.short_mode = false;
            g_synth.noise.volume = 0.30f;
            Envelope_Init(&g_synth.noise.env, 0.2f, 0.6f, 0.1f, 0.4f);
            Envelope_Gate(&g_synth.noise.env, true);
            break;
    }
}

static void Sequencer_AdvanceStep(void) {
    if (g_synth.current_act < 0 || g_synth.current_act >= NUM_SONGS) return;
    const SongTrack* song = &g_songs[g_synth.current_act];

    int step = g_synth.current_step;

    // --- Channel 1 (Lead Pulse) ---
    uint8_t note1 = song->ch1[step];
    if (note1 == OFF) {
        Envelope_Gate(&g_synth.pulse1.env, false);
    } else if (note1 != ___) {
        g_synth.pulse1.freq = g_synth.note_freq[note1];
        g_synth.pulse1.duty = song->ch1_duty;
        g_synth.pulse1.lfo_rate = song->ch1_vibrato_rate;
        g_synth.pulse1.lfo_depth = song->ch1_vibrato_depth;
        g_synth.pulse1.env = song->ch1_env;
        Envelope_Gate(&g_synth.pulse1.env, true);
    }

    // --- Channel 2 (Arp / Harmony Pulse) ---
    uint8_t note2 = song->ch2[step];
    if (note2 == OFF) {
        Envelope_Gate(&g_synth.pulse2.env, false);
    } else if (note2 != ___) {
        g_synth.pulse2.freq = g_synth.note_freq[note2];
        g_synth.pulse2.duty = song->ch2_duty;
        g_synth.pulse2.env = song->ch2_env;

        // Configure Arpeggio
        if (song->ch2_arp_mode == 1) {
            // Major: 0, 4, 7, 12
            g_synth.pulse2.arp_offsets[0] = 0;
            g_synth.pulse2.arp_offsets[1] = 4;
            g_synth.pulse2.arp_offsets[2] = 7;
            g_synth.pulse2.arp_offsets[3] = 12;
            g_synth.pulse2.arp_count = 4;
            g_synth.pulse2.arp_speed = 0.035f;
        } else if (song->ch2_arp_mode == 2) {
            // Minor: 0, 3, 7, 12
            g_synth.pulse2.arp_offsets[0] = 0;
            g_synth.pulse2.arp_offsets[1] = 3;
            g_synth.pulse2.arp_offsets[2] = 7;
            g_synth.pulse2.arp_offsets[3] = 12;
            g_synth.pulse2.arp_count = 4;
            g_synth.pulse2.arp_speed = 0.030f;
        } else {
            g_synth.pulse2.arp_count = 0;
        }
        g_synth.pulse2.arp_step = 0;
        g_synth.pulse2.arp_timer = 0.0f;

        Envelope_Gate(&g_synth.pulse2.env, true);
    }

    // --- Channel 3 (Triangle Bass) ---
    uint8_t note3 = song->ch3[step];
    if (note3 == OFF) {
        Envelope_Gate(&g_synth.triangle.env, false);
    } else if (note3 != ___) {
        g_synth.triangle.freq = g_synth.note_freq[note3];
        g_synth.triangle.env = song->ch3_env;
        Envelope_Gate(&g_synth.triangle.env, true);
    }

    // --- Channel 4 (LFSR Noise / Drums) ---
    uint8_t drum = song->ch4[step];
    if (drum > 0) {
        Trigger_Drum(drum);
    }

    // Next step
    g_synth.current_step = (g_synth.current_step + 1) % song->total_steps;
}

// -----------------------------------------------------------------
// AudioStream Callback (Realtime Thread)
// -----------------------------------------------------------------

static void AudioSynth_Callback(void* buffer, unsigned int frames) {
    float* out = (float*)buffer;
    const float dt = 1.0f / (float)SAMPLE_RATE;

    const SongTrack* song = (g_synth.current_act >= 0 && g_synth.current_act < NUM_SONGS)
                            ? &g_songs[g_synth.current_act] : NULL;
    
    float tick_duration = 0.125f;
    if (song != NULL && song->bpm > 0 && song->ticks_per_beat > 0) {
        tick_duration = 60.0f / (float)(song->bpm * song->ticks_per_beat);
    }

    for (unsigned int i = 0; i < frames; ++i) {
        // Step sequencer clock
        if (song != NULL) {
            g_synth.song_tick_timer += dt;
            if (g_synth.song_tick_timer >= tick_duration) {
                g_synth.song_tick_timer -= tick_duration;
                Sequencer_AdvanceStep();
            }
        }

        // Synthesize 4 APU Channels
        float ch1 = Process_PulseVoice(&g_synth.pulse1, dt);
        float ch2 = Process_PulseVoice(&g_synth.pulse2, dt);
        float ch3 = Process_TriangleVoice(&g_synth.triangle, dt);
        float ch4 = Process_NoiseVoice(&g_synth.noise, dt);

        float bgm_mix = (ch1 * 0.32f) + (ch2 * 0.26f) + (ch3 * 0.42f) + (ch4 * 0.30f);

        // Synthesize SFX Voices
        float sfx_mix = 0.0f;
        for (int v = 0; v < MAX_SFX_VOICES; ++v) {
            if (g_synth.sfx_voices[v].active) {
                sfx_mix += Process_SFXVoice(&g_synth.sfx_voices[v], dt);
            }
        }

        // Soft Clip Limiter & Master Volume
        float mixed = bgm_mix + sfx_mix * 0.85f;
        float saturated = Fast_SoftClip(mixed);
        out[i] = saturated * g_synth.master_volume;
    }
}

// -----------------------------------------------------------------
// Public API Implementation
// -----------------------------------------------------------------

void AudioSynth_Init(void) {
    if (g_synth.initialized) return;

    // 1. Calculate Note Frequencies: f = 440 * 2^((n - 69) / 12)
    for (int n = 0; n < 128; ++n) {
        g_synth.note_freq[n] = 440.0f * powf(2.0f, (float)(n - 69) / 12.0f);
    }

    // 2. Initialize Voices
    memset(&g_synth.pulse1, 0, sizeof(PulseVoice));
    g_synth.pulse1.volume = 1.0f;
    g_synth.pulse1.duty = 0.5f;

    memset(&g_synth.pulse2, 0, sizeof(PulseVoice));
    g_synth.pulse2.volume = 0.85f;
    g_synth.pulse2.duty = 0.25f;

    memset(&g_synth.triangle, 0, sizeof(TriangleVoice));
    g_synth.triangle.volume = 1.0f;

    memset(&g_synth.noise, 0, sizeof(NoiseVoice));
    g_synth.noise.lfsr = 0x7FFF;
    g_synth.noise.volume = 0.7f;

    for (int i = 0; i < MAX_SFX_VOICES; ++i) {
        memset(&g_synth.sfx_voices[i], 0, sizeof(SFXVoice));
        g_synth.sfx_voices[i].lfsr = 0x7FFF;
    }

    g_synth.master_volume = 0.8f;
    g_synth.current_act = ACT_0_INFANT;
    g_synth.target_act = ACT_0_INFANT;
    g_synth.song_tick_timer = 0.0f;
    g_synth.current_step = 0;

    // 3. Initialize Raylib Audio Device & Stream
    if (!IsAudioDeviceReady()) {
        InitAudioDevice();
    }
    SetAudioStreamBufferSizeDefault(1024);
    g_synth.stream = LoadAudioStream(SAMPLE_RATE, 32, 1);
    SetAudioStreamCallback(g_synth.stream, AudioSynth_Callback);
    PlayAudioStream(g_synth.stream);

    g_synth.initialized = true;
}

void AudioSynth_Update(float dt) {
    (void)dt;
    // Realtime audio stream updates are driven cleanly by Raylib AudioCallback.
}

void AudioSynth_PlayBGM(LifeAct act) {
    if (act < 0 || act >= NUM_SONGS) return;

    if (g_synth.current_act != (int)act) {
        g_synth.current_act = (int)act;
        g_synth.current_step = 0;
        g_synth.song_tick_timer = 0.0f;

        // Reset channel envelopes
        Envelope_Gate(&g_synth.pulse1.env, false);
        Envelope_Gate(&g_synth.pulse2.env, false);
        Envelope_Gate(&g_synth.triangle.env, false);
        Envelope_Gate(&g_synth.noise.env, false);
    }
}

void AudioSynth_PlaySFX(SFXType sfx) {
    if (sfx < 0 || sfx >= SFX_COUNT) return;

    // Find available SFX voice slot
    int slot = -1;
    for (int i = 0; i < MAX_SFX_VOICES; ++i) {
        if (!g_synth.sfx_voices[i].active) {
            slot = i;
            break;
        }
    }
    if (slot == -1) {
        // Voice steal slot 0
        slot = 0;
    }

    SFXVoice* v = &g_synth.sfx_voices[slot];
    v->active = true;
    v->type = sfx;
    v->time = 0.0f;
    v->phase = 0.0f;
    v->filter_lp = 0.0f;
    v->lfsr = 0x7FFF;

    switch (sfx) {
        case SFX_BLINK:
            v->duration = 0.09f;
            v->freq_start = 160.0f;
            v->freq_end = 55.0f;
            v->volume = 0.55f;
            break;

        case SFX_STEP:
            v->duration = 0.05f;
            v->volume = 0.40f;
            break;

        case SFX_BABY_COO:
            v->duration = 0.36f;
            v->volume = 0.65f;
            break;

        case SFX_CHOICE_SELECT:
            v->duration = 0.18f;
            v->volume = 0.60f;
            break;

        case SFX_MEMORY_DISCOVER:
            v->duration = 0.50f;
            v->volume = 0.75f;
            break;

        case SFX_WIND_AMBIENT:
            v->duration = 1.6f;
            v->volume = 0.45f;
            break;

        case SFX_HEARTBEAT:
            v->duration = 0.42f;
            v->volume = 0.85f;
            break;

        default:
            v->active = false;
            break;
    }
}

void AudioSynth_SetMasterVolume(float volume) {
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;
    g_synth.master_volume = volume;
}

void AudioSynth_Close(void) {
    if (!g_synth.initialized) return;

    if (IsAudioStreamPlaying(g_synth.stream)) {
        StopAudioStream(g_synth.stream);
    }
    UnloadAudioStream(g_synth.stream);
    
    if (IsAudioDeviceReady()) {
        CloseAudioDevice();
    }
    g_synth.initialized = false;
}
