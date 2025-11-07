#include "sound.h"
#include "hardware/pwm.h"
#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/clocks.h"
#include "pico/time.h"
#include <stdio.h>
#include <math.h>

#include "samples.h"

static uint sound_slice;
static uint32_t sound_wrap_value;

static int sound_dma_chan;
static int sound_dma_timer;
static dma_channel_config sound_dma_config;

#define SOUND_BUFFER_SIZE 1024
#define SOUND_BUFFER_COUNT 4
static uint16_t sound_buffer[SOUND_BUFFER_COUNT][SOUND_BUFFER_SIZE];
static uint8_t sound_buffer_select;
static absolute_time_t buffer_time_corrector;

static sound_channel_t sound_chs[CHANNELS];
static sound_channel_t schedule_chs[CHANNELS];

static int16_t sound_process(sound_channel_t* ch) {
	float amp;

	// ADSR calculation
	if (ch->counter <= ch->attack_cnt) {
		amp = (float)ch->counter / (float)ch->attack_cnt;
	} else if (ch->counter <= ch->attack_cnt + ch->decay_cnt) {
		amp = 1.0f - (float)(ch->counter - ch->attack_cnt) / (float)ch->decay_cnt * (1.0f - ch->sustain);
	} else if (ch->counter_released > 0) {
		if (ch->counter >= ch->counter_released + ch->release_cnt) {
			amp = 0;
			ch->playing = false;
		} else if (ch->counter >= ch->counter_released) {
			amp = ch->sustain - (float)(ch->counter - ch->counter_released) / (float)ch->release_cnt * ch->sustain;
		}
	} else if (ch->sustain == 0) {
			amp = 0;
			ch->playing = false;
	} else {
		amp = ch->sustain;
	}
	ch->counter++;
	
	int16_t out = ch->sample[(ch->sample_pos >> 8) % ch->sample_len] * amp * ch->volume;
	ch->sample_pos += ch->pos_increment;

	if (!ch->repeat && ch->sample_pos>>8 >= ch->sample_len) {
		ch->playing = false;
		ch->sample_pos = 0;
	}
	return out;
}

#define CLAMP(x, a, b) (x < a ? a : (x > b ? b : x))

static void sound_fillbuffer(uint16_t* buffer) {
	for (uint16_t i = 0; i < SOUND_BUFFER_SIZE; i++) {
		int32_t out = BITDEPTH / 2 + 1;
		for (uint8_t n = 0; n < CHANNELS; n++) {
			if (schedule_chs[n].start_at > 0 && i >= schedule_chs[n].start_at) {
				schedule_chs[n].start_at = -1;
				sound_chs[n] = schedule_chs[n];
			}
			sound_channel_t *ch = &sound_chs[n];
			if (ch->playing) {
				out += sound_process(ch);
			}
		}
		out = CLAMP(out, 0, BITDEPTH);
		buffer[i] = (sound_wrap_value-1) * out / (BITDEPTH);
	}
}

static void sound_initialbuffer(uint16_t* buffer) {
	// ramp from 0 to dc middle to diminish pop when booting
	for (uint16_t i = 0; i < SOUND_BUFFER_SIZE; i++) {
		buffer[i] = (sound_wrap_value-1)/2 * i / SOUND_BUFFER_SIZE;
	}
}

static void sound_dma_handler(void) {
	dma_hw->ints0 = 1u << sound_dma_chan;
	dma_channel_set_read_addr(sound_dma_chan, sound_buffer[sound_buffer_select], true);
	sound_buffer_select = (sound_buffer_select + 1) % SOUND_BUFFER_COUNT;
	buffer_time_corrector = get_absolute_time();
	sound_fillbuffer(sound_buffer[sound_buffer_select]);
}

