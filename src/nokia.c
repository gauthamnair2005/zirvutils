/* zirvutils/src/nokia.c — Nokia Composer: alphabet-based music generator
 *
 * Usage: nokia [tempo] <note> [note] ...
 *
 * Note format: [duration][pitch][octave][.]
 *   duration: 1,2,4,8,16,32 (denominator; default=4 = quarter)
 *   pitch:    A-G (uppercase), R = rest
 *   octave:   4-7 (default=4)
 *   .:        dotted (×1.5 duration)
 *
 * Examples:
 *   nokia "4C4 4D4 4E4 4F4 4G4 4A4 4B4 4C5"
 *   nokia 180 "8C5 8D5 8E5 8F5 8G5 8A5 8B5 8C6"
 *   nokia "4E5 4E5 8E5 4C5 4E5 4G5 4G5 8G5 4C6"
 *
 * Uses a 48 kHz PCM audio output via the audio_play() syscall.
 */
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define SAMPLE_RATE 48000
#define MAX_NOTES   256
#define PCM_BUF_MAX (SAMPLE_RATE * 5)

static int16_t g_pcm[PCM_BUF_MAX * 2];
static int g_pcm_frames = 0;

static const float note_freq[12] = {
    261.63f, 277.18f, 293.66f, 311.13f, 329.63f, 349.23f,
    369.99f, 392.00f, 415.30f, 440.00f, 466.16f, 493.88f
};

static int note_to_semitone(char pitch, char sharp)
{
    switch (pitch) {
    case 'C': return sharp ? 1 : 0;
    case 'D': return sharp ? 3 : 2;
    case 'E': return 4;
    case 'F': return sharp ? 6 : 5;
    case 'G': return sharp ? 8 : 7;
    case 'A': return sharp ? 10 : 9;
    case 'B': return 11;
    default:  return -1;
    }
}

static void get_freq(const char *s, int *consumed, float *freq_out)
{
    *consumed = 0;
    *freq_out = 0.0f;
    if (!s || !*s) return;
    if (*s == 'R') { *consumed = 1; return; }

    char sharp = 0;
    int idx = 1;
    if (s[idx] == '#' || s[idx] == 'S') { sharp = 1; idx++; }

    int semitone = note_to_semitone(s[0], sharp);
    if (semitone < 0) return;

    int octave = 4;
    if (s[idx] >= '0' && s[idx] <= '9')
        octave = s[idx++] - '0';

    *consumed = idx;
    *freq_out = note_freq[semitone] * (float)(1 << (octave - 4));
}

static int parse_note(const char *s, float *dur_out, float *freq_out)
{
    *dur_out = 0.25f;
    *freq_out = 0.0f;
    if (!s || !*s) return 0;

    const char *p = s;
    int dur = 4;
    if (*p >= '0' && *p <= '9') {
        dur = 0;
        while (*p >= '0' && *p <= '9') dur = dur * 10 + (*p++ - '0');
        if (dur < 1) dur = 1;
        if (dur > 64) dur = 64;
    }
    if (!*p) return 0;

    if (*p == 'R' || *p == 'r') {
        p++;
        int dot = (*p == '.') ? (p++, 1) : 0;
        *dur_out = 1.0f / (float)dur;
        if (dot) *dur_out *= 1.5f;
        *freq_out = 0.0f;
        return (int)(p - s);
    }

    int consumed;
    float freq;
    get_freq(p, &consumed, &freq);
    if (consumed == 0) return 0;
    p += consumed;

    int dot = (*p == '.') ? (p++, 1) : 0;
    *freq_out = freq;
    *dur_out = 1.0f / (float)dur;
    if (dot) *dur_out *= 1.5f;
    return (int)(p - s);
}

static void generate_square(float freq, float duration_s,
                            int start_frame, float volume)
{
    int frames = (int)(duration_s * SAMPLE_RATE);
    if (frames <= 0) return;
    if (start_frame + frames > PCM_BUF_MAX)
        frames = PCM_BUF_MAX - start_frame;
    if (frames <= 0) return;

    if (freq <= 0.0f) {
        for (int i = 0; i < frames; i++)
            g_pcm[(start_frame + i) * 2] = g_pcm[(start_frame + i) * 2 + 1] = 0;
        return;
    }

    float period = (float)SAMPLE_RATE / freq;
    for (int i = 0; i < frames; i++) {
        int16_t s = (int16_t)(volume * 32767.0f *
                     (((int)((start_frame + i) / period) & 1) ? 1.0f : -1.0f));
        g_pcm[(start_frame + i) * 2] = s;
        g_pcm[(start_frame + i) * 2 + 1] = s;
    }
}

int main(int argc, char **argv)
{
    int tempo = 120;
    int note_start = 1;

    if (argc < 2) {
        printf("Usage: %s [tempo] <note> [note] ...\n", argv[0]);
        printf("  nokia \"4C4 4E4 4G4 8C5\"\n");
        printf("  nokia 180 \"8E5 8D5 8C5 8D5\"\n");
        return 1;
    }

    if (argc >= 3) {
        tempo = atoi(argv[1]);
        if (tempo < 20) tempo = 20;
        if (tempo > 400) tempo = 400;
        note_start = 2;
    }

    g_pcm_frames = 0;

    for (int a = note_start; a < argc; a++) {
        const char *p = argv[a];
        while (*p && g_pcm_frames < PCM_BUF_MAX) {
            while (*p == ' ' || *p == '\t') p++;
            if (!*p) break;

            float dur, freq;
            int c = parse_note(p, &dur, &freq);
            if (c <= 0) { p++; continue; }

            float duration_s = dur * (60.0f / (float)tempo);
            generate_square(freq, duration_s, g_pcm_frames, 0.5f);
            g_pcm_frames += (int)(duration_s * SAMPLE_RATE);
            p += c;
        }
    }

    if (g_pcm_frames <= 0) {
        printf("No audio generated\n");
        return 1;
    }

    int played = 0;
    while (played < g_pcm_frames) {
        int chunk = g_pcm_frames - played;
        if (chunk > 4096) chunk = 4096;
        int w = audio_play(g_pcm + played * 2, (unsigned int)chunk);
        if (w <= 0) break;
        played += w;
    }

    printf("Played %d frames (%d seconds)\n",
           g_pcm_frames, g_pcm_frames / SAMPLE_RATE);
    return 0;
}
