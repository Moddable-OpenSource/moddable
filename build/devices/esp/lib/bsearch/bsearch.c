/* 
 * bsearch.c --
 *
 *	Source code for the bsearch library routine.
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

// modified 5/30/2018 - jph for use in Moddable SDK on ESP8266

#include <stdlib.h>

#include <stdio.h>
#include <sys/types.h>

/*
 *----------------------------------------------------------------------
 *
 * bsearch --
 *
 *	Bsearch searches base[0] to base[n - 1] for an item that
 *      matches *key.  The function cmp must return negative if its first
 *      argument (the search key) is less that its second (a table entry),
 *      zero if equal, and positive if greater.  Items in the array must
 *      be in ascending order.  
 *
 * Results:
 *	Returns a pointer to a matching item, or NULL if none exits.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void *
bsearch(const void *key, const void *base, size_t n, size_t size, int (*cmp)(const void *, const void *))
{
    const char *middle;
    int c;


    for (middle = base; n != 0;) {
	middle += (n/2) * size;
	if ((c = (*cmp)(key, middle)) > 0) {
	    n = (n/2) - ((n&1) == 0);
	    base = (middle += size);
	} else if (c < 0) {
	    n /= 2;
	    middle = base;
	} else {
	    return (char *) middle;
	}
    }
    return NULL;
}

