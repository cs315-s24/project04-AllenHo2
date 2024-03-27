#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rv_emu.h"
#include "bits.h"

#define DEBUG 0

//global variables to count dynamic analysis
// int ir_count = 0;
// int total_count = 0;
// int load_count = 0;
// int store_count = 0;
// int jump_count = 0;
// int taken = 0;
// int not_taken = 0;


static void unsupported(char *s, uint32_t n) {
    printf("unsupported %s 0x%x\n", s, n);
    exit(-1);
}

void emu_r_type(rv_state *rsp, uint32_t iw) {
    uint32_t rd = (iw >> 7) & 0b11111;
    uint32_t rs1 = get_bits(iw, 15 ,5);
    uint32_t rs2 = get_bits(iw, 20 ,5);
    uint32_t funct3 = get_bits(iw, 12 ,3);
    uint32_t funct7 = (iw >> 25) & 0b1111111;
    rsp->analysis.ir_count++;
    if (funct3 == 0b000 && funct7 == 0b0000000) { // add instruction 
        rsp->regs[rd] = rsp->regs[rs1] + rsp->regs[rs2];
    } else if (funct3 == 0b000 && funct7 == 0b0100000) { // sub instruction
        rsp->regs[rd] = rsp->regs[rs1] - rsp->regs[rs2];
    } else if (funct3 == 0b000 && funct7 == 0b0000001) { // mul instruction
        rsp->regs[rd] = rsp->regs[rs1] * rsp->regs[rs2];      
    } else if (funct3 == 0b100 && funct7 == 0b0000001) { // div instruction
        rsp->regs[rd] = rsp->regs[rs1] / rsp->regs[rs2];      
    } else if (funct3 == 0b110 && funct7 == 0b0000001) { // rem instruction
        rsp->regs[rd] = rsp->regs[rs1] % rsp->regs[rs2];  
    } else if (funct3 == 0b111 && funct7 == 0b0000000) {  //and instruction
        rsp->regs[rd] = rsp->regs[rs1] & rsp->regs[rs2];   
    } else if (funct3 == 0b001 && funct7 == 0b0000000) {  //sll
        rsp->regs[rd] = rsp->regs[rs1] << rsp->regs[rs2];     
    } else if (funct3 == 0b101 && funct7 == 0b0000000) {  //srl
        rsp->regs[rd] = rsp->regs[rs1] >> rsp->regs[rs2];            
    } else {
        unsupported("R-type funct3", funct3);
    }
    rsp->pc += 4; // Next instruction
}

void emu_i_type (rv_state *rsp, uint32_t iw) {
    uint32_t rd = (iw >> 7) & 0b11111;
    uint32_t rs1 = (iw >> 15) & 0b11111;
    uint32_t imm = (iw >> 25) & 0b1111111;
    uint32_t shamt = (iw >> 20) & 0b11111;
    uint32_t funct3 = (iw >> 12) & 0b111;
    uint32_t imm2 = get_bits(iw, 20, 12);
    
    int signed_im = sign_extend(imm2, 12);
    rsp->analysis.ir_count++;
    if (funct3 == 0b101 && imm == 0b0000000) { //srl
        rsp->regs[rd] = rsp->regs[rs1] >> shamt;
    } else if (funct3 == 0b001) { //slli
        rsp->regs[rd] = rsp->regs[rs1] << shamt; 
    } else if (funct3 == 0b000) { //addi
        rsp->regs[rd] = rsp->regs[rs1] + signed_im;    
    } else {
        unsupported("I-type funct3", funct3);
    }
    rsp->pc += 4;
}

