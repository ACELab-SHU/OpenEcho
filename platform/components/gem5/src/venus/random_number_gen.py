from m5.SimObject import SimObject
from m5.params import *

class random_number_gen(SimObject):
    type = 'random_number_gen'
    cxx_header = "venus/random_number_gen.hh"
    cxx_class = 'gem5::random_number_gen'

    max_count = Param.Int(10, "How many ticks to run before stopping")
