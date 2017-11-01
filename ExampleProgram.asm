#Example Simulation File
#C code: d[2] = d[1] + a*b;
lw $t0, 4($s4)      # $t0 = d[1] 
lw $t1, 0($s1)      # $t1 = a
lw $t2,  0($s2)      # $t2 = b
mul $t1, $t1, $t2   # $t1 = = $t1 * $t2 = a*b
add $t0,  $t0, $t1   # $t0 = d[1] + a*b
sw $t0, 8($s4)      # store $t0 in d[2]
haltSimulation