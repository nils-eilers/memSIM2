#ifndef __PARSE_IHEX_H__CNXRKTJLF4__
#define __PARSE_IHEX_H__CNXRKTJLF4__

#include <stdint.h>
#include <stdio.h>

int
parse_ihex(FILE *file, uint8_t *buffer, unsigned int size,
	   unsigned int *min, unsigned int *max);

#endif /* __PARSE_IHEX_H__CNXRKTJLF4__ */
