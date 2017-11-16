#Example Simulation File
#C code: d[2] = d[1] + a*b;
addi $t0, $t0, 56
sw $t0, 0($s4)      #load a data address into s4
addi $t0, $zero, 0
addi $t0, $t0, 5
sw $t0, 4($s4)      #initialize d[1]=5
lw $t0, 4($s4)      #$t0 = d[1] 
addi $t1, $zero, 2
sw $t1, 0($s1)       #initialize a=2
lw $t1,, 0($s1)      #$t1 = a
addi $t2, $zero, 3
sw $t2, 0($s2)       #initialize b=3
lw $t2,  0($s2)      # $t2 = b
mul $t1, $t1, $t2   # $t1 = = $t1 * $t2 = a*b
add $t0,  $t0, $t1   # $t0 = d[1] + a*b
sw $t0, 8($s4)      # store $t0 in d[2]
haltSimulation