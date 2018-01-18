#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <asm/termios.h>
#include <asm/ioctls.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <poll.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>
#include <ctype.h>

#include "parse_ihex.h"
#include "parse_srec.h"
#include "memsim2.h"

#ifndef BOTHER
#define    BOTHER CBAUDEX
#endif

#if (INT_MAX < 2147483647UL)
#error This code assumes int of at least 32 bit width
#endif

// Global variables
bool mem_type_given = false;
bool offset_given = false;
static uint8_t mem[SIMMEMSIZE];

#define MEM_TYPE_INDEX          2
#define RESET_ENABLE_INDEX      3
#define RESET_TIME_INDEX        4
#define EMU_ENA_INDEX           7
#define SELFTEST_INDEX          8
#define CHKSUM_INDEX           12

struct MemType
{
   const char *name;
   char cmd;
   int size;
};

const struct MemType memory_types[] =
{
   { "2764",  '0',   8 * 1024 },
   { "27128", '1',  16 * 1024 },
   { "27256", '2',  32 * 1024 },
   { "27512", '3',  64 * 1024 },
   { "27010", '4', 128 * 1024 },
   { "27020", '5', 256 * 1024 },
   { "27040", '6', 512 * 1024 }
};


extern int ioctl(int d, unsigned long request, ...);

static int
serial_open(const char *device)
{
   struct termios2 settings;
   int fd;
   fd = open(device, O_RDWR);
   if (fd < 0)
   {
      perror(device);
      return -1;
   }
   if (ioctl(fd, TCGETS2, &settings) < 0)
   {
      perror("ioctl TCGETS2 failed");
      close(fd);
      return -1;
   }
   settings.c_iflag &= ~(IGNBRK | BRKINT | IGNPAR | INPCK | ISTRIP
         | INLCR | IGNCR | ICRNL | IXON | PARMRK);
   settings.c_iflag |= IGNBRK | IGNPAR;
   settings.c_oflag &= ~OPOST;
   settings.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
   settings.c_cflag &= ~(CSIZE | PARODD | CBAUD | PARENB);
   settings.c_cflag |= CS8 | BOTHER | CREAD;
   settings.c_ispeed = BPS;
   settings.c_ospeed = BPS;
   if (ioctl(fd, TCSETS2, &settings) < 0)
   {
      perror("ioctl TCSETS2 failed");
      close(fd);
      return -1;
   }
   return fd;
}

static void
usage(void)
{
   fputs("Usage: [OPTION].. FILE\n"
         "Upload image file to memSIM2 EPROM emulator\n\n"
         "Options:\n"
         "\t-d DEVICE     Serial device, defaults to " DEFAULT_DEVICE "\n"
         "\t-m MEMTYPE    Memory type (2764,27128,27256,27512,27010,27020,27040)\n"
         "\t-r RESETTIME  Time of reset pulse in milliseconds.\n"
         "\t              > 0 for positive pulse, < 0 for negative pulse\n"
         "\t-e            Enable emulation\n"
         "\t-o BYTES      Specify an offset value with different meaning for:\n"
         "\t   binary files: skip first n bytes of file\n"
         "\t   Hex files: start address in memory map of simulated memory chip\n"
         "\t-h            This help\n\n"
         "Numbers prefixed by '0x' are interpreted as hexadecimal numbers,\n"
         "octal for numbers beginning with '0' and decimal for everything else.\n",
         stderr);

}


static int
read_binary(FILE *file, uint8_t *mem, int file_offset)
{
   int res;
   unsigned long addr = 0;
   int mem_size = SIMMEMSIZE;
   fseek(file, 0L, SEEK_END);
   long detected_binary_size = ftell(file);

   if (detected_binary_size > SIMMEMSIZE)
   {
      fprintf(stderr, "Error: file too large\n");
      return -1;
   }
   if (file_offset > 0)
   {
      res = fseek(file, file_offset, SEEK_SET);
      if (res < 0)
      {
         perror("Error: Failed to seek to file_offset in binary file");
         return -1;
      }
   }
   else
   {
      rewind(file);
      addr = -file_offset;
   }
   if (addr >= SIMMEMSIZE)
   {
      fprintf(stderr,"Error: Offset outside memory");
      return -1;
   }
   mem_size -= addr;
   res = fread(mem + addr, sizeof(uint8_t), mem_size, file);
   if (res < 0)
   {
      perror("Error: Failed to read from binary file");
      return -1;
   }

   return detected_binary_size;
}

