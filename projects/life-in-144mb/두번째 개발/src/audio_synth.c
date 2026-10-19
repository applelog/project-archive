#include "audio_synth.h"
#include <raylib.h>
#include <math.h>
#include <stdlib.h>
#include <stdint.h>

#define SAMPLE_RATE 22050
#define AUDIO_BUFFER_SIZE 1024
#define PI_VAL 3.14159265358979323846f

#define MAX_VOICES 24
#define MAX_TIMED_EVENTS 32

// Note Frequencies (Hz)
#define NOTE_REST 0.0f

#define NOTE_C2   65.41f
#define NOTE_Cs2  69.30f
#define NOTE_D2   73.42f
#define NOTE_Eb2  77.78f
#define NOTE_E2   82.41f
#define NOTE_F2   87.31f
#define NOTE_Fs2  92.50f
#define NOTE_G2   98.00f
#define NOTE_Ab2 103.83f
#define NOTE_A2  110.00f
#define NOTE_Bb2 116.54f
#define NOTE_As2 116.54f
#define NOTE_B2  123.47f

#define NOTE_C3  130.81f
#define NOTE_D3  146.83f
#define NOTE_Eb3 155.56f
#define NOTE_E3  164.81f
#define NOTE_F3  174.61f
#define NOTE_Fs3 185.00f
#define NOTE_G3  196.00f
#define NOTE_Ab3 207.65f
#define NOTE_A3  220.00f
#define NOTE_Bb3 233.08f
#define NOTE_B3  246.94f

#define NOTE_C4  261.63f
#define NOTE_Cs4 277.18f
#define NOTE_D4  293.66f
#define NOTE_Eb4 311.13f
#define NOTE_E4  329.63f
#define NOTE_F4  349.23f
#define NOTE_Fs4 369.99f
#define NOTE_G4  392.00f
#define NOTE_Ab4 415.30f
#define NOTE_A4  440.00f
#define NOTE_Bb4 466.16f
#define NOTE_B4  493.88f

#define NOTE_C5  523.25f
#define NOTE_Cs5 554.37f
#define NOTE_D5  587.33f
#define NOTE_Eb5 622.25f
#define NOTE_E5  659.25f
#define NOTE_F5  698.46f
#define NOTE_Fs5 739.99f
#define NOTE_G5  783.99f
#define NOTE_Ab5 830.61f
#define NOTE_A5  880.00f
#define NOTE_Bb5 932.33f
#define NOTE_B5  987.77f

#define NOTE_C6  1046.50f
#define NOTE_D6  1174.66f
#define NOTE_Eb6 1244.51f
#define NOTE_E6  1318.51f
#define NOTE_G6  1567.98f
#define NOTE_A6  1760.00f
#define NOTE_B6  1975.53f
#define NOTE_C7  2093.00f
#define NOTE_E7  2637.02f

typedef enum {
    TIMBRE_PIANO = 0,
    TIMBRE_BELL,
    TIMBRE_HEARTBEAT,
    TIMBRE_NOISE
} VoiceTimbre;

typedef struct {
    float freq;
    float phase;
    float env;
    float decay;
    float volume;
    VoiceTimbre timbre;
    bool is_sfx;
    bool active;
} SynthVoice;

typedef struct {
    float time_remaining;
    float freq;
    float volume;
    float decay;
    VoiceTimbre timbre;
    bool is_sfx;
    bool active;
} TimedNoteEvent;

typedef struct {
    float freq;
    float duration;
    float volume;
    float decay;
} MelodicNote;

