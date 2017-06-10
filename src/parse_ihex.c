#include <ctype.h>
#include <limits.h>

#include "parse_ihex.h"

static void
skip_white(FILE *file)
{
  int ch;
  while((ch = getc(file)) != EOF && isspace(ch));
  ungetc(ch, file);
}

static int
get_hex(FILE *file)
{
  int ch;
  ch = getc(file);
  if (ch >= '0' && ch <= '9') return ch - '0';
  if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
  if (ch >= 'a' && ch <= 'a') return ch - 'a' + 10;
  ungetc(ch, file);
  return -1;
}

static int
get_hex2(FILE *file, int *check)
{
  int v1, v2;
  v1 = get_hex(file);
  if (v1 < 0) return v1;
  v2 = get_hex(file);
  if (v2 < 0) return v2;
  v2 += v1*16;
  *check += v2;
  return v2;
}

static int
get_hex4(FILE *file, int *check)
{
  int v1, v2;
  v1 = get_hex2(file, check);
  if (v1 < 0) return v1;
  v2 = get_hex2(file, check);
  if (v2 < 0) return v2;
  return v1*256 + v2;
}

int
parse_ihex(FILE *file, uint8_t *buffer, int size, int *min, int *max)
{
  int ch;
  *max = 0;
  *min = INT_MAX;
  int actual_size = 0;

  while(1) {
    int check;
    int b;
    int length;
    int addr;
    int type;
    skip_white(file);
    if (feof(file)) break;
    ch = getc(file);
    if (ch != ':') return -1;
    check = 0;
    length = get_hex2(file, &check);
    if (length < 0) return length;
    addr = get_hex4(file, &check);
    if (addr < 0) return addr;
    type = get_hex2(file, &check);
    if (type < 0) return type;
    if (type == 0) {
      if (addr >= size) return -2;
      for (b = 0; b < length; b++) {
	int v = get_hex2(file, &check);
	if (v < 0) return v;
	buffer[addr] = v;
	if (addr > *max) *max = addr;
	if (addr < *min) *min = addr;
        actual_size = *max - *min + 1;
        if (actual_size > size) {
          fprintf(stderr, "%sData exceeds %d bytes\n", errmsg, size);
          return -5;
        }
	addr++;
      }

    } else if (type == 1) {
      return 0;
    }
    ch = get_hex2(file, &check);
    if (ch < 0) return ch;
    if ((check & 0xff) != 0) return -3;
  }
  printf("Info: Intel hex data from %04Xh - %04Xh = %d bytes\n", *min, *max, actual_size);
  return actual_size;
}
