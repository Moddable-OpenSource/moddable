#include "stdint.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"

void hello(char* string, uint8_t* buffer, uint32_t length)
{
	uint32_t i = 0;
	while (i < length) {
		buffer[i] = (uint8_t)string[i];
		i++;
	}
}

char* catenate(char* arg0, char* arg1)
{
	size_t len0 = strlen(arg0);
	size_t len1 = strlen(arg1);
	char* result = malloc(len0 + len1 + 1);
	if (result) {
		memcpy(result, arg0, len0);
		memcpy(result + len0, arg1, len1);
		result[len0 + len1] = 0;
	}
	return result;
}

const char* days[7] = {
	"dimanche",
	"lundi",
	"mardi",
	"mercredi",
	"jeudi",
	"vendredi",
	"samedi",
};

const char* nameDay(uint8_t i)
{
	return days[i];
}

typedef struct {
	uint8_t a;
	uint16_t b;
	uint32_t c;
} ABC;

char* abcToString(void* ptr)
{
	ABC* abc = ptr;
	char buffer[256];
	int len = snprintf(buffer, sizeof(buffer), "a: %d b: %d c: %ld", abc->a, abc->b, abc->c);
	abc->a = 3;
	abc->b = 4;
	abc->c = 5;
	char* result = malloc(len + 1);
	memcpy(result, buffer, len + 1);
	return result;
}