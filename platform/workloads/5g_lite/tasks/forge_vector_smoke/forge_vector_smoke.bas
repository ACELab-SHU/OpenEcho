return_value short sum[32]
return_value short restored[32]
parameter short a = {
  -16,-15,-14,-13,-12,-11,-10,-9,-8,-7,-6,-5,-4,-3,-2,-1,
  0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
}
parameter short b = {
  31,29,27,25,23,21,19,17,15,13,11,9,7,5,3,1,
  -1,-3,-5,-7,-9,-11,-13,-15,-17,-19,-21,-23,-25,-27,-29,-31
}
dag dag1 = {
  [sum] = Task_forgeAdd(a, b)
  [restored] = Task_forgeRestore(sum, b)
}
END
