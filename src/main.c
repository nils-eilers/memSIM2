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
#include <parse_ihex.h>

#define BPS 460800

#ifndef BOTHER
#define    BOTHER CBAUDEX
#endif
extern int ioctl(int d, unsigned long request, ...);

static int
serial_open(const char *device)
{
  struct termios2 settings;
  int fd;
  fd = open(device, O_RDWR);
  if (fd < 0) {
    perror(device);
    return -1;
  }
  if (ioctl(fd, TCGETS2, &settings) < 0) {
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
  if (ioctl(fd, TCSETS2, &settings) < 0) {
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
	"\t-d DEVICE     Serial device\n"
	"\t-m MEMTYPE    Memory type (2764,27128,27256,27512,27010,27020,27040)\n"
	"\t-r RESETTIME  Time of reset pulse in milliseconds.\n"
	"\t              > 0 for positive pulse, < 0 for negative pulse\n"
	"\t-e            Enable emulation\n"
	"\t-h            This help\n",
	stderr);

}

static int
read_binary(FILE *file, uint8_t *mem, size_t mem_size, long offset)
{
  int res;
  unsigned long addr = 0;
  if (offset > 0) {
    res = fseek(file, offset, SEEK_SET);
    if (res < 0) {
      perror("Error: Failed to seek to offset in binary file\n");
      return -1;
    }
  } else {
    addr = -offset;
  }
  if (addr >= mem_size) {
    fprintf(stderr,"Error: Offset outside memory");
    return -1;
  }
  mem_size -= addr;
  res = fread(mem + addr, sizeof(uint8_t), mem_size, file);
  if (res < 0) {
    perror("Error: Failed to read from binary file\n");
    return -1;
  }

  return 0;
}

static int
read_image(const char *filename, uint8_t *mem, size_t mem_size, long offset)
{
  char *suffix;
  FILE *file = fopen(filename, "rb");
  if (!file) {
    fprintf(stderr, "Error: Failed to open file '%s': %s\n",
	    filename, strerror(errno));
    return -1;
  }
  suffix = rindex(filename,'.');
  if (!suffix) {
    fprintf(stderr, "Error: Filename has no suffix\n");
    fclose(file);
    return -1;
  }
  suffix++;
  if (strcasecmp(suffix, "HEX") == 0) {
    unsigned int min;
    unsigned int max;
    if (parse_ihex(file, mem, mem_size, &min, &max) < 0) {
      fclose(file);
      return -1;
    }
  } else if (strcasecmp(suffix, "BIN") == 0) {
    if (read_binary(file, mem, mem_size, offset) < 0) {
      fclose(file);
      return -1;
    }
  } else {
    fprintf(stderr, "Error: Unknown suffix (no .hex or .bin)\n");
    fclose(file);
    return -1;
  }
  fclose(file);
  return 0;
}

static int
write_all(int fd, const uint8_t *data, size_t count)
{
  size_t full = count;
  int w;
  while(count > 0) {
    w = write(fd, data, count);
    /* fprintf(stderr, "Wrote %d\n", w); */
    if (w < 0) {
      return w;
    }
    data += w;
    count -= w;
  }
  return full;
}

static int
read_all(int fd, uint8_t *data, size_t count, int timeout)
{
  struct pollfd fds;
  size_t full = count;
  fds.fd = fd;
  fds.events = POLLIN;
  while(count > 0) {
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

static uint8_t mem[512*1024];

#define MEM_TYPE_INDEX 2
#define RESET_ENABLE_INDEX 3
#define RESET_TIME_INDEX 4
#define EMU_ENA_INDEX 7
#define SELFTEST_INDEX 8
#define CHKSUM_INDEX 12

struct MemType
{
  const char *name;
  char cmd;
  unsigned long size;
};

const struct MemType memory_types[] =
  {
    {"2764", '0', 8*1024},
    {"27128", '1', 16*1024},
    {"27256", '2', 32*1024},
    {"27512", '3', 64*1024},
    {"27010", '4', 128*1024},
    {"27020", '5', 256*1024},
    {"27040", '6', 512*1024}
  };

int
main(int argc, char *argv[])
{
  int res;
  int fd;
  int i;
  long offset = 0;
  char reset_enable = '0';
  int reset_time = 100;
  const struct MemType *mem_type = &memory_types[3];
  char emu_enable = 'D';
  char selftest = 'N';
  char *device = "/dev/ttyUSB0";
  int opt;
  char emu_cmd[16+1];
  char emu_reply[16+1];
  while((opt = getopt(argc, argv, "hd:m:r:e")) != -1) {
    switch(opt) {
    case 'd':
      device = optarg;
      break;
    case 'm':
      mem_type = NULL;
      for (i = 0; i < (sizeof(memory_types) / sizeof(memory_types[0])); i++) {
	if (strcmp(optarg, memory_types[i].name) == 0) {
	  mem_type = &memory_types[i];
	  break;
	}
      }
      if (!mem_type) {
	fprintf(stderr, "Error: Unknown memory type\n");
	return EXIT_FAILURE;
      }
      break;
    case 'r':
      reset_time = atoi(optarg);
      if (reset_time < -255 || reset_time > 255) {
	fprintf(stderr, "Error: Reset time out of range\n");
	return EXIT_FAILURE;
      }
      if (reset_time == 0) {
	reset_enable = '0';
      } else if (reset_time > 0) {
	reset_enable = 'P';
      } else {
	reset_enable = 'N';
	reset_time = -reset_time;
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

  fd = serial_open(device);
  if (fd < 0) return EXIT_FAILURE;

#if 0
  /* Identify simulator */

  memcpy(emu_cmd, "MI000000000000\r\n",sizeof(emu_cmd));
  res = write_all(fd, (uint8_t*)emu_cmd, sizeof(emu_cmd) - 1);
  if (res != sizeof(emu_cmd) - 1) {
    perror("Failed to write initialization");
  }
  res = read_all(fd, (uint8_t*)emu_reply, 16, 5000);
  if (res == 0) {
    fprintf(stderr, "Timeout while waiting for initialization reply\n");
    close(fd);
    return EXIT_FAILURE;
  }
  emu_reply[16] = '\0';
  printf("Reply: %s\n", emu_reply);
#endif

  /* Configuration */
  snprintf(emu_cmd, sizeof(emu_cmd), "MC%c%c%03d%c%c00023\r\n",mem_type->cmd,reset_enable, reset_time,emu_enable, selftest);
#if DEBUG
  fprintf(stderr, "Config: %s\n", emu_cmd);
#endif
  res = write_all(fd, (uint8_t*)emu_cmd, sizeof(emu_cmd) - 1);
  if (res != sizeof(emu_cmd) - 1) {
    perror("Failed to write configuration");
  }
  res = read_all(fd, (uint8_t*)emu_reply, 16, 5000);
  if (res == 0) {
    fprintf(stderr, "Error: Timeout while waiting for configuration reply\n");
    close(fd);
    return EXIT_FAILURE;
  }
  if (res != 16) {
    perror("Error: Failed to read configuration reply");
    close(fd);
    return EXIT_FAILURE;
  }
#if DEBUG
  emu_reply[16] = '\0';
  printf("Reply: %s\n", emu_reply);
#endif
  if (memcmp(emu_cmd, emu_reply, 8) != 0) {
    fprintf(stderr, "Error: Response didn't match command\n");
    close(fd);
    return EXIT_FAILURE;
  }
  if (argc > optind) {
    res = read_image(argv[optind], mem, mem_type->size, offset);
    if (res < 0) {
      close(fd);
      return EXIT_FAILURE;
    }

    snprintf(emu_cmd, sizeof(emu_cmd), "MD%04ld00000058\r\n",mem_type->size / 1024);
#ifdef DEBUG
    fprintf(stderr, "Data: %s\n", emu_cmd);
#endif
    fprintf(stderr, "Writing %ld bytes to simulator...\n", mem_type->size);
    res = write_all(fd, (uint8_t*)emu_cmd, sizeof(emu_cmd) - 1);
    if (res != sizeof(emu_cmd) - 1) {
      perror("Error: Failed to write data header");
    }
    res = write_all(fd, mem, mem_type->size);
    if (res < 0) {
      perror("Error: Failed to write data");
      close(fd);
      return EXIT_FAILURE;
    }

    res = read_all(fd, (uint8_t*)emu_reply, 16, 15000);
    if (res == 0) {
      fprintf(stderr, "Error: Timeout while waiting for write operation\n");
      close(fd);
      return EXIT_FAILURE;
    }
    if (res != 16) {
      perror("Error: Failed to read data reply");
      close(fd);
      return EXIT_FAILURE;
    }
#ifdef DEBUG
    emu_reply[16] = '\0';
    printf("Reply: %s\n", emu_reply);
#endif
    if (memcmp(emu_cmd, emu_reply, 8) != 0) {
      fprintf(stderr, "Error: Response didn't match command\n");
      close(fd);
      return EXIT_FAILURE;
    }
    fprintf(stderr, "Done\n");
  }
  close(fd);
  return EXIT_SUCCESS;
}