void emu_b_type (rv_state *rsp, uint32_t iw) {
    uint32_t rs1 = (iw >> 15) & 0b11111;
    uint32_t rs2 = (iw >> 20) & 0b11111;
    uint32_t funct3 = (iw >> 12) & 0b111;
    uint32_t imm11 = (iw >> 7) & 0b1;
    uint32_t imm41 = (iw >> 8) & 0b1111;
    uint32_t imm105 = (iw >> 25) & 0b111111;
    uint32_t imm12 = (iw >> 31) & 0b1;
    uint32_t imm = (imm12 << 12) | (imm11 << 11) | (imm105 << 5) | (imm41 << 1);
    int bit = sign_extend(imm, 13); // sign extend
    if (((int)rsp->regs[rs1]) < ((int)rsp->regs[rs2]) && funct3 == 0b100) { // blt
        rsp->analysis.b_taken++;
        rsp->pc += bit;  
    } else if (((int)rsp->regs[rs1]) != ((int)rsp->regs[rs2]) && funct3 == 0b001){ //bne
        rsp->analysis.b_taken++;
        rsp->pc += bit;      
    } else if (((int)rsp->regs[rs1]) == ((int)rsp->regs[rs2]) && funct3 == 0b000){ //beq
        rsp->analysis.b_taken++;
        rsp->pc += bit;     
    } else if (((int)rsp->regs[rs1]) >= ((int)rsp->regs[rs2]) && funct3 == 0b101){ //bge
        rsp->analysis.b_taken++;
        rsp->pc += bit;     
    } else {
        rsp->analysis.b_not_taken++;
        rsp->pc += 4;     
    }
}

void emu_jalr(rv_state *rsp, uint32_t iw) {
    uint32_t rs1 = (iw >> 15) & 0b11111;  // Will be ra (aka x1)
    uint64_t val = rsp->regs[rs1];  // Value of regs[1]
    rsp->analysis.j_count++;
    rsp->pc = val;  // PC = return address
}

void emu_jal(rv_state *rsp, uint32_t iw) {
    uint32_t imm12 = get_bits(iw, 12, 8);  
    uint32_t imm11 = get_bits(iw, 20, 1);
    uint32_t imm1 = get_bits(iw, 21, 10);
    uint32_t imm20 = get_bits(iw, 31, 1);
    uint32_t imm = (imm1 << 1) | (imm11 << 11) | (imm12 << 12) | (imm20 << 20);
    uint32_t rd = (iw >> 7) & 0b11111;
    int64_t offset = sign_extend(imm, 21);
    rsp->analysis.j_count++;
    if(rd != 0) { //has to not be x0, which makes this into a jump instead of a jal
    rsp->regs[rd] = rsp->pc + 4;
    }
    rsp->pc += offset;  // PC = return address
}

void emu_load(rv_state *rsp, uint32_t iw) {
    uint32_t rd = (iw >> 7) & 0b11111;
    uint32_t rs1 = (iw >> 15) & 0b11111;
    uint32_t funct3 = (iw >> 12) & 0b111;
    uint32_t imm2 = get_bits(iw, 20, 12);
    
    int64_t signed_im = sign_extend(imm2, 12);
    rsp->analysis.ld_count++;
    if (funct3 == 0b000) { //lb
       uint8_t *pt = (uint8_t*)(rsp->regs[rs1] + signed_im);
       rsp->regs[rd] = *(uint8_t*)pt;
    } else if (funct3 == 0b010) { //lw
       uint32_t *pt = (uint32_t*)(rsp->regs[rs1] + signed_im);
       rsp->regs[rd] = *(uint32_t*)pt;
    } else if (funct3 == 0b001) {
       uint64_t *pt = (uint64_t*)(rsp->regs[rs1] + signed_im);
       rsp->regs[rd] = *(uint64_t*)pt;  
    } else if (funct3 == 0b011) { //ld
       uint64_t *pt = (uint64_t*)(rsp->regs[rs1] + signed_im);
       rsp->regs[rd] = *(uint64_t*)pt;
    } else {
        unsupported("Load-type funct3", funct3);
    }
  rsp->pc += 4;      
}

