#include "sleep.h"

void usleep(unsigned long useconds) {
  long Begin_Time = 0;
  long End_Time   = 0;
  long User_Time  = 0;
  Begin_Time      = get_time((long*)0);
  // 100MHz为10ns，1ns为1000us
  while ((User_Time * ((1.0 / (CPU_CYCLE_MHZ * 1000000.0)) * 1000000.0)) < useconds) {
    End_Time  = get_time((long*)0);
    User_Time = End_Time - Begin_Time;
  }
}

void sleep(unsigned int seconds) {
  long Begin_Time = 0;
  long End_Time   = 0;
  long User_Time  = 0;
  Begin_Time      = get_time((long*)0);
  // 100MHz为10ns，1s为1000_000_000ns
  while ((User_Time * (1.0 / (CPU_CYCLE_MHZ * 1000000.0))) < seconds) {
    End_Time  = get_time((long*)0);
    User_Time = End_Time - Begin_Time;
  }
}

long get_time() {
  int cycles;
  asm volatile("rdcycle %0"
               : "=r"(cycles));
  // printf("[time() -> %d]", cycles);
  return cycles;
}