#ifndef __VENUS_H_
#define __VENUS_H_

#include "coe.h"
#include "common.h"
#include "dagfire.h"
#include "daginfo.h"
#include "fifo.h"
#include "rfdata.h"
#include "venusmmap.h"

extern int check_data_ready(int data_num, ...);
extern void fire_dag_fence(void);
extern int MUTEX_dag_done;

#endif /* __VENUS_H_ */