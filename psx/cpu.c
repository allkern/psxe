#include "cpu.h"
#include "bus.h"

#include <stdlib.h>
#include <string.h>

psx_cpu_t* psx_cpu_create() {
    return (psx_cpu_t*)malloc(sizeof(psx_cpu_t));
}

void psx_cpu_init(psx_cpu_t* cpu, psx_bus_t* bus) {
    memset(cpu, 0, sizeof(psx_cpu_t));

    cpu->bus = bus;
    cpu->pc = 0xbfc00000;

    cpu->buf[0] = psx_bus_read32(cpu->bus, cpu->pc);

    cpu->pc += 4;
}

void psx_cpu_destroy(psx_cpu_t* cpu) {
    free(cpu);
}

/*
    SLL     N/A     SRL     SRA
    SLLV    N/A     SRLV    SRAV
    JR      JALR    N/A     N/A
    SYSCALL BREAK   N/A     N/A
    MFHI    MTHI    MFLO    MTLO
    N/A     N/A     N/A     N/A
    MULT    MULTU   DIV     DIVU
    N/A     N/A     N/A     N/A
    ADD     ADDU    SUB     SUBU
    AND     OR      XOR     NOR
    N/A     N/A     SLT     SLTU
    N/A     N/A     N/A     N/A
    N/A     N/A     N/A     N/A
    N/A     N/A     N/A     N/A
    N/A     N/A     N/A     N/A
    N/A     N/A     N/A     N/A
*/

psx_cpu_instruction_t g_psx_cpu_secondary_table[] = {
    psx_cpu_i_sll    , psx_cpu_i_invalid, psx_cpu_i_srl    , psx_cpu_i_sra    ,
    psx_cpu_i_sllv   , psx_cpu_i_invalid, psx_cpu_i_srlv   , psx_cpu_i_srav   ,
    psx_cpu_i_jr     , psx_cpu_i_jalr   , psx_cpu_i_invalid, psx_cpu_i_invalid,
    psx_cpu_i_syscall, psx_cpu_i_break  , psx_cpu_i_invalid, psx_cpu_i_invalid,
    psx_cpu_i_mfhi   , psx_cpu_i_mthi   , psx_cpu_i_mflo   , psx_cpu_i_mtlo   ,
    psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid,
    psx_cpu_i_mult   , psx_cpu_i_multu  , psx_cpu_i_div    , psx_cpu_i_divu   ,
    psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid,
    psx_cpu_i_add    , psx_cpu_i_addu   , psx_cpu_i_sub    , psx_cpu_i_subu   ,
    psx_cpu_i_and    , psx_cpu_i_or     , psx_cpu_i_xor    , psx_cpu_i_nor    ,
    psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_slt    , psx_cpu_i_sltu   ,
    psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid,
    psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid,
    psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid,
    psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid,
    psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid
};

/*
    SPC     BCZ     J       JAL
    BEQ     BNE     BLEZ    BGTZ
    ADDI    ADDIU   SLTI    SLTIU
    ANDI    ORI     XORI    LUI
    COP0    COP1    COP2    COP3
    N/A     N/A     N/A     N/A
    N/A     N/A     N/A     N/A
    N/A     N/A     N/A     N/A
    LB      LH      LWL     LW
    LBU     LHU     LWR     N/A
    SB      SH      SWL     SW
    N/A     N/A     SWR     N/A
    LWC0    LWC1    LWC2    LWC3
    SWC0    SWC1    SWC2    SWC3
    N/A     N/A     N/A     N/A
*/

