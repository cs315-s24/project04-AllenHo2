.global str_to_int_s



# a0 is str
# a1 is base
# t0 is retval 
# t1 is place_val
# t2 is digit 
# t3 is strlen
count:
    mul t1, t1, a1
    addi t3, t3, 1
    addi a0, a0, 1

    j loop

str_to_int_s:
    mv t0, zero
    li t1, 1
    li t3, 0
    addi sp, sp, -40
    sd ra, (sp)
    sd a0, 8(sp)
    
loop:
    lb t2, (a0)
    bne t2, zero, count
    ld a0, 8(sp)

loop_1:
    lb t2, (a0)
    beq t2, zero, done
    li t4, 16
    beq a1, t4, base2
    li t4, 10
    beq a1, t4, base2
    li t4, 2
    beq a1, t4, base2

    j done

base2:
    li t4, '9'
    bgt t2, t4, base10
    li t4, '0'
    sub t2, t2, t4

    j convert
base10:
    li t4, 'F'
    bgt t2, t4, base16
    li t4, 'A'
    sub t2, t2, t4
    addi t2, t2, 10

    j convert
base16:
    li t4, 'f'
    bgt t2, t4, skip
    li t4, 'a'
    sub t2, t2, t4
    addi t2, t2, 10
    
convert:
    div t1, t1, a1
    mul t2, t2, t1
    add t0, t0,t2
    #sb t2, (a0)
    addi a0, a0, 1
    j loop_1

skip:
    div t1, t1, a1
    div t1, t1, a1
    addi a0, a0, 1
    j loop_1

done:
    mv a0, t0
    ld ra, (sp)
    addi sp, sp, 40
    ret