// BGM Sequences
static const MelodicNote SEQ_INTRO[] = {
    // Am
    { NOTE_A2, 0.40f, 0.55f, 1.2f },
    { NOTE_E3, 0.28f, 0.40f, 1.5f },
    { NOTE_A3, 0.28f, 0.45f, 1.5f },
    { NOTE_C4, 0.28f, 0.50f, 1.8f },
    { NOTE_E4, 0.45f, 0.60f, 2.0f },
    { NOTE_C4, 0.32f, 0.45f, 1.5f },
    // F
    { NOTE_F2, 0.40f, 0.55f, 1.2f },
    { NOTE_C3, 0.28f, 0.40f, 1.5f },
    { NOTE_F3, 0.28f, 0.45f, 1.5f },
    { NOTE_A3, 0.28f, 0.50f, 1.8f },
    { NOTE_C4, 0.45f, 0.60f, 2.0f },
    { NOTE_A3, 0.32f, 0.45f, 1.5f },
    // C
    { NOTE_C3, 0.40f, 0.55f, 1.2f },
    { NOTE_G3, 0.28f, 0.40f, 1.5f },
    { NOTE_C4, 0.28f, 0.50f, 1.8f },
    { NOTE_E4, 0.28f, 0.55f, 1.8f },
    { NOTE_G4, 0.50f, 0.65f, 2.2f },
    { NOTE_E4, 0.32f, 0.45f, 1.5f },
    // G
    { NOTE_G2, 0.40f, 0.55f, 1.2f },
    { NOTE_D3, 0.28f, 0.40f, 1.5f },
    { NOTE_G3, 0.28f, 0.45f, 1.5f },
    { NOTE_B3, 0.28f, 0.50f, 1.8f },
    { NOTE_D4, 0.50f, 0.60f, 2.0f },
    { NOTE_B3, 0.32f, 0.45f, 1.5f },
    // Dm
    { NOTE_D3, 0.38f, 0.55f, 1.3f },
    { NOTE_A3, 0.28f, 0.45f, 1.5f },
    { NOTE_D4, 0.28f, 0.50f, 1.8f },
    { NOTE_F4, 0.40f, 0.60f, 2.0f },
    { NOTE_A4, 0.60f, 0.65f, 2.2f },
    // Em
    { NOTE_E3, 0.38f, 0.55f, 1.3f },
    { NOTE_B3, 0.28f, 0.45f, 1.5f },
    { NOTE_E4, 0.28f, 0.50f, 1.8f },
    { NOTE_G4, 0.40f, 0.60f, 2.0f },
    { NOTE_B4, 0.60f, 0.65f, 2.2f },
    // Fmaj7
    { NOTE_F2, 0.40f, 0.55f, 1.2f },
    { NOTE_C3, 0.28f, 0.40f, 1.5f },
    { NOTE_E3, 0.28f, 0.45f, 1.5f },
    { NOTE_A3, 0.30f, 0.50f, 1.8f },
    { NOTE_C4, 0.40f, 0.55f, 2.0f },
    { NOTE_E4, 0.65f, 0.60f, 2.2f },
    // E7 & Pause
    { NOTE_E2, 0.40f, 0.55f, 1.2f },
    { NOTE_B2, 0.28f, 0.40f, 1.5f },
    { NOTE_Ab3, 0.30f, 0.45f, 1.6f },
    { NOTE_B3, 0.35f, 0.50f, 1.8f },
    { NOTE_E4, 0.70f, 0.60f, 2.2f },
    { NOTE_REST, 0.50f, 0.0f, 0.0f }
};

