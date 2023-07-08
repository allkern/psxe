#include "cpu.h"
#include "bus.h"
#include "log.h"

#include <stdlib.h>
#include <string.h>

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

psx_cpu_instruction_t g_psx_cpu_primary_table[] = {
    psx_cpu_i_special, psx_cpu_i_bxx    , psx_cpu_i_j      , psx_cpu_i_jal    ,
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
    psx_cpu_i_mfc0   , psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid,
    psx_cpu_i_mtc0   , psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid,
    psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid,
    psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid,
    psx_cpu_i_rfe    , psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid,
    psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid,
    psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid,
    psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid
};

psx_cpu_instruction_t g_psx_cpu_cop2_table[] = {
    psx_cpu_i_mfc2   , psx_cpu_i_invalid, psx_cpu_i_cfc2   , psx_cpu_i_invalid,
    psx_cpu_i_mtc2   , psx_cpu_i_invalid, psx_cpu_i_ctc2   , psx_cpu_i_invalid,
    psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid,
    psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid,
    psx_cpu_i_gte    , psx_cpu_i_gte    , psx_cpu_i_gte    , psx_cpu_i_gte    ,
    psx_cpu_i_gte    , psx_cpu_i_gte    , psx_cpu_i_gte    , psx_cpu_i_gte    ,
    psx_cpu_i_gte    , psx_cpu_i_gte    , psx_cpu_i_gte    , psx_cpu_i_gte    ,
    psx_cpu_i_gte    , psx_cpu_i_gte    , psx_cpu_i_gte    , psx_cpu_i_gte
};

psx_cpu_instruction_t g_psx_cpu_bxx_table[] = {
    psx_cpu_i_bltz   , psx_cpu_i_bgez   , psx_cpu_i_bltz   , psx_cpu_i_bgez   ,
    psx_cpu_i_bltz   , psx_cpu_i_bgez   , psx_cpu_i_bltz   , psx_cpu_i_bgez   ,
    psx_cpu_i_bltz   , psx_cpu_i_bgez   , psx_cpu_i_bltz   , psx_cpu_i_bgez   ,
    psx_cpu_i_bltz   , psx_cpu_i_bgez   , psx_cpu_i_bltz   , psx_cpu_i_bgez   ,
    psx_cpu_i_bltzal , psx_cpu_i_bgezal , psx_cpu_i_bltz   , psx_cpu_i_bgez   ,
    psx_cpu_i_bltz   , psx_cpu_i_bgez   , psx_cpu_i_bltz   , psx_cpu_i_bgez   ,
    psx_cpu_i_bltz   , psx_cpu_i_bgez   , psx_cpu_i_bltz   , psx_cpu_i_bgez   ,
    psx_cpu_i_bltz   , psx_cpu_i_bgez   , psx_cpu_i_bltz   , psx_cpu_i_bgez
};

/*
  Opc  Name      Clk Expl.
  00h  -             N/A (modifies similar registers than RTPS...)
  01h  RTPS      15  Perspective Transformation single
  0xh  -             N/A
  06h  NCLIP     8   Normal clipping
  0xh  -             N/A
  0Ch  OP(sf)    6   Outer product of 2 vectors
  0xh  -             N/A
  10h  DPCS      8   Depth Cueing single
  11h  INTPL     8   Interpolation of a vector and far color vector
  12h  MVMVA(..) 8   Multiply vector by matrix and add vector (see below)
  13h  NCDS      19  Normal color depth cue single vector
  14h  CDP       13  Color Depth Que
  15h  -             N/A
  16h  NCDT      44  Normal color depth cue triple vectors
  1xh  -             N/A
  1Bh  NCCS      17  Normal Color Color single vector
  1Ch  CC        11  Color Color
  1Dh  -             N/A
  1Eh  NCS       14  Normal color single
  1Fh  -             N/A
  20h  NCT       30  Normal color triple
  2xh  -             N/A
  28h  SQR(sf)   5   Square of vector IR
  29h  DCPL      8   Depth Cue Color light
  2Ah  DPCT      17  Depth Cueing triple (should be fake=08h, but isn't)
  2xh  -             N/A
  2Dh  AVSZ3     5   Average of three Z values
  2Eh  AVSZ4     6   Average of four Z values
  2Fh  -             N/A
  30h  RTPT      23  Perspective Transformation triple
  3xh  -             N/A
  3Dh  GPF(sf)   5   General purpose interpolation
  3Eh  GPL(sf)   5   General purpose interpolation with base
  3Fh  NCCT      39  Normal Color Color triple vector
*/

psx_cpu_instruction_t g_psx_gte_table[] = {
    psx_gte_i_invalid, psx_gte_i_rtps   , psx_gte_i_invalid, psx_gte_i_invalid,
    psx_gte_i_invalid, psx_gte_i_invalid, psx_gte_i_nclip  , psx_gte_i_invalid,
    psx_gte_i_invalid, psx_gte_i_invalid, psx_gte_i_invalid, psx_gte_i_invalid,
    psx_gte_i_op     , psx_gte_i_invalid, psx_gte_i_invalid, psx_gte_i_invalid,
    psx_gte_i_dpcs   , psx_gte_i_intpl  , psx_gte_i_mvmva  , psx_gte_i_ncds   ,
    psx_gte_i_cdp    , psx_gte_i_invalid, psx_gte_i_ncdt   , psx_gte_i_invalid,
    psx_gte_i_invalid, psx_gte_i_invalid, psx_gte_i_invalid, psx_gte_i_nccs   ,
    psx_gte_i_cc     , psx_gte_i_invalid, psx_gte_i_ncs    , psx_gte_i_invalid,
    psx_gte_i_nct    , psx_gte_i_invalid, psx_gte_i_invalid, psx_gte_i_invalid,
    psx_gte_i_invalid, psx_gte_i_invalid, psx_gte_i_invalid, psx_gte_i_invalid,
    psx_gte_i_sqr    , psx_gte_i_dcpl   , psx_gte_i_dpct   , psx_gte_i_invalid,
    psx_gte_i_invalid, psx_gte_i_avsz3  , psx_gte_i_avsz4  , psx_gte_i_invalid,
    psx_gte_i_rtpt   , psx_gte_i_invalid, psx_gte_i_invalid, psx_gte_i_invalid,
    psx_gte_i_invalid, psx_gte_i_invalid, psx_gte_i_invalid, psx_gte_i_invalid,
    psx_gte_i_invalid, psx_gte_i_invalid, psx_gte_i_invalid, psx_gte_i_invalid,
    psx_gte_i_invalid, psx_gte_i_gpf    , psx_gte_i_gpl    , psx_gte_i_ncct
};

