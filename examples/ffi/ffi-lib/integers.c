#include "stdint.h"

extern int32_t add32_t(int32_t arg0, int32_t arg1);
extern int64_t add64_t(int64_t arg0, int64_t arg1);

int32_t add32_t(int32_t arg0, int32_t arg1)
{
	return arg0 + arg1;
}

int64_t add64_t(int64_t arg0, int64_t arg1)
{
	return arg0 + arg1;
}