static const MelodicNote SEQ_SEARCHING[] = {
    // Dm walking pattern
    { NOTE_D3, 0.32f, 0.55f, 1.5f },
    { NOTE_A3, 0.22f, 0.42f, 1.8f },
    { NOTE_D4, 0.22f, 0.48f, 1.8f },
    { NOTE_F4, 0.32f, 0.58f, 2.0f },
    { NOTE_D4, 0.22f, 0.45f, 1.8f },
    { NOTE_A3, 0.22f, 0.40f, 1.8f },
    // Gm
    { NOTE_G2, 0.32f, 0.55f, 1.5f },
    { NOTE_D3, 0.22f, 0.42f, 1.8f },
    { NOTE_Bb3, 0.22f, 0.48f, 1.8f },
    { NOTE_D4, 0.32f, 0.55f, 2.0f },
    { NOTE_G4, 0.38f, 0.62f, 2.2f },
    { NOTE_D4, 0.22f, 0.42f, 1.8f },
    // Bb
    { NOTE_Bb2, 0.32f, 0.55f, 1.5f },
    { NOTE_F3, 0.22f, 0.42f, 1.8f },
    { NOTE_D4, 0.22f, 0.48f, 1.8f },
    { NOTE_F4, 0.32f, 0.55f, 2.0f },
    { NOTE_D4, 0.22f, 0.45f, 1.8f },
    { NOTE_Bb3, 0.22f, 0.40f, 1.8f },
    // A7
    { NOTE_A2, 0.32f, 0.55f, 1.5f },
    { NOTE_E3, 0.22f, 0.42f, 1.8f },
    { NOTE_G3, 0.22f, 0.48f, 1.8f },
    { NOTE_Cs4, 0.32f, 0.55f, 2.0f },
    { NOTE_E4, 0.48f, 0.60f, 2.2f },
    { NOTE_REST, 0.18f, 0.0f, 0.0f },
    // Dm Melody variation
    { NOTE_D3, 0.32f, 0.55f, 1.5f },
    { NOTE_F3, 0.22f, 0.42f, 1.8f },
    { NOTE_A3, 0.22f, 0.48f, 1.8f },
    { NOTE_F4, 0.32f, 0.55f, 2.0f },
    { NOTE_E4, 0.22f, 0.48f, 1.8f },
    { NOTE_D4, 0.30f, 0.52f, 1.8f },
    // C
    { NOTE_C3, 0.32f, 0.55f, 1.5f },
    { NOTE_E3, 0.22f, 0.42f, 1.8f },
    { NOTE_G3, 0.22f, 0.48f, 1.8f },
    { NOTE_E4, 0.32f, 0.55f, 2.0f },
    { NOTE_D4, 0.22f, 0.48f, 1.8f },
    { NOTE_C4, 0.30f, 0.50f, 1.8f },
    // Asus4 -> A
    { NOTE_A2, 0.32f, 0.55f, 1.5f },
    { NOTE_D3, 0.22f, 0.42f, 1.8f },
    { NOTE_E3, 0.22f, 0.45f, 1.8f },
    { NOTE_A3, 0.38f, 0.55f, 2.0f },
    { NOTE_Cs4, 0.60f, 0.60f, 2.2f },
    { NOTE_REST, 0.40f, 0.0f, 0.0f }
};

static const MelodicNote SEQ_CLUE_FOUND[] = {
    // C blooming
    { NOTE_C3, 0.28f, 0.55f, 1.4f },
    { NOTE_G3, 0.18f, 0.42f, 1.8f },
    { NOTE_C4, 0.18f, 0.48f, 1.8f },
    { NOTE_E4, 0.28f, 0.58f, 2.0f },
    { NOTE_G4, 0.28f, 0.62f, 2.2f },
    { NOTE_C5, 0.42f, 0.70f, 2.4f },
    // G/B
    { NOTE_B2, 0.28f, 0.55f, 1.4f },
    { NOTE_G3, 0.18f, 0.42f, 1.8f },
    { NOTE_D4, 0.18f, 0.48f, 1.8f },
    { NOTE_G4, 0.28f, 0.58f, 2.0f },
    { NOTE_B4, 0.42f, 0.68f, 2.4f },
    // Am
    { NOTE_A2, 0.28f, 0.55f, 1.4f },
    { NOTE_E3, 0.18f, 0.42f, 1.8f },
    { NOTE_A3, 0.18f, 0.48f, 1.8f },
    { NOTE_C4, 0.28f, 0.58f, 2.0f },
    { NOTE_E4, 0.28f, 0.62f, 2.2f },
    { NOTE_A4, 0.42f, 0.70f, 2.4f },
    // Em/G
    { NOTE_G2, 0.28f, 0.55f, 1.4f },
    { NOTE_E3, 0.18f, 0.42f, 1.8f },
    { NOTE_B3, 0.18f, 0.48f, 1.8f },
    { NOTE_E4, 0.28f, 0.58f, 2.0f },
    { NOTE_G4, 0.42f, 0.68f, 2.4f },
    // F
    { NOTE_F2, 0.28f, 0.55f, 1.4f },
    { NOTE_C3, 0.18f, 0.42f, 1.8f },
    { NOTE_F3, 0.18f, 0.48f, 1.8f },
    { NOTE_A3, 0.28f, 0.58f, 2.0f },
    { NOTE_C4, 0.28f, 0.62f, 2.2f },
    { NOTE_F4, 0.42f, 0.70f, 2.4f },
    // G7 & Bloom Resolution
    { NOTE_G2, 0.28f, 0.55f, 1.4f },
    { NOTE_D3, 0.18f, 0.42f, 1.8f },
    { NOTE_B3, 0.18f, 0.48f, 1.8f },
    { NOTE_F4, 0.30f, 0.60f, 2.0f },
    { NOTE_G4, 0.60f, 0.68f, 2.4f },
    { NOTE_REST, 0.35f, 0.0f, 0.0f }
};

