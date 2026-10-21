/*
 * Copyright (c) 2016-2026  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Tools.
 * 
 *   The Moddable SDK Tools is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   The Moddable SDK Tools is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 * 
 *   You should have received a copy of the GNU General Public License
 *   along with the Moddable SDK Tools.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/*
	SanitizerCoverage callbacks for Fuzzilli.

	These run on every edge of the instrumented engine, so this file is built
	optimized and without sanitizer or coverage instrumentation (see xst.mk).
*/

#if FUZZILLI
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define SHM_SIZE 0x100000
#define MAX_EDGES ((SHM_SIZE - 4) * 8)

struct shmem_data {
	uint32_t num_edges;
	unsigned char edges[];
};

struct shmem_data* __shmem;
uint32_t *__edges_start, *__edges_stop;

void __sanitizer_cov_reset_edgeguards()
{
	uint64_t N = 0;
	for (uint32_t *x = __edges_start; x < __edges_stop && N < MAX_EDGES; x++)
		*x = ++N;
}

void __sanitizer_cov_trace_pc_guard_init(uint32_t *start, uint32_t *stop)
{
	// Avoid duplicate initialization
	if (start == stop || *start)
		return;

	if (__edges_start != NULL || __edges_stop != NULL) {
		fprintf(stderr, "Coverage instrumentation is only supported for a single module\n");
		exit(-1);
	}

	__edges_start = start;
	__edges_stop = stop;

	// Map the shared memory region
	const char* shm_key = getenv("SHM_ID");
	if (!shm_key) {
		puts("[COV] no shared memory bitmap available, skipping");
		__shmem = (struct shmem_data*) malloc(SHM_SIZE);
	} else {
		int fd = shm_open(shm_key, O_RDWR, S_IREAD | S_IWRITE);
		if (fd <= -1) {
			fprintf(stderr, "Failed to open shared memory region: %s\n", strerror(errno));
			exit(-1);
		}

		__shmem = (struct shmem_data*) mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		if (__shmem == MAP_FAILED) {
			fprintf(stderr, "Failed to mmap shared memory region\n");
			exit(-1);
		}
	}

	__sanitizer_cov_reset_edgeguards();

	__shmem->num_edges = stop - start;
	printf("[COV] edge counters initialized. Shared memory: %s with %u edges\n", shm_key, __shmem->num_edges);
}

void __sanitizer_cov_trace_pc_guard(uint32_t *guard)
{
	// There's a small race condition here: if this function executes in two threads for the same
	// edge at the same time, the first thread might disable the edge (by setting the guard to zero)
	// before the second thread fetches the guard value (and thus the index). However, our
	// instrumentation ignores the first edge (see libcoverage.c) and so the race is unproblematic.
	uint32_t index = *guard;
	// If this function is called before coverage instrumentation is properly initialized we want to return early.
	if (!index) return;
	__shmem->edges[index / 8] |= 1 << (index % 8);
	*guard = 0;
}

#endif
