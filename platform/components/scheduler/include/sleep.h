#ifndef __SLEEP_H__
#define __SLEEP_H__

#include "config.h"
#include "printf.h"
#include "stdio.h"

void usleep(unsigned long useconds);
void sleep(unsigned int seconds);
long get_time();
#endif
