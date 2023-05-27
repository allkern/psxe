#ifndef CPU_H
#define CPU_H

#include <stdint.h>

#include "bus.h"

typedef struct {
    // Pointer to bus structure
    psx_bus_t* bus;

    // Registers (including $zero and $ra)
    uint32_t r[32];

    // Pipeline simulation (for branch delay slots)
    uint32_t buf[2];

    // Program Counter
    uint32_t pc;

    // Result registers for mult and div
    uint32_t hi, lo;

    // Pending load data
    uint32_t load_d, load_v;

    /*
        cop0r0      - N/A
        cop0r1      - N/A
        cop0r2      - N/A
        cop0r3      - BPC - Breakpoint on execute (R/W)
        cop0r4      - N/A
        cop0r5      - BDA - Breakpoint on data access (R/W)
        cop0r6      - JUMPDEST - Randomly memorized jump address (R)
        cop0r7      - DCIC - Breakpoint control (R/W)
        cop0r8      - BadVaddr - Bad Virtual Address (R)
        cop0r9      - BDAM - Data Access breakpoint mask (R/W)
        cop0r10     - N/A
        cop0r11     - BPCM - Execute breakpoint mask (R/W)
        cop0r12     - SR - System status register (R/W)
        cop0r13     - CAUSE - Describes the most recently recognised exception (R)
        cop0r14     - EPC - Return Address from Trap (R)
        cop0r15     - PRID - Processor ID (R)
    */

    // r3 - BPC - Breakpoint on execute (R/W)
    uint32_t cop0_bpc;

    // r5 - BDA - Breakpoint on data access (R/W)
    uint32_t cop0_bda;

    // r6 - JUMPDEST - Randomly memorized jump address (R)
    uint32_t cop0_jumpdest;

    // r7 - DCIC - Breakpoint control (R/W)
    uint32_t cop0_dcic;

    // r8 - BadVaddr - Bad Virtual Address (R)
    uint32_t cop0_badvaddr;

    // r9 - BDAM - Data Access breakpoint mask (R/W)
    uint32_t cop0_bdam;

    // r11 - BPCM - Execute breakpoint mask (R/W)
    uint32_t cop0_bpcm;

    // r12 - SR - System status register (R/W)
    uint32_t cop0_sr;

    // r13 - CAUSE - Describes the most recently recognised exception (R)
    uint32_t cop0_cause;

    // r14 - EPC - Return Address from Trap (R)
    uint32_t cop0_epc;

    // r15 - PRID - Processor ID (R)
    uint32_t cop0_prid;
} psx_cpu_t;

/*
  0     IEc Current Interrupt Enable  (0=Disable, 1=Enable) ;rfe pops IUp here
  1     KUc Current Kernel/User Mode  (0=Kernel, 1=User)    ;rfe pops KUp here
  2     IEp Previous Interrupt Disable                      ;rfe pops IUo here
  3     KUp Previous Kernel/User Mode                       ;rfe pops KUo here
  4     IEo Old Interrupt Disable                       ;left unchanged by rfe
  5     KUo Old Kernel/User Mode                        ;left unchanged by rfe
  6-7   -   Not used (zero)
  8-15  Im  8 bit interrupt mask fields. When set the corresponding
            interrupts are allowed to cause an exception.
  16    Isc Isolate Cache (0=No, 1=Isolate)
              When isolated, all load and store operations are targetted
              to the Data cache, and never the main memory.
              (Used by PSX Kernel, in combination with Port FFFE0130h)
  17    Swc Swapped cache mode (0=Normal, 1=Swapped)
              Instruction cache will act as Data cache and vice versa.
              Use only with Isc to access & invalidate Instr. cache entries.
              (Not used by PSX Kernel)
  18    PZ  When set cache parity bits are written as 0.
  19    CM  Shows the result of the last load operation with the D-cache
            isolated. It gets set if the cache really contained data
            for the addressed memory location.
  20    PE  Cache parity error (Does not cause exception)
  21    TS  TLB shutdown. Gets set if a programm address simultaneously
            matches 2 TLB entries.
            (initial value on reset allows to detect extended CPU version?)
  22    BEV Boot exception vectors in RAM/ROM (0=RAM/KSEG0, 1=ROM/KSEG1)
  23-24 -   Not used (zero)
  25    RE  Reverse endianness   (0=Normal endianness, 1=Reverse endianness)
              Reverses the byte order in which data is stored in
              memory. (lo-hi -> hi-lo)
              (Affects only user mode, not kernel mode) (?)
              (The bit doesn't exist in PSX ?)
  26-27 -   Not used (zero)
  28    CU0 COP0 Enable (0=Enable only in Kernel Mode, 1=Kernel and User Mode)
  29    CU1 COP1 Enable (0=Disable, 1=Enable) (none in PSX)
  30    CU2 COP2 Enable (0=Disable, 1=Enable) (GTE in PSX)
  31    CU3 COP3 Enable (0=Disable, 1=Enable) (none in PSX)
*/

#define SR_IEC 0x00000001
#define SR_KUC 0x00000002
#define SR_IEP 0x00000004
#define SR_KUP 0x00000008
#define SR_IEO 0x00000010
#define SR_KUO 0x00000020
#define SR_IM  0x0000ff00
#define SR_IM0 0x00000100
#define SR_IM1 0x00000200
#define SR_IM2 0x00000400
#define SR_IM3 0x00000800
#define SR_IM4 0x00001000
#define SR_IM5 0x00002000
#define SR_IM6 0x00004000
#define SR_IM7 0x00008000
#define SR_ISC 0x00010000
#define SR_SWC 0x00020000
#define SR_PZ  0x00040000
#define SR_CM  0x00080000
#define SR_PE  0x00100000
#define SR_TS  0x00200000
#define SR_BEV 0x00400000
#define SR_RE  0x02000000
#define SR_CU0 0x10000000
#define SR_CU1 0x20000000
#define SR_CU2 0x40000000
#define SR_CU3 0x80000000

psx_cpu_t* psx_cpu_create();
void psx_cpu_init(psx_cpu_t*, psx_bus_t*);
void psx_cpu_destroy(psx_cpu_t*);
void psx_cpu_cycle(psx_cpu_t*);

void psx_cpu_i_invalid(psx_cpu_t*);

// Primary
void psx_cpu_i_special(psx_cpu_t*);
void psx_cpu_i_bxx(psx_cpu_t*);
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

// BXX
void psx_cpu_i_bltz(psx_cpu_t*);
void psx_cpu_i_bgez(psx_cpu_t*);
void psx_cpu_i_bltzal(psx_cpu_t*);
void psx_cpu_i_bgezal(psx_cpu_t*);

typedef void (*psx_cpu_instruction_t)(psx_cpu_t*);

#endif