/*
 * Copyright (c) 2026  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

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
