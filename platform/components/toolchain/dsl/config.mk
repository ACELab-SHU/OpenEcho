# LLVM_PATH=/server17/ic/llvm-bin/bin

#LLVM for VENUS 1p0 64lane512row
# LLVM_PATH=/home/xusiyi/new/llvm-project-cmake-build-debug/bin
#LLVM legacy
# LLVM_PATH=/home/xusiyi/sys_llvm/llvm-project-cmake-build-debug/bin
#LLVM for VENUS 1p0 32lane1024row
# --=^LLVM_PATH=/home/xusiyi/R1024_L32/llvm-project-cmake-build-debug/bin
#LLVM for VENUS 2p0
LLVM_PATH=/home/chenxiaoxiao/venus-llvm-project_bk/build/bin
# LLVM_PATH=/home/chenxiaoxiao/venus-llvm-project_417/build/bin
RVPATH=/server17/ic/riscv32im
RM=/usr/bin/rm
PYTHON=/usr/bin/python3
# TARGET_DAG=PDSCHDag2
# TARGET_DAG=ltePCFICH
# TARGET_DAG=ltePBCHDag2_hw
# TARGET_DAG=ltePBCHDag1_hw
# TARGET_DAG=Simpledagfire_test
# TARGET_DAG=ltePDSCH
# TARGET_DAG=ltePDCCHDag2_hw
# TARGET_DAG=ltePDCCHDag1_hw
# TARGET_DAG=lrsc_test
# TARGET_DAG=nrPBCH
# TARGET_DAG=nrPDCCH
# TARGET_DAG=LDPC_SPMD
# TARGET_DAG=nrPDSCHDag2_hw
# TARGET_DAG=vbrdcst_shuffle_probe
# TARGET_DAG=nrPDSCHDag2_hw_replay_Task_nrDemodulate_nrDemodulate_256QAM_len1792
# TARGET_DAG=nrPDSCHDag2_hw_2p0_replay_Task_nrChannelEstimate
# TARGET_DAG=Venus2p0_LDPC
# TARGET_DAG=spmd_test
# TARGET_DAG=Load_test
# TARGET_DAG=random_vlm512_inst500_0
# TARGET_DAG=Single_shuffle8_test_lane16row128
# TARGET_DAG=Single_task_test_lane16row128
# TARGET_DAG=Simpledagfire_test
# TARGET_DAG=Complexdagfire_test
# TARGET_DAG=Complexpointerdagfire
# TARGET_DAG=Longdagfire_test
# TARGET_DAG=postsim_simpleinst_0
# TARGET_DAG=Complexdagfire_row_test
# TARGET_DAG=nrPDSCHDag1_hw_2p0
TARGET_DAG=nrPDSCHDag2_hw_2p0
# TARGET_DAG=pdschdecramble
# TARGET_DAG=nrPDSCHDag1_hw_2p0_2cores
# TARGET_DAG=nrPDSCHDag2_hw_2p0_2cores
# TARGET_DAG=nrPDSCHDag2_hw_2p0_3cores
# TARGET_DAG=nrPDSCHDag2_hw_2p0_4cores

mode=release

CC=$(LLVM_PATH)/clang
OPT=$(LLVM_PATH)/opt
LLC=$(LLVM_PATH)/llc
DUMP=$(LLVM_PATH)/llvm-objdump
CPY=$(LLVM_PATH)/llvm-objcopy
# VENUSROW=2048
# VENUSLANE=32
VENUSROW=128
VENUSLANE=16
# VENUSROW=512
# VENUSLANE=64
# VENUSROW=1024
# VENUSLANE=64
# --=^VENUSROW=1024
# --=^VENUSLANE=32
VENUS_VRFADDR=0x80100000
TEMP_VRF_BASE_ADDR=0x80200000
TEMP_VRF_TOTAL_BYTES=256000
TEMP_ALLOC_ALIGN_BYTES=64
VenusInputStructAddr=0x80022000
# VENUSARCH= --target=riscv32-unknown-elf --gcc-toolchain=/server17/ic/riscv32im -march=rv32imzvenus
VENUSARCH= --target=riscv32-unknown-elf --gcc-toolchain=/server17/ic/riscv32im -march=rv32imazvenus	#启用原子扩展
VENUSCFLAGS= -mllvm --venus -mllvm --venus-nr-row=$(VENUSROW) -mllvm --venus-nr-lane=$(VENUSLANE) 
VENUSIRFLAGS= -S -emit-llvm -Xclang -disable-O0-optnone
CFLAGS= $(VENUSCFLAGS) $(VENUSARCH) $(VENUSIRFLAGS)
# VENUSOPTFLAGS= --mattr=+m,+zvenus --venus-nr-row=$(VENUSROW) --venus-nr-lane=$(VENUSLANE) --venus-istruct-baseaddr=$(VenusInputStructAddr) -S -passes=mem2reg,venusplit 
VENUSOPTFLAGS= --mattr=+m,+a,+zvenus --venus-nr-row=$(VENUSROW) --venus-nr-lane=$(VENUSLANE) --venus-istruct-baseaddr=$(VenusInputStructAddr) -S -passes=mem2reg,venusplit#启用原子扩展
# VENUSOPTFLAGS= --mattr=+m,+zvenus --venus-nr-row=$(VENUSROW) --venus-nr-lane=$(VENUSLANE) --venus-istruct-baseaddr=$(VenusInputStructAddr) -S -passes=mem2reg 
# VENUSLLCFLAGS= -O3 --mattr=+m,+zvenus --venus-nr-row=$(VENUSROW) --venus-nr-lane=$(VENUSLANE) -regalloc=basic --venus-vrf-baseaddr=$(VENUS_VRFADDR) --enable-venus-alu-anlysis
VENUSLLCFLAGS= -O3 --mattr=+m,+a,+zvenus --venus-nr-row=$(VENUSROW) --venus-nr-lane=$(VENUSLANE) -regalloc=basic --venus-vrf-baseaddr=$(VENUS_VRFADDR) --enable-venus-alu-anlysis #启用原子扩展 
VENUSDUMPFLAGS= -d --mattr=+m,+zvenus -M no-aliases
# VENUSLLCFLAGS2= -O3 --mattr=+m,+zvenus --venus-nr-row=$(VENUSROW) --venus-nr-lane=$(VENUSLANE) -regalloc=basic -venus-disable-vimls
VENUSLLCFLAGS2= -O3 --mattr=+m,+a,+zvenus --venus-nr-row=$(VENUSROW) --venus-nr-lane=$(VENUSLANE) -regalloc=basic -venus-disable-vimls #启用原子扩展
