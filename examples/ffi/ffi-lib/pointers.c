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

uint32_t* newTriple(uint32_t* buffer, uint32_t delta)
{
	uint32_t* result = malloc(3 * sizeof(uint32_t));
	if (result) {
		result[0] = buffer[0] + delta;
		result[1] = buffer[1] + delta;
		result[2] = buffer[2] + delta;
	}
	return result;
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
