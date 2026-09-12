#ifndef  __UTILITY_H
#define  __UTILITY_H

#define  FLG_READ     (0)
#define  FLG_WRITE    (1)
#define  FLG_DELAY    (2)
#define  FLG_FPGA_R   (3)
#define  FLG_FPGA_W   (4)

typedef unsigned int regs;

typedef struct reg_type
{
	char flag;
	unsigned short addr;
	unsigned char val;
} reg_t;

typedef enum REG_BIT
{
	BIT0 = 0,
	BIT1,
	BIT2,
	BIT3,
	BIT4,
	BIT5,
	BIT6,
	BIT7,
}REG_BIT;

typedef enum BIT_CNT
{
	CNT8 = 8,
	CNT16 = 16,
	CNT11 = 11,
}BIT_CNT;


#define   REG_MASK                  (0xff)
#define   VAL_MASK(val)             (val & REG_MASK)

//#ifndef GET_BIT
#define   GET_BIT(val, bit)     ((VAL_MASK(val) >> bit) & 1)
//#endif

//#ifndef SET_BIT
#define   SET_BIT(val, bit)     (VAL_MASK(val) | (1<<bit))
//#endif

//#ifndef CLR_BIT
#define   CLR_BIT(val, bit)     (VAL_MASK(val) & VAL_MASK(~(1<<bit)))
//#endif

#define   GET_BITS(val, bit, cnt)             ((val >> bit) & (~((~0) << cnt)))
#define   SET_BITS(val, bit, cnt, setval)     ((val & (~((~((~0) << cnt)) << bit))) | ((setval & (~((~0) << cnt))) << bit))

extern long long IntAbs(long long val);
extern char IsParity(int val);
extern char my_log2(char bit, short n);

extern void simple_sort(int n, int a[]);
extern int get_right_index(unsigned long long comp_val, const unsigned long long arr[], int cnt);
extern void *my_memset(void *dest, int set, unsigned len);
extern void M16bithex2bin(unsigned int val, char result[]);

extern unsigned long long do_div(unsigned long long *n, unsigned long long  base);
extern int get_round_case(unsigned long long val, unsigned long long div);
extern short get_round_case_short(unsigned int val, unsigned int div);

#endif

