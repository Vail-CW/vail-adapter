/*
 * I2S Audio Library for MAX98357A Class-D Amplifier
 * Replaces PWM buzzer with high-quality audio output
 * Includes software volume control (0-100%)
 */

#ifndef I2S_AUDIO_H
#define I2S_AUDIO_H

#include <driver/i2s.h>
#include <driver/gpio.h>
#include <math.h>
#include <Preferences.h>
#include "../core/config.h"

// I2S port number
#define I2S_NUM I2S_NUM_0

// Forward declarations
void continueTone(int frequency);

// Global audio state
static bool i2s_initialized = false;
static bool tone_playing = false;
static unsigned long tone_start_time = 0;
static unsigned long tone_duration = 0;
static float phase = 0.0;  // Phase accumulator for continuous tone
static int current_frequency = 0;
static int audio_volume = DEFAULT_VOLUME;  // Volume 0-100%
static Preferences volumePrefs;

/*
 * Load volume from preferences
 */
void loadVolume() {
  volumePrefs.begin("audio", false);
  audio_volume = volumePrefs.getInt("volume", DEFAULT_VOLUME);
  if (audio_volume < VOLUME_MIN || audio_volume > VOLUME_MAX) {
    audio_volume = DEFAULT_VOLUME;
  }
  volumePrefs.end();
  Serial.printf("Loaded volume: %d%%\n", audio_volume);
}

/*
 * Save volume to preferences
 */
void saveVolume() {
  volumePrefs.begin("audio", false);
  volumePrefs.putInt("volume", audio_volume);
  volumePrefs.end();
  Serial.printf("Saved volume: %d%%\n", audio_volume);
}

/*
 * Set volume (0-100%)
 */
void setVolume(int vol) {
  audio_volume = constrain(vol, VOLUME_MIN, VOLUME_MAX);
  saveVolume();
}

/*
 * Get current volume
 */
int getVolume() {
  return audio_volume;
}

/*
 * Initialize I2S interface for MAX98357A amplifier
 */
void initI2SAudio() {
  if (i2s_initialized) {
    return;
  }

  // Load saved volume
  loadVolume();

  // I2S configuration for ESP32-S3 with MAX98357A
  // CRITICAL: Match the working test sketch exactly
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = I2S_SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL3,  // Highest priority - must beat SPI DMA
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false,
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
  };

  Serial.println("Configuring I2S for ESP32-S3 with MAX98357A...");

  // Pin configuration - MAX98357A needs BCK, LRC, and DIN
  // GAIN pin controls hardware gain (leave floating for 9dB default)
  // SD (shutdown) pin: leave floating for always-on
  i2s_pin_config_t pin_config = {
    .mck_io_num = I2S_PIN_NO_CHANGE,  // No MCLK needed for MAX98357A
    .bck_io_num = I2S_BCK_PIN,
    .ws_io_num = I2S_LCK_PIN,
    .data_out_num = I2S_DATA_PIN,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  Serial.printf("Pin config: BCLK=%d, LRC=%d, DIN=%d\n",
                pin_config.bck_io_num,
                pin_config.ws_io_num,
                pin_config.data_out_num);

  // Install and start I2S driver
  esp_err_t err = i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
  if (err != ESP_OK) {
    Serial.printf("Failed to install I2S driver: %d\n", err);
    return;
  }

  err = i2s_set_pin(I2S_NUM, &pin_config);
  if (err != ESP_OK) {
    Serial.printf("Failed to set I2S pins: %d\n", err);
    i2s_driver_uninstall(I2S_NUM);
    return;
  }

  // Set GPIO drive strength to maximum for reliable I2S signals
  gpio_set_drive_capability((gpio_num_t)I2S_BCK_PIN, GPIO_DRIVE_CAP_3);
  gpio_set_drive_capability((gpio_num_t)I2S_LCK_PIN, GPIO_DRIVE_CAP_3);
  gpio_set_drive_capability((gpio_num_t)I2S_DATA_PIN, GPIO_DRIVE_CAP_3);
  Serial.println("Set I2S GPIO drive strength to maximum");

  // Clear the DMA buffers
  i2s_zero_dma_buffer(I2S_NUM);

  i2s_initialized = true;
  Serial.println("I2S Audio initialized successfully");
  Serial.printf("  BCK: GPIO %d\n", I2S_BCK_PIN);
  Serial.printf("  LCK: GPIO %d\n", I2S_LCK_PIN);
  Serial.printf("  DATA: GPIO %d\n", I2S_DATA_PIN);
  Serial.printf("  Sample Rate: %d Hz\n", I2S_SAMPLE_RATE);
}

/*
 * Generate and play a tone at specified frequency for specified duration
 * Non-blocking - call updateTone() in loop to handle timing
 */