void emu_store(rv_state *rsp, uint32_t iw) {
    uint32_t imm1 = (iw >> 7) & 0b11111;
    uint32_t rs1 = (iw >> 15) & 0b11111;
    uint32_t rs2 = get_bits(iw, 20, 5);
    uint32_t funct3 = get_bits(iw, 12 ,3);
    uint32_t imm2 = get_bits(iw, 25, 7);
    uint32_t imm = (imm1) | (imm2 << 5);
    int64_t signed_im = sign_extend(imm, 12);
    rsp->analysis.st_count++;
    if (funct3 == 0b000) { //sb
        uint8_t *pt = (uint8_t*)(rsp->regs[rs1] + signed_im);    
        *pt = (uint8_t)rsp->regs[rs2];
    } else if (funct3 == 0b010) { //sw
        uint32_t *pt = (uint32_t*)(rsp->regs[rs1] + signed_im);    
        *pt = (uint32_t)rsp->regs[rs2];
    } else if (funct3 == 0b001) { //sh
        uint64_t *pt = (uint64_t*)(rsp->regs[rs1] + signed_im);    
        *pt = (uint64_t)rsp->regs[rs2]; 
    } else if (funct3 == 0b011) { //sd
        uint64_t *pt = (uint64_t*)(rsp->regs[rs1] + signed_im);    
        *pt = (uint64_t)rsp->regs[rs2];
    } else {
        unsupported("Store-type funct3", funct3);
    }
  rsp->pc += 4;      
}

static void rv_one(rv_state *state) {
    uint32_t iw  = *((uint32_t*) state->pc);
    iw = cache_lookup(&state->i_cache, (uint64_t) state->pc);

    uint32_t opcode = get_bits(iw, 0, 7);

    
#if DEBUG
    printf("iw: %x\n", iw);
#endif
    state->analysis.i_count++;
    switch (opcode) {
        case FMT_I_JALR:
        emu_jalr(state,iw);
        break;
        case FMT_R: //R type instructions
        emu_r_type(state, iw);
        break;
        case FMT_I_ARITH: //I type arithmetic instructions
        emu_i_type(state,iw);
        break;
        case FMT_I_LOAD: //I type load instructions
        emu_load(state,iw);
        break;
        case FMT_SB: //branch instructions
        emu_b_type(state,iw);
        break;  
        case FMT_UJ: //jump instructions
        emu_jal(state,iw);
        break;  
        case FMT_S: //store instructions
        emu_store(state,iw);
        break;  
        default:
            unsupported("Unknown opcode: ", opcode);
    }
}

void rv_init(rv_state *state, uint32_t *target, 
             uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3) {
    state->pc = (uint64_t) target;
    state->regs[RV_A0] = a0;
    state->regs[RV_A1] = a1;
    state->regs[RV_A2] = a2;
    state->regs[RV_A3] = a3;

    state->regs[RV_ZERO] = 0;  // zero is always 0  (:
    state->regs[RV_RA] = RV_STOP;
    state->regs[RV_SP] = (uint64_t) &state->stack[STACK_SIZE];

    memset(&state->analysis, 0, sizeof(rv_analysis));
    cache_init(&state->i_cache);
}

uint64_t rv_emulate(rv_state *state) {
    while (state->pc != RV_STOP) {
        rv_one(state);
    }
    return state->regs[RV_A0];
}

static void print_pct(char *fmt, int numer, int denom) {
    double pct = 0.0;

    if (denom)
        pct = (double) numer / (double) denom * 100.0;
    printf(fmt, numer, pct);
}

void rv_print(rv_analysis *a) {
    int b_total = a->b_taken + a->b_not_taken;

    printf("=== Analysis\n");
    print_pct("Instructions Executed  = %d\n", a->i_count, a->i_count);
    print_pct("R-type + I-type        = %d (%.2f%%)\n", a->ir_count, a->i_count);
    print_pct("Loads                  = %d (%.2f%%)\n", a->ld_count, a->i_count);
    print_pct("Stores                 = %d (%.2f%%)\n", a->st_count, a->i_count);    
    print_pct("Jumps/JAL/JALR         = %d (%.2f%%)\n", a->j_count, a->i_count);
    print_pct("Conditional branches   = %d (%.2f%%)\n", b_total, a->i_count);
    print_pct("  Branches taken       = %d (%.2f%%)\n", a->b_taken, b_total);
    print_pct("  Branches not taken   = %d (%.2f%%)\n", a->b_not_taken, b_total);
}
