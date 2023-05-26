#ifndef CPU_H
#define CPU_H

#include <stdint.h>

#include "bus.h"

typedef struct {
    psx_bus_t* bus;

    uint32_t r[32];

    uint32_t buf[2], pc;
    uint32_t hi, lo;
} psx_cpu_t;

psx_cpu_t* psx_cpu_create();
void psx_cpu_init(psx_cpu_t*, psx_bus_t*);
void psx_cpu_destroy(psx_cpu_t*);
void psx_cpu_cycle(psx_cpu_t*);

void psx_cpu_i_invalid(psx_cpu_t*);

// Primary
void psx_cpu_i_special(psx_cpu_t*);
void psx_cpu_i_bcz(psx_cpu_t*);
void psx_cpu_i_j(psx_cpu_t*);
void psx_cpu_i_jal(psx_cpu_t*);
void psx_cpu_i_beq(psx_cpu_t*);
void psx_cpu_i_bne(psx_cpu_t*);
void psx_cpu_i_blez(psx_cpu_t*);
void psx_cpu_i_bgtz(psx_cpu_t*);
void psx_cpu_i_addi(psx_cpu_t*);
void psx_cpu_i_addiu(psx_cpu_t*);
void psx_cpu_i_slti(psx_cpu_t*);
void psx_cpu_i_sltiu(psx_cpu_t*);
void psx_cpu_i_andi(psx_cpu_t*);
void psx_cpu_i_ori(psx_cpu_t*);
void psx_cpu_i_xori(psx_cpu_t*);
void psx_cpu_i_lui(psx_cpu_t*);
void psx_cpu_i_cop0(psx_cpu_t*);
void psx_cpu_i_cop1(psx_cpu_t*);
void psx_cpu_i_cop2(psx_cpu_t*);
void psx_cpu_i_cop3(psx_cpu_t*);
void psx_cpu_i_lb(psx_cpu_t*);
void psx_cpu_i_lh(psx_cpu_t*);
void psx_cpu_i_lwl(psx_cpu_t*);
void psx_cpu_i_lw(psx_cpu_t*);
void psx_cpu_i_lbu(psx_cpu_t*);
void psx_cpu_i_lhu(psx_cpu_t*);
void psx_cpu_i_lwr(psx_cpu_t*);
void psx_cpu_i_sb(psx_cpu_t*);
void psx_cpu_i_sh(psx_cpu_t*);
void psx_cpu_i_swl(psx_cpu_t*);
void psx_cpu_i_sw(psx_cpu_t*);
void psx_cpu_i_swr(psx_cpu_t*);
void psx_cpu_i_lwc0(psx_cpu_t*);
void psx_cpu_i_lwc1(psx_cpu_t*);
void psx_cpu_i_lwc2(psx_cpu_t*);
void psx_cpu_i_lwc3(psx_cpu_t*);
void psx_cpu_i_swc0(psx_cpu_t*);
void psx_cpu_i_swc1(psx_cpu_t*);
void psx_cpu_i_swc2(psx_cpu_t*);
void psx_cpu_i_swc3(psx_cpu_t*);

// Secondary
void psx_cpu_i_sll(psx_cpu_t*);
void psx_cpu_i_srl(psx_cpu_t*);
void psx_cpu_i_sra(psx_cpu_t*);
void psx_cpu_i_sllv(psx_cpu_t*);
void psx_cpu_i_srlv(psx_cpu_t*);
void psx_cpu_i_srav(psx_cpu_t*);
void psx_cpu_i_jr(psx_cpu_t*);
void psx_cpu_i_jalr(psx_cpu_t*);
void psx_cpu_i_syscall(psx_cpu_t*);
void psx_cpu_i_break(psx_cpu_t*);
void psx_cpu_i_mfhi(psx_cpu_t*);
void psx_cpu_i_mthi(psx_cpu_t*);
void psx_cpu_i_mflo(psx_cpu_t*);
void psx_cpu_i_mtlo(psx_cpu_t*);
void psx_cpu_i_mult(psx_cpu_t*);
void psx_cpu_i_multu(psx_cpu_t*);
void psx_cpu_i_div(psx_cpu_t*);
void psx_cpu_i_divu(psx_cpu_t*);
void psx_cpu_i_add(psx_cpu_t*);
void psx_cpu_i_addu(psx_cpu_t*);
void psx_cpu_i_sub(psx_cpu_t*);
void psx_cpu_i_subu(psx_cpu_t*);
void psx_cpu_i_and(psx_cpu_t*);
void psx_cpu_i_or(psx_cpu_t*);
void psx_cpu_i_xor(psx_cpu_t*);
void psx_cpu_i_nor(psx_cpu_t*);
void psx_cpu_i_slt(psx_cpu_t*);
void psx_cpu_i_sltu(psx_cpu_t*);

// COP0
void psx_cpu_i_mfc0(psx_cpu_t*);
void psx_cpu_i_cfc0(psx_cpu_t*);
void psx_cpu_i_mtc0(psx_cpu_t*);
void psx_cpu_i_ctc0(psx_cpu_t*);
void psx_cpu_i_bc0c(psx_cpu_t*);

// Unimplemented
// void psx_cpu_i_cop0_tlbr(psx_cpu_t*);
// void psx_cpu_i_cop0_tlbwi(psx_cpu_t*);
// void psx_cpu_i_cop0_tlbwr(psx_cpu_t*);
// void psx_cpu_i_cop0_tlbp(psx_cpu_t*);

// COP0-specific
void psx_cpu_i_rfe(psx_cpu_t*);

typedef void (*psx_cpu_instruction_t)(psx_cpu_t*);

#endif