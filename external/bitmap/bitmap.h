#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define UNIT char
#define BITS_PER_UNIT 8

class BitmapFilter {

private:
  uint64_t size;
  char *bitmap;
  uint64_t offset;

  uint16_t bit_index(uint64_t index) {
    return (index - offset) % BITS_PER_UNIT;
  }
  uint64_t unit_index(uint64_t index) {
    return (index - offset) / BITS_PER_UNIT;
  }

  void init() {
    char a = 0xff;
    for (uint64_t i = 0; i < (size - 1) / BITS_PER_UNIT + 1; i++) {
      bitmap[i] |= a;
    }
  }

public:
  BitmapFilter(uint64_t _size, uint64_t _offset = 0) {
    size = _size;
    offset = _offset;
    uint64_t needed_units = (size - 1) / (BITS_PER_UNIT) + 1;
    bitmap = (char *)malloc(needed_units);
    if (bitmap == NULL) {
      printf("malloc error");
      exit(0);
    }
    init();
  }

  ~BitmapFilter() { delete (bitmap); }

  void set(uint64_t index) { // takes an index, sets the bit
    unsigned int a = 1;
    a <<= bit_index(index);
    bitmap[unit_index(index)] |= a;
  }

  void unset(uint64_t index) { // takes an index, unsets the bit
    unsigned int a = 1;
    a <<= bit_index(index);
    bitmap[unit_index(index)] &= (!a);
  }

  bool isset(uint64_t index) { // check if a specific bit is set
    unsigned int a = 1;
    a <<= bit_index(index);
    if ((bitmap[unit_index(index)] & a) == 0)
      return false;
    else
      return true;
  }
};