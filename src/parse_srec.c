#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "memsim2.h"
#include "parse_srec.h"

static const char* errmsg = "Error in S-Record file: ";

int
parse_srec(FILE *file, uint8_t *buffer, int *min, int *max, long offset)
{
   int ch;
   *max = 0;
   *min = INT_MAX;
   int actual_size = 0;
   int bytes_ignored = 0;
   long records = 0;
   char header[255];
   int v;
   int i;
   char expected_termination = 0;
   int type;
   int count;
   int addr;
   int check;
   int expected_number_of_records = -1;

   skip_white(file);
   while (!feof(file))
   {

      ch = getc(file);
      if (ch != 'S')
      {
         // Lines that doesn't start with 'S' are silently treated as comments
         ignore_rest_of_line(file);
         continue;
      }
      type = getc(file);
      if ((type < '0' ) || (type > '9') || (type == '4'))
      {
         fprintf(stderr, "%sillegal record type '%c'\n", errmsg, isprint(type) ? type : '?');
         return -1;
      }
      check = 0;
      count = get_hex2(file, &check);
      if (count < 0)
      {
         fprintf(stderr, "%sillegal character in count field\n", errmsg);
         return count;
      }
      switch(type)
      {
         case '2':        // S2 24 bit address
         case '6':        // S6 24 bit record count
         case '8':        // S8 24 bit start address (termination)
            v = get_hex6(file, &check);
            count -= 3;
            break;
         case '3':       // S3 32 bit address
         case '7':       // S7 32 bit start address (termination)
            v = get_hex8(file, &check);
            count -= 4;
            break;
         default:
            v = get_hex4(file, &check);
            count -= 2;
      }
      if (v < 0)
      {
         fprintf(stderr, "%sillegal character in address field\n", errmsg);
         return v;
      }

      switch (type)
      {
         case '0':
            memset(header, 0, sizeof(header));
            for (i = 0; i < count - 1 ; i++)
            {
               v = get_hex2(file, &check);
               if (v < 0)
               {
                  fprintf(stderr, "%s: illegal character in S0 data field\n", errmsg);
                  return v;
               }
               header[i] = v;
            }
            printf("S0 header: \"%s\"\n", header);
            break;
         case '1':         // S1 data with 16 bit address
         case '2':         // S2 data with 24 bit address
         case '3':         // S3 data with 32 bit address
            records++;
            addr = v;
            expected_termination = '0' + (9 - (type - '0') + 1);
            // printf("S%c --> S%c\n", type, expected_termination);
            for (i = 0; i < count - 1; i++)
            {
               v = get_hex2(file, &check);
               if (v < 0)
               {
                  fprintf(stderr, "%s: illegal character in S%c data field\n", errmsg, type);
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
            break;
         case '5':         // S5 16 bit record counter
         case '6':         // S6 24 bit record counter
            expected_number_of_records = v;
            break;
         default:
            // everything else handled earlier, so this is S7, S8 or S9
            printf("Info: S%c start address 0x%X\n", type, v);
            if (type != expected_termination)
            {
               fprintf(stderr, "Warning: expected S%c but found S%c\n", expected_termination, type);
            }
            if (records != expected_number_of_records)
            {
               fprintf(stderr, "%s: file claims %d records but contains %ld records instead\n", errmsg, expected_number_of_records, records);
               return -1;
            }

      }

      // Verify checksum
      ch = get_hex2(file, NULL);
      if (ch < 0)
      {
         fprintf(stderr, "%sillegal character in checksum field\n", errmsg);
         return ch;
      }
      check = ~check;
      check = check & 0xFF;
      if (check  != ch)
      {
         fprintf(stderr, "%sWrong checksum\n", errmsg);
         return -3;
      }

      // Ignore everything after checksum, may contain comments
      ignore_rest_of_line(file);
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