static const MelodicNote SEQ_TRUE_ENDING[] = {
    // Fmaj7
    { NOTE_F2, 0.34f, 0.58f, 1.3f },
    { NOTE_C3, 0.24f, 0.45f, 1.8f },
    { NOTE_E3, 0.24f, 0.48f, 1.8f },
    { NOTE_A3, 0.32f, 0.55f, 2.0f },
    { NOTE_C4, 0.32f, 0.60f, 2.2f },
    { NOTE_E4, 0.45f, 0.68f, 2.5f },
    // G
    { NOTE_G2, 0.34f, 0.58f, 1.3f },
    { NOTE_D3, 0.24f, 0.45f, 1.8f },
    { NOTE_G3, 0.24f, 0.48f, 1.8f },
    { NOTE_B3, 0.32f, 0.55f, 2.0f },
    { NOTE_D4, 0.32f, 0.60f, 2.2f },
    { NOTE_G4, 0.45f, 0.68f, 2.5f },
    // Em7
    { NOTE_E2, 0.34f, 0.58f, 1.3f },
    { NOTE_B2, 0.24f, 0.45f, 1.8f },
    { NOTE_G3, 0.24f, 0.48f, 1.8f },
    { NOTE_D4, 0.32f, 0.55f, 2.0f },
    { NOTE_E4, 0.32f, 0.60f, 2.2f },
    { NOTE_G4, 0.45f, 0.68f, 2.5f },
    // Am7
    { NOTE_A2, 0.34f, 0.58f, 1.3f },
    { NOTE_E3, 0.24f, 0.45f, 1.8f },
    { NOTE_A3, 0.24f, 0.48f, 1.8f },
    { NOTE_C4, 0.32f, 0.55f, 2.0f },
    { NOTE_E4, 0.32f, 0.60f, 2.2f },
    { NOTE_A4, 0.45f, 0.70f, 2.5f },
    // Dm7
    { NOTE_D2, 0.34f, 0.58f, 1.3f },
    { NOTE_A2, 0.24f, 0.45f, 1.8f },
    { NOTE_F3, 0.24f, 0.48f, 1.8f },
    { NOTE_C4, 0.32f, 0.55f, 2.0f },
    { NOTE_F4, 0.32f, 0.60f, 2.2f },
    { NOTE_A4, 0.45f, 0.70f, 2.5f },
    // G7
    { NOTE_G2, 0.34f, 0.58f, 1.3f },
    { NOTE_D3, 0.24f, 0.45f, 1.8f },
    { NOTE_F3, 0.24f, 0.48f, 1.8f },
    { NOTE_B3, 0.32f, 0.55f, 2.0f },
    { NOTE_D4, 0.32f, 0.60f, 2.2f },
    { NOTE_G4, 0.45f, 0.70f, 2.5f },
    // Cmaj7 Final Cord Flourish
    { NOTE_C2, 0.40f, 0.65f, 1.0f },
    { NOTE_G2, 0.28f, 0.50f, 1.4f },
    { NOTE_E3, 0.28f, 0.52f, 1.6f },
    { NOTE_B3, 0.35f, 0.58f, 1.8f },
    { NOTE_C4, 0.45f, 0.62f, 2.0f },
    { NOTE_E4, 0.55f, 0.68f, 2.2f },
    { NOTE_G4, 0.85f, 0.75f, 2.5f },
    { NOTE_REST, 0.80f, 0.0f, 0.0f }
};