void sound_init() {
	gpio_set_function(AUDIO_PIN_L, GPIO_FUNC_PWM);
	gpio_set_function(AUDIO_PIN_R, GPIO_FUNC_PWM);
	sound_slice = pwm_gpio_to_slice_num(AUDIO_PIN_L); // 26 and 27 are in same slice, different channels
	sound_wrap_value = 1024;
	pwm_set_clkdiv_int_frac(sound_slice, 1, 0);
	pwm_set_wrap(sound_slice, sound_wrap_value-1);

	sound_dma_chan = dma_claim_unused_channel(true);
	sound_dma_timer = dma_claim_unused_timer(true);
	sound_dma_config = dma_channel_get_default_config(sound_dma_chan);
	channel_config_set_transfer_data_size(&sound_dma_config, DMA_SIZE_16); // PWM HW register is 32bit, sending 16bit relpicates same sound on both channels
	channel_config_set_read_increment(&sound_dma_config, true);
	channel_config_set_write_increment(&sound_dma_config, false);
	channel_config_set_dreq(&sound_dma_config, dma_get_timer_dreq(sound_dma_timer));
	sound_setclk();

	dma_channel_configure(
		sound_dma_chan,
		&sound_dma_config,
		&pwm_hw->slice[sound_slice].cc,
		0,
		SOUND_BUFFER_SIZE,
		false
	);

	pwm_set_enabled(sound_slice, true);

	for (int i = 0; i < CHANNELS; i++) {
		sound_chs[i].playing = false;
		schedule_chs[i].start_at = -1;
	}

	dma_channel_set_irq1_enabled(sound_dma_chan, true);
	irq_set_exclusive_handler(DMA_IRQ_1, sound_dma_handler);
	irq_set_priority(DMA_IRQ_1, PICO_DEFAULT_IRQ_PRIORITY - 10);
	irq_set_enabled(DMA_IRQ_1, true);

	sound_buffer_select = 0;
	sound_initialbuffer(sound_buffer[sound_buffer_select]);
	sound_dma_handler();
}

void sound_setclk() {
	// https://github.com/tannewt/circuitpython/blob/191b143e7ba3b9f5786996cbffb42e2460f1c14d/ports/raspberrypi/common-hal/audiopwmio/PWMAudioOut.c#L178
  uint32_t sample_rate = BITRATE;
	uint32_t system_clock = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS)*1000;
	uint32_t best_numerator = 0;
	uint32_t best_denominator = 0;
	uint32_t best_error = system_clock;
	for (uint32_t denominator = 0xffff; denominator > 0; denominator--) {
		uint32_t numerator = (denominator * sample_rate) / system_clock;
		uint32_t remainder = (denominator * sample_rate) % system_clock;
		if (remainder > (system_clock / 2)) {
			numerator += 1;
			remainder = system_clock - remainder;
		}
		if (remainder < best_error) {
			best_denominator = denominator;
			best_numerator = numerator;
			best_error = remainder;
			// Stop early if we can't do better.
			if (remainder == 0) {
				break;
			}
		}
	}
	dma_timer_set_fraction(sound_dma_timer, best_numerator, best_denominator);
}

static inline int16_t get_sampletime_correction() {
	return absolute_time_diff_us(buffer_time_corrector, get_absolute_time()) * (float)((float)BITRATE / 1000000.0f);
}

void sound_playnote(uint8_t ch, int note, instrument_t *inst) {
	if (ch >= CHANNELS) return;

	const float pitches[] = {
		1.0,
		1.0594,
		1.1224,
		1.1892,
		1.2599,
		1.3348,
		1.4142,
		1.4983,
		1.5874,
		1.6817,
		1.7817,
		1.8877,
	};

	float pitch = pitches[note % 12];
	int8_t octave = note / 12 - 3;
	if (octave < 0)
		pitch /= pow(2, -octave);
	else
		pitch *= pow(2, octave);
	sound_playpitch(ch, pitch, inst);
}

void sound_playpitch(uint8_t ch, float pitch, instrument_t *inst) {
	if (ch >= CHANNELS) return;
	if (inst->wave > 12) return;

	schedule_chs[ch].sample = sample_waves[inst->wave];
	schedule_chs[ch].sample_len = sample_lens[inst->wave];
	schedule_chs[ch].sample_pos = 0;
	schedule_chs[ch].counter = 0;
	schedule_chs[ch].counter_released = 0;
	schedule_chs[ch].playing = false;
	schedule_chs[ch].repeat = (inst->wave < 6);
	schedule_chs[ch].volume = inst->volume;
	schedule_chs[ch].attack_cnt = inst->attack * BITRATE / 1000;
	schedule_chs[ch].decay_cnt = inst->decay * BITRATE / 1000;
	schedule_chs[ch].sustain = inst->sustain;
	schedule_chs[ch].release_cnt = inst->release * BITRATE / 1000;

	schedule_chs[ch].playing = true;
	schedule_chs[ch].start_at = get_sampletime_correction();
	schedule_chs[ch].sample_pos = 0;
	schedule_chs[ch].pos_increment = pitch * 256;
	schedule_chs[ch].counter = 0;
	schedule_chs[ch].counter_released = 0;
}

void sound_off(uint8_t ch) {
	if (ch >= CHANNELS) return;

	if (sound_chs[ch].playing && sound_chs[ch].counter_released == 0) {
		sound_chs[ch].counter_released = sound_chs[ch].counter;
	}
}

void sound_stop(uint8_t ch) {
	if (ch >= CHANNELS) return;

	sound_chs[ch].playing = false;
}