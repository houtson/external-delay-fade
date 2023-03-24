/* Audio Library for Teensy 3.X
 * Copyright (c) 2014, Paul Stoffregen, paul@pjrc.com
 *
 * Development of this audio library was funded by PJRC.COM, LLC by sales of
 * Teensy and Audio Adaptor boards.  Please support PJRC's efforts to develop
 * open source software by purchasing Teensy or other PJRC products.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice, development funding notice, and this permission
 * notice shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * 
 * 
 *  03/2023 PMF - added method to fade from current delay time to a new desired delay time with a linear fade crossfade
 * 
 */

#ifndef effect_delay_fade_ext_h_
#define effect_delay_fade_ext_h_
#include <effect_delay_ext.h>

#include "Arduino.h"
#include "AudioStream.h"
#include "spi_interrupt.h"

class AudioEffectDelayFadeExternal : public AudioStream {
 public:
  AudioEffectDelayFadeExternal() : AudioStream(1, inputQueueArray) {
    initialize(AUDIO_MEMORY_23LC1024, 65536);
  }
  AudioEffectDelayFadeExternal(AudioEffectDelayMemoryType_t type,
                               float milliseconds = 1e6)
      : AudioStream(1, inputQueueArray) {
    uint32_t n = (milliseconds * (AUDIO_SAMPLE_RATE_EXACT / 1000.0f)) + 0.5f;
    initialize(type, n);
  }

  void delay(uint8_t channel, float milliseconds) {
    if (channel >= 8 || memory_type >= AUDIO_MEMORY_UNDEFINED) return;
    if (milliseconds < 0.0f) milliseconds = 0.0f;
    uint32_t n = (milliseconds * (AUDIO_SAMPLE_RATE_EXACT / 1000.0f)) + 0.5f;
    n += AUDIO_BLOCK_SAMPLES;
    if (n > memory_length - AUDIO_BLOCK_SAMPLES)
      n = memory_length - AUDIO_BLOCK_SAMPLES;
    delay_length[channel] = n;
    uint8_t mask = activemask;
    if (activemask == 0) AudioStartUsingSPI();
    activemask = mask | (1 << channel);
  }

  // fade to new desired delay time over fade_millis with a straight linear fade
  void delay(uint8_t channel, float milliseconds, float fade_millis) {
    if (channel >= 8 || memory_type >= AUDIO_MEMORY_UNDEFINED) return;
    if (milliseconds < 0.0f) milliseconds = 0.0f;
    if (fade_millis < 0.0f) fade_millis = 0.0f;
    uint32_t n = (milliseconds * (AUDIO_SAMPLE_RATE_EXACT / 1000.0f)) + 0.5f;
    n += AUDIO_BLOCK_SAMPLES;
    if (n > memory_length - AUDIO_BLOCK_SAMPLES)
      n = memory_length - AUDIO_BLOCK_SAMPLES;
    fade_increment[channel] = 1.0f / millisToSamples(fade_millis);
    uint8_t mask = activemask;
    uint8_t fmask = fade_mask;
    if (activemask == 0) {
      AudioStartUsingSPI();
      delay_length[channel] = n;
      activemask = mask | (1 << channel);
    } else {
      fade_mask = fmask | (1 << channel);
      desired_length[channel] = n;
      fade_multiplier[channel] = 1.0f;
    }
  }

  void disable(uint8_t channel) {
    if (channel >= 8) return;
    uint8_t mask = activemask & ~(1 << channel);
    activemask = mask;
    if (mask == 0) AudioStopUsingSPI();
  }
  virtual void update(void);

 private:
  void initialize(AudioEffectDelayMemoryType_t type, uint32_t samples);
  void read(uint32_t address, uint32_t count, int16_t *data);
  void write(uint32_t address, uint32_t count, const int16_t *data);
  void zero(uint32_t address, uint32_t count) { write(address, count, NULL); }
  uint32_t memory_begin;   // the first address in the memory we're using
  uint32_t memory_length;  // the amount of memory we're using
  uint32_t head_offset;    // head index (incoming) data into external memory
  uint32_t
      delay_length[8];  // # of sample delay for each channel (128 = no delay)
  uint32_t desired_length[8];  // desired # of sample delay for each channel
  uint8_t activemask;          // which output channels are active
  uint8_t fade_mask = 0;       // which output channels are to be faded
  float fade_increment[8] = {0.0f};   // increment for each change in fade
  float fade_multiplier[8] = {0.0f};  // multiplier for the fade
  uint8_t memory_type;                // 0=23LC1024, 1=Frank's Memoryboard
  static uint32_t allocated[2];
  audio_block_t *inputQueueArray[1];
  float millisToSamples(float milliseconds) {
    return milliseconds * (AUDIO_SAMPLE_RATE_EXACT / 1000.0f) + 0.5f;
  };  // convert milliseconds to number of samples at sample rate
};

#endif