uint32_t g_psx_cpu_cop0_write_mask_table[] = {
    0x00000000, // cop0r0-r2   - N/A
    0x00000000, // cop0r0-r2   - N/A
    0x00000000, // cop0r0-r2   - N/A
    0xffffffff, // cop0r3      - BPC - Breakpoint on execute (R/W)
    0x00000000, // cop0r4      - N/A
    0xffffffff, // cop0r5      - BDA - Breakpoint on data access (R/W)
    0x00000000, // cop0r6      - JUMPDEST - Randomly memorized jump address (R)
    0xffc0f03f, // cop0r7      - DCIC - Breakpoint control (R/W)
    0x00000000, // cop0r8      - BadVaddr - Bad Virtual Address (R)
    0xffffffff, // cop0r9      - BDAM - Data Access breakpoint mask (R/W)
    0x00000000, // cop0r10     - N/A
    0xffffffff, // cop0r11     - BPCM - Execute breakpoint mask (R/W)
    0xffffffff, // cop0r12     - SR - System status register (R/W)
    0x00000300, // cop0r13     - CAUSE - (R)  Describes the most recently recognised exception
    0x00000000, // cop0r14     - EPC - Return Address from Trap (R)
    0x00000000  // cop0r15     - PRID - Processor ID (R)
};

#define OP ((cpu->opcode >> 26) & 0x3f)
#define S ((cpu->opcode >> 21) & 0x1f)
#define T ((cpu->opcode >> 16) & 0x1f)
#define D ((cpu->opcode >> 11) & 0x1f)
#define IMM5 ((cpu->opcode >> 6) & 0x1f)
#define CMT ((cpu->opcode >> 6) & 0xfffff)
#define SOP (cpu->opcode & 0x3f)
#define IMM26 (cpu->opcode & 0x3ffffff)
#define IMM16 (cpu->opcode & 0xffff)
#define IMM16S ((int32_t)((int16_t)IMM16))

#define R_R0 (cpu->r[0])
#define R_AT (cpu->r[1])
#define R_V0 (cpu->r[2])
#define R_V1 (cpu->r[3])
#define R_A0 (cpu->r[4])
#define R_A1 (cpu->r[5])
#define R_A2 (cpu->r[6])
#define R_A3 (cpu->r[7])
#define R_T0 (cpu->r[8])
#define R_T1 (cpu->r[9])
#define R_T2 (cpu->r[10])
#define R_T3 (cpu->r[11])
#define R_T4 (cpu->r[12])
#define R_T5 (cpu->r[13])
#define R_T6 (cpu->r[14])
#define R_T7 (cpu->r[15])
#define R_S0 (cpu->r[16])
#define R_S1 (cpu->r[17])
#define R_S2 (cpu->r[18])
#define R_S3 (cpu->r[19])
#define R_S4 (cpu->r[20])
#define R_S5 (cpu->r[21])
#define R_S6 (cpu->r[22])
#define R_S7 (cpu->r[23])
#define R_T8 (cpu->r[24])
#define R_T9 (cpu->r[25])
#define R_K0 (cpu->r[26])
#define R_K1 (cpu->r[27])
#define R_GP (cpu->r[28])
#define R_SP (cpu->r[29])
#define R_FP (cpu->r[30])
#define R_RA (cpu->r[31])

#define CPU_TRACE

#ifdef CPU_TRACE
#define TRACE_M(m) \
    log_trace("%08x: %-7s $%s, %+i($%s)", cpu->pc-8, m, g_mips_cc_register_names[T], IMM16S, g_mips_cc_register_names[S])

#define TRACE_I16S(m) \
    log_trace("%08x: %-7s $%s, 0x%04x", cpu->pc-8, m, g_mips_cc_register_names[T], IMM16)

#define TRACE_I16D(m) \
    log_trace("%08x: %-7s $%s, $%s, 0x%04x", cpu->pc-8, m, g_mips_cc_register_names[T], g_mips_cc_register_names[S], IMM16)

#define TRACE_I5D(m) \
    log_trace("%08x: %-7s $%s, $%s, %u", cpu->pc-8, m, g_mips_cc_register_names[D], g_mips_cc_register_names[T], IMM5)

#define TRACE_I26(m) \
    log_trace("%08x: %-7s 0x%07x", cpu->pc-8, m, ((cpu->pc & 0xf0000000) | (IMM26 << 2)))

#define TRACE_RT(m) \
    log_trace("%08x: %-7s $%s, $%s, $%s", cpu->pc-8, m, g_mips_cc_register_names[D], g_mips_cc_register_names[S], g_mips_cc_register_names[T])

#define TRACE_C0M(m) \
    log_trace("%08x: %-7s $%s, $%s", cpu->pc-8, m, g_mips_cc_register_names[T], g_mips_cop0_register_names[D])

#define TRACE_B(m) \
    log_trace("%08x: %-7s $%s, $%s, %-i", cpu->pc-8, m, g_mips_cc_register_names[S], g_mips_cc_register_names[T], IMM16S << 2)

#define TRACE_RS(m) \
    log_trace("%08x: %-7s $%s", cpu->pc-8, m, g_mips_cc_register_names[S])

#define TRACE_MTF(m) \
    log_trace("%08x: %-7s $%s", cpu->pc-8, m, g_mips_cc_register_names[D])

#define TRACE_RD(m) \
    log_trace("%08x: %-7s $%s, $%s", cpu->pc-8, m, g_mips_cc_register_names[D], g_mips_cc_register_names[S])

#define TRACE_MD(m) \
    log_trace("%08x: %-7s $%s, $%s", cpu->pc-8, m, g_mips_cc_register_names[S], g_mips_cc_register_names[T]);

#define TRACE_I20(m) \
    log_trace("%08x: %-7s 0x%05x", cpu->pc-8, m, CMT);

#define TRACE_N(m) \
    log_trace("%08x: %-7s", cpu->pc-8, m);
#else
#define TRACE_M(m)
#define TRACE_I16S(m)
#define TRACE_I16D(m)
#define TRACE_I5D(m)
#define TRACE_I26(m)
#define TRACE_RT(m)
#define TRACE_C0M(m)
#define TRACE_B(m)
#define TRACE_RS(m)
#define TRACE_MTF(m)
#define TRACE_RD(m)
#define TRACE_MD(m)
#define TRACE_I20(m)
#define TRACE_N(m)
#endif