static const MelodicNote SEQ_HOSPITAL[] = {
    // Minimalist, solemn single notes with distant medical echo
    { NOTE_C2, 1.70f, 0.55f, 0.40f },
    { NOTE_E6, 1.70f, 0.12f, 1.80f },
    { NOTE_G2, 1.70f, 0.55f, 0.40f },
    { NOTE_E6, 1.70f, 0.12f, 1.80f },
    { NOTE_Ab2, 1.70f, 0.55f, 0.40f },
    { NOTE_Eb6, 1.70f, 0.12f, 1.80f },
    { NOTE_G2, 1.70f, 0.55f, 0.40f },
    { NOTE_D6, 1.70f, 0.12f, 1.80f },
    { NOTE_REST, 1.00f, 0.0f, 0.0f }
};

// State
static AudioStream audio_stream;
static bool is_initialized = false;
static float master_volume = 0.85f;
static float bgm_volume = 0.75f;
static float sfx_volume = 0.90f;

static BGMTheme current_theme = BGM_THEME_INTRO;
static int bgm_note_cursor = 0;
static float bgm_timer = 0.0f;

static SynthVoice voices[MAX_VOICES];
static TimedNoteEvent timed_events[MAX_TIMED_EVENTS];

// Fast pseudo-random number generator for procedural noise
static uint32_t noise_seed = 123456789;
static inline float NextNoise(void) {
    noise_seed = (1103515245 * noise_seed + 12345);
    return ((float)(noise_seed & 0x7FFFFFFF) / (float)0x3FFFFFFF) - 1.0f;
}

// Internal Note Allocator
static void TriggerVoiceInternal(float freq, float volume, float decay, VoiceTimbre timbre, bool is_sfx) {
    int best_idx = -1;
    float lowest_env = 999.0f;

    for (int i = 0; i < MAX_VOICES; i++) {
        if (!voices[i].active) {
            best_idx = i;
            break;
        }
        if (voices[i].env < lowest_env) {
            lowest_env = voices[i].env;
            best_idx = i;
        }
    }

    if (best_idx >= 0) {
        voices[best_idx].freq = freq;
        voices[best_idx].phase = 0.0f;
        voices[best_idx].env = 1.0f;
        voices[best_idx].decay = decay > 0.05f ? decay : 1.5f;
        voices[best_idx].volume = volume;
        voices[best_idx].timbre = timbre;
        voices[best_idx].is_sfx = is_sfx;
        voices[best_idx].active = true;
    }
}

static void ScheduleNoteEvent(float delay, float freq, float volume, float decay, VoiceTimbre timbre, bool is_sfx) {
    if (delay <= 0.001f) {
        TriggerVoiceInternal(freq, volume, decay, timbre, is_sfx);
        return;
    }

    for (int i = 0; i < MAX_TIMED_EVENTS; i++) {
        if (!timed_events[i].active) {
            timed_events[i].time_remaining = delay;
            timed_events[i].freq = freq;
            timed_events[i].volume = volume;
            timed_events[i].decay = decay;
            timed_events[i].timbre = timbre;
            timed_events[i].is_sfx = is_sfx;
            timed_events[i].active = true;
            break;
        }
    }
}

