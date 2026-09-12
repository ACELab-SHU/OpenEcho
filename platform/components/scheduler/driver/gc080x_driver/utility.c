#include "utility.h"
#include "platform.h"

void simple_sort(int n, int a[])
{
	int i,j,t;

	for(i=0; i<(n-1); i++)
		for (j=i+1; j<=(n-1); j++)
			if (a[i] > a[j])
			{
				t = a[i];
				a[i] = a[j];
				a[j] = t;
			}
}

void *my_memset(void *dest, int set, unsigned len)
{
	char *pdest = (char *)dest;
	
	if (dest == NULL || len < 0)
	{
		return NULL;
	}
	
	while (len-- > 0)
	{
		*pdest++ = set;
	}
	
	return dest;
}

int get_right_index(unsigned long long comp_val, const unsigned long long arr[], int cnt)
{
	int i, ret;
    int store_index;
	long long scomp_val, tmp, curr_delta, old_delta;
	long long *sarr;

	scomp_val = (long long)comp_val;
	sarr = (long long *)arr;
	
    curr_delta = 0;
    old_delta = 0;
	store_index = 0;
    
    for (i=1; i<=cnt; i++)
    {
        tmp = sarr[i-1];
        curr_delta = IntAbs(scomp_val - tmp);
        if (curr_delta == 0)
        {
            store_index = i - 1;
            goto finished;
        }
        else
        {
            if (old_delta == 0)
            {
                old_delta = curr_delta;
            }
            else
            {
                if (curr_delta < old_delta)
                {
                    old_delta = curr_delta;
                    store_index = i - 1;
                }
            }
        }
    }
    
finished:
    ret = store_index;
	return ret;
}

long long IntAbs(long long val)
{
	long long ret;

    if (val >= 0)
    {
        ret = val;
    }
    else
    {
        ret = 0 - val;
    }

	return ret;
}

char IsParity(int val)
{
	int ret;

	if (val % 2 == 0)
	{
        ret = 0;
	}
    else
    {
        ret = 1;
    }

	return ret;
}

char my_log2(char bit, short n)
{
	unsigned int val;
	char i;

	for (i=0; i<bit; i++)
	{
		val = 1 << i;
		if (n == val)
			break;
	}

	return i;
}

void M16bithex2bin(unsigned int val, char result[])
{
	char bit_val;
	int i;

	for (i=15; i>=0; i--)
	{
		bit_val = (val >> i) & 1;
		if (bit_val == 1) result[15-i] = '1';
		else result[15-i] = '0';
	}
}

int get_round_case(unsigned long long val, unsigned long long div)
{
	unsigned long long mod;

	mod = do_div(&val, div);
	//mod = val % div;
	mod = mod << 1;

	if (mod >= div)
		return 1;
	else
		return 0;
}

short get_round_case_short(unsigned int val, unsigned int div)
{
	unsigned int mod;

	//mod = do_div(val, div);
	mod = val % div;
	mod = mod << 1;

	if (mod >= div)
		return 1;
	else
		return 0;
}

unsigned long long do_div(unsigned long long *n, unsigned long long  base)
{
	unsigned long long  mod = 0;

	mod = *n % base;
	*n = *n / base;

	return mod;
}