#define DO_PENDING_LOAD \
    cpu->r[cpu->load_d] = cpu->load_v; \
    R_R0 = 0; \
    cpu->load_v = 0xffffffff; \
    cpu->load_d = 0;

#define DEBUG_ALL \
    log_fatal("r0=%08x at=%08x v0=%08x v1=%08x", R_R0, R_AT, R_V0, R_V1); \
    log_fatal("a0=%08x a1=%08x a2=%08x a3=%08x", R_A0, R_A1, R_A2, R_A3); \
    log_fatal("t0=%08x t1=%08x t2=%08x t3=%08x", R_T0, R_T1, R_T2, R_T3); \
    log_fatal("t4=%08x t5=%08x t6=%08x t7=%08x", R_T4, R_T5, R_T6, R_T7); \
    log_fatal("s0=%08x s1=%08x s2=%08x s3=%08x", R_S0, R_S1, R_S2, R_S3); \
    log_fatal("s4=%08x s5=%08x s6=%08x s7=%08x", R_S4, R_S5, R_S6, R_S7); \
    log_fatal("t8=%08x t9=%08x k0=%08x k1=%08x", R_T8, R_T9, R_K0, R_K1); \
    log_fatal("gp=%08x sp=%08x fp=%08x ra=%08x", R_GP, R_SP, R_FP, R_RA); \
    log_fatal("pc=%08x hi=%08x lo=%08x l:%s=%08x", cpu->pc, cpu->hi, cpu->lo, g_mips_cc_register_names[cpu->load_d], cpu->load_v); \
    exit(1)

#define SE8(v) ((int32_t)((int8_t)v))
#define SE16(v) ((int32_t)((int16_t)v))

#define BRANCH(offset) \
    cpu->next_pc = cpu->next_pc + (offset); \
    cpu->next_pc = cpu->next_pc - 4; \
    cpu->branch = 1; \
    cpu->branch_taken = 1;

const char* g_mips_cop0_register_names[] = {
    "cop0_r0",
    "cop0_r1",
    "cop0_r2",
    "cop0_bpc",
    "cop0_r4",
    "cop0_bda",
    "cop0_jumpdest",
    "cop0_dcic",
    "cop0_badvaddr",
    "cop0_bdam",
    "cop0_r10",
    "cop0_bpcm",
    "cop0_sr",
    "cop0_cause",
    "cop0_epc",
    "cop0_prid"
};

const char* g_mips_cc_register_names[] = {
    "r0", "at", "v0", "v1", "a0", "a1", "a2", "a3",
    "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
    "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
    "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra"
};

const char* g_psx_cpu_syscall_function_symbol_table[] = {
    "NoFunction",
    "EnterCriticalSection",
    "ExitCriticalSection",
    "ChangeThreadSubFunction"
    // DeliverEvent (invalid)
};

void cpu_a_kcall_hook(psx_cpu_t* cpu) {
    switch (cpu->r[9]) {
        case 0x09: putc(R_A0, stdout); break;
        case 0x3c: putchar(R_A0); break;
        case 0x3e: {
            uint32_t src = R_A0;

            char c = psx_bus_read8(cpu->bus, src++);

            while (c) {
                putchar(c);

                c = psx_bus_read8(cpu->bus, src++);
            }
        } break;
    }
}

void cpu_b_kcall_hook(psx_cpu_t* cpu) {
    switch (cpu->r[9]) {
        case 0x3b: putc(R_A0, stdout); break;
        case 0x3d: putchar(R_A0); break;
        case 0x3f: {
            uint32_t src = R_A0;

            char c = psx_bus_read8(cpu->bus, src++);

            while (c) {
                putchar(c);

                c = psx_bus_read8(cpu->bus, src++);
            }
        } break;
    }
}

psx_cpu_t* psx_cpu_create() {
    return (psx_cpu_t*)malloc(sizeof(psx_cpu_t));
}

void cpu_a_kcall_hook(psx_cpu_t*);
void cpu_b_kcall_hook(psx_cpu_t*);

void psx_cpu_fetch(psx_cpu_t* cpu) {
    //cpu->buf[0] = psx_bus_read32(cpu->bus, cpu->pc);
    //cpu->pc += 4;

    // Discard fetch cycles
    psx_bus_get_access_cycles(cpu->bus);
}

void psx_cpu_destroy(psx_cpu_t* cpu) {
    free(cpu);
}

void psx_cpu_set_a_kcall_hook(psx_cpu_t* cpu, psx_cpu_kcall_hook_t hook) {
    cpu->a_function_hook = hook;
}

void psx_cpu_set_b_kcall_hook(psx_cpu_t* cpu, psx_cpu_kcall_hook_t hook) {
    cpu->b_function_hook = hook;
}

void psx_cpu_save_state(psx_cpu_t* cpu, FILE* file) {
    fwrite((char*)cpu, sizeof(*cpu) - sizeof(psx_bus_t*), 1, file);
}

void psx_cpu_load_state(psx_cpu_t* cpu, FILE* file) {
    fread((char*)cpu, sizeof(*cpu) - sizeof(psx_bus_t*), 1, file);
}

void psx_cpu_init(psx_cpu_t* cpu, psx_bus_t* bus) {
    memset(cpu, 0, sizeof(psx_cpu_t));

    psx_cpu_set_a_kcall_hook(cpu, cpu_a_kcall_hook);
    psx_cpu_set_b_kcall_hook(cpu, cpu_b_kcall_hook);

    cpu->bus = bus;
    cpu->pc = 0xbfc00000;
    cpu->next_pc = cpu->pc + 4;

    cpu->cop0_r[COP0_SR] = 0x10900000;
    cpu->cop0_r[COP0_PRID] = 0x00000002;
}

void psx_cpu_cycle(psx_cpu_t* cpu) {
    if ((cpu->pc & 0x3fffffff) == 0x000000b4)
        if (cpu->b_function_hook) cpu->b_function_hook(cpu);

    cpu->saved_pc = cpu->pc;
    cpu->delay_slot = cpu->branch;
    cpu->branch = 0;
    cpu->branch_taken = 0;

    if (cpu->saved_pc & 3) {
        psx_cpu_exception(cpu, CAUSE_ADEL);
    }

    cpu->opcode = psx_bus_read32(cpu->bus, cpu->pc);

    cpu->pc = cpu->next_pc;
    cpu->next_pc += 4;

    if (psx_cpu_check_irq(cpu)) {
        psx_cpu_exception(cpu, CAUSE_INT);

        return;
    }

    g_psx_cpu_primary_table[OP](cpu);

    cpu->last_cycles = 2;
    cpu->total_cycles += cpu->last_cycles;

    cpu->r[0] = 0;
}