static int
read_image(const char *filename, uint8_t *mem, int offset, int *min, int *max)
{
   int detected_binary_size;
   char *suffix;
   FILE *file = fopen(filename, "rb");

   if (!file)
   {
      fprintf(stderr, "Error: Failed to open file '%s': %s\n",
            filename, strerror(errno));
      return -1;
   }
   suffix = rindex(filename,'.');
   if (!suffix)
   {
      fprintf(stderr, "Error: Filename has no suffix\n");
      fclose(file);
      return -1;
   }
   suffix++;
   if (strcasecmp(suffix, "HEX") == 0)
   {
      if ((detected_binary_size = parse_ihex(file, mem, min, max, offset)) < 0)
      {
         fclose(file);
         return -1;
      }
   }
   else if (!strcasecmp(suffix, "S19")  || !strcasecmp(suffix, "S28")  ||
            !strcasecmp(suffix, "S37")  || !strcasecmp(suffix, "SREC") ||
            !strcasecmp(suffix, "MOT"))
   {
      if ((detected_binary_size = parse_srec(file, mem, min, max, offset)) < 0)
      {
         fclose(file);
         return -1;
      }
   }
   else if (strcasecmp(suffix, "BIN") == 0)
   {
      if ((detected_binary_size = read_binary(file, mem, offset)) < 0)
      {
         fclose(file);
         return -1;
      }
   }
   else
   {
      fprintf(stderr, "Error: Unknown suffix (no .hex or .bin)\n");
      fclose(file);
      return -1;
   }
   fclose(file);
   return detected_binary_size;
}

static int
write_all(int fd, const uint8_t *data, size_t count)
{
   size_t full = count;
   int w;

   while (count > 0)
   {
      w = write(fd, data, count);
      /* fprintf(stderr, "Wrote %d\n", w); */
      if (w < 0)
      {
         return w;
      }
      data += w;
      count -= w;
   }
   return full;
}

#ifdef DEBUG

#define debug_printf(format, ...) printf((format), __VA_ARGS__)

