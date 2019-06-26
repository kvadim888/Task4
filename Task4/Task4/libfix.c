#include "libfix.h"

int32_t	float_to_fix(double num)
{
	int32_t	fixed;

	if (num < -1)
		return INT32_MIN;
	if (num >= 1)
		return INT32_MAX;
	return num * SCALE;
}

double	fix_to_float(int32_t num)
{
	return num / SCALE;
}

int32_t	fix_saturate(int64_t num)
{
	long_fix tmp;

	if (num > (int64_t)INT32_MAX)
		return (INT32_MAX);
	if (num < (int64_t)INT32_MIN)
		return (INT32_MIN);
	tmp.num = num;
	return tmp.range[0];
}

int32_t	fix_round(int64_t num)
{
	long_fix tmp;

	tmp.num = num + ((uint64_t)1 << 31);
	return tmp.range[1];
}

int32_t fix_add(int32_t a, int32_t b)
{
	int64_t tmp_a = a;
	int64_t tmp_b = b;
	int64_t sum = tmp_a + tmp_b;
	return fix_saturate(sum);
}

int32_t fix_sub(int32_t a, int32_t b)
{
	int64_t tmp_a = a;
	int64_t tmp_b = b;
	int64_t sub = tmp_a - tmp_b;
	return fix_saturate(sub);
}

int32_t fix_mul(int32_t a, int32_t b)
{
	int64_t tmp_a = a;
	int64_t tmp_b = b;
	int64_t	mul = (tmp_a * tmp_b) << 1;
	return fix_round(mul);
}

int32_t fix_mac(int64_t *acc, int32_t a, int32_t b)
 {
	int64_t tmp_a = a;
	int64_t tmp_b = b;
	*acc += (tmp_a * tmp_b) << 1;
 	return fix_round(*acc);
 }
 
int32_t fix_msub(int64_t *acc, int32_t a, int32_t b)
 {
	int64_t tmp_a = a;
	int64_t tmp_b = b;
	*acc -= (tmp_a * tmp_b) << 1;
 	return fix_round(*acc);
 }

int32_t	fix_leftshift(int32_t num, int8_t shift)
{
	int64_t res = num;
	return fix_saturate(res << shift);
}

int32_t	fix_rightshift(int32_t num, int8_t shift)
{
	long_fix res;

	res.range[1] = num;
	return fix_round(res.num >> shift);
}
