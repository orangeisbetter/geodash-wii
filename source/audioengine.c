#include "audioengine.h"

#include <stdio.h>
#include <unistd.h>
#include <gccore.h>
#include <string.h>
#include <mad.h>

#define MAD_INPUT_BUFFER_SIZE 8192
#define AUDIO_BUF_SIZE 8192
#define AUDIO_BUF_COUNT 4

static u8 audio_buffer[AUDIO_BUF_COUNT][AUDIO_BUF_SIZE] __attribute__((aligned(32)));
volatile int write_idx = 0;
volatile int read_idx = 0;
volatile int filled_buffers = 0;

static int buffer_sizes[AUDIO_BUF_COUNT];

static u8 input_buf[MAD_INPUT_BUFFER_SIZE + MAD_BUFFER_GUARD];

static struct {
    FILE* file;
    size_t filesize;
    struct mad_stream stream;
    struct mad_frame frame;
    struct mad_synth synth;
    bool eof;
} mp3_ctx;

static lwp_t audio_thread;

static bool playing = false;

#define STACKSIZE 32760
static u8 mp3_stack[STACKSIZE];

void mp3_callback();
void* MP3DecodeThread(void* arg);

void start_mp3(const char* filename) {
    mp3_ctx.file = fopen(filename, "rb");
    if (!mp3_ctx.file)
        return;

    mad_stream_init(&mp3_ctx.stream);
    mad_frame_init(&mp3_ctx.frame);
    mad_synth_init(&mp3_ctx.synth);
    // mad_timer_reset(&mp3_ctx.timer);

    memset(audio_buffer[0], 0, AUDIO_BUF_SIZE);
    memset(audio_buffer[1], 0, AUDIO_BUF_SIZE);

    mp3_ctx.eof = false;

    AUDIO_Init(NULL);
    AUDIO_SetDSPSampleRate(AI_SAMPLERATE_48KHZ);
    AUDIO_RegisterDMACallback(mp3_callback);

    LWP_CreateThread(&audio_thread, MP3DecodeThread, NULL, mp3_stack, STACKSIZE, 50);
}

s16 mad_fixed_to_short(mad_fixed_t sample) {
    if (sample >= MAD_F_ONE)
        sample = MAD_F_ONE - 1;
    else if (sample < -MAD_F_ONE)
        sample = -MAD_F_ONE;
    return (s16)(sample >> (MAD_F_FRACBITS - 15));
}

void* MP3DecodeThread(void* arg) {
    while (!mp3_ctx.eof) {
        if (filled_buffers >= AUDIO_BUF_COUNT) {
            usleep(1000);
            continue;
        }

        u8* buf = audio_buffer[write_idx];
        int buf_pos = 0;

        while (buf_pos + 4 <= AUDIO_BUF_SIZE) {
            if (mp3_ctx.stream.buffer == NULL || mp3_ctx.stream.error == MAD_ERROR_BUFLEN) {
                size_t read_size = fread(input_buf, 1, MAD_INPUT_BUFFER_SIZE, mp3_ctx.file);
                if (read_size == 0) {
                    mp3_ctx.eof = true;
                    break;
                }
                if (read_size < MAD_INPUT_BUFFER_SIZE) {
                    memset(input_buf + read_size, 0, MAD_BUFFER_GUARD);
                }
                mad_stream_buffer(&mp3_ctx.stream, input_buf, read_size);
            }

            if (mad_frame_decode(&mp3_ctx.frame, &mp3_ctx.stream)) {
                if (MAD_RECOVERABLE(mp3_ctx.stream.error)) {
                    continue;
                } else if (mp3_ctx.stream.error == MAD_ERROR_BUFLEN) {
                    continue;
                } else {
                    break;
                }
            }

            mad_synth_frame(&mp3_ctx.synth, &mp3_ctx.frame);

            for (unsigned int i = 0; i < mp3_ctx.synth.pcm.length && buf_pos + 4 <= AUDIO_BUF_SIZE; i++) {
                s16 left = mad_fixed_to_short(mp3_ctx.synth.pcm.samples[0][i]);
                s16 right = mad_fixed_to_short(mp3_ctx.synth.pcm.samples[1][i]);

                buf[buf_pos++] = left >> 8;
                buf[buf_pos++] = left & 0xFF;
                buf[buf_pos++] = right >> 8;
                buf[buf_pos++] = right & 0xFF;
            }
        }

        if (buf_pos > 0) {
            buffer_sizes[write_idx] = buf_pos;
            write_idx = (write_idx + 1) % AUDIO_BUF_COUNT;
            filled_buffers++;

            if (!playing) {
                playing = true;
                mp3_callback();
            }
        }
    }

    return NULL;
}

void mp3_callback() {
    if (filled_buffers == 0) {
        return;
    }

    u8* buf = audio_buffer[read_idx];
    int size = buffer_sizes[read_idx];

    AUDIO_InitDMA((u32)buf, size);
    AUDIO_StartDMA();

    read_idx = (read_idx + 1) % AUDIO_BUF_COUNT;
    filled_buffers--;
}

// static void Resample(struct mad_pcm* Pcm, EQState eqs[2], u32 stereo, u32 src_samplerate) {
//     u16 val16;
//     u32 val32;
//     dword pos;
//     s32 incr;

//     pos.adword = 0;
//     incr = (u32)(((f32)src_samplerate / 48000.0F) * 65536.0F);
//     while (pos.aword.hi < Pcm->length) {
//         val16 = Do3Band(&eqs[0], FixedToShort(Pcm->samples[0][pos.aword.hi]));
//         val32 = (val16 << 16);

//         if (stereo)
//             val16 = Do3Band(&eqs[1], FixedToShort(Pcm->samples[1][pos.aword.hi]));
//         val32 |= val16;

//         buf_put(&OutputRingBuffer, &val32, sizeof(u32));
//         pos.adword += incr;
//     }
// }

void AE_Init() {
    AUDIO_Init(NULL);
    AUDIO_SetDSPSampleRate(AI_SAMPLERATE_48KHZ);
    AUDIO_RegisterDMACallback(NULL);
}