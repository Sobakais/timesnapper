#include <cmath>
#include <cstddef>
#include <iostream>
#include <miniaudio/miniaudio.c>

const int SAMPLE_RATE = 44100;
const int AUDIO_CHANNELS = 2;

int main(int argc, char** argv) {
  if (argc < 2) {
    printf("No input file.\n");
    return -1;
  }
  if (argc > 2) {
    printf("Too many arguments provided.\n");
    return -1;
  }
  const char* filepath = argv[1];

  ma_result result;

  ma_engine engine;
  ma_engine_config engineConfig = ma_engine_config_init();
  engineConfig.noDevice = MA_TRUE;
  engineConfig.channels = AUDIO_CHANNELS;
  engineConfig.sampleRate = SAMPLE_RATE;

  result = ma_engine_init(&engineConfig, &engine);
  if (result != MA_SUCCESS) {
    std::cerr << "Could not initialize engine\n";
    return result;
  }

  ma_sound sound;
  result = ma_sound_init_from_file(&engine, filepath, MA_SOUND_FLAG_DECODE,
                                   NULL, NULL, &sound);
  if (result != MA_SUCCESS) {
    std::cerr << "Could not initialize sound\n";
    return result;
  }

  ma_channel_converter converter;
  ma_channel_converter_config config =
      ma_channel_converter_config_init(ma_format_f32, AUDIO_CHANNELS, NULL, 1,
                                       NULL, ma_channel_mix_mode_default);

  result = ma_channel_converter_init(&config, NULL, &converter);
  if (result != MA_SUCCESS) {
    std::cerr << "Could not initialize converter\n";
  }

  ma_uint64 length;
  ma_sound_get_length_in_pcm_frames(&sound, &length);

  result =
      ma_channel_converter_process_pcm_frames(&converter, NULL, &sound, length);
  if (result != MA_SUCCESS) {
    std::cerr << "Error during channels conversion\n";
    return result;
  }

  return 0;
}
