#pragma once
#include <types.h>
#include <stdint.h>
#include <stdbool.h>

#define AUDIO_PIN_L 26
#define AUDIO_PIN_R 27

#define BITRATE  16000 // in hz
#define BITDEPTH 65535 // 16bit samples
#define CHANNELS 8

typedef struct {
	int len;
	char* period;
} period_t;

enum AMP_MODE {
	AMP_ADSR,
	AMP_ARRAY,
};

typedef struct {
	const int16_t *sample;
	uint32_t sample_len;
	uint32_t sample_pos;
	uint16_t pos_increment;
	uint32_t counter;
	uint32_t counter_released;
	float volume;
	enum AMP_MODE amp_mode;
	uint32_t attack_cnt;
	uint32_t decay_cnt;
	float sustain;
	uint32_t release_cnt;
	bool repeat;
	bool playing;
	int16_t start_at;
} sound_channel_t;

void sound_init();
void sound_setclk();
void sound_playnote(uint8_t ch, int note);
void sound_playpitch(uint8_t ch, float pitch);
void sound_stop(uint8_t ch);
void sound_off(uint8_t ch);
void sound_setup(uint8_t ch, uint8_t wave, float volume, float attack, float decay, float sustain, float release);