int psx_cpu_check_irq(psx_cpu_t* cpu) {
    return (cpu->cop0_r[COP0_SR] & SR_IEC) && (cpu->cop0_r[COP0_SR] & cpu->cop0_r[COP0_CAUSE] & 0x00000700);
}

void psx_cpu_exception(psx_cpu_t* cpu, uint32_t cause) {
    cpu->cop0_r[COP0_CAUSE] &= 0x0000ff00;

    // Set excode and clear 3 LSBs
    cpu->cop0_r[COP0_CAUSE] &= 0xffffff80;
    cpu->cop0_r[COP0_CAUSE] |= cause;

    cpu->cop0_r[COP0_EPC] = cpu->saved_pc;

    if (cpu->delay_slot) {
        cpu->cop0_r[COP0_EPC] -= 4;
        cpu->cop0_r[COP0_CAUSE] |= 0x80000000;
    }

    if ((cause == CAUSE_INT) && (cpu->cop0_r[COP0_EPC] & 0xfe000000) == 0x4a000000) {
        cpu->cop0_r[COP0_EPC] += 4;
    }

    // Do exception stack push
    uint32_t mode = cpu->cop0_r[COP0_SR] & 0x3f;

    cpu->cop0_r[COP0_SR] &= 0xffffffc0;
    cpu->cop0_r[COP0_SR] |= (mode << 2) & 0x3f;

    // Set PC to the vector selected on BEV
    cpu->pc = (cpu->cop0_r[COP0_SR] & SR_BEV) ? 0xbfc00180 : 0x80000080;
    cpu->next_pc = cpu->pc + 4;
}

void psx_cpu_irq(psx_cpu_t* cpu, uint32_t irq) {
    cpu->cop0_r[COP0_CAUSE] |= irq ? SR_IM2 : 0;
}

void psx_cpu_i_invalid(psx_cpu_t* cpu) {
    log_fatal("%08x: Illegal instruction %08x", cpu->pc - 8, cpu->opcode);

    psx_cpu_exception(cpu, CAUSE_RI);
}

// Primary
void psx_cpu_i_special(psx_cpu_t* cpu) {
    g_psx_cpu_secondary_table[SOP](cpu);
}

void psx_cpu_i_bxx(psx_cpu_t* cpu) {
    cpu->branch = 1;
    cpu->branch_taken = 0;

    g_psx_cpu_bxx_table[T](cpu);
}

// BXX
void psx_cpu_i_bltz(psx_cpu_t* cpu) {
    TRACE_B("bltz");

    int32_t s = (int32_t)cpu->r[S];

    DO_PENDING_LOAD;

    if ((int32_t)s < (int32_t)0) {
        BRANCH(IMM16S << 2);
    }
}

void psx_cpu_i_bgez(psx_cpu_t* cpu) {
    TRACE_B("bgez");

    int32_t s = (int32_t)cpu->r[S];

    DO_PENDING_LOAD;

    if ((int32_t)s >= (int32_t)0) {
        BRANCH(IMM16S << 2);
    }
}

void psx_cpu_i_bltzal(psx_cpu_t* cpu) {
    TRACE_B("bltzal");

    int32_t s = (int32_t)cpu->r[S];

    DO_PENDING_LOAD;

    R_RA = cpu->next_pc;

    if ((int32_t)s < (int32_t)0) {
        BRANCH(IMM16S << 2);
    }
}

void psx_cpu_i_bgezal(psx_cpu_t* cpu) {
    TRACE_B("bgezal");

    int32_t s = (int32_t)cpu->r[S];

    DO_PENDING_LOAD;

    R_RA = cpu->next_pc;

    if ((int32_t)s >= (int32_t)0) {
        BRANCH(IMM16S << 2);
    }
}

void psx_cpu_i_j(psx_cpu_t* cpu) {
    cpu->branch = 1;

    TRACE_I26("j");

    DO_PENDING_LOAD;

    cpu->next_pc = (cpu->next_pc & 0xf0000000) | (IMM26 << 2);
}

void psx_cpu_i_jal(psx_cpu_t* cpu) {
    cpu->branch = 1;

    TRACE_I26("jal");

    DO_PENDING_LOAD;

    R_RA = cpu->next_pc;

    cpu->next_pc = (cpu->next_pc & 0xf0000000) | (IMM26 << 2);
}

void psx_cpu_i_beq(psx_cpu_t* cpu) {
    cpu->branch = 1;
    cpu->branch_taken = 0;

    TRACE_B("beq");

    uint32_t s = cpu->r[S];
    uint32_t t = cpu->r[T];

    DO_PENDING_LOAD;

    if (s == t) {
        BRANCH(IMM16S << 2);
    }
}

void psx_cpu_i_bne(psx_cpu_t* cpu) {
    cpu->branch = 1;
    cpu->branch_taken = 0;

    TRACE_B("bne");

    uint32_t s = cpu->r[S];
    uint32_t t = cpu->r[T];

    DO_PENDING_LOAD;

    if (s != t) {
        BRANCH(IMM16S << 2);
    }
}

void psx_cpu_i_blez(psx_cpu_t* cpu) {
    cpu->branch = 1;
    cpu->branch_taken = 0;

    TRACE_B("blez");

    int32_t s = (int32_t)cpu->r[S];

    DO_PENDING_LOAD;

    if ((int32_t)s <= (int32_t)0) {
        BRANCH(IMM16S << 2);
    }
}

void psx_cpu_i_bgtz(psx_cpu_t* cpu) {
    cpu->branch = 1;
    cpu->branch_taken = 0;

    TRACE_B("bgtz");

    int32_t s = (int32_t)cpu->r[S];

    DO_PENDING_LOAD;

    if ((int32_t)s > (int32_t)0) {
        BRANCH(IMM16S << 2);
    }
}

void psx_cpu_i_addi(psx_cpu_t* cpu) {
    TRACE_I16D("addi");

    uint32_t s = cpu->r[S];

    DO_PENDING_LOAD;

    uint32_t i = IMM16S;
    uint32_t r = s + i;
    uint32_t o = (s ^ r) & (i ^ r);

    if (o & 0x80000000) {
        psx_cpu_exception(cpu, CAUSE_OV);
    } else {
        cpu->r[T] = r;
    }
}

