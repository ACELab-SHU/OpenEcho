import m5
from m5.objects import *

system = System()

system.clk_domain = SrcClockDomain()
system.clk_domain.clock = '1GHz'
system.clk_domain.voltage_domain = VoltageDomain()

system.mem_mode = 'timing'
system.mem_ranges = [AddrRange('128MiB')]

system.membus = SystemXBar()

system.cpu = MinorCPU()

system.cpu.createInterruptController()

system.cpu.icache_port = system.membus.cpu_side_ports
system.cpu.dcache_port = system.membus.cpu_side_ports

system.VenusSequencer = VenusSequencer()
system.cpu.venus_sequencer = system.VenusSequencer
system.VenusLane = VenusLane()

system.VenusSequencer.port_venussequencer_receivefrom_venuspacketgen = \
    system.cpu.port_venusminorcpu_sendto_venussequencer

system.VenusLane.port_venuslane_receivefrom_venussequencer = \
    system.VenusSequencer.port_venussequencer_sendto_venuslane

system.VenusLane.port_venuslane_hazardtable_listen = \
    system.VenusSequencer.port_venussequencer_hazardtable_boardcast

binary = 'tests/test-progs/riscv32/os.elf'
system.workload = SEWorkload.init_compatible(binary)
process = Process()
process.cmd = [binary]
system.cpu.workload = process

system.cpu.createThreads()

system.physmem = SimpleMemory()
system.physmem.port = system.membus.mem_side_ports
system.system_port = system.membus.cpu_side_ports

root = Root(full_system=False, system=system)

m5.instantiate()

print("Beginning simulation!")
exit_event = m5.simulate(1000000)
print(f'Exiting @ tick {m5.curTick()} because {exit_event.getCause()}')
