from m5.objects.TickedObject import TickedObject
from m5.params import *

class MyTickedModule(TickedObject):
    type = 'MyTickedModule'
    cxx_header = "demo_module/MyTickedModule.hh"
    cxx_class = 'gem5::MyTickedModule'

    max_count = Param.Int(10, "How many ticks to run before stopping")