static int
dump_sim_mem(const uint8_t *data, size_t count)
{
   int fd;
   size_t full = count;
   int w;
   const uint8_t *orig_data = data;

   fd = open("dump.bin", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
   if (fd < 0)
   {
      fprintf(stderr, "Error: creating dump file failed\n");
      return fd;
   }
   while (count > 0)
   {
      w = write(fd, data, count);
      /* fprintf(stderr, "Wrote %d\n", w); */
      if (w < 0)
      {
         fprintf(stderr, "Error: write error on dump file\n");
         return w;
      }
      data += w;
      count -= w;
   }
   if (close(fd) < 0)
   {
      perror("Error closing dump file");
      return -1;
   }

   count = SIMMEMSIZE;
   full = count;
   data = orig_data;
   fd = open("whole-sim-mem.bin", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
   if (fd < 0)
   {
      fprintf(stderr, "Error: creating dump file failed\n");
      return fd;
   }
   while (count > 0)
   {
      w = write(fd, data, count);
      /* fprintf(stderr, "Wrote %d\n", w); */
      if (w < 0)
      {
         perror("Error: write error on dump file");
         return w;
      }
      data += w;
      count -= w;
   }
   if (close(fd) < 0)
   {
      perror("Error closing dump file");
      return -1;
   }
   return full;
}
#else
#define debug_printf(format, ...)

static int
dump_sim_mem(const uint8_t *data, size_t count)
{
   (void) data;
   return count;
}
#endif


static int
read_all(int fd, uint8_t *data, size_t count, int timeout)
{
   struct pollfd fds;
   size_t full = count;
   fds.fd = fd;
   fds.events = POLLIN;

   while (count > 0)
   {
      int r;
      r = poll(&fds, 1, timeout);
      if (r <= 0) return 0;
      r = read(fd, data, count);
      if (r <= 0) return r;
      count -= r;
      data += r;
   }
   return full;
}

void
check_input(const char *userinput, const char *endptr)
{
   const char *p = userinput;

   if (*endptr == '\0') return;
   fprintf(stderr, "Error: invalid characters found:\n%s\n", userinput);
   while (*p)
   {
      if (p++ == endptr)
      {
         fprintf(stderr, "^\n");
         break;
      }
      else
         fputc(' ', stderr);
   }
   exit(EXIT_FAILURE);
}

void
skip_white(FILE *file)
{
   int ch;

   while((ch = getc(file)) != EOF && isspace(ch));
   ungetc(ch, file);
}

void
ignore_rest_of_line(FILE *file)
{
   int c;

   // Skip all characters until line end found
   do {
      if ((c = getc(file)) == EOF) return;
   } while (c != '\n' && c != '\r');
   // Discard one or several line ending characters
   do {
      if ((c = getc(file)) == EOF) return;
   } while (c == '\n' || c == '\r');
   if (c == EOF) return;
   // c is not a line ending character, push back to stream
   ungetc(c, file);
}


int
get_hex(FILE *file)                             // nibble
{
   int ch;

   ch = getc(file);
   if (ch >= '0' && ch <= '9') return ch - '0';
   if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
   if (ch >= 'a' && ch <= 'a') return ch - 'a' + 10;
   ungetc(ch, file);
   return -1;
}

int
get_hex2(FILE *file, int *check)                // 8 bit
{
   int v1, v2;

   v1 = get_hex(file);
   if (v1 < 0) return v1;
   v2 = get_hex(file);
   if (v2 < 0) return v2;
   v2 += v1 * 16;
   if (check != NULL) *check += v2;
   return v2;
}

int
get_hex4(FILE *file, int *check)                // 16 bit
{
   int v1, v2;

   v1 = get_hex2(file, check);
   if (v1 < 0) return v1;
   v2 = get_hex2(file, check);
   if (v2 < 0) return v2;
   return v1 * 256 + v2;
}

int
get_hex6(FILE *file, int *check)                // 24 bit
{
   int v1, v2;

   v1 = get_hex4(file, check);
   if (v1 < 0) return v1;
   v2 = get_hex2(file, check);
   if (v2 < 0) return v2;
   return v1 * 256 + v2;
}


long long int
get_hex8(FILE *file, int *check)                // 32 bit
{
   long long v1, v2;

   v1 = get_hex4(file, check);
   if (v1 < 0) return v1;
   v2 = get_hex4(file, check);
   if (v2 < 0) return v2;
   return v1 * 65536 + v2;
}



int
main(int argc, char *argv[])
{
   int res;
   int fd;
   unsigned int i;
   long offset = 0;
   char reset_enable = 'N';
   int8_t reset_time = 200;
   const struct MemType *mem_type = &memory_types[3];
   int detected_size = 0;
   int sim_size;
   char emu_enable = 'D';
   char selftest = 'N';
   char *device = DEFAULT_DEVICE;
   int opt;
   char emu_cmd[16+1];
   char emu_reply[16+1];
   int value;
   int min, max;
   char *endptr;

   while ((opt = getopt(argc, argv, "hd:m:o:r:e")) != -1) {
      switch (opt) {
         case 'd':
            device = optarg;
            break;
         case 'm':
            mem_type_given = true;
            mem_type = NULL;
            for (i = 0; i < (sizeof(memory_types) / sizeof(memory_types[0])); i++)
            {
               if (strcmp(optarg, memory_types[i].name) == 0)
               {
                  mem_type = &memory_types[i];
                  break;
               }
            }
            if (!mem_type)
            {
               fprintf(stderr, "Error: Unknown memory type\n");
               return EXIT_FAILURE;
            }
            break;
         case 'o':
            offset_given = true;
            offset = strtol(optarg, &endptr, 0);
            check_input(optarg, endptr);
            break;
         case 'r':
            value = strtol(optarg, &endptr, 0);
            check_input(optarg, endptr);
            if (value < -255 || value > 255)
            {
               fprintf(stderr, "Error: Reset time out of range\n");
               return EXIT_FAILURE;
            }
            if (value == 0)
            {
               reset_enable = '0';
               reset_time = 0;
            }
            else if (value > 0)
            {
               reset_enable = 'P';
               reset_time = value;
            }
            else
            {
               reset_enable = 'N';
               reset_time = -value;
            }
            break;
         case 'e':
            emu_enable = 'E';
            break;
         case 'h':
            usage();
            return EXIT_SUCCESS;
         case '?':
            return EXIT_FAILURE;
      }

   }

   if (argc <= optind) return EXIT_SUCCESS;

   res = read_image(argv[optind], mem, offset, &min, &max);
   if (res < 0)
   {
      return EXIT_FAILURE;
   }
   detected_size = res;
   if (mem_type_given && (detected_size > mem_type->size))
   {
      fprintf(stderr, "Too much data (%d bytes) for specified memory type (%d bytes)\n", detected_size, mem_type->size);
      return EXIT_FAILURE;
   }
   bool size_is_standard_size = false;
   for (i = 0; i < (sizeof(memory_types) / sizeof(memory_types[0])); i++)
   {
      if (memory_types[i].size == detected_size)
      {
         size_is_standard_size = true;
         break;
      }
   }
   sim_size = mem_type_given ? mem_type->size : detected_size;
   if (!size_is_standard_size)
   {
      printf("Warning: non-standard binary size of %d bytes\n", detected_size);
      if (!mem_type_given) {
         for (i = 0; i < (sizeof(memory_types) / sizeof(memory_types[0])); i++)
         {
            sim_size = memory_types[i].size;
            if (sim_size >= detected_size)
            {
               printf("Simulated size increased to %d bytes\n", sim_size);
               break;
            }
         }
      }
   }
   if (mem_type_given && (detected_size != mem_type->size))
   {
      printf("Warning: binary size (%d bytes) doesn't match memory size (%d bytes)\n",
            detected_size, mem_type->size);
   }

   /* Guess chip type from file size */
   if (!mem_type_given)
   {
      mem_type = NULL;
      for (i = 0; i < (sizeof(memory_types) / sizeof(memory_types[0])); i++)
      {
         if (memory_types[i].size == sim_size)
         {
            mem_type = &memory_types[i];
            printf("%d bytes, must be a %s chip.\n", sim_size, mem_type->name);
            break;
         }
      }
      if (!mem_type)
      {
         fprintf(stderr, "Can't autodetect chip type for %d bytes\n", sim_size);
         return EXIT_FAILURE;
      }

   }

   fd = serial_open(device);
   if (fd < 0) return EXIT_FAILURE;

   /* Configuration */
   snprintf(emu_cmd, sizeof(emu_cmd), "MC%c%c%03d%c%c00023\r\n",
         mem_type->cmd, reset_enable, reset_time, emu_enable, selftest);
   debug_printf("Config: %s\n", emu_cmd);
   res = write_all(fd, (uint8_t*)emu_cmd, sizeof(emu_cmd) - 1);
   if (res != sizeof(emu_cmd) - 1) {
      perror("Failed to write configuration");
   }
   res = read_all(fd, (uint8_t*)emu_reply, 16, 5000);
   if (res == 0)
   {
      fprintf(stderr, "Error: Timeout while waiting for configuration reply\n");
      close(fd);
      return EXIT_FAILURE;
   }
   if (res != 16)
   {
      perror("Error: Failed to read configuration reply");
      close(fd);
      return EXIT_FAILURE;
   }
   emu_reply[16] = '\0';
   debug_printf("Reply: %s\n", emu_reply);
   if (memcmp(emu_cmd, emu_reply, 8) != 0)
   {
      fprintf(stderr, "Error: Response didn't match command\n");
      close(fd);
      return EXIT_FAILURE;
   }


   snprintf(emu_cmd, sizeof(emu_cmd), "MD%04d00000058\r\n",sim_size / 1024 % 1000);
   debug_printf("Data: %s\n", emu_cmd);
   fprintf(stderr, "Writing %d bytes to simulator...\n", sim_size);
   res = write_all(fd, (uint8_t*)emu_cmd, sizeof(emu_cmd) - 1);
   if (res != sizeof(emu_cmd) - 1)
   {
      perror("Error: Failed to write data header");
   }
   res = write_all(fd, mem, sim_size);
   if (res < 0)
   {
      perror("Error: Failed to write data");
      close(fd);
      return EXIT_FAILURE;
   }
   dump_sim_mem(mem, sim_size);

   res = read_all(fd, (uint8_t*)emu_reply, 16, 15000);
   if (res == 0)
   {
      fprintf(stderr, "Error: Timeout while waiting for write operation\n");
      close(fd);
      return EXIT_FAILURE;
   }
   if (res != 16)
   {
      perror("Error: Failed to read data reply");
      close(fd);
      return EXIT_FAILURE;
   }
   emu_reply[16] = '\0';
   debug_printf("Reply: %s\n", emu_reply);
   if (memcmp(emu_cmd, emu_reply, 8) != 0)
   {
      fprintf(stderr, "Error: Response didn't match command\n");
      close(fd);
      return EXIT_FAILURE;
   }
   fprintf(stderr, "Done\n");

   close(fd);
   return EXIT_SUCCESS;
}
