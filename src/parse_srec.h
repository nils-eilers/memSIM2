#pragma once

int
parse_srec(FILE *file, uint8_t *buffer, int *min, int *max, long offset);