psx_cpu_instruction_t g_psx_cpu_primary_table[] = {
    psx_cpu_i_special, psx_cpu_i_bcz    , psx_cpu_i_j      , psx_cpu_i_jal    ,
    psx_cpu_i_beq    , psx_cpu_i_bne    , psx_cpu_i_blez   , psx_cpu_i_bgtz   ,
    psx_cpu_i_addi   , psx_cpu_i_addiu  , psx_cpu_i_slti   , psx_cpu_i_sltiu  ,
    psx_cpu_i_andi   , psx_cpu_i_ori    , psx_cpu_i_xori   , psx_cpu_i_lui    ,
    psx_cpu_i_cop0   , psx_cpu_i_cop1   , psx_cpu_i_cop2   , psx_cpu_i_cop3   ,
    psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid,
    psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid,
    psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid,
    psx_cpu_i_lb     , psx_cpu_i_lh     , psx_cpu_i_lwl    , psx_cpu_i_lw     ,
    psx_cpu_i_lbu    , psx_cpu_i_lhu    , psx_cpu_i_lwr    , psx_cpu_i_invalid,
    psx_cpu_i_sb     , psx_cpu_i_sh     , psx_cpu_i_swl    , psx_cpu_i_sw     ,
    psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_swr    , psx_cpu_i_invalid,
    psx_cpu_i_lwc0   , psx_cpu_i_lwc1   , psx_cpu_i_lwc2   , psx_cpu_i_lwc3   ,
    psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid,
    psx_cpu_i_swc0   , psx_cpu_i_swc1   , psx_cpu_i_swc2   , psx_cpu_i_swc3   ,
    psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid
};

psx_cpu_instruction_t g_psx_cpu_cop0_table[] = {
    psx_cpu_i_mfc0   , psx_cpu_i_invalid, psx_cpu_i_cfc0   , psx_cpu_i_invalid,
    psx_cpu_i_mtc0   , psx_cpu_i_invalid, psx_cpu_i_ctc0   , psx_cpu_i_invalid,
    psx_cpu_i_bc0c   , psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid,
    psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid,
    psx_cpu_i_rfe    , psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid,
    psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid,
    psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid,
    psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid
};

#define OP ((cpu->buf[1] >> 26) & 0x3f) 
#define S ((cpu->buf[1] >> 21) & 0x1f)
#define T ((cpu->buf[1] >> 16) & 0x1f)
#define D ((cpu->buf[1] >> 11) & 0x1f)
#define IMM5 ((cpu->buf[1] >> 6) & 0x1f)
#define CMT ((cpu->buf[1] >> 6) & 0xfffff)
#define SOP (cpu->buf[1] & 0x3f)
#define IMM26 (cpu->buf[1] & 0x3ffffff)
#define IMM16 (cpu->buf[1] & 0xffff)
#define IMM16S ((int32_t)((int16_t)IMM16))

#define TRACE_M(m) \
    log_trace("%08x: %-7s $%s, $%04x($%s)", cpu->pc-8, m, g_mips_cc_register_names[T], IMM16S, g_mips_cc_register_names[S])

#define TRACE_I16S(m) \
    log_trace("%08x: %-7s $%s, $%04x", cpu->pc-8, m, g_mips_cc_register_names[T], IMM16)

#define TRACE_I16D(m) \
    log_trace("%08x: %-7s $%s, $%s, $%04x", cpu->pc-8, m, g_mips_cc_register_names[T], g_mips_cc_register_names[S], IMM16)

#define TRACE_I5D(m) \
    log_trace("%08x: %-7s $%s, $%s, %u", cpu->pc-8, m, g_mips_cc_register_names[T], g_mips_cc_register_names[S], IMM5)

#define TRACE_I26(m) \
    log_trace("%08x: %-7s $%07x", cpu->pc-8, m, ((cpu->pc & 0xf0000000) | (IMM26 << 2)))

#define TRACE_RT(m) \
    log_trace("%08x: %-7s $%s, $%s, $%s", cpu->pc-8, m, g_mips_cc_register_names[D], g_mips_cc_register_names[S], g_mips_cc_register_names[T])

const char* g_mips_cc_register_names[] = {
    "r0", "at", "v0", "v1", "a0", "a1", "a2", "a3",
    "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
    "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
    "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra"
};