void psx_cpu_i_addiu(psx_cpu_t* cpu) {
    TRACE_I16D("addiu");

    uint32_t s = cpu->r[S];

    DO_PENDING_LOAD;

    cpu->r[T] = s + IMM16S;
}

void psx_cpu_i_slti(psx_cpu_t* cpu) {
    TRACE_I16D("slti");

    int32_t s = (int32_t)cpu->r[S];

    DO_PENDING_LOAD;

    cpu->r[T] = s < IMM16S;
}

void psx_cpu_i_sltiu(psx_cpu_t* cpu) {
    TRACE_I16D("sltiu");

    uint32_t s = cpu->r[S];

    DO_PENDING_LOAD;

    cpu->r[T] = s < IMM16S;
}

void psx_cpu_i_andi(psx_cpu_t* cpu) {
    TRACE_I16D("andi");

    uint32_t s = cpu->r[S];

    DO_PENDING_LOAD;

    cpu->r[T] = s & IMM16;
}

void psx_cpu_i_ori(psx_cpu_t* cpu) {
    TRACE_I16D("ori");

    uint32_t s = cpu->r[S];

    DO_PENDING_LOAD;

    cpu->r[T] = s | IMM16;
}

void psx_cpu_i_xori(psx_cpu_t* cpu) {
    TRACE_I16D("xori");

    uint32_t s = cpu->r[S];

    DO_PENDING_LOAD;

    cpu->r[T] = s ^ IMM16;
}

void psx_cpu_i_lui(psx_cpu_t* cpu) {
    TRACE_I16S("lui");

    DO_PENDING_LOAD;

    cpu->r[T] = IMM16 << 16;
}

void psx_cpu_i_cop0(psx_cpu_t* cpu) {
    g_psx_cpu_cop0_table[S](cpu);
}

void psx_cpu_i_cop1(psx_cpu_t* cpu) {
    DO_PENDING_LOAD;

    psx_cpu_exception(cpu, CAUSE_CPU);
}

void psx_cpu_i_cop2(psx_cpu_t* cpu) {
    g_psx_cpu_cop2_table[S](cpu);
}

void psx_cpu_i_cop3(psx_cpu_t* cpu) {
    DO_PENDING_LOAD;

    psx_cpu_exception(cpu, CAUSE_CPU);
}

void psx_cpu_i_lb(psx_cpu_t* cpu) {
    TRACE_M("lb");

    uint32_t s = cpu->r[S];

    DO_PENDING_LOAD;

    cpu->load_d = T;
    cpu->load_v = SE8(psx_bus_read8(cpu->bus, s + IMM16S));
}

void psx_cpu_i_lh(psx_cpu_t* cpu) {
    TRACE_M("lh");

    uint32_t s = cpu->r[S];

    DO_PENDING_LOAD;

    uint32_t addr = s + IMM16S;

    if (addr & 0x1) {
        psx_cpu_exception(cpu, CAUSE_ADEL);
    } else {
        cpu->load_d = T;
        cpu->load_v = SE16(psx_bus_read16(cpu->bus, addr));
    }
}

void psx_cpu_i_lwl(psx_cpu_t* cpu) {
    TRACE_M("lwl");

    uint32_t addr = cpu->r[S] + IMM16S;

    uint32_t aligned = psx_bus_read32(cpu->bus, addr & ~0x3);

    cpu->load_v = cpu->r[T];

    switch (addr & 0x3) {
        case 0: cpu->load_v = (cpu->load_v & 0x00ffffff) | (aligned << 24); break;
        case 1: cpu->load_v = (cpu->load_v & 0x0000ffff) | (aligned << 16); break;
        case 2: cpu->load_v = (cpu->load_v & 0x000000ff) | (aligned << 8 ); break;
        case 3: cpu->load_v =                               aligned       ; break;
    }

    cpu->load_d = T;
}

void psx_cpu_i_lw(psx_cpu_t* cpu) {
    TRACE_M("lw");

    uint32_t s = cpu->r[S];
    uint32_t addr = s + IMM16S;

    DO_PENDING_LOAD;

    if (addr & 0x3) {
        psx_cpu_exception(cpu, CAUSE_ADEL);
    } else {
        cpu->load_d = T;
        cpu->load_v = psx_bus_read32(cpu->bus, addr);
    }
}

void psx_cpu_i_lbu(psx_cpu_t* cpu) {
    TRACE_M("lbu");

    uint32_t s = cpu->r[S];

    DO_PENDING_LOAD;

    cpu->load_d = T;
    cpu->load_v = psx_bus_read8(cpu->bus, s + IMM16S);
}

void psx_cpu_i_lhu(psx_cpu_t* cpu) {
    TRACE_M("lhu");

    uint32_t s = cpu->r[S];
    uint32_t addr = s + IMM16S;

    DO_PENDING_LOAD;

    if (addr & 0x1) {
        psx_cpu_exception(cpu, CAUSE_ADEL);
    } else {
        cpu->load_d = T;
        cpu->load_v = psx_bus_read16(cpu->bus, addr);
    }
}

void psx_cpu_i_lwr(psx_cpu_t* cpu) {
    TRACE_M("lwr");

    uint32_t addr = cpu->r[S] + IMM16S;

    uint32_t aligned = psx_bus_read32(cpu->bus, addr & ~0x3);

    cpu->load_v = cpu->r[T];

    switch (addr & 0x3) {
        case 0: cpu->load_v =                               aligned       ; break;
        case 1: cpu->load_v = (cpu->load_v & 0xff000000) | (aligned >> 8 ); break;
        case 2: cpu->load_v = (cpu->load_v & 0xffff0000) | (aligned >> 16); break;
        case 3: cpu->load_v = (cpu->load_v & 0xffffff00) | (aligned >> 24); break;
    }

    cpu->load_d = T;
}

void psx_cpu_i_sb(psx_cpu_t* cpu) {
    TRACE_M("sb");

    uint32_t s = cpu->r[S];
    uint32_t t = cpu->r[T];

    DO_PENDING_LOAD;

    // Cache isolated
    if (cpu->cop0_r[COP0_SR] & SR_ISC) {
        log_debug("Ignoring write while cache is isolated");

        return;
    }

    psx_bus_write8(cpu->bus, s + IMM16S, t);
}

