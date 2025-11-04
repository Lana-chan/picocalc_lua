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
	uint32_t position;
	uint32_t position_released;
	const period_t* period;
	uint8_t period_pos;
	uint8_t period_mult;
	float volume;
	float attack;
	float decay;
	float sustain;
	float release;
	uint32_t attack_pos;
	uint32_t decay_pos;
	uint32_t release_pos;
	bool repeat;
	bool playing;
} sound_channel_t;

void sound_init();
void sound_setclk();
void sound_play(uint8_t ch, int note);
void sound_stop(uint8_t ch);
void sound_off(uint8_t ch);
void sound_setup(uint8_t ch, uint8_t wave, float volume, float attack, float decay, float sustain, float release);