void psx_cpu_cycle(psx_cpu_t* cpu) {
    cpu->buf[1] = cpu->buf[0];
    cpu->buf[0] = psx_bus_read32(cpu->bus, cpu->pc);

    cpu->pc += 4;

    g_psx_cpu_primary_table[OP](cpu);
}

void psx_cpu_i_invalid(psx_cpu_t* cpu) {
    log_error("Instruction invalid unimplemented");

    exit(1);
}

// Primary
void psx_cpu_i_special(psx_cpu_t* cpu) {
    g_psx_cpu_secondary_table[SOP](cpu);
}

void psx_cpu_i_bcz(psx_cpu_t* cpu) {
    log_error("Instruction bcz unimplemented");

    exit(1);
}

void psx_cpu_i_j(psx_cpu_t* cpu) {
    TRACE_I26("j");

    cpu->pc = (cpu->pc & 0xf0000000) | (IMM26 << 2);
}

void psx_cpu_i_jal(psx_cpu_t* cpu) {
    log_error("Instruction jal unimplemented");

    exit(1);
}

void psx_cpu_i_beq(psx_cpu_t* cpu) {
    log_error("Instruction beq unimplemented");

    exit(1);
}

void psx_cpu_i_bne(psx_cpu_t* cpu) {
    log_error("Instruction bne unimplemented");

    exit(1);
}

void psx_cpu_i_blez(psx_cpu_t* cpu) {
    log_error("Instruction blez unimplemented");

    exit(1);
}

void psx_cpu_i_bgtz(psx_cpu_t* cpu) {
    log_error("Instruction bgtz unimplemented");

    exit(1);
}

void psx_cpu_i_addi(psx_cpu_t* cpu) {
    log_error("Instruction addi unimplemented");

    exit(1);
}

void psx_cpu_i_addiu(psx_cpu_t* cpu) {
    TRACE_I16D("addiu");

    cpu->r[T] = cpu->r[S] + IMM16S;
}

void psx_cpu_i_slti(psx_cpu_t* cpu) {
    log_error("Instruction slti unimplemented");

    exit(1);
}

void psx_cpu_i_sltiu(psx_cpu_t* cpu) {
    log_error("Instruction sltiu unimplemented");

    exit(1);
}

void psx_cpu_i_andi(psx_cpu_t* cpu) {
    log_error("Instruction andi unimplemented");

    exit(1);
}

void psx_cpu_i_ori(psx_cpu_t* cpu) {
    TRACE_I16D("ori");

    cpu->r[T] = cpu->r[S] | IMM16;
}

void psx_cpu_i_xori(psx_cpu_t* cpu) {
    log_error("Instruction xori unimplemented");

    exit(1);
}

void psx_cpu_i_lui(psx_cpu_t* cpu) {
    TRACE_I16S("lui");

    cpu->r[T] = IMM16 << 16;
}

void psx_cpu_i_cop0(psx_cpu_t* cpu) {
    g_psx_cpu_cop0_table[S](cpu);
}

void psx_cpu_i_cop1(psx_cpu_t* cpu) {
    log_error("Instruction cop1 unimplemented");

    exit(1);
}

void psx_cpu_i_cop2(psx_cpu_t* cpu) {
    log_error("Instruction cop2 unimplemented");

    exit(1);
}

void psx_cpu_i_cop3(psx_cpu_t* cpu) {
    log_error("Instruction cop3 unimplemented");

    exit(1);
}

void psx_cpu_i_lb(psx_cpu_t* cpu) {
    log_error("Instruction lb unimplemented");

    exit(1);
}

void psx_cpu_i_lh(psx_cpu_t* cpu) {
    log_error("Instruction lh unimplemented");

    exit(1);
}

void psx_cpu_i_lwl(psx_cpu_t* cpu) {
    log_error("Instruction lwl unimplemented");

    exit(1);
}