void psx_cpu_i_sh(psx_cpu_t* cpu) {
    TRACE_M("sh");

    uint32_t s = cpu->r[S];
    uint32_t t = cpu->r[T];
    uint32_t addr = s + IMM16S;

    DO_PENDING_LOAD;

    // Cache isolated
    if (cpu->cop0_r[COP0_SR] & SR_ISC) {
        log_debug("Ignoring write while cache is isolated");

        return;
    }

    if (addr & 0x1) {
        psx_cpu_exception(cpu, CAUSE_ADES);
    } else {
        psx_bus_write16(cpu->bus, addr, t);
    }
}

void psx_cpu_i_swl(psx_cpu_t* cpu) {
    TRACE_M("swl");

    uint32_t s = cpu->r[S];

    DO_PENDING_LOAD;

    uint32_t addr = s + IMM16S;
    uint32_t aligned = addr & ~0x3;
    uint32_t v = psx_bus_read32(cpu->bus, aligned);

    switch (addr & 0x3) {
        case 0: v = (v & 0xffffff00) | (cpu->r[T] >> 24); break;
        case 1: v = (v & 0xffff0000) | (cpu->r[T] >> 16); break;
        case 2: v = (v & 0xff000000) | (cpu->r[T] >> 8 ); break;
        case 3: v =                     cpu->r[T]       ; break;
    }

    psx_bus_write32(cpu->bus, aligned, v);
}

void psx_cpu_i_sw(psx_cpu_t* cpu) {
    TRACE_M("sw");

    uint32_t s = cpu->r[S];
    uint32_t t = cpu->r[T];
    uint32_t addr = s + IMM16S;

    DO_PENDING_LOAD;

    // Cache isolated
    if (cpu->cop0_r[COP0_SR] & SR_ISC) {
        log_debug("Ignoring write while cache is isolated");

        return;
    }

    if (addr & 0x3) {
        psx_cpu_exception(cpu, CAUSE_ADES);
    } else {
        psx_bus_write32(cpu->bus, addr, t);
    }
}

void psx_cpu_i_swr(psx_cpu_t* cpu) {
    TRACE_M("swr");

    uint32_t s = cpu->r[S];

    DO_PENDING_LOAD;

    uint32_t addr = s + IMM16S;
    uint32_t aligned = addr & ~0x3;
    uint32_t v = psx_bus_read32(cpu->bus, aligned);

    switch (addr & 0x3) {
        case 0: v =                     cpu->r[T]       ; break;
        case 1: v = (v & 0x000000ff) | (cpu->r[T] << 8 ); break;
        case 2: v = (v & 0x0000ffff) | (cpu->r[T] << 16); break;
        case 3: v = (v & 0x00ffffff) | (cpu->r[T] << 24); break;
    }

    psx_bus_write32(cpu->bus, aligned, v);
}

void psx_cpu_i_lwc0(psx_cpu_t* cpu) {
    psx_cpu_exception(cpu, CAUSE_CPU);
}

void psx_cpu_i_lwc1(psx_cpu_t* cpu) {
    psx_cpu_exception(cpu, CAUSE_CPU);
}

void psx_cpu_i_lwc2(psx_cpu_t* cpu) {
    log_error("%08x: GTE LWC2 (GTE unimplemented)", cpu->pc - 8);

    DO_PENDING_LOAD;
}

void psx_cpu_i_lwc3(psx_cpu_t* cpu) {
    psx_cpu_exception(cpu, CAUSE_CPU);
}

void psx_cpu_i_swc0(psx_cpu_t* cpu) {
    psx_cpu_exception(cpu, CAUSE_CPU);
}

void psx_cpu_i_swc1(psx_cpu_t* cpu) {
    psx_cpu_exception(cpu, CAUSE_CPU);
}

void psx_cpu_i_swc2(psx_cpu_t* cpu) {
    log_error("%08x: GTE SWC2 (GTE unimplemented)", cpu->pc - 8);

    DO_PENDING_LOAD;
}

void psx_cpu_i_swc3(psx_cpu_t* cpu) {
    psx_cpu_exception(cpu, CAUSE_CPU);
}

// Secondary
void psx_cpu_i_sll(psx_cpu_t* cpu) {
    TRACE_I5D("sll");

    uint32_t t = cpu->r[T];

    DO_PENDING_LOAD;

    cpu->r[D] = t << IMM5;
}

void psx_cpu_i_srl(psx_cpu_t* cpu) {
    TRACE_I5D("srl");

    uint32_t t = cpu->r[T];

    DO_PENDING_LOAD;

    cpu->r[D] = t >> IMM5;
}

void psx_cpu_i_sra(psx_cpu_t* cpu) {
    TRACE_I5D("sra");

    int32_t t = (int32_t)cpu->r[T];

    DO_PENDING_LOAD;

    cpu->r[D] = t >> IMM5;
}

void psx_cpu_i_sllv(psx_cpu_t* cpu) {
    TRACE_RT("sllv");

    uint32_t s = cpu->r[S];
    uint32_t t = cpu->r[T];

    DO_PENDING_LOAD;

    cpu->r[D] = t << (s & 0x1f);
}

void psx_cpu_i_srlv(psx_cpu_t* cpu) {
    TRACE_RT("srlv");

    uint32_t s = cpu->r[S];
    uint32_t t = cpu->r[T];

    DO_PENDING_LOAD;

    cpu->r[D] = t >> (s & 0x1f);
}

void psx_cpu_i_srav(psx_cpu_t* cpu) {
    TRACE_RT("srav");

    uint32_t s = cpu->r[S];
    int32_t t = (int32_t)cpu->r[T];

    DO_PENDING_LOAD;

    cpu->r[D] = t >> (s & 0x1f);
}

void psx_cpu_i_jr(psx_cpu_t* cpu) {
    cpu->branch = 1;

    TRACE_RS("jr");

    uint32_t s = cpu->r[S];

    DO_PENDING_LOAD;

    cpu->next_pc = s;
}

void psx_cpu_i_jalr(psx_cpu_t* cpu) {
    cpu->branch = 1;

    TRACE_RD("jalr");

    uint32_t s = cpu->r[S];

    DO_PENDING_LOAD;

    cpu->r[D] = cpu->next_pc;

    cpu->next_pc = s;
}

void psx_cpu_i_syscall(psx_cpu_t* cpu) {
    TRACE_I20("syscall");
    
    DO_PENDING_LOAD;

    psx_cpu_exception(cpu, CAUSE_SYSCALL);
}

