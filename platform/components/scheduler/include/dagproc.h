#ifndef __PROC_H__
#define __PROC_H__

#define DAGPROC_PSEUDO_DSC 0x80000000
#define NORMAL_TRANSFER 0x80000001

typedef enum dagprocstate { IDLE,  // idle
                            D2C,   // dma transfer ddr data to cluster
                            RUN,   // dag running
                            C2D    // dma transfer cluster result to ddr
} dagprocstate_t;

// typedef enum lockstatus {
//   UNLOCK,
//   ENTER,
//   LOCK,
//   QUIT
// } lockstatus_t;

#define NCLUSTER 1
#define NPROC    1  // 模拟方便起见，假设一个cluster只能处理一个dag

/*
 * These lists record the L1 addresses consumed and produced by one DAG
 * launch.  The JSON generator supports up to 256 global descriptors, so a
 * 16-entry input list is not a safe runtime limit for a valid DAG.
 */
#define DAGPROC_MAX_INPUTS  256
#define DAGPROC_MAX_OUTPUTS 256

typedef struct cluster cluster_t;

typedef struct dagproc_config {
  int prog;  // task code bin addr
  int config;
  int datacontainer;
  int globalparacontainer;
  int outputnumreg;
  int tasknumreg;
  int returnvalue;
  int outputaddr;
} dagproc_config_t;

typedef struct dagproc {
  dagprocstate_t state;  // IDLE - D2C - RUN - C2D - IDLE
  dagproc_config_t config;
  cluster_t* cluster;  // this dagproc belongs to which cluster
  int daguid;          // dagbin's address
  int inputlist[DAGPROC_MAX_INPUTS];
  int outputlist[DAGPROC_MAX_OUTPUTS];
  int inputnum;
  int outputnum;
} dagproc_t;

typedef struct cluster {
  dagproc_t dagproc[NPROC];
  int payload;  // how many dags running on this cluster
} cluster_t;

#endif /* __PROC_H__ */