void psx_cpu_i_lw(psx_cpu_t* cpu) {
    log_error("Instruction lw unimplemented");

    exit(1);
}

void psx_cpu_i_lbu(psx_cpu_t* cpu) {
    log_error("Instruction lbu unimplemented");

    exit(1);
}

void psx_cpu_i_lhu(psx_cpu_t* cpu) {
    log_error("Instruction lhu unimplemented");

    exit(1);
}

void psx_cpu_i_lwr(psx_cpu_t* cpu) {
    log_error("Instruction lwr unimplemented");

    exit(1);
}

void psx_cpu_i_sb(psx_cpu_t* cpu) {
    log_error("Instruction sb unimplemented");

    exit(1);
}

void psx_cpu_i_sh(psx_cpu_t* cpu) {
    log_error("Instruction sh unimplemented");

    exit(1);
}

void psx_cpu_i_swl(psx_cpu_t* cpu) {
    log_error("Instruction swl unimplemented");

    exit(1);
}

void psx_cpu_i_sw(psx_cpu_t* cpu) {
    TRACE_M("sw");

    psx_bus_write32(cpu->bus, cpu->r[S] + IMM16S, cpu->r[T]);
}

void psx_cpu_i_swr(psx_cpu_t* cpu) {
    log_error("Instruction swr unimplemented");

    exit(1);
}

void psx_cpu_i_lwc0(psx_cpu_t* cpu) {
    log_error("Instruction lwc0 unimplemented");

    exit(1);
}

void psx_cpu_i_lwc1(psx_cpu_t* cpu) {
    log_error("Instruction lwc1 unimplemented");

    exit(1);
}

void psx_cpu_i_lwc2(psx_cpu_t* cpu) {
    log_error("Instruction lwc2 unimplemented");

    exit(1);
}

void psx_cpu_i_lwc3(psx_cpu_t* cpu) {
    log_error("Instruction lwc3 unimplemented");

    exit(1);
}

void psx_cpu_i_swc0(psx_cpu_t* cpu) {
    log_error("Instruction swc0 unimplemented");

    exit(1);
}

void psx_cpu_i_swc1(psx_cpu_t* cpu) {
    log_error("Instruction swc1 unimplemented");

    exit(1);
}

void psx_cpu_i_swc2(psx_cpu_t* cpu) {
    log_error("Instruction swc2 unimplemented");

    exit(1);
}

void psx_cpu_i_swc3(psx_cpu_t* cpu) {
    log_error("Instruction swc3 unimplemented");

    exit(1);
}

// Secondary
void psx_cpu_i_sll(psx_cpu_t* cpu) {
    TRACE_I5D("sll");

    cpu->r[T] = cpu->r[S] << IMM5;
}

void psx_cpu_i_srl(psx_cpu_t* cpu) {
    log_error("Instruction srl unimplemented");

    exit(1);
}

void psx_cpu_i_sra(psx_cpu_t* cpu) {
    log_error("Instruction sra unimplemented");

    exit(1);
}

void psx_cpu_i_sllv(psx_cpu_t* cpu) {
    log_error("Instruction sllv unimplemented");

    exit(1);
}

void psx_cpu_i_srlv(psx_cpu_t* cpu) {
    log_error("Instruction srlv unimplemented");

    exit(1);
}

void psx_cpu_i_srav(psx_cpu_t* cpu) {
    log_error("Instruction srav unimplemented");

    exit(1);
}

void psx_cpu_i_jr(psx_cpu_t* cpu) {
    log_error("Instruction jr unimplemented");

    exit(1);
}

void psx_cpu_i_jalr(psx_cpu_t* cpu) {
    log_error("Instruction jalr unimplemented");

    exit(1);
}

void psx_cpu_i_syscall(psx_cpu_t* cpu) {
    log_error("Instruction syscall unimplemented");

    exit(1);
}

