#pragma once

#define BPS 460800

// Symlink for device name, assigned by udev rule:
#define UDEV_DEVICE "/dev/memsim2"

// Fallback if UDEV_DEVICE cannot be found:
#if defined(__linux__)
#define DEFAULT_DEVICE "/dev/ttyUSB0"
#else
#if defined(__FreeBSD__)
#define DEFAULT_DEVICE "/dev/cuaU0"
#else
#if defined(__APPLE__) && defined(__MACH__)
#define DEFAULT_DEVICE  "/dev/cu.usbserial-1"
#else
#error Please define DEFAULT_DEVICE for your operating system!
#endif
#endif
#endif

#define SIMMEMSIZE (512 * 1024)

extern bool offset_given;
extern bool mem_type_given;

void skip_white(FILE *file);
void ignore_rest_of_line(FILE *file);

int get_hex(FILE *file);                            // nibble
int get_hex2(FILE *file, int *check);               // 8 bit
int get_hex4(FILE *file, int *check);               // 16 bit
int get_hex6(FILE *file, int *check);               // 24 bit
long long int get_hex8(FILE *file, int *check);     // 32 bit


int parse_ihex(FILE *file, uint8_t *buffer, int *min, int *max, long offset);
int parse_srec(FILE *file, uint8_t *buffer, int *min, int *max, long offset);