void playTone(int frequency, int duration_ms) {
  if (!i2s_initialized) {
    Serial.println("ERROR: I2S not initialized in playTone!");
    return;
  }

  Serial.printf("playTone(%d Hz, %d ms) - generating samples\n", frequency, duration_ms);

  tone_playing = true;
  tone_start_time = millis();
  tone_duration = duration_ms;

  // Reset phase for clean start
  float local_phase = 0.0;
  float phase_increment = 2.0 * PI * frequency / I2S_SAMPLE_RATE;

  int16_t sample_buffer[I2S_BUFFER_SIZE];

  // Write samples to I2S - keep writing while tone should be playing
  size_t bytes_written;
  unsigned long samples_to_write = (unsigned long)I2S_SAMPLE_RATE * duration_ms / 1000;
  unsigned long samples_written = 0;

  Serial.printf("Will write %lu samples\n", samples_to_write);

  while (samples_written < samples_to_write && tone_playing) {
    // Calculate volume scaling (0-100% maps to 0.0-1.0)
    float volume_scale = audio_volume / 100.0;

    // Generate sine wave samples using phase accumulator with volume control
    for (int i = 0; i < I2S_BUFFER_SIZE / 2; i++) {
      // Apply volume scaling (0-100%)
      float volume_scale = audio_volume / 100.0;
      int16_t sample = (int16_t)(sin(local_phase) * 8000.0 * volume_scale);

      // Stereo output: send same signal to both channels
      sample_buffer[i * 2] = sample;       // Left
      sample_buffer[i * 2 + 1] = sample;   // Right

      // Increment phase
      local_phase += phase_increment;
      if (local_phase >= 2.0 * PI) {
        local_phase -= 2.0 * PI;
      }
    }

    esp_err_t result = i2s_write(I2S_NUM, sample_buffer, I2S_BUFFER_SIZE * sizeof(int16_t), &bytes_written, portMAX_DELAY);
    if (result != ESP_OK) {
      Serial.printf("I2S write error: %d\n", result);
    }
    samples_written += I2S_BUFFER_SIZE / 2;

    // Allow other tasks to run
    yield();
  }

  Serial.printf("Wrote %lu samples total\n", samples_written);

  // Silence at end
  memset(sample_buffer, 0, sizeof(sample_buffer));
  i2s_write(I2S_NUM, sample_buffer, I2S_BUFFER_SIZE * sizeof(int16_t), &bytes_written, portMAX_DELAY);

  tone_playing = false;
}

/*
 * Start playing a continuous tone at specified frequency
 * Use for morse code where you control start/stop timing
 */
void startTone(int frequency) {
  if (!i2s_initialized) {
    Serial.println("ERROR: I2S not initialized in startTone!");
    return;
  }

  if (!tone_playing || current_frequency != frequency) {
    phase = 0.0;  // Reset phase when starting new tone or changing frequency
    current_frequency = frequency;
    Serial.printf("Starting tone: %d Hz\n", frequency);
  }

  tone_playing = true;

  // Immediately fill the I2S buffer to start playback
  // This prevents the clicking issue by ensuring continuous data flow
  continueTone(frequency);
}

/*
 * Continue playing the current tone
 * Call this repeatedly in loop while tone should continue
 */
void continueTone(int frequency) {
  if (!i2s_initialized || !tone_playing) {
    return;
  }

  // Update frequency if changed
  if (current_frequency != frequency) {
    current_frequency = frequency;
  }

  int16_t sample_buffer[I2S_BUFFER_SIZE];
  float phase_increment = 2.0 * PI * current_frequency / I2S_SAMPLE_RATE;

  // Generate continuous sine wave using phase accumulator with volume control
  for (int i = 0; i < I2S_BUFFER_SIZE / 2; i++) {
    // Apply volume scaling (0-100%)
    float volume_scale = audio_volume / 100.0;
    int16_t sample = (int16_t)(sin(phase) * 8000.0 * volume_scale);

    sample_buffer[i * 2] = sample;       // Left
    sample_buffer[i * 2 + 1] = sample;   // Right

    // Increment phase and wrap around
    phase += phase_increment;
    if (phase >= 2.0 * PI) {
      phase -= 2.0 * PI;
    }
  }

  // Write samples - MUST block to ensure continuous playback
  size_t bytes_written;
  i2s_write(I2S_NUM, sample_buffer, I2S_BUFFER_SIZE * sizeof(int16_t), &bytes_written, portMAX_DELAY);
}

/*
 * Stop the currently playing tone
 */
void stopTone() {
  if (!i2s_initialized) {
    return;
  }

  tone_playing = false;
  phase = 0.0;  // Reset phase
  current_frequency = 0;

  // Write silence to clear the buffer
  int16_t silence[I2S_BUFFER_SIZE] = {0};
  size_t bytes_written;
  i2s_write(I2S_NUM, silence, I2S_BUFFER_SIZE * sizeof(int16_t), &bytes_written, 10);
  i2s_zero_dma_buffer(I2S_NUM);
}

/*
 * Completely deinitialize I2S driver to free DMA memory
 * Use this before OTA updates or deep sleep to reclaim memory
 */
void deinitI2SAudio() {
  if (!i2s_initialized) {
    return;
  }

  Serial.println("Deinitializing I2S audio...");

  // Stop any playing tone first
  stopTone();

  // Uninstall I2S driver (frees DMA buffers)
  esp_err_t err = i2s_driver_uninstall(I2S_NUM);
  if (err != ESP_OK) {
    Serial.printf("Failed to uninstall I2S driver: %d\n", err);
  } else {
    Serial.println("I2S driver uninstalled successfully");
  }

  i2s_initialized = false;
}

/*
 * Check if a tone is currently playing
 */
bool isTonePlaying() {
  return tone_playing;
}

/*
 * Blocking beep function for UI feedback
 * Compatible with existing code
 */
void beep(int frequency, int duration) {
  Serial.printf("beep(%d Hz, %d ms)\n", frequency, duration);
  playTone(frequency, duration);
  delay(duration + 10); // Small gap after beep
}

#endif // I2S_AUDIO_H