// Synthesis rendering loop
static void RenderAudioBlock(short* output_buffer, int num_samples) {
    const float dt = 1.0f / (float)SAMPLE_RATE;

    for (int s = 0; s < num_samples; s++) {
        // 1. Advance timed note events
        for (int e = 0; e < MAX_TIMED_EVENTS; e++) {
            if (timed_events[e].active) {
                timed_events[e].time_remaining -= dt;
                if (timed_events[e].time_remaining <= 0.0f) {
                    TriggerVoiceInternal(timed_events[e].freq,
                                         timed_events[e].volume,
                                         timed_events[e].decay,
                                         timed_events[e].timbre,
                                         timed_events[e].is_sfx);
                    timed_events[e].active = false;
                }
            }
        }

        // 2. Advance BGM Sequencer
        bgm_timer -= dt;
        if (bgm_timer <= 0.0f) {
            const MelodicNote* seq = NULL;
            int seq_len = 0;

            switch (current_theme) {
                case BGM_THEME_INTRO:
                    seq = SEQ_INTRO;
                    seq_len = (int)(sizeof(SEQ_INTRO) / sizeof(SEQ_INTRO[0]));
                    break;
                case BGM_THEME_SEARCHING:
                    seq = SEQ_SEARCHING;
                    seq_len = (int)(sizeof(SEQ_SEARCHING) / sizeof(SEQ_SEARCHING[0]));
                    break;
                case BGM_THEME_CLUE_FOUND:
                    seq = SEQ_CLUE_FOUND;
                    seq_len = (int)(sizeof(SEQ_CLUE_FOUND) / sizeof(SEQ_CLUE_FOUND[0]));
                    break;
                case BGM_THEME_TRUE_ENDING:
                    seq = SEQ_TRUE_ENDING;
                    seq_len = (int)(sizeof(SEQ_TRUE_ENDING) / sizeof(SEQ_TRUE_ENDING[0]));
                    break;
                case BGM_THEME_HOSPITAL:
                    seq = SEQ_HOSPITAL;
                    seq_len = (int)(sizeof(SEQ_HOSPITAL) / sizeof(SEQ_HOSPITAL[0]));
                    break;
            }

            if (seq != NULL && seq_len > 0) {
                if (bgm_note_cursor >= seq_len) {
                    bgm_note_cursor = 0;
                }

                MelodicNote note = seq[bgm_note_cursor];
                bgm_timer = note.duration;
                bgm_note_cursor++;

                if (note.freq > 10.0f && bgm_volume > 0.01f) {
                    TriggerVoiceInternal(note.freq, note.volume, note.decay, TIMBRE_PIANO, false);
                }
            } else {
                bgm_timer = 0.5f;
            }
        }

        // 3. Render active synth voices
        float mix = 0.0f;

        for (int v = 0; v < MAX_VOICES; v++) {
            if (!voices[v].active) continue;

            float sample = 0.0f;
            float p = voices[v].phase * 2.0f * PI_VAL;

            if (voices[v].timbre == TIMBRE_PIANO) {
                // Procedural Piano with Rich Overtones
                float env = voices[v].env;
                float env2 = env * env;
                float h1 = sinf(p);
                float h2 = sinf(p * 2.0f) * 0.38f * env2;
                float h3 = sinf(p * 3.0f) * 0.16f * (env2 * env);
                float h4 = sinf(p * 4.0f) * 0.07f * (env2 * env2);
                sample = (h1 + h2 + h3 + h4) * env;
            } else if (voices[v].timbre == TIMBRE_BELL) {
                // High Shimmering Bell/Chime with inharmonic overtone
                float env = voices[v].env;
                float h1 = sinf(p);
                float h2 = sinf(p * 2.76f) * 0.42f * env;
                float h3 = sinf(p * 5.40f) * 0.22f * (env * env);
                sample = (h1 + h2 + h3) * env;
            } else if (voices[v].timbre == TIMBRE_HEARTBEAT) {
                // Low Muffled Sine Pulse
                sample = sinf(p) * voices[v].env * 1.6f;
            } else if (voices[v].timbre == TIMBRE_NOISE) {
                // Filtered Noise Burst
                sample = NextNoise() * voices[v].env;
            }

            // Apply voice category volume
            float cat_vol = voices[v].is_sfx ? sfx_volume : bgm_volume;
            mix += sample * voices[v].volume * cat_vol;

            // Phase and Envelope decay
            voices[v].phase += voices[v].freq * dt;
            if (voices[v].phase >= 1.0f) {
                voices[v].phase -= floorf(voices[v].phase);
            }

            // Natural exponential decay
            voices[v].env -= dt * voices[v].decay * (0.35f + 0.65f * voices[v].env);
            if (voices[v].env <= 0.0005f) {
                voices[v].env = 0.0f;
                voices[v].active = false;
            }
        }

        // 4. Master Volume & Soft Limiter
        mix *= master_volume;
        if (mix > 1.0f) mix = 1.0f;
        if (mix < -1.0f) mix = -1.0f;

        output_buffer[s] = (short)(mix * 28000.0f);
    }
}