void psx_cpu_i_break(psx_cpu_t* cpu) {
    log_error("Instruction break unimplemented");

    exit(1);
}

void psx_cpu_i_mfhi(psx_cpu_t* cpu) {
    log_error("Instruction mfhi unimplemented");

    exit(1);
}

void psx_cpu_i_mthi(psx_cpu_t* cpu) {
    log_error("Instruction mthi unimplemented");

    exit(1);
}

void psx_cpu_i_mflo(psx_cpu_t* cpu) {
    log_error("Instruction mflo unimplemented");

    exit(1);
}

void psx_cpu_i_mtlo(psx_cpu_t* cpu) {
    log_error("Instruction mtlo unimplemented");

    exit(1);
}

void psx_cpu_i_mult(psx_cpu_t* cpu) {
    log_error("Instruction mult unimplemented");

    exit(1);
}

void psx_cpu_i_multu(psx_cpu_t* cpu) {
    log_error("Instruction multu unimplemented");

    exit(1);
}

void psx_cpu_i_div(psx_cpu_t* cpu) {
    log_error("Instruction div unimplemented");

    exit(1);
}

void psx_cpu_i_divu(psx_cpu_t* cpu) {
    log_error("Instruction divu unimplemented");

    exit(1);
}

void psx_cpu_i_add(psx_cpu_t* cpu) {
    log_error("Instruction add unimplemented");

    exit(1);
}

void psx_cpu_i_addu(psx_cpu_t* cpu) {
    log_error("Instruction addu unimplemented");

    exit(1);
}

void psx_cpu_i_sub(psx_cpu_t* cpu) {
    log_error("Instruction sub unimplemented");

    exit(1);
}

void psx_cpu_i_subu(psx_cpu_t* cpu) {
    log_error("Instruction subu unimplemented");

    exit(1);
}

void psx_cpu_i_and(psx_cpu_t* cpu) {
    log_error("Instruction and unimplemented");

    exit(1);
}

void psx_cpu_i_or(psx_cpu_t* cpu) {
    TRACE_RT("or");

    cpu->r[D] = cpu->r[S] | cpu->r[T];
}

void psx_cpu_i_xor(psx_cpu_t* cpu) {
    log_error("Instruction xor unimplemented");

    exit(1);
}

void psx_cpu_i_nor(psx_cpu_t* cpu) {
    log_error("Instruction nor unimplemented");

    exit(1);
}

void psx_cpu_i_slt(psx_cpu_t* cpu) {
    log_error("Instruction slt unimplemented");

    exit(1);
}

void psx_cpu_i_sltu(psx_cpu_t* cpu) {
    log_error("Instruction sltu unimplemented");

    exit(1);
}

// COP0
void psx_cpu_i_mfc0(psx_cpu_t* cpu) {
    log_error("Instruction mfc0 unimplemented");

    exit(1);
}

void psx_cpu_i_cfc0(psx_cpu_t* cpu) {
    log_error("Instruction cfc0 unimplemented");

    exit(1);
}

void psx_cpu_i_mtc0(psx_cpu_t* cpu) {
    log_error("Instruction mtc0 unimplemented");

    exit(1);
}

void psx_cpu_i_ctc0(psx_cpu_t* cpu) {
    log_error("Instruction ctc0 unimplemented");

    exit(1);
}

void psx_cpu_i_bc0c(psx_cpu_t* cpu) {
    log_error("Instruction bc0c unimplemented");

    exit(1);
}

// COP0-specific
void psx_cpu_i_rfe(psx_cpu_t* cpu) {
    log_error("Instruction rfe unimplemented");

    exit(1);
}

#undef OP
#undef S
#undef T
#undef D
#undef IMM5
#undef CMT
#undef SOP
#undef IMM26
#undef IMM16

#undef TRACE_M
#undef TRACE_I16S
#undef TRACE_I16D
#undef TRACE_I5D
#undef TRACE_I26
#undef TRACE_RT