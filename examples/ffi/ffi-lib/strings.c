#include "stdint.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"

extern const char* nameDay(uint8_t i);
extern char* catenate(char* arg0, char* arg1);

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