void psx_cpu_i_break(psx_cpu_t* cpu) {
    TRACE_I20("break");

    DO_PENDING_LOAD;

    psx_cpu_exception(cpu, CAUSE_BP);
}

void psx_cpu_i_mfhi(psx_cpu_t* cpu) {
    TRACE_MTF("mfhi");

    DO_PENDING_LOAD;

    cpu->r[D] = cpu->hi;
}

void psx_cpu_i_mthi(psx_cpu_t* cpu) {
    TRACE_MTF("mthi");

    DO_PENDING_LOAD;

    cpu->hi = cpu->r[S];
}

void psx_cpu_i_mflo(psx_cpu_t* cpu) {
    TRACE_MTF("mflo");

    DO_PENDING_LOAD;

    cpu->r[D] = cpu->lo;
}

void psx_cpu_i_mtlo(psx_cpu_t* cpu) {
    TRACE_MTF("mtlo");

    DO_PENDING_LOAD;

    cpu->lo = cpu->r[S];
}

void psx_cpu_i_mult(psx_cpu_t* cpu) {
    TRACE_MD("mult");

    int64_t s = (int64_t)((int32_t)cpu->r[S]);
    int64_t t = (int64_t)((int32_t)cpu->r[T]);

    DO_PENDING_LOAD;

    uint64_t r = s * t;

    cpu->hi = r >> 32;
    cpu->lo = r & 0xffffffff;
}

void psx_cpu_i_multu(psx_cpu_t* cpu) {
    TRACE_MD("multu");

    uint64_t s = (uint64_t)cpu->r[S];
    uint64_t t = (uint64_t)cpu->r[T];

    DO_PENDING_LOAD;

    uint64_t r = s * t;

    cpu->hi = r >> 32;
    cpu->lo = r & 0xffffffff;
}

void psx_cpu_i_div(psx_cpu_t* cpu) {
    TRACE_MD("div");

    int32_t s = (int32_t)cpu->r[S];
    int32_t t = (int32_t)cpu->r[T];

    DO_PENDING_LOAD;

    if (!t) {
        cpu->hi = s;
        cpu->lo = (s >= 0) ? 0xffffffff : 1;
    } else if ((((uint32_t)s) == 0x80000000) && (t == -1)) {
        cpu->hi = 0;
        cpu->lo = 0x80000000;
    } else {
        cpu->hi = (uint32_t)(s % t);
        cpu->lo = (uint32_t)(s / t);
    }
}

void psx_cpu_i_divu(psx_cpu_t* cpu) {
    TRACE_MD("divu");

    uint32_t s = cpu->r[S];
    uint32_t t = cpu->r[T];

    DO_PENDING_LOAD;

    if (!t) {
        cpu->hi = s;
        cpu->lo = 0xffffffff;
    } else {
        cpu->hi = s % t;
        cpu->lo = s / t;
    }
}

void psx_cpu_i_add(psx_cpu_t* cpu) {
    TRACE_RT("add");

    int32_t s = cpu->r[S];
    int32_t t = cpu->r[T];

    DO_PENDING_LOAD;

    int32_t r = s + t;
    uint32_t o = (s ^ r) & (t ^ r);

    if (o & 0x80000000) {
        psx_cpu_exception(cpu, CAUSE_OV);
    } else {
        cpu->r[D] = (uint32_t)r;
    }
}

void psx_cpu_i_addu(psx_cpu_t* cpu) {
    TRACE_RT("addu");

    uint32_t s = cpu->r[S];
    uint32_t t = cpu->r[T];

    DO_PENDING_LOAD;

    cpu->r[D] = s + t;
}

void psx_cpu_i_sub(psx_cpu_t* cpu) {
    TRACE_RT("sub");

    int32_t s = (int32_t)cpu->r[S];
    int32_t t = (int32_t)cpu->r[T];
    int32_t r;

    DO_PENDING_LOAD;

    int o = __builtin_ssub_overflow(s, t, &r);

    if (o) {
        psx_cpu_exception(cpu, CAUSE_OV);
    } else {
        cpu->r[D] = r;
    }
}

void psx_cpu_i_subu(psx_cpu_t* cpu) {
    TRACE_RT("subu");

    uint32_t s = cpu->r[S];
    uint32_t t = cpu->r[T];

    DO_PENDING_LOAD;

    cpu->r[D] = s - t;
}

void psx_cpu_i_and(psx_cpu_t* cpu) {
    TRACE_RT("and");

    uint32_t s = cpu->r[S];
    uint32_t t = cpu->r[T];

    DO_PENDING_LOAD;

    cpu->r[D] = s & t;
}

void psx_cpu_i_or(psx_cpu_t* cpu) {
    TRACE_RT("or");

    uint32_t s = cpu->r[S];
    uint32_t t = cpu->r[T];

    DO_PENDING_LOAD;

    cpu->r[D] = s | t;
}

void psx_cpu_i_xor(psx_cpu_t* cpu) {
    TRACE_RT("xor");

    uint32_t s = cpu->r[S];
    uint32_t t = cpu->r[T];

    DO_PENDING_LOAD;

    cpu->r[D] = (s ^ t);
}

void psx_cpu_i_nor(psx_cpu_t* cpu) {
    TRACE_RT("nor");

    uint32_t s = cpu->r[S];
    uint32_t t = cpu->r[T];

    DO_PENDING_LOAD;

    cpu->r[D] = ~(s | t);
}

void psx_cpu_i_slt(psx_cpu_t* cpu) {
    TRACE_RT("slt");

    int32_t s = (int32_t)cpu->r[S];
    int32_t t = (int32_t)cpu->r[T];

    DO_PENDING_LOAD;

    cpu->r[D] = s < t;
}

void psx_cpu_i_sltu(psx_cpu_t* cpu) {
    TRACE_RT("sltu");

    uint32_t s = cpu->r[S];
    uint32_t t = cpu->r[T];

    DO_PENDING_LOAD;

    cpu->r[D] = s < t;
}

// COP0
void psx_cpu_i_mfc0(psx_cpu_t* cpu) {
    TRACE_C0M("mfc0");

    DO_PENDING_LOAD;

    cpu->load_v = cpu->cop0_r[D];
    cpu->load_d = T;
}

void psx_cpu_i_cfc0(psx_cpu_t* cpu) {
    log_error("%08x: cfc0 (unimplemented)", cpu->pc - 8);

    exit(1);
}