// Public API Implementations

void AudioSynth_Init(void) {
    if (is_initialized) return;

    InitAudioDevice();
    if (!IsAudioDeviceReady()) {
        is_initialized = false;
        return;
    }

    SetAudioStreamBufferSizeDefault(AUDIO_BUFFER_SIZE);
    audio_stream = LoadAudioStream(SAMPLE_RATE, 16, 1);
    if (!IsAudioStreamValid(audio_stream)) {
        CloseAudioDevice();
        is_initialized = false;
        return;
    }

    PlayAudioStream(audio_stream);
    is_initialized = true;

    for (int i = 0; i < MAX_VOICES; i++) {
        voices[i].active = false;
    }
    for (int i = 0; i < MAX_TIMED_EVENTS; i++) {
        timed_events[i].active = false;
    }

    current_theme = BGM_THEME_INTRO;
    bgm_note_cursor = 0;
    bgm_timer = 0.05f;
}

void AudioSynth_Update(float dt) {
    (void)dt;
    if (!is_initialized || !IsAudioStreamValid(audio_stream)) return;

    int max_passes = 4;
    while (IsAudioStreamProcessed(audio_stream) && max_passes > 0) {
        short buffer[AUDIO_BUFFER_SIZE];
        RenderAudioBlock(buffer, AUDIO_BUFFER_SIZE);
        UpdateAudioStream(audio_stream, buffer, AUDIO_BUFFER_SIZE);
        max_passes--;
    }
}

void AudioSynth_PlaySFX(SFXType sfx) {
    switch (sfx) {
        case SFX_SUBWAY_CHIME: {
            // Authentic Seoul Subway Transfer Arpeggio Chime
            ScheduleNoteEvent(0.00f, NOTE_E5, 0.70f, 2.0f, TIMBRE_BELL, true);
            ScheduleNoteEvent(0.12f, NOTE_G5, 0.75f, 2.2f, TIMBRE_BELL, true);
            ScheduleNoteEvent(0.24f, NOTE_B5, 0.80f, 2.4f, TIMBRE_BELL, true);
            ScheduleNoteEvent(0.36f, NOTE_E6, 0.85f, 2.6f, TIMBRE_BELL, true);
            ScheduleNoteEvent(0.50f, NOTE_G6, 0.90f, 2.8f, TIMBRE_BELL, true);
            ScheduleNoteEvent(0.66f, NOTE_B6, 0.95f, 3.2f, TIMBRE_BELL, true);
            // Sustained chime resolution
            ScheduleNoteEvent(0.82f, NOTE_E6, 0.60f, 3.5f, TIMBRE_BELL, true);
            ScheduleNoteEvent(0.82f, NOTE_G6, 0.60f, 3.5f, TIMBRE_BELL, true);
            ScheduleNoteEvent(0.82f, NOTE_E7, 0.70f, 4.0f, TIMBRE_BELL, true);
            break;
        }
        case SFX_TRAIN_RAIL_LOOP: {
            // Rail Joint Wheel Clatter & Low Rumble
            ScheduleNoteEvent(0.00f, 85.0f,  0.55f, 9.0f, TIMBRE_NOISE, true);
            ScheduleNoteEvent(0.04f, 130.0f, 0.70f, 12.0f, TIMBRE_NOISE, true);
            ScheduleNoteEvent(0.16f, 110.0f, 0.50f, 12.0f, TIMBRE_NOISE, true);
            ScheduleNoteEvent(0.55f, 85.0f,  0.45f, 9.0f, TIMBRE_NOISE, true);
            ScheduleNoteEvent(0.60f, 125.0f, 0.65f, 12.0f, TIMBRE_NOISE, true);
            ScheduleNoteEvent(0.72f, 105.0f, 0.45f, 12.0f, TIMBRE_NOISE, true);
            break;
        }
        case SFX_DOOR_OPEN: {
            // Melodic Door Open Beep + Pneumatic Hiss
            ScheduleNoteEvent(0.00f, NOTE_G5, 0.65f, 4.0f, TIMBRE_BELL, true);
            ScheduleNoteEvent(0.18f, NOTE_C6, 0.75f, 3.0f, TIMBRE_BELL, true);
            ScheduleNoteEvent(0.05f, 180.0f,  0.30f, 3.0f, TIMBRE_NOISE, true);
            break;
        }
        case SFX_DOOR_CLOSE: {
            // 3 Door Warning Beeps + Door Closing Clunk
            ScheduleNoteEvent(0.00f, NOTE_B5, 0.75f, 5.5f, TIMBRE_BELL, true);
            ScheduleNoteEvent(0.15f, NOTE_B5, 0.75f, 5.5f, TIMBRE_BELL, true);
            ScheduleNoteEvent(0.30f, NOTE_B5, 0.80f, 5.5f, TIMBRE_BELL, true);
            ScheduleNoteEvent(0.48f, 75.0f,   0.60f, 8.0f, TIMBRE_NOISE, true);
            break;
        }
        case SFX_HEARTBEAT: {
            // Double Heartbeat Thump (Lub-Dub)
            ScheduleNoteEvent(0.00f, 58.0f, 0.85f, 10.0f, TIMBRE_HEARTBEAT, true);
            ScheduleNoteEvent(0.15f, 48.0f, 0.65f, 9.0f, TIMBRE_HEARTBEAT, true);
            break;
        }
        case SFX_MEMORY_SHIMMER: {
            // Crystalline Ascending Shimmer
            ScheduleNoteEvent(0.00f, NOTE_C6, 0.65f, 2.5f, TIMBRE_BELL, true);
            ScheduleNoteEvent(0.08f, NOTE_E6, 0.70f, 2.6f, TIMBRE_BELL, true);
            ScheduleNoteEvent(0.16f, NOTE_G6, 0.75f, 2.8f, TIMBRE_BELL, true);
            ScheduleNoteEvent(0.24f, NOTE_B6, 0.80f, 3.0f, TIMBRE_BELL, true);
            ScheduleNoteEvent(0.32f, NOTE_C7, 0.85f, 3.4f, TIMBRE_BELL, true);
            ScheduleNoteEvent(0.42f, NOTE_E7, 0.90f, 4.0f, TIMBRE_BELL, true);
            break;
        }
    }
}

