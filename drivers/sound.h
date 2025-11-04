#pragma once
#include <types.h>
#include <stdint.h>
#include <stdbool.h>

#define AUDIO_PIN_L 26
#define AUDIO_PIN_R 27

#define BITRATE  16000 // in hz
#define BITDEPTH 65535 // 16bit samples
#define CHANNELS 4

typedef struct {
	int len;
	char* period;
} period_t;

typedef struct {
	const int16_t *sample;
	uint32_t sample_len;
	uint32_t sample_pos;
	uint32_t count;
	uint32_t count_released;
	const period_t* period;
	uint8_t period_pos;
	uint8_t period_mult;
	float volume;
	uint32_t attack_cnt;
	uint32_t decay_cnt;
	float sustain;
	uint32_t release_cnt;
	bool repeat;
	bool playing;
} sound_channel_t;

void sound_init();
void sound_setclk();
void sound_play(uint8_t ch, int note);
void sound_stop(uint8_t ch);
void sound_off(uint8_t ch);
void sound_setup(uint8_t ch, uint8_t wave, float volume, float attack, float decay, float sustain, float release);
