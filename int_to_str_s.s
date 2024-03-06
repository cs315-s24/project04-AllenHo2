.global int_to_str_s


# a0 is  uint32_t value
# a1 is *result_str
# a2 is base
# t0 is len 
# t1 is div
# t2 is rem 
# Do in line reverse for string using a temp

#add prefix to my code in the original value 
#can maybe try to add it in the temp(t3) later 
base2:
    addi a1, a1, -1
    li t1, 'b'
    sb t1, (a1)
    addi a1, a1, -1
    li t1, '0'
    sb t1, (a1)
    
    j done

base16:
    addi a1, a1, -1
    li t1, 'x'
    sb t1, (a1)
    addi a1, a1, -1
    li t1, '0'
    sb t1, (a1)
    
    j done

#load in starting values
int_to_str_s:
    addi sp, sp, -40
    sd ra, (sp)
    mv t0, zero     #set len to 0
    mv t2, a0
    mv t3, a1                   #move a0 into t3 for temp manipulation
    li t4, 10
    beq a2 , t4, strlen
    addi a1, a1, 2
    beq a0, zero, fill_zero     #if base case is 0, return 0

strlen:
    div t2, t2, a2
    addi t0, t0, 1

    bne t2, zero, strlen
    
    add a1, a1, t0
    sb zero, (a1)

loop:
    beq t0, zero, prefix        # if len is 0, break out of loop
    rem t2, a0, a2              # rem
    li t4, 10                   
    blt t2, t4, less_nine       # if rem is less than 10 (<= 9) do convert
    li t4, 15
    bgt t2, t4, increment       # if rem is more than 15, increment
    li t4, 'a'                  
    addi t4, t4, -10
    add t4, t4, t2
    addi a1, a1, -1            #increment t3 forward 
    sb t4, (a1)                 #store that into t3
    
increment:
    addi t0, t0, -1              # increment len
    div a0, a0, a2
    
    j loop

less_nine:
    li t4, '0'                      
    add t4, t4, t2             # temp[len] = '0' + rem
    addi a1, a1, -1
    sb t4, (a1)

    j increment

fill_zero:
    li t4, '0'  
    sb t4, (a1)
    
prefix:
    li t1, 2        #t1 is currently temp 
    beq a2, t1, base2
    li t1, 16
    beq a2, t1, base16


done:
    ld ra, (sp)
    addi sp, sp, 40

    ret