void AudioSynth_PlaySubwayChime(void) {
    AudioSynth_PlaySFX(SFX_SUBWAY_CHIME);
}

void AudioSynth_PlayPianoNote(float freq, float volume, float decay) {
    TriggerVoiceInternal(freq, volume, decay, TIMBRE_PIANO, true);
}

void AudioSynth_SetBGM(BGMTheme theme) {
    if (current_theme != theme) {
        current_theme = theme;
        bgm_note_cursor = 0;
        bgm_timer = 0.05f;
    }
}

void AudioSynth_SetTheme(BGMTheme theme) {
    AudioSynth_SetBGM(theme);
}

BGMTheme AudioSynth_GetTheme(void) {
    return current_theme;
}

void AudioSynth_SetMasterVolume(float vol) {
    if (vol < 0.0f) vol = 0.0f;
    if (vol > 1.0f) vol = 1.0f;
    master_volume = vol;
}

void AudioSynth_SetBGMVolume(float vol) {
    if (vol < 0.0f) vol = 0.0f;
    if (vol > 1.0f) vol = 1.0f;
    bgm_volume = vol;
}

void AudioSynth_SetSFXVolume(float vol) {
    if (vol < 0.0f) vol = 0.0f;
    if (vol > 1.0f) vol = 1.0f;
    sfx_volume = vol;
}

bool AudioSynth_IsInitialized(void) {
    return is_initialized;
}

void AudioSynth_Close(void) {
    if (is_initialized) {
        if (IsAudioStreamValid(audio_stream)) {
            StopAudioStream(audio_stream);
            UnloadAudioStream(audio_stream);
        }
        CloseAudioDevice();
        is_initialized = false;
    }
}