void psx_cpu_i_mtc0(psx_cpu_t* cpu) {
    TRACE_C0M("mtc0");

    uint32_t t = cpu->r[T];

    DO_PENDING_LOAD;

    cpu->cop0_r[D] = t & g_psx_cpu_cop0_write_mask_table[D];
}

// COP0-specific
void psx_cpu_i_rfe(psx_cpu_t* cpu) {
    TRACE_N("rfe");

    DO_PENDING_LOAD;

    uint32_t mode = cpu->cop0_r[COP0_SR] & 0x3f;

    cpu->cop0_r[COP0_SR] &= 0xfffffff0;
    cpu->cop0_r[COP0_SR] |= mode >> 2;
}

// COP2
void psx_cpu_i_mfc2(psx_cpu_t* cpu) {
    DO_PENDING_LOAD;

    cpu->load_v = ((uint32_t*)(&cpu->cop2_dr))[D];
    cpu->load_d = T;
}

void psx_cpu_i_cfc2(psx_cpu_t* cpu) {
    DO_PENDING_LOAD;

    cpu->load_v = ((uint32_t*)(&cpu->cop2_cr))[D];
    cpu->load_d = T;
}

void psx_cpu_i_mtc2(psx_cpu_t* cpu) {
    uint32_t t = cpu->r[T];

    DO_PENDING_LOAD;

    ((uint32_t*)(&cpu->cop2_dr))[D] = t & g_psx_cpu_cop0_write_mask_table[D];
}

void psx_cpu_i_ctc2(psx_cpu_t* cpu) {
    uint32_t t = cpu->r[T];

    DO_PENDING_LOAD;

    ((uint32_t*)(&cpu->cop2_cr))[D] = t & g_psx_cpu_cop0_write_mask_table[D];
}

void psx_cpu_i_gte(psx_cpu_t* cpu) {
    g_psx_gte_table[cpu->opcode & 0x3f](cpu);
}


void psx_gte_i_invalid(psx_cpu_t* cpu) {
    log_fatal("invalid: Unimplemented GTE instruction %02x, %02x", cpu->opcode & 0x3f, cpu->opcode >> 25);
}

void psx_gte_i_rtps(psx_cpu_t* cpu) {
    log_fatal("rtps: Unimplemented GTE instruction");
}

void psx_gte_i_nclip(psx_cpu_t* cpu) {
    log_fatal("nclip: Unimplemented GTE instruction");
}

void psx_gte_i_op(psx_cpu_t* cpu) {
    log_fatal("op: Unimplemented GTE instruction");
}

void psx_gte_i_dpcs(psx_cpu_t* cpu) {
    log_fatal("dpcs: Unimplemented GTE instruction");
}

void psx_gte_i_intpl(psx_cpu_t* cpu) {
    log_fatal("intpl: Unimplemented GTE instruction");
}

void psx_gte_i_mvmva(psx_cpu_t* cpu) {
    log_fatal("mvmva: Unimplemented GTE instruction");
}

void psx_gte_i_ncds(psx_cpu_t* cpu) {
    log_fatal("ncds: Unimplemented GTE instruction");
}

void psx_gte_i_cdp(psx_cpu_t* cpu) {
    log_fatal("cdp: Unimplemented GTE instruction");
}

void psx_gte_i_ncdt(psx_cpu_t* cpu) {
    log_fatal("ncdt: Unimplemented GTE instruction");
}

void psx_gte_i_nccs(psx_cpu_t* cpu) {
    log_fatal("nccs: Unimplemented GTE instruction");
}

void psx_gte_i_cc(psx_cpu_t* cpu) {
    log_fatal("cc: Unimplemented GTE instruction");
}

void psx_gte_i_ncs(psx_cpu_t* cpu) {
    log_fatal("ncs: Unimplemented GTE instruction");
}

void psx_gte_i_nct(psx_cpu_t* cpu) {
    log_fatal("nct: Unimplemented GTE instruction");
}

void psx_gte_i_sqr(psx_cpu_t* cpu) {
    log_fatal("sqr: Unimplemented GTE instruction");
}

void psx_gte_i_dcpl(psx_cpu_t* cpu) {
    log_fatal("dcpl: Unimplemented GTE instruction");
}

void psx_gte_i_dpct(psx_cpu_t* cpu) {
    log_fatal("dpct: Unimplemented GTE instruction");
}

void psx_gte_i_avsz3(psx_cpu_t* cpu) {
    log_fatal("avsz3: Unimplemented GTE instruction");
}

void psx_gte_i_avsz4(psx_cpu_t* cpu) {
    log_fatal("avsz4: Unimplemented GTE instruction");
}

void psx_gte_i_rtpt(psx_cpu_t* cpu) {
    log_fatal("rtpt: Unimplemented GTE instruction");
}

void psx_gte_i_gpf(psx_cpu_t* cpu) {
    log_fatal("gpf: Unimplemented GTE instruction");
}

void psx_gte_i_gpl(psx_cpu_t* cpu) {
    log_fatal("gpl: Unimplemented GTE instruction");
}

void psx_gte_i_ncct(psx_cpu_t* cpu) {
    log_fatal("ncct: Unimplemented GTE instruction");
}

#undef R_R0
#undef R_AT
#undef R_V0
#undef R_V1
#undef R_A0
#undef R_A1
#undef R_A2
#undef R_A3
#undef R_T0
#undef R_T1
#undef R_T2
#undef R_T3
#undef R_T4
#undef R_T5
#undef R_T6
#undef R_T7
#undef R_S0
#undef R_S1
#undef R_S2
#undef R_S3
#undef R_S4
#undef R_S5
#undef R_S6
#undef R_S7
#undef R_T8
#undef R_T9
#undef R_K0
#undef R_K1
#undef R_GP
#undef R_SP
#undef R_FP
#undef R_RA

#undef OP
#undef S
#undef T
#undef D
#undef IMM5
#undef CMT
#undef SOP
#undef IMM26
#undef IMM16
#undef IMM16S

#undef TRACE_M
#undef TRACE_I16S
#undef TRACE_I16D
#undef TRACE_I5D
#undef TRACE_I26
#undef TRACE_RT
#undef TRACE_C0M
#undef TRACE_B
#undef TRACE_RS
#undef TRACE_MTF
#undef TRACE_RD
#undef TRACE_MD
#undef TRACE_I20
#undef TRACE_N

#undef DO_PENDING_LOAD

#undef DEBUG_ALL

#undef SE8
#undef SE16