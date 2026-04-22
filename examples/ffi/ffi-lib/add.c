#include "stdint.h"

extern int32_t add(int32_t arg0, int32_t arg1);

int32_t add(int32_t arg0, int32_t arg1)
{
	return arg0 + arg1;
}

int32_t addSquares(int32_t arg0, int32_t arg1)
{
	return (arg0 * arg0) + (arg1 * arg1);
}
