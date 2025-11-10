#pragma once
#include <types.h>
#include <stdint.h>
#include <stdbool.h>

#define AUDIO_PIN_L 26
#define AUDIO_PIN_R 27

#define BITRATE  16000 // in hz
#define BITDEPTH 65535 // 16bit samples
#define CHANNELS 8

#define PITCH_RESOLUTION 256

typedef struct {
	int len;
	char* period;
} period_t;

enum TABLE_MODE {
	TABLE_SINGLE,   // one sample, then end
	TABLE_ONESHOT,  // sweep table, loop last wave
	TABLE_PINGPONG, // sweep table forwards then backwards
	TABLE_LOOP,     // sweep table forwards and start over
};

typedef struct {
	const int16_t *sample;
	uint32_t sample_len;
	uint32_t sample_pos;
	uint16_t sample_pos_increment;
	uint32_t counter;
	uint32_t counter_released;
	float volume;
	uint32_t attack_cnt;
	uint32_t decay_cnt;
	float sustain;
	uint32_t release_cnt;
	bool playing;
	int16_t start_at;
	enum TABLE_MODE table_mode;
	uint16_t table_len;
	uint16_t table_pos;
	uint16_t table_start;
	uint16_t table_end;
	int16_t table_playcount;
	int16_t table_playrate;
} sound_channel_t;

typedef struct {
	uint8_t wave;
	float volume;
	uint16_t attack;
	uint16_t decay;
	float sustain;
	uint16_t release;
	enum TABLE_MODE table_mode;
	uint16_t table_start;
	uint16_t table_end;
	int16_t table_playrate;
} instrument_t;

void sound_init();
void sound_setclk();
void sound_playnote(uint8_t ch, int note, instrument_t *inst);
void sound_playpitch(uint8_t ch, float pitch, instrument_t *inst);
void sound_stop(uint8_t ch);
void sound_off(uint8_t ch);
void sound_setvolume(uint8_t ch, float volume, bool relative);
void sound_setpitch(uint8_t ch, float pitch, bool relative);
void sound_stopall();
const int16_t* sound_getsampledata(uint8_t wave, uint16_t* table_len, uint16_t* sample_len);
uint8_t sound_getsamplecount();