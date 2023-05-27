#include "cpu.h"
#include "bus.h"

#include <stdlib.h>
#include <string.h>

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
    psx_cpu_i_mfc0   , psx_cpu_i_invalid, psx_cpu_i_cfc0   , psx_cpu_i_invalid,
    psx_cpu_i_mtc0   , psx_cpu_i_invalid, psx_cpu_i_ctc0   , psx_cpu_i_invalid,
    psx_cpu_i_bc0c   , psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid,
    psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid,
    psx_cpu_i_rfe    , psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid,
    psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid,
    psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid,
    psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid
};

psx_cpu_instruction_t g_psx_cpu_bxx_table[] = {
    psx_cpu_i_bltz   , psx_cpu_i_bgez   , psx_cpu_i_invalid, psx_cpu_i_invalid,
    psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid,
    psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid,
    psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid,
    psx_cpu_i_bltzal , psx_cpu_i_bgezal , psx_cpu_i_invalid, psx_cpu_i_invalid
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

void psx_cpu_cycle(psx_cpu_t* cpu) {
    cpu->r[0] = 0;

    cpu->buf[1] = cpu->buf[0];
    cpu->buf[0] = psx_bus_read32(cpu->bus, cpu->pc);

    cpu->pc += 4;

    g_psx_cpu_primary_table[OP](cpu);

    cpu->r[0] = 0;
}

#define DO_PENDING_LOAD \
    cpu->r[cpu->load_d] = cpu->load_v; \
    R_R0 = 0; \
    cpu->load_d = 0;

void psx_cpu_i_invalid(psx_cpu_t* cpu) {
    log_error("%08x: invalid (unimplemented)", cpu->pc - 8);

    exit(1);
}

// Primary
void psx_cpu_i_special(psx_cpu_t* cpu) {
    g_psx_cpu_secondary_table[SOP](cpu);
}

void psx_cpu_i_bxx(psx_cpu_t* cpu) {
    g_psx_cpu_bxx_table[T](cpu);
}

// BXX
void psx_cpu_i_bltz(psx_cpu_t* cpu) {
    TRACE_B("bltz");

    int32_t s = (int32_t)cpu->r[S];

    DO_PENDING_LOAD;

    if ((int32_t)s < (int32_t)0) {
        cpu->pc -= 4;
        cpu->pc += (IMM16S << 2);
    }
}

void psx_cpu_i_bgez(psx_cpu_t* cpu) {
    TRACE_B("bgez");

    int32_t s = (int32_t)cpu->r[S];

    DO_PENDING_LOAD;

    if ((int32_t)s >= (int32_t)0) {
        cpu->pc -= 4;
        cpu->pc += (IMM16S << 2);
    }
}

void psx_cpu_i_bltzal(psx_cpu_t* cpu) {
    TRACE_B("bltzal");

    int32_t s = (int32_t)cpu->r[S];

    DO_PENDING_LOAD;

    R_RA = cpu->pc - 4;

    if ((int32_t)s < (int32_t)0) {
        cpu->pc -= 4;
        cpu->pc += (IMM16S << 2);
    }
}

void psx_cpu_i_bgezal(psx_cpu_t* cpu) {
    TRACE_B("bgezal");

    int32_t s = (int32_t)cpu->r[S];

    DO_PENDING_LOAD;

    R_RA = cpu->pc - 4;

    if ((int32_t)s >= (int32_t)0) {
        cpu->pc -= 4;
        cpu->pc += (IMM16S << 2);
    }
}
void psx_cpu_i_j(psx_cpu_t* cpu) {
    TRACE_I26("j");

    DO_PENDING_LOAD;

    cpu->pc = (cpu->pc & 0xf0000000) | (IMM26 << 2);
}

#define DEBUG_ALL \
    log_debug("r0=%08x at=%08x v0=%08x v1=%08x", R_R0, R_AT, R_V0, R_V1); \
    log_debug("a0=%08x a1=%08x a2=%08x a3=%08x", R_A0, R_A1, R_A2, R_A3); \
    log_debug("t0=%08x t1=%08x t2=%08x t3=%08x", R_T0, R_T1, R_T2, R_T3); \
    log_debug("t4=%08x t5=%08x t6=%08x t7=%08x", R_T4, R_T5, R_T6, R_T7); \
    log_debug("s0=%08x s1=%08x s2=%08x s3=%08x", R_S0, R_S1, R_S2, R_S3); \
    log_debug("s4=%08x s5=%08x s6=%08x s7=%08x", R_S4, R_S5, R_S6, R_S7); \
    log_debug("t8=%08x t9=%08x k0=%08x k1=%08x", R_T8, R_T9, R_K0, R_K1); \
    log_debug("gp=%08x sp=%08x fp=%08x ra=%08x", R_GP, R_SP, R_FP, R_RA); \
    log_debug("pc=%08x hi=%08x lo=%08x l:%s=%08x", cpu->pc, cpu->hi, cpu->lo, g_mips_cc_register_names[cpu->load_d], cpu->load_v); \
    exit(1)

void psx_cpu_i_jal(psx_cpu_t* cpu) {
    TRACE_I26("jal");

    DO_PENDING_LOAD;

    cpu->r[31] = cpu->pc;

    cpu->pc = (cpu->pc & 0xf0000000) | (IMM26 << 2);
}

void psx_cpu_i_beq(psx_cpu_t* cpu) {
    TRACE_B("beq");

    uint32_t s = cpu->r[S];
    uint32_t t = cpu->r[T];

    DO_PENDING_LOAD;

    if (s == t) {
        cpu->pc -= 4;
        cpu->pc += (IMM16S << 2);
    }
}

void psx_cpu_i_bne(psx_cpu_t* cpu) {
    TRACE_B("bne");

    uint32_t s = cpu->r[S];
    uint32_t t = cpu->r[T];

    DO_PENDING_LOAD;

    if (s != t) {
        cpu->pc -= 4;
        cpu->pc += (IMM16S << 2);
    }
}

void psx_cpu_i_blez(psx_cpu_t* cpu) {
    TRACE_B("blez");

    int32_t s = (int32_t)cpu->r[S];

    DO_PENDING_LOAD;

    if ((int32_t)s <= (int32_t)0) {
        cpu->pc -= 4;
        cpu->pc += (IMM16S << 2);
    }
}

void psx_cpu_i_bgtz(psx_cpu_t* cpu) {
    TRACE_B("bgtz");

    int32_t s = (int32_t)cpu->r[S];

    DO_PENDING_LOAD;

    if ((int32_t)s > (int32_t)0) {
        cpu->pc -= 4;
        cpu->pc += (IMM16S << 2);
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
        log_warn("Overflow on addi %08x + %08x = %08x (%08x)", s, i, r, o);
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
    log_error("%08x: xori (unimplemented)", cpu->pc - 8);

    exit(1);
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
    log_error("%08x: cop1 (unimplemented)", cpu->pc - 8);

    exit(1);
}

void psx_cpu_i_cop2(psx_cpu_t* cpu) {
    log_error("%08x: cop2 (unimplemented)", cpu->pc - 8);

    exit(1);
}

void psx_cpu_i_cop3(psx_cpu_t* cpu) {
    log_error("%08x: cop3 (unimplemented)", cpu->pc - 8);

    exit(1);
}

#define SE8(v) ((int32_t)((int8_t)v))
#define SE16(v) ((int32_t)((int16_t)v))

void psx_cpu_i_lb(psx_cpu_t* cpu) {
    TRACE_M("lb");

    // To-do: Emulate load delay slots
    // In my mind, an instruction is executed in two stages.
    // First, the instruction "microcode" fetches input registers
    // and puts them in latches, the CPU then takes over and
    // reads the data bus into the pending load register.
    // Finally, the instruction is actually executed using
    // the values fetched on the first stage.
    // This is impossible to emulate using our current method

    uint32_t s = cpu->r[S];

    DO_PENDING_LOAD;

    cpu->load_d = T;
    cpu->load_v = SE8(psx_bus_read8(cpu->bus, s + IMM16S));
}

void psx_cpu_i_lh(psx_cpu_t* cpu) {
    log_error("%08x: lh (unimplemented)", cpu->pc - 8);

    exit(1);
}

void psx_cpu_i_lwl(psx_cpu_t* cpu) {
    log_error("%08x: lwl (unimplemented)", cpu->pc - 8);

    exit(1);
}

void psx_cpu_i_lw(psx_cpu_t* cpu) {
    TRACE_M("lw");

    // To-do: Emulate load delay slots
    // In my mind, an instruction is executed in two stages.
    // First, the instruction "microcode" fetches input registers
    // and puts them in latches, the CPU then takes over and
    // reads the data bus into the pending load register.
    // Finally, the instruction is actually executed using
    // the values fetched on the first stage.
    // This is impossible to emulate using our current method

    uint32_t s = cpu->r[S];

    DO_PENDING_LOAD;

    cpu->load_d = T;
    cpu->load_v = psx_bus_read32(cpu->bus, s + IMM16S);
}

void psx_cpu_i_lbu(psx_cpu_t* cpu) {
    TRACE_M("lbu");

    uint32_t s = cpu->r[S];

    DO_PENDING_LOAD;

    cpu->load_d = T;
    cpu->load_v = psx_bus_read8(cpu->bus, s + IMM16S);
}

void psx_cpu_i_lhu(psx_cpu_t* cpu) {
    log_error("%08x: lhu (unimplemented)", cpu->pc - 8);

    exit(1);
}

void psx_cpu_i_lwr(psx_cpu_t* cpu) {
    log_error("%08x: lwr (unimplemented)", cpu->pc - 8);

    exit(1);
}

void psx_cpu_i_sb(psx_cpu_t* cpu) {
    TRACE_M("sb");

    uint32_t s = cpu->r[S];
    uint32_t t = cpu->r[T];

    DO_PENDING_LOAD;

    // Cache isolated
    if (cpu->cop0_sr & SR_ISC) {
        log_warn("Ignoring write while cache is isolated");

        return;
    }

    psx_bus_write8(cpu->bus, s + IMM16S, t);
}

void psx_cpu_i_sh(psx_cpu_t* cpu) {
    TRACE_M("sh");

    uint32_t s = cpu->r[S];
    uint32_t t = cpu->r[T];

    DO_PENDING_LOAD;

    // Cache isolated
    if (cpu->cop0_sr & SR_ISC) {
        log_warn("Ignoring write while cache is isolated");

        return;
    }

    psx_bus_write16(cpu->bus, s + IMM16S, t);
}

void psx_cpu_i_swl(psx_cpu_t* cpu) {
    log_error("%08x: swl (unimplemented)", cpu->pc - 8);

    exit(1);
}

void psx_cpu_i_sw(psx_cpu_t* cpu) {
    TRACE_M("sw");

    uint32_t s = cpu->r[S];
    uint32_t t = cpu->r[T];

    DO_PENDING_LOAD;

    // Cache isolated
    if (cpu->cop0_sr & SR_ISC) {
        log_warn("Ignoring write while cache is isolated");

        return;
    }

    psx_bus_write32(cpu->bus, s + IMM16S, t);
}

void psx_cpu_i_swr(psx_cpu_t* cpu) {
    log_error("%08x: swr (unimplemented)", cpu->pc - 8);

    exit(1);
}

void psx_cpu_i_lwc0(psx_cpu_t* cpu) {
    log_error("%08x: lwc0 (unimplemented)", cpu->pc - 8);

    exit(1);
}

void psx_cpu_i_lwc1(psx_cpu_t* cpu) {
    log_error("%08x: lwc1 (unimplemented)", cpu->pc - 8);

    exit(1);
}

void psx_cpu_i_lwc2(psx_cpu_t* cpu) {
    log_error("%08x: lwc2 (unimplemented)", cpu->pc - 8);

    exit(1);
}

void psx_cpu_i_lwc3(psx_cpu_t* cpu) {
    log_error("%08x: lwc3 (unimplemented)", cpu->pc - 8);

    exit(1);
}

void psx_cpu_i_swc0(psx_cpu_t* cpu) {
    log_error("%08x: swc0 (unimplemented)", cpu->pc - 8);

    exit(1);
}

void psx_cpu_i_swc1(psx_cpu_t* cpu) {
    log_error("%08x: swc1 (unimplemented)", cpu->pc - 8);

    exit(1);
}

void psx_cpu_i_swc2(psx_cpu_t* cpu) {
    log_error("%08x: swc2 (unimplemented)", cpu->pc - 8);

    exit(1);
}

void psx_cpu_i_swc3(psx_cpu_t* cpu) {
    log_error("%08x: swc3 (unimplemented)", cpu->pc - 8);

    exit(1);
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
    log_error("%08x: sllv (unimplemented)", cpu->pc - 8);

    exit(1);
}

void psx_cpu_i_srlv(psx_cpu_t* cpu) {
    log_error("%08x: srlv (unimplemented)", cpu->pc - 8);

    exit(1);
}

void psx_cpu_i_srav(psx_cpu_t* cpu) {
    log_error("%08x: srav (unimplemented)", cpu->pc - 8);

    exit(1);
}

void psx_cpu_i_jr(psx_cpu_t* cpu) {
    TRACE_RS("jr");

    uint32_t s = cpu->r[S];

    DO_PENDING_LOAD;

    cpu->pc = s;
}

void psx_cpu_i_jalr(psx_cpu_t* cpu) {
    TRACE_RD("jalr");

    uint32_t s = cpu->r[S];

    DO_PENDING_LOAD;

    cpu->r[D] = cpu->pc;

    cpu->pc = s;
}

void psx_cpu_i_syscall(psx_cpu_t* cpu) {
    log_error("%08x: syscall (unimplemented)", cpu->pc - 8);

    exit(1);
}

void psx_cpu_i_break(psx_cpu_t* cpu) {
    log_error("%08x: break (unimplemented)", cpu->pc - 8);

    exit(1);
}

void psx_cpu_i_mfhi(psx_cpu_t* cpu) {
    TRACE_MTF("mfhi");

    DO_PENDING_LOAD;

    cpu->r[D] = cpu->hi;
}

void psx_cpu_i_mthi(psx_cpu_t* cpu) {
    log_error("%08x: mthi (unimplemented)", cpu->pc - 8);

    exit(1);
}

void psx_cpu_i_mflo(psx_cpu_t* cpu) {
    TRACE_MTF("mflo");

    DO_PENDING_LOAD;

    cpu->r[D] = cpu->lo;
}

void psx_cpu_i_mtlo(psx_cpu_t* cpu) {
    log_error("%08x: mtlo (unimplemented)", cpu->pc - 8);

    exit(1);
}

void psx_cpu_i_mult(psx_cpu_t* cpu) {
    log_error("%08x: mult (unimplemented)", cpu->pc - 8);

    exit(1);
}

void psx_cpu_i_multu(psx_cpu_t* cpu) {
    log_error("%08x: multu (unimplemented)", cpu->pc - 8);

    exit(1);
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

    uint32_t s = cpu->r[S];
    uint32_t t = cpu->r[T];

    DO_PENDING_LOAD;

    uint32_t r = s + t;
    uint32_t o = (s ^ r) & (t ^ r);

    if (o & 0x80000000) {
        log_warn("Overflow on add %08x + %08x = %08x (%08x)", s, t, r, o);
    } else {
        cpu->r[D] = r;
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
    log_error("%08x: sub (unimplemented)", cpu->pc - 8);

    exit(1);
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
    log_error("%08x: xor (unimplemented)", cpu->pc - 8);

    exit(1);
}

void psx_cpu_i_nor(psx_cpu_t* cpu) {
    log_error("%08x: nor (unimplemented)", cpu->pc - 8);

    exit(1);
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

    switch (D) {
        case 3: cpu->load_v = cpu->cop0_bpc; break;
        case 5: cpu->load_v = cpu->cop0_bda; break;
        case 6: cpu->load_v = cpu->cop0_jumpdest; break;
        case 7: cpu->load_v = cpu->cop0_dcic; break;
        case 8: cpu->load_v = cpu->cop0_badvaddr; break;
        case 9: cpu->load_v = cpu->cop0_bdam; break;
        case 11: cpu->load_v = cpu->cop0_bpcm; break;
        case 12: cpu->load_v = cpu->cop0_sr; break;
        case 13: cpu->load_v = cpu->cop0_cause; break;
        case 14: cpu->load_v = cpu->cop0_epc; break;
        case 15: cpu->load_v = cpu->cop0_prid; break;
    }

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

    switch (D) {
        case 3: cpu->cop0_bpc = t; break;
        case 5: cpu->cop0_bda = t; break;
        case 7: cpu->cop0_dcic = t; break;
        case 9: cpu->cop0_bdam = t; break;
        case 11: cpu->cop0_bpcm = t; break;
        case 12: cpu->cop0_sr = t; break;
    }
}

void psx_cpu_i_ctc0(psx_cpu_t* cpu) {
    log_error("%08x: ctc0 (unimplemented)", cpu->pc - 8);

    exit(1);
}

void psx_cpu_i_bc0c(psx_cpu_t* cpu) {
    log_error("%08x: bc0c (unimplemented)", cpu->pc - 8);

    exit(1);
}

// COP0-specific
void psx_cpu_i_rfe(psx_cpu_t* cpu) {
    log_error("%08x: rfe (unimplemented)", cpu->pc - 8);

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