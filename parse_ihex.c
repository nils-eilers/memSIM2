#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "memsim2.h"

static const char* errmsg = "Error in Intel hex file: ";

int
parse_ihex(FILE *file, uint8_t *buffer, int *min, int *max, long offset)
{
   int ch;
   *max = 0;
   *min = INT_MAX;
   int actual_size = 0;
   int bytes_ignored = 0;
   long long upper16 = 0;
   long long segment = 0;

   while (1)
   {
      int check;
      int b;
      int length;
      int addr;
      int type;

      skip_white(file);
      if (feof(file)) break;
      ch = getc(file);
      if (ch != ':')
      {
         fprintf(stderr, "%s':' expected\n", errmsg);
         return -1;
      }
      check = 0;
      length = get_hex2(file, &check);
      if (length < 0)
      {
         fprintf(stderr, "%sillegal character in length field\n", errmsg);
         return length;
      }
      addr = get_hex4(file, &check);
      if (addr < 0)
      {
         fprintf(stderr, "%sillegal character in address field\n", errmsg);
         return addr;
      }
      addr += segment;
      addr += upper16;
      type = get_hex2(file, &check);
      if (type < 0)
      {
         fprintf(stderr, "%sillegal character in type field\n", errmsg);
         return type;
      }
      if (type == 0)
      {
         for (b = 0; b < length; b++)
         {
            int v = get_hex2(file, &check);
            if (v < 0)
            {
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
      }
      else if (type == 2)
      {
         // Type 2 Extended Segment address
         if (length != 2)
         {
            fprintf(stderr, "Error: byte count 0x02 expected for Extended Segment Address field\n");
            return -1;
         }
         segment = get_hex4(file, &check);
         if (segment < 0)
         {
            fprintf(stderr, "%sillegal character in Extended Segment Address field\n", errmsg);
            return segment;
         }
         segment = segment * 16;
         upper16 = 0;
      }
      else if (type == 4)
      {
         // Type 4 Extended linear address
         if (length != 2)
         {
            fprintf(stderr, "Error: byte count 0x02 expected for Extended Linear Address field\n");
            return -1;
         }
         upper16 = get_hex4(file, &check);
         if (upper16 < 0)
         {
            fprintf(stderr, "%sillegal character in Extended Linear Address field\n", errmsg);
            return upper16;
         }
         upper16 = upper16 << 16;
         segment = 0;
      }
      else if (type == 3)
      {
         // Type 3 Start Segment Address
         if (length != 4)
         {
            fprintf(stderr, "Error: byte count 0x04 expected for Start Segment Address\n");
            return -1;
         }
         long long int start = get_hex8(file, &check);
         printf("Info: ignoring Start Segment address (CS:IP) %04llX\n", start);
      }
      else if (type == 5)
      {
         // Type 5 Start Linear Address
         if (length != 4)
         {
            fprintf(stderr, "Error: byte count 0x04 expected for Start Linear Address\n");
            return -1;
         }
         long long int start = get_hex8(file, &check);
         printf("Info: ignoring Start Linear address (EIP) %04llX\n", start);
      }
      else if (type == 1)
      {
         break;
      }
      else
      {
         fprintf(stderr, "Error: Intel hex file with unsupported type %d entry\n", type);
         return -4;
      }
      ch = get_hex2(file, &check);
      if (ch < 0)
      {
         fprintf(stderr, "%sillegal character in checksum field\n", errmsg);
         return ch;
      }
      if ((check & 0xFF) != 0)
      {
         fprintf(stderr, "%sWrong checksum\n", errmsg);
         return -3;
      }
   }
   printf("Info: Intel hex data from %04Xh - %04Xh = %d bytes\n", *min, *max, actual_size);
   if (bytes_ignored) printf("Info: %d bytes outside storage area ignored\n", bytes_ignored);
   if (!offset_given)
   {
      printf("Info: no offset specified, simulated data starts at %Xh\n", *min);
      memmove(buffer, buffer + *min, actual_size);
      memset(buffer + actual_size, 0, SIMMEMSIZE - actual_size);
   }
   return actual_size;
}
