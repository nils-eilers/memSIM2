#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>

#include "parse_ihex.h"
#include "memsim2.h"

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


static const char* errmsg = "Error in Intel hex file: ";

int
parse_ihex(FILE *file, uint8_t *buffer, int *min, int *max, long offset)
{
  int ch;
  *max = 0;
  *min = INT_MAX;
  int actual_size = 0;
  int bytes_ignored = 0;

  while(1) {
    int check;
    int b;
    int length;
    int addr;
    int type;
    skip_white(file);
    if (feof(file)) break;
    ch = getc(file);
    if (ch != ':') {
      fprintf(stderr, "%s':' expected\n", errmsg);
      return -1;
    }
    check = 0;
    length = get_hex2(file, &check);
    if (length < 0) {
      fprintf(stderr, "%sillegal character in length field\n", errmsg);
      return length;
    }
    addr = get_hex4(file, &check);
    if (addr < 0) {
      fprintf(stderr, "%sillegal character in address field\n", errmsg);
      return addr;
    }
    type = get_hex2(file, &check);
    if (type < 0) {
      fprintf(stderr, "%sillegal character in type field\n", errmsg);
      return type;
    }
    if (type == 0) {
      for (b = 0; b < length; b++) {
	int v = get_hex2(file, &check);
	if (v < 0) {
          fprintf(stderr, "%sillegal character in data field\n", errmsg);
          return v;
        }
	if ((addr - offset) >= 0)
          buffer[addr - offset] = v;
        else
          bytes_ignored++;
	if (addr > *max) *max = addr;
	if (addr < *min) *min = addr;
        actual_size = *max - *min + 1;
        addr++;
      }

    } else if (type == 1) {
      break;
    } else {
      fprintf(stderr, "Error: Intel hex file with unsupported type %d entry\n", type);
      return -4;
    }
    ch = get_hex2(file, &check);
    if (ch < 0) {
      fprintf(stderr, "%sillegal character in checksum field\n", errmsg);
      return ch;
    }
    if ((check & 0xFF) != 0) {
      fprintf(stderr, "%sWrong checksum\n", errmsg);
      return -3;
    }
  }
  printf("Info: Intel hex data from %04Xh - %04Xh = %d bytes\n", *min, *max, actual_size);
  if (bytes_ignored) printf("Info: %d bytes outside storage area ignored\n", bytes_ignored);
  if (!offset_given) {
    printf("Info: no offset specified, simulated data starts at %Xh\n", *min);
    memmove(buffer, buffer + *min, actual_size);
    memset(buffer + actual_size, 0, SIMMEMSIZE - actual_size);
  }
  return actual_size;
}
