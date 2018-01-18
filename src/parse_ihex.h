#pragma once

int
parse_ihex(FILE *file, uint8_t *buffer, int *min, int *max, long offset);
