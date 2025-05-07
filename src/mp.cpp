#include <mpg123.h>

#include <iostream>
#include <stdexcept>
#include <vector>

void decodeMP3(const char* filename) {
  int err = MPG123_OK;
  mpg123_handle* mh = nullptr;

  mpg123_init();
  mh = mpg123_new(nullptr, &err);
  if (mh == nullptr) {
    throw std::runtime_error("Unable to create mpg123 handle");
  }

  if (mpg123_open(mh, filename) != MPG123_OK) {
    mpg123_delete(mh);
    throw std::runtime_error("Unable to open MP3 file");
  }

  long rate;
  int channels, encoding;
  if (mpg123_getformat(mh, &rate, &channels, &encoding) != MPG123_OK) {
    mpg123_close(mh);
    mpg123_delete(mh);
    throw std::runtime_error("Failed to get audio format");
  }

  size_t buffer_size = mpg123_outblock(mh);
  std::vector<unsigned char> buffer(buffer_size);
  size_t done = 0;

  while (mpg123_read(mh, buffer.data(), buffer_size, &done) == MPG123_OK) {
    // Here 'buffer' contains 'done' bytes of decoded audio (PCM samples)
    // Process the PCM data here, e.g.:
    std::cout << "Decoded " << done << " bytes of audio data." << std::endl;
  }

  mpg123_close(mh);
  mpg123_delete(mh);
  mpg123_exit();
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: ./mp3processor <filename.mp3>" << std::endl;
    return -1;
  }

  try {
    decodeMP3(argv[1]);
  } catch (const std::exception& ex) {
    std::cerr << "Error: " << ex.what() << std::endl;
    return -1;
  }
  return 0;
}
