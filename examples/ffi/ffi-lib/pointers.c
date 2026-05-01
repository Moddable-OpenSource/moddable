#include "math.h"
#include "stdint.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"

uint64_t sumBytes(uint8_t* buffer, uint32_t offset, uint32_t length)
{
	uint64_t result = 0;
	buffer += offset;
	uint8_t* limit = buffer + length;
	while (buffer < limit) {
		result += *buffer++;
	}
	return result;
}

void fillRandom(uint8_t* buffer, uint32_t offset, uint32_t length)
{
	uint32_t i = 0;
	while (i < length) {
		buffer[offset + i] = (uint8_t)floor(((double)rand() / (double)RAND_MAX) * 255);
		i++;
	}
}

typedef struct {
	uint32_t a;
	uint32_t b;
	double c;
} ABC;

void sampleABCSensor(void* ptr)
{
	ABC* abc = ptr;
	abc->a = 1;
	abc->b = 2;
	abc->c = 3.4;
}
