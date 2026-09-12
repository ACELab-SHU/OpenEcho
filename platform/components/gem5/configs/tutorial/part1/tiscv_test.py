import m5
from m5.objects import *

system = System()

system.clk_domain = SrcClockDomain()
system.clk_domain.clock = '1GHz'
system.clk_domain.voltage_domain = VoltageDomain()

system.mem_mode = 'timing'
system.mem_ranges = [AddrRange('512MB')]  

system.membus = SystemXBar()

# CLINT配置
system.clint = Clint()
system.clint.num_threads = 1  
system.clint.pio_addr = 0x20000000  
system.clint.pio_size = 0x10000    
system.clint.pio = system.membus.mem_side_ports  

system.cpu = MinorCPU()

system.cpu.icache_port = system.membus.cpu_side_ports
system.cpu.dcache_port = system.membus.cpu_side_ports

binary = 'tests/test-progs/riscv32/RTOSDemo.elf'
system.workload = SEWorkload.init_compatible(binary)
process = Process()
process.cmd = [binary]
system.cpu.workload = process

system.cpu.createThreads()

# 关联CLINT
system.cpu.createInterruptController()
system.cpu.interrupts[0].clint = system.clint  

system.system_port = system.membus.cpu_side_ports
system.physmem = SimpleMemory()
system.physmem.port = system.membus.mem_side_ports

root = Root(full_system=False, system=system)
m5.instantiate()
print("Beginning simulation!")
exit_event = m5.simulate()
print(f'Exiting @ tick {m5.curTick()} because {exit_event.getCause()}')
