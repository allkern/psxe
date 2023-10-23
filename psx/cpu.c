#include "cpu.h"
#include "bus.h"
#include "log.h"

#include <stdlib.h>
#include <string.h>

static const psx_cpu_instruction_t g_psx_cpu_secondary_table[] = {
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

static const psx_cpu_instruction_t g_psx_cpu_primary_table[] = {
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

static const psx_cpu_instruction_t g_psx_cpu_cop0_table[] = {
    psx_cpu_i_mfc0   , psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid,
    psx_cpu_i_mtc0   , psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid,
    psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid,
    psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid,
    psx_cpu_i_rfe    , psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid,
    psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid,
    psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid,
    psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid
};

static const psx_cpu_instruction_t g_psx_cpu_cop2_table[] = {
    psx_cpu_i_mfc2   , psx_cpu_i_invalid, psx_cpu_i_cfc2   , psx_cpu_i_invalid,
    psx_cpu_i_mtc2   , psx_cpu_i_invalid, psx_cpu_i_ctc2   , psx_cpu_i_invalid,
    psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid,
    psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid, psx_cpu_i_invalid,
    psx_cpu_i_gte    , psx_cpu_i_gte    , psx_cpu_i_gte    , psx_cpu_i_gte    ,
    psx_cpu_i_gte    , psx_cpu_i_gte    , psx_cpu_i_gte    , psx_cpu_i_gte    ,
    psx_cpu_i_gte    , psx_cpu_i_gte    , psx_cpu_i_gte    , psx_cpu_i_gte    ,
    psx_cpu_i_gte    , psx_cpu_i_gte    , psx_cpu_i_gte    , psx_cpu_i_gte
};

static const psx_cpu_instruction_t g_psx_cpu_bxx_table[] = {
    psx_cpu_i_bltz   , psx_cpu_i_bgez   , psx_cpu_i_bltz   , psx_cpu_i_bgez   ,
    psx_cpu_i_bltz   , psx_cpu_i_bgez   , psx_cpu_i_bltz   , psx_cpu_i_bgez   ,
    psx_cpu_i_bltz   , psx_cpu_i_bgez   , psx_cpu_i_bltz   , psx_cpu_i_bgez   ,
    psx_cpu_i_bltz   , psx_cpu_i_bgez   , psx_cpu_i_bltz   , psx_cpu_i_bgez   ,
    psx_cpu_i_bltzal , psx_cpu_i_bgezal , psx_cpu_i_bltz   , psx_cpu_i_bgez   ,
    psx_cpu_i_bltz   , psx_cpu_i_bgez   , psx_cpu_i_bltz   , psx_cpu_i_bgez   ,
    psx_cpu_i_bltz   , psx_cpu_i_bgez   , psx_cpu_i_bltz   , psx_cpu_i_bgez   ,
    psx_cpu_i_bltz   , psx_cpu_i_bgez   , psx_cpu_i_bltz   , psx_cpu_i_bgez
};

static const psx_cpu_instruction_t g_psx_gte_table[] = {
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

static const uint32_t g_psx_cpu_cop0_write_mask_table[] = {
    0x00000000, // cop0r0   - N/A
    0x00000000, // cop0r1   - N/A
    0x00000000, // cop0r2   - N/A
    0xffffffff, // BPC      - Breakpoint on execute (R/W)
    0x00000000, // cop0r4   - N/A
    0xffffffff, // BDA      - Breakpoint on data access (R/W)
    0x00000000, // JUMPDEST - Randomly memorized jump address (R)
    0xffc0f03f, // DCIC     - Breakpoint control (R/W)
    0x00000000, // BadVaddr - Bad Virtual Address (R)
    0xffffffff, // BDAM     - Data Access breakpoint mask (R/W)
    0x00000000, // cop0r10  - N/A
    0xffffffff, // BPCM     - Execute breakpoint mask (R/W)
    0xffffffff, // SR       - System status register (R/W)
    0x00000300, // CAUSE    - Describes the most recently recognised exception (R)
    0x00000000, // EPC      - Return Address from Trap (R)
    0x00000000  // PRID     - Processor ID (R)
};

static const uint8_t g_psx_gte_unr_table[] = {
    0xff, 0xfd, 0xfb, 0xf9, 0xf7, 0xf5, 0xf3, 0xf1,
    0xef, 0xee, 0xec, 0xea, 0xe8, 0xe6, 0xe4, 0xe3,
    0xe1, 0xdf, 0xdd, 0xdc, 0xda, 0xd8, 0xd6, 0xd5,
    0xd3, 0xd1, 0xd0, 0xce, 0xcd, 0xcb, 0xc9, 0xc8,
    0xc6, 0xc5, 0xc3, 0xc1, 0xc0, 0xbe, 0xbd, 0xbb,
    0xba, 0xb8, 0xb7, 0xb5, 0xb4, 0xb2, 0xb1, 0xb0,
    0xae, 0xad, 0xab, 0xaa, 0xa9, 0xa7, 0xa6, 0xa4,
    0xa3, 0xa2, 0xa0, 0x9f, 0x9e, 0x9c, 0x9b, 0x9a,
    0x99, 0x97, 0x96, 0x95, 0x94, 0x92, 0x91, 0x90,
    0x8f, 0x8d, 0x8c, 0x8b, 0x8a, 0x89, 0x87, 0x86,
    0x85, 0x84, 0x83, 0x82, 0x81, 0x7f, 0x7e, 0x7d,
    0x7c, 0x7b, 0x7a, 0x79, 0x78, 0x77, 0x75, 0x74,
    0x73, 0x72, 0x71, 0x70, 0x6f, 0x6e, 0x6d, 0x6c,
    0x6b, 0x6a, 0x69, 0x68, 0x67, 0x66, 0x65, 0x64,
    0x63, 0x62, 0x61, 0x60, 0x5f, 0x5e, 0x5d, 0x5d,
    0x5c, 0x5b, 0x5a, 0x59, 0x58, 0x57, 0x56, 0x55,
    0x54, 0x53, 0x53, 0x52, 0x51, 0x50, 0x4f, 0x4e,
    0x4d, 0x4d, 0x4c, 0x4b, 0x4a, 0x49, 0x48, 0x48,
    0x47, 0x46, 0x45, 0x44, 0x43, 0x43, 0x42, 0x41,
    0x40, 0x3f, 0x3f, 0x3e, 0x3d, 0x3c, 0x3c, 0x3b,
    0x3a, 0x39, 0x39, 0x38, 0x37, 0x36, 0x36, 0x35,
    0x34, 0x33, 0x33, 0x32, 0x31, 0x31, 0x30, 0x2f,
    0x2e, 0x2e, 0x2d, 0x2c, 0x2c, 0x2b, 0x2a, 0x2a,
    0x29, 0x28, 0x28, 0x27, 0x26, 0x26, 0x25, 0x24,
    0x24, 0x23, 0x22, 0x22, 0x21, 0x20, 0x20, 0x1f,
    0x1e, 0x1e, 0x1d, 0x1d, 0x1c, 0x1b, 0x1b, 0x1a,
    0x19, 0x19, 0x18, 0x18, 0x17, 0x16, 0x16, 0x15,
    0x15, 0x14, 0x14, 0x13, 0x12, 0x12, 0x11, 0x11,
    0x10, 0x0f, 0x0f, 0x0e, 0x0e, 0x0d, 0x0d, 0x0c,
    0x0c, 0x0b, 0x0a, 0x0a, 0x09, 0x09, 0x08, 0x08,
    0x07, 0x07, 0x06, 0x06, 0x05, 0x05, 0x04, 0x04,
    0x03, 0x03, 0x02, 0x02, 0x01, 0x01, 0x00, 0x00,
    0x00
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

#define COP2_DR(idx) ((uint32_t*)(&cpu->cop2_dr))[idx]
#define COP2_CR(idx) ((uint32_t*)(&cpu->cop2_cr))[idx]

#define R_R0 (cpu->r[0])
#define R_A0 (cpu->r[4])
#define R_RA (cpu->r[31])

//#define CPU_TRACE

static const char* g_psx_cpu_a_kcall_symtable[] = {
    "open(filename=%08x,accessmode=%08x)",
    "lseek(fd=%08x,offset=%08x,seektype=%08x)",
    "read(fd=%08x,dst=%08x,length=%08x)",
    "write(fd=%08x,src=%08x,length=%08x)",
    "close(fd=%08x)",
    "ioctl(fd=%08x,cmd=%08x,arg=%08x)",
    "exit(exitcode=%08x)",
    "isatty(fd=%08x)",
    "getc(fd=%08x)",
    "putc(char=%08x,fd=%08x)",
    "todigit(char=%08x)",
    "atof(src=%08x)",
    "strtoul(src=%08x,src_end=%08x,base=%08x)",
    "strtol(src=%08x,src_end=%08x,base=%08x)",
    "abs(val=%08x)",
    "labs(val=%08x)",
    "atoi(src=%08x)",
    "atol(src=%08x)",
    "atob(src=%08x,num_dst=%08x)",
    "setjmp(buf=%08x)",
    "longjmp(buf=%08x,param=%08x)",
    "strcat(dst=%08x,src=%08x)",
    "strncat(dst=%08x,src=%08x,maxlen=%08x)",
    "strcmp(str1=%08x,str2=%08x)",
    "strncmp(str1=%08x,str2=%08x,maxlen=%08x)",
    "strcpy(dst=%08x,src=%08x)",
    "strncpy(dst=%08x,src=%08x,maxlen=%08x)",
    "strlen(src=%08x)",
    "index(src=%08x,char=%08x)",
    "rindex(src=%08x,char=%08x)",
    "strchr(src=%08x,char=%08x)",
    "strrchr(src=%08x,char=%08x)",
    "strpbrk(src=%08x,list=%08x)",
    "strspn(src=%08x,list=%08x)",
    "strcspn(src=%08x,list=%08x)",
    "strtok(src=%08x,list=%08x)",
    "strstr(str=%08x,substr=%08x)",
    "toupper(char=%08x)",
    "tolower(char=%08x)",
    "bcopy(src=%08x,dst=%08x,len=%08x)",
    "bzero(dst=%08x,len=%08x)",
    "bcmp(ptr1=%08x,ptr2=%08x,len=%08x)",
    "memcpy(dst=%08x,src=%08x,len=%08x)",
    "memset(dst=%08x,fillbyte=%08x,len=%08x)",
    "memmove(dst=%08x,src=%08x,len=%08x)",
    "memcmp(src1=%08x,src2=%08x,len=%08x)",
    "memchr(src=%08x,scanbyte=%08x,len=%08x)",
    "rand()",
    "srand(seed=%08x)",
    "qsort(base=%08x,nel=%08x,width=%08x,callback=%08x)",
    "strtod(src=%08x,src_end=%08x)",
    "malloc(size=%08x)",
    "free(buf=%08x)",
    "lsearch(key=%08x,base=%08x,nel=%08x,width=%08x,callback=%08x)",
    "bsearch(key=%08x,base=%08x,nel=%08x,width=%08x,callback=%08x)",
    "calloc(sizx=%08x,sizy=%08x)",
    "realloc(old_buf=%08x,new_siz=%08x)",
    "InitHeap(addr=%08x,size=%08x)",
    "_exit(exitcode=%08x)",
    "getchar()",
    "putchar(char=%08x)",
    "gets(dst=%08x)",
    "puts(src=%08x)",
    "printf(txt=%08x,param1=%08x,param2=%08x,etc.=%08x)",
    "SystemErrorUnresolvedException()",
    "LoadTest(filename=%08x,headerbuf=%08x)",
    "Load(filename=%08x,headerbuf=%08x)",
    "Exec(headerbuf=%08x,param1=%08x,param2=%08x)",
    "FlushCache()",
    "init_a0_b0_c0_vectors()",
    "GPU_dw(Xdst=%08x,Ydst=%08x,Xsiz=%08x,Ysiz=%08x,src=%08x)",
    "gpu_send_dma(Xdst=%08x,Ydst=%08x,Xsiz=%08x,Ysiz=%08x,src=%08x)",
    "SendGP1Command(gp1cmd=%08x)",
    "GPU_cw(gp0cmd=%08x)",
    "GPU_cwp(src=%08x,num=%08x)",
    "send_gpu_linked_list(src=%08x)",
    "gpu_abort_dma()",
    "GetGPUStatus()",
    "gpu_sync()",
    "SystemError()",
    "SystemError()",
    "LoadExec(filename=%08x,stackbase=%08x,stackoffset=%08x)",
    "GetSysSp()",
    "SystemError()",
    "_96_init()",
    "_bu_init()",
    "_96_remove()",
    "return 0",
    "return 0",
    "return 0",
    "return 0",
    "dev_tty_init()",
    "dev_tty_open(fcb=%08x, (unused)path=%08x,accessmode=%08x)",
    "dev_tty_in_out(fcb=%08x,cmd=%08x)",
    "dev_tty_ioctl(fcb=%08x,cmd=%08x,arg=%08x)",
    "dev_cd_open(fcb=%08x,path=%08x,accessmode=%08x)",
    "dev_cd_read(fcb=%08x,dst=%08x,len=%08x)",
    "dev_cd_close(fcb=%08x)",
    "dev_cd_firstfile(fcb=%08x,path=%08x,direntry=%08x)",
    "dev_cd_nextfile(fcb=%08x,direntry=%08x)",
    "dev_cd_chdir(fcb=%08x,path=%08x)",
    "dev_card_open(fcb=%08x,path=%08x,accessmode=%08x)",
    "dev_card_read(fcb=%08x,dst=%08x,len=%08x)",
    "dev_card_write(fcb=%08x,src=%08x,len=%08x)",
    "dev_card_close(fcb=%08x)",
    "dev_card_firstfile(fcb=%08x,path=%08x,direntry=%08x)",
    "dev_card_nextfile(fcb=%08x,direntry=%08x)",
    "dev_card_erase(fcb=%08x,path=%08x)",
    "dev_card_undelete(fcb=%08x,path=%08x)",
    "dev_card_format(fcb=%08x)",
    "dev_card_rename(fcb1=%08x,path=%08x)",
    "card_clear_error(fcb=%08x) (?)",
    "_bu_init()",
    "_96_init()",
    "_96_remove()",
    "return 0",
    "return 0",
    "return 0",
    "return 0",
    "return 0",
    "CdAsyncSeekL(src=%08x)",
    "return 0",
    "return 0",
    "return 0",
    "CdAsyncGetStatus(dst=%08x)",
    "return 0",
    "CdAsyncReadSector(count=%08x,dst=%08x,mode=%08x)",
    "return 0",
    "return 0",
    "CdAsyncSetMode(mode=%08x)",
    "return 0",
    "return 0",
    "return 0",
    "return 0",
    "return 0",
    "return 0",
    "return 0",
    "return 0",
    "return 0",
    "return 0",
    "return 0",
    "return 0",
    "return 0",
    "return 0",
    "CdromIoIrqFunc1()",
    "CdromDmaIrqFunc1()",
    "CdromIoIrqFunc2()",
    "CdromDmaIrqFunc2()",
    "CdromGetInt5errCode(dst1=%08x,dst2=%08x)",
    "CdInitSubFunc()",
    "AddCDROMDevice()",
    "AddMemCardDevice()",
    "AddDuartTtyDevice()",
    "add_nullcon_driver()",
    "SystemError()",
    "SystemError()",
    "SetConf(num_EvCB=%08x,num_TCB=%08x,stacktop=%08x)",
    "GetConf(num_EvCB_dst=%08x,num_TCB_dst=%08x,stacktop_dst=%08x)",
    "SetCdromIrqAutoAbort(type=%08x,flag=%08x)",
    "SetMem(megabytes=%08x)",
    "_boot()",
    "SystemError(type=%08x,errorcode=%08x)",
    "EnqueueCdIntr()",
    "DequeueCdIntr()",
    "CdGetLbn(filename=%08x)",
    "CdReadSector(count=%08x,sector=%08x,buffer=%08x)",
    "CdGetStatus()",
    "bufs_cb_0()",
    "bufs_cb_1()",
    "bufs_cb_2()",
    "bufs_cb_3()",
    "_card_info(port=%08x)",
    "_card_load(port=%08x)",
    "_card_auto(flag=%08x)",
    "bufs_cb_4()",
    "card_write_test(port=%08x)",
    "return 0",
    "return 0",
    "ioabort_raw(param=%08x)",
    "return 0",
    "GetSystemInfo(index=%08x)"
};

static const char* g_psx_cpu_b_kcall_symtable[] = {
    "alloc_kernel_memory(size=%08x)",
    "free_kernel_memory(buf=%08x)",
    "init_timer(t=%08x,reload=%08x,flags=%08x)",
    "get_timer(t=%08x)",
    "enable_timer_irq(t=%08x)",
    "disable_timer_irq(t=%08x)",
    "restart_timer(t=%08x)",
    "DeliverEvent(class=%08x, spec=%08x)",
    "OpenEvent(class=%08x,spec=%08x,mode=%08x,func=%08x)",
    "CloseEvent(event=%08x)",
    "WaitEvent(event=%08x)",
    "TestEvent(event=%08x)",
    "EnableEvent(event=%08x)",
    "DisableEvent(event=%08x)",
    "OpenTh(reg_PC=%08x,reg_SP_FP=%08x,reg_GP=%08x)",
    "CloseTh(handle=%08x)",
    "ChangeTh(handle=%08x)",
    "jump_to_00000000h()",
    "InitPAD2(buf1=%08x,siz1=%08x,buf2=%08x,siz2=%08x)",
    "StartPAD2()",
    "StopPAD2()",
    "PAD_init2(type=%08x,button_dest=%08x,unused=%08x,unused=%08x)",
    "PAD_dr()",
    "ReturnFromException()",
    "ResetEntryInt()",
    "HookEntryInt(addr=%08x)",
    "SystemError()",
    "SystemError()",
    "SystemError()",
    "SystemError()",
    "SystemError()",
    "SystemError()",
    "UnDeliverEvent(class=%08x,spec=%08x)",
    "SystemError()",
    "SystemError()",
    "SystemError()",
    "jump_to_00000000h()",
    "jump_to_00000000h()",
    "jump_to_00000000h()",
    "jump_to_00000000h()",
    "jump_to_00000000h()",
    "jump_to_00000000h()",
    "SystemError()",
    "SystemError()",
    "jump_to_00000000h()",
    "jump_to_00000000h()",
    "jump_to_00000000h()",
    "jump_to_00000000h()",
    "jump_to_00000000h()",
    "jump_to_00000000h()",
    "open(filename=%08x,accessmode=%08x)",
    "lseek(fd=%08x,offset=%08x,seektype=%08x)",
    "read(fd=%08x,dst=%08x,length=%08x)",
    "write(fd=%08x,src=%08x,length=%08x)",
    "close(fd=%08x)",
    "ioctl(fd=%08x,cmd=%08x,arg=%08x)",
    "exit(exitcode=%08x)",
    "isatty(fd=%08x)",
    "getc(fd=%08x)",
    "putc(char=%08x,fd=%08x)",
    "getchar()",
    "putchar(char=%08x)",
    "gets(dst=%08x)",
    "puts(src=%08x)",
    "cd(name=%08x)",
    "format(devicename=%08x)",
    "firstfile2(filename=%08x,direntry=%08x)",
    "nextfile(direntry=%08x)",
    "rename(old_filename=%08x,new_filename=%08x)",
    "erase(filename=%08x)",
    "undelete(filename=%08x)",
    "AddDrv(device_info=%08x)",
    "DelDrv(device_name_lowercase=%08x)",
    "PrintInstalledDevices()",
    "InitCARD2(pad_enable=%08x)",
    "StartCARD2()",
    "StopCARD2()",
    "_card_info_subfunc(port=%08x)",
    "_card_write(port=%08x,sector=%08x,src=%08x)",
    "_card_read(port=%08x,sector=%08x,dst=%08x)",
    "_new_card()",
    "Krom2RawAdd(shiftjis_code=%08x)",
    "SystemError()",
    "Krom2Offset(shiftjis_code=%08x)",
    "_get_errno()",
    "_get_error(fd=%08x)",
    "GetC0Table()",
    "GetB0Table()",
    "_card_chan()",
    "testdevice(devicename=%08x)",
    "SystemError()",
    "ChangeClearPAD(int=%08x)",
    "_card_status(slot=%08x)",
    "_card_wait(slot=%08x)"
};

static const char* g_psx_cpu_c_kcall_symtable[] = {
    "EnqueueTimerAndVblankIrqs(priority=%08x)",
    "EnqueueSyscallHandler(priority=%08x)",
    "SysEnqIntRP(priority=%08x,struc=%08x)",
    "SysDeqIntRP(priority=%08x,struc=%08x)",
    "get_free_EvCB_slot()",
    "get_free_TCB_slot()",
    "ExceptionHandler()",
    "InstallExceptionHandlers()",
    "SysInitMemory(addr=%08x,size=%08x)",
    "SysInitKernelVariables()",
    "ChangeClearRCnt(t=%08x,flag=%08x)",
    "SystemError()",
    "InitDefInt(priority=%08x)",
    "SetIrqAutoAck(irq=%08x,flag=%08x)",
    "return 0",
    "return 0",
    "return 0",
    "return 0",
    "InstallDevices(ttyflag=%08x)",
    "FlushStdInOutPut()",
    "return 0",
    "_cdevinput(circ=%08x,char=%08x)",
    "_cdevscan()",
    "_circgetc(circ=%08x)",
    "_circputc(char=%08x,circ=%08x)",
    "_ioabort(txt1=%08x,txt2=%08x)",
    "set_card_find_mode(mode=%08x)",
    "KernelRedirect(ttyflag=%08x)",
    "AdjustA0Table()",
    "get_card_find_mode()"
};

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

#define TRACE_C2M(m) \
    log_trace("%08x: %-7s $%s, $cop2_r%u", cpu->pc-8, m, g_mips_cc_register_names[T], D)

#define TRACE_C2MC(m) \
    log_trace("%08x: %-7s $%s, $cop2_r%u", cpu->pc-8, m, g_mips_cc_register_names[T], D + 32)

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
#define TRACE_C2M(m)
#define TRACE_C2MC(m)
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

#ifdef CPU_TRACE
#define DEBUG_ALL \
    log_fatal("r0=%08x at=%08x v0=%08x v1=%08x", cpu->r[0] , cpu->r[1] , cpu->r[2] , cpu->r[3] ); \
    log_fatal("a0=%08x a1=%08x a2=%08x a3=%08x", cpu->r[4] , cpu->r[5] , cpu->r[6] , cpu->r[7] ); \
    log_fatal("t0=%08x t1=%08x t2=%08x t3=%08x", cpu->r[8] , cpu->r[9] , cpu->r[10], cpu->r[11]); \
    log_fatal("t4=%08x t5=%08x t6=%08x t7=%08x", cpu->r[12], cpu->r[13], cpu->r[14], cpu->r[15]); \
    log_fatal("s0=%08x s1=%08x s2=%08x s3=%08x", cpu->r[16], cpu->r[17], cpu->r[18], cpu->r[19]); \
    log_fatal("s4=%08x s5=%08x s6=%08x s7=%08x", cpu->r[20], cpu->r[21], cpu->r[22], cpu->r[23]); \
    log_fatal("t8=%08x t9=%08x k0=%08x k1=%08x", cpu->r[24], cpu->r[25], cpu->r[26], cpu->r[27]); \
    log_fatal("gp=%08x sp=%08x fp=%08x ra=%08x", cpu->r[28], cpu->r[29], cpu->r[30], cpu->r[31]); \
    log_fatal("pc=%08x hi=%08x lo=%08x l:%s=%08x", cpu->pc, cpu->hi, cpu->lo, g_mips_cc_register_names[cpu->load_d], cpu->load_v); \
    exit(1)

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

static const char* g_mips_cc_register_names[] = {
    "r0", "at", "v0", "v1", "a0", "a1", "a2", "a3",
    "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
    "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
    "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra"
};

static const char* g_psx_cpu_syscall_function_symbol_table[] = {
    "NoFunction",
    "EnterCriticalSection",
    "ExitCriticalSection",
    "ChangeThreadSubFunction"
    // DeliverEvent (invalid)
};
#endif

#define SE8(v) ((int32_t)((int8_t)v))
#define SE16(v) ((int32_t)((int16_t)v))

#define BRANCH(offset) \
    cpu->next_pc = cpu->next_pc + (offset); \
    cpu->next_pc = cpu->next_pc - 4; \
    cpu->branch = 1; \
    cpu->branch_taken = 1;

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
    // if ((cpu->pc & 0x3fffffff) == 0x000000a4) {
    //     if (cpu->r[9] == 0x2f)
    //         goto no_putchar;

    //     char buf[256];
    
    //     sprintf(buf, g_psx_cpu_a_kcall_symtable[cpu->r[9]],
    //         cpu->r[4],
    //         cpu->r[5],
    //         cpu->r[6],
    //         cpu->r[7]
    //     );
    //     log_set_quiet(0);
    //     log_fatal("A(%02x) %s", cpu->r[9], buf);
    //     log_set_quiet(1);
    // }

    if ((cpu->pc & 0x3fffffff) == 0x000000b4) {
        if (cpu->b_function_hook) cpu->b_function_hook(cpu);

        if ((cpu->r[9] == 0x3b) || (cpu->r[9] == 0x3d) || (cpu->r[9] == 0x3f) || (cpu->r[9] == 0x0b))
            goto no_putchar;
        
        // char buf[256];

        // sprintf(buf, g_psx_cpu_b_kcall_symtable[cpu->r[9]],
        //     cpu->r[4],
        //     cpu->r[5],
        //     cpu->r[6],
        //     cpu->r[7]
        // );
        // log_set_quiet(0);
        // log_fatal("B(%02x) %s", cpu->r[9], buf);
        // log_set_quiet(1);
    }

    no_putchar:

    // if ((cpu->pc & 0x3fffffff) == 0x000000c4) {
    //     char buf[256];

    //     sprintf(buf, g_psx_cpu_c_kcall_symtable[cpu->r[9]],
    //         cpu->r[4],
    //         cpu->r[5],
    //         cpu->r[6],
    //         cpu->r[7]
    //     );
    //     log_set_quiet(0);
    //     log_fatal("C(%02x) %s", cpu->r[9], buf);
    //     log_set_quiet(1);
    // }

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

void psx_cpu_set_irq_pending(psx_cpu_t* cpu) {
    cpu->cop0_r[COP0_CAUSE] |= SR_IM2;
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

void psx_cpu_i_lwc3(psx_cpu_t* cpu) {
    psx_cpu_exception(cpu, CAUSE_CPU);
}

void psx_cpu_i_swc0(psx_cpu_t* cpu) {
    psx_cpu_exception(cpu, CAUSE_CPU);
}

void psx_cpu_i_swc1(psx_cpu_t* cpu) {
    psx_cpu_exception(cpu, CAUSE_CPU);
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

void psx_cpu_i_mtc0(psx_cpu_t* cpu) {
    TRACE_C0M("mtc0");

    uint32_t t = cpu->r[T];

    DO_PENDING_LOAD;

    cpu->cop0_r[D] = t & g_psx_cpu_cop0_write_mask_table[D];
}

void psx_cpu_i_rfe(psx_cpu_t* cpu) {
    TRACE_N("rfe");

    DO_PENDING_LOAD;

    uint32_t mode = cpu->cop0_r[COP0_SR] & 0x3f;

    cpu->cop0_r[COP0_SR] &= 0xfffffff0;
    cpu->cop0_r[COP0_SR] |= mode >> 2;
}

// COP2
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define CLAMP(v, a, b) (((v) < (a)) ? (a) : (((v) > (b)) ? (b) : (v)))

void gte_handle_irgb_write(psx_cpu_t* cpu) {
    cpu->cop2_dr.ir[1] = ((cpu->cop2_dr.irgb >> 0) & 0x1f) * 0x80;
    cpu->cop2_dr.ir[2] = ((cpu->cop2_dr.irgb >> 5) & 0x1f) * 0x80;
    cpu->cop2_dr.ir[3] = ((cpu->cop2_dr.irgb >> 10) & 0x1f) * 0x80;
}

void gte_handle_irgb_read(psx_cpu_t* cpu) {
    int r = CLAMP(cpu->cop2_dr.ir[1] >> 7, 0x00, 0x1f);
    int g = CLAMP(cpu->cop2_dr.ir[2] >> 7, 0x00, 0x1f);
    int b = CLAMP(cpu->cop2_dr.ir[3] >> 7, 0x00, 0x1f);

    cpu->cop2_dr.irgb = r | (g << 5) | (b << 10);
}

void gte_handle_sxyp_write(psx_cpu_t* cpu) {
    cpu->cop2_dr.sxy[0] = cpu->cop2_dr.sxy[1];
    cpu->cop2_dr.sxy[1] = cpu->cop2_dr.sxy[2];
    cpu->cop2_dr.sxy[2] = cpu->cop2_dr.sxy[3];
}

void gte_handle_lzcs_write(psx_cpu_t* cpu) {
    if ((cpu->cop2_dr.lzcs == 0xffffffff) || !cpu->cop2_dr.lzcs) {
        cpu->cop2_dr.lzcr = 32;

        return;
    }

    int b = (cpu->cop2_dr.lzcs >> 31) & 1;

    cpu->cop2_dr.lzcr = __builtin_clz(b ? ~cpu->cop2_dr.lzcs : cpu->cop2_dr.lzcs);
}

uint32_t gte_read_register(psx_cpu_t* cpu, uint32_t r) {
    switch (r) {
        case 0 : return cpu->cop2_dr.v[0].xy;
        case 1 : return (int32_t)cpu->cop2_dr.v[0].z;
        case 2 : return cpu->cop2_dr.v[1].xy;
        case 3 : return (int32_t)cpu->cop2_dr.v[1].z;
        case 4 : return cpu->cop2_dr.v[2].xy;
        case 5 : return (int32_t)cpu->cop2_dr.v[2].z;
        case 6 : return cpu->cop2_dr.rgbc.rgbc;
        case 7 : return cpu->cop2_dr.otz;
        case 8 : return (int32_t)cpu->cop2_dr.ir[0];
        case 9 : return (int32_t)cpu->cop2_dr.ir[1];
        case 10: return (int32_t)cpu->cop2_dr.ir[2];
        case 11: return (int32_t)cpu->cop2_dr.ir[3];
        case 12: return cpu->cop2_dr.sxy[0].xy;
        case 13: return cpu->cop2_dr.sxy[1].xy;
        case 14: return cpu->cop2_dr.sxy[2].xy;
        case 15: return cpu->cop2_dr.sxy[2].xy; // SXY2 Mirror
        case 16: return cpu->cop2_dr.sz[0];
        case 17: return cpu->cop2_dr.sz[1];
        case 18: return cpu->cop2_dr.sz[2];
        case 19: return cpu->cop2_dr.sz[3];
        case 20: return cpu->cop2_dr.rgb[0].rgbc;
        case 21: return cpu->cop2_dr.rgb[1].rgbc;
        case 22: return cpu->cop2_dr.rgb[2].rgbc;
        case 23: return cpu->cop2_dr.res1;
        case 24: return cpu->cop2_dr.mac[0];
        case 25: return cpu->cop2_dr.mac[1];
        case 26: return cpu->cop2_dr.mac[2];
        case 27: return cpu->cop2_dr.mac[3];
        case 28: gte_handle_irgb_read(cpu); return cpu->cop2_dr.irgb;
        case 29: return cpu->cop2_dr.irgb; // IRGB mirror
        case 30: return cpu->cop2_dr.lzcs;
        case 31: return cpu->cop2_dr.lzcr;
        case 32: return cpu->cop2_cr.rt.m[0].u32;
        case 33: return cpu->cop2_cr.rt.m[1].u32;
        case 34: return cpu->cop2_cr.rt.m[2].u32;
        case 35: return cpu->cop2_cr.rt.m[3].u32;
        case 36: return (int32_t)cpu->cop2_cr.rt.m33;
        case 37: return cpu->cop2_cr.tr.x;
        case 38: return cpu->cop2_cr.tr.y;
        case 39: return cpu->cop2_cr.tr.z;
        case 40: return cpu->cop2_cr.l.m[0].u32;
        case 41: return cpu->cop2_cr.l.m[1].u32;
        case 42: return cpu->cop2_cr.l.m[2].u32;
        case 43: return cpu->cop2_cr.l.m[3].u32;
        case 44: return (int32_t)cpu->cop2_cr.l.m33;
        case 45: return cpu->cop2_cr.bk.x;
        case 46: return cpu->cop2_cr.bk.y;
        case 47: return cpu->cop2_cr.bk.z;
        case 48: return cpu->cop2_cr.lr.m[0].u32;
        case 49: return cpu->cop2_cr.lr.m[1].u32;
        case 50: return cpu->cop2_cr.lr.m[2].u32;
        case 51: return cpu->cop2_cr.lr.m[3].u32;
        case 52: return (int32_t)cpu->cop2_cr.lr.m33;
        case 53: return cpu->cop2_cr.fc.x;
        case 54: return cpu->cop2_cr.fc.y;
        case 55: return cpu->cop2_cr.fc.z;
        case 56: return cpu->cop2_cr.ofx;
        case 57: return cpu->cop2_cr.ofy;
        case 58: return (int32_t)(int16_t)cpu->cop2_cr.h;
        case 59: return cpu->cop2_cr.dqa;
        case 60: return cpu->cop2_cr.dqb;
        case 61: return cpu->cop2_cr.zsf3;
        case 62: return cpu->cop2_cr.zsf4;
        case 63: return (cpu->cop2_cr.flag & 0x7ffff000) | 
                        (((cpu->cop2_cr.flag & 0x7f87e000) != 0) << 31);
    }

    return 0x00000000;
}

void gte_write_register(psx_cpu_t* cpu, uint32_t r, uint32_t value) {
    switch (r) {
        case 0 : cpu->cop2_dr.v[0].xy = value; break;
        case 1 : cpu->cop2_dr.v[0].z = value; break;
        case 2 : cpu->cop2_dr.v[1].xy = value; break;
        case 3 : cpu->cop2_dr.v[1].z = value; break;
        case 4 : cpu->cop2_dr.v[2].xy = value; break;
        case 5 : cpu->cop2_dr.v[2].z = value; break;
        case 6 : cpu->cop2_dr.rgbc.rgbc = value; break;
        case 7 : cpu->cop2_dr.otz = value; break;
        case 8 : cpu->cop2_dr.ir[0] = value; break;
        case 9 : cpu->cop2_dr.ir[1] = value; break;
        case 10: cpu->cop2_dr.ir[2] = value; break;
        case 11: cpu->cop2_dr.ir[3] = value; break;
        case 12: cpu->cop2_dr.sxy[0].xy = value; break;
        case 13: cpu->cop2_dr.sxy[1].xy = value; break;
        case 14: cpu->cop2_dr.sxy[2].xy = value; break;
        case 15: cpu->cop2_dr.sxy[3].xy = value; gte_handle_sxyp_write(cpu); break;
        case 16: cpu->cop2_dr.sz[0] = value; break;
        case 17: cpu->cop2_dr.sz[1] = value; break;
        case 18: cpu->cop2_dr.sz[2] = value; break;
        case 19: cpu->cop2_dr.sz[3] = value; break;
        case 20: cpu->cop2_dr.rgb[0].rgbc = value; break;
        case 21: cpu->cop2_dr.rgb[1].rgbc = value; break;
        case 22: cpu->cop2_dr.rgb[2].rgbc = value; break;
        case 23: cpu->cop2_dr.res1 = value; break;
        case 24: cpu->cop2_dr.mac[0] = value; break;
        case 25: cpu->cop2_dr.mac[1] = value; break;
        case 26: cpu->cop2_dr.mac[2] = value; break;
        case 27: cpu->cop2_dr.mac[3] = value; break;
        case 28: cpu->cop2_dr.irgb = value & 0x7fff; gte_handle_irgb_write(cpu); break;
        case 29: /* ORGB RO */ break;
        case 30: cpu->cop2_dr.lzcs = value; gte_handle_lzcs_write(cpu); break;
        case 31: /* LZCR RO */ break;
        case 32: cpu->cop2_cr.rt.m[0].u32 = value; break;
        case 33: cpu->cop2_cr.rt.m[1].u32 = value; break;
        case 34: cpu->cop2_cr.rt.m[2].u32 = value; break;
        case 35: cpu->cop2_cr.rt.m[3].u32 = value; break;
        case 36: cpu->cop2_cr.rt.m33 = value; break;
        case 37: cpu->cop2_cr.tr.x = value; break;
        case 38: cpu->cop2_cr.tr.y = value; break;
        case 39: cpu->cop2_cr.tr.z = value; break;
        case 40: cpu->cop2_cr.l.m[0].u32 = value; break;
        case 41: cpu->cop2_cr.l.m[1].u32 = value; break;
        case 42: cpu->cop2_cr.l.m[2].u32 = value; break;
        case 43: cpu->cop2_cr.l.m[3].u32 = value; break;
        case 44: cpu->cop2_cr.l.m33 = value; break;
        case 45: cpu->cop2_cr.bk.x = value; break;
        case 46: cpu->cop2_cr.bk.y = value; break;
        case 47: cpu->cop2_cr.bk.z = value; break;
        case 48: cpu->cop2_cr.lr.m[0].u32 = value; break;
        case 49: cpu->cop2_cr.lr.m[1].u32 = value; break;
        case 50: cpu->cop2_cr.lr.m[2].u32 = value; break;
        case 51: cpu->cop2_cr.lr.m[3].u32 = value; break;
        case 52: cpu->cop2_cr.lr.m33 = value; break;
        case 53: cpu->cop2_cr.fc.x = value; break;
        case 54: cpu->cop2_cr.fc.y = value; break;
        case 55: cpu->cop2_cr.fc.z = value; break;
        case 56: cpu->cop2_cr.ofx = value; break;
        case 57: cpu->cop2_cr.ofy = value; break;
        case 58: cpu->cop2_cr.h = value; break;
        case 59: cpu->cop2_cr.dqa = value; break;
        case 60: cpu->cop2_cr.dqb = value; break;
        case 61: cpu->cop2_cr.zsf3 = value; break;
        case 62: cpu->cop2_cr.zsf4 = value; break;
        case 63: cpu->cop2_cr.flag = value & 0x7ffff000; break;
    }
}

void psx_cpu_i_lwc2(psx_cpu_t* cpu) {
    uint32_t s = cpu->r[S];
    uint32_t addr = s + IMM16S;

    DO_PENDING_LOAD;

    if (addr & 0x3) {
        psx_cpu_exception(cpu, CAUSE_ADEL);
    } else {
        gte_write_register(cpu, T, psx_bus_read32(cpu->bus, addr));
    }
}

void psx_cpu_i_swc2(psx_cpu_t* cpu) {
    uint32_t s = cpu->r[S];
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
        psx_bus_write32(cpu->bus, addr, gte_read_register(cpu, T));
    }
}

void psx_cpu_i_mfc2(psx_cpu_t* cpu) {
    TRACE_C2M("mfc2");

    DO_PENDING_LOAD;

    cpu->load_v = gte_read_register(cpu, D);
    cpu->load_d = T;
}

void psx_cpu_i_cfc2(psx_cpu_t* cpu) {
    TRACE_C2MC("cfc2");

    DO_PENDING_LOAD;

    cpu->load_v = gte_read_register(cpu, D + 32);
    cpu->load_d = T;
}

void psx_cpu_i_mtc2(psx_cpu_t* cpu) {
    TRACE_C2M("mtc2");

    uint32_t t = cpu->r[T];

    DO_PENDING_LOAD;

    gte_write_register(cpu, D, t);
}

void psx_cpu_i_ctc2(psx_cpu_t* cpu) {
    TRACE_C2MC("ctc2");

    uint32_t t = cpu->r[T];

    DO_PENDING_LOAD;

    gte_write_register(cpu, D + 32, t);
}

#define R_FLAG cpu->cop2_cr.flag

int64_t gte_clamp_mac0(psx_cpu_t* cpu, int64_t value) {
    cpu->s_mac0 = value;

    if (value < (-0x80000000ll)) {
        R_FLAG |= 0x8000;
    } else if (value > (0x7fffffffll)) {
        R_FLAG |= 0x10000;
    }

    return value;
}

int64_t gte_clamp_mac(psx_cpu_t* cpu, int i, int64_t value) {
    if (i == 3)
        cpu->s_mac3 = value;

    if (value < -0x80000000000) {
        R_FLAG |= 0x8000000 >> (i - 1);
    } else if (value > 0x7ffffffffff) {
        R_FLAG |= 0x40000000 >> (i - 1);
    }

    return ((value << 20) >> 20) >> cpu->gte_sf;
}

int64_t gte_clamp_ir0(psx_cpu_t* cpu, int64_t value) {
    if (value < 0) {
        R_FLAG |= 0x1000;

        return 0;
    } else if (value > 0x1000) {
        R_FLAG |= 0x1000;

        return 0x1000;
    }

    return value;
}

int64_t gte_clamp_sxy(psx_cpu_t* cpu, int i, int32_t value) {
    if (value < -0x400) {
        R_FLAG |= (uint32_t)(0x4000 >> (i - 1));

        return -0x400;
    } else if (value > 0x3ff) {
        R_FLAG |= (uint32_t)(0x4000 >> (i - 1));

        return 0x3ff;
    }

    return value;
}

int64_t gte_clamp_sz3(psx_cpu_t* cpu, int64_t value) {
    if (value < 0) {
        R_FLAG |= 0x40000;

        return 0;
    } else if (value > 0xffff) {
        R_FLAG |= 0x40000;

        return 0xffff;
    }

    return value;
}

uint8_t gte_clamp_rgb(psx_cpu_t* cpu, int i, int value) {
    if (value < 0) {
        R_FLAG |= (uint32_t)0x200000 >> (i - 1);

        return 0;
    } else if (value > 0xff) {
        R_FLAG |= (uint32_t)0x200000 >> (i - 1);

        return 0xff;
    }

    return (uint8_t)value;
}

int64_t gte_clamp_ir(psx_cpu_t* cpu, int i, int value, int lm) {
    if (lm && (value < 0)) {
        R_FLAG |= (uint32_t)(0x1000000 >> (i - 1));

        return 0;
    } else if ((value < -0x8000) && !lm) {
        R_FLAG |= (uint32_t)(0x1000000 >> (i - 1));

        return -0x8000;
    } else if (value > 0x7fff) {
        R_FLAG |= (uint32_t)(0x1000000 >> (i - 1));

        return 0x7fff;
    }

    return value;
}

int64_t gte_clamp_ir_z(psx_cpu_t* cpu, int64_t value, int sf, int lm) {
    int32_t value_sf = value >> sf;
    int32_t value_12 = value >> 12;
    int32_t min = 0;

    if (lm == 0)
        min = -0x8000;

    if (value_12 < -0x8000 || value_12 > 0x7fff)
        R_FLAG |= (1 << 22);

    return CLAMP(value_sf, min, 0x7fff);
}

int clz(uint32_t value) {
    if (!value)
        return 32;
    
    return __builtin_clz(value);
}

uint32_t gte_divide(psx_cpu_t* cpu, uint16_t n, uint16_t d) {
    // Overflow
    if (n >= d * 2) {
        R_FLAG |= (1 << 31) | (1 << 17);

        return 0x1ffff;
    }

    int shift = clz(d) - 16;

    int r1 = (d << shift) & 0x7fff;
    int r2 = g_psx_gte_unr_table[((r1 + 0x40) >> 7)] + 0x101;
    int r3 = ((0x80 - (r2 * (r1 + 0x8000))) >> 8) & 0x1ffff;

    uint32_t reciprocal = ((r2 * r3) + 0x80) >> 8;
    uint32_t res = ((((uint64_t)reciprocal * (n << shift)) + 0x8000) >> 16);

    return MIN(0x1ffff, res);
}

void gte_interpolate_color(psx_cpu_t* cpu, int mac1, int mac2, int mac3) {
    // PSX SPX is very convoluted about this and it lacks some info
    // [MAC1, MAC2, MAC3] = MAC + (FC - MAC) * IR0;< --- for NCDx only
    // Note: Above "[IR1,IR2,IR3]=(FC-MAC)" is saturated to - 8000h..+7FFFh(ie. as if lm = 0)
    // Details on "MAC+(FC-MAC)*IR0":
    // [IR1, IR2, IR3] = (([RFC, GFC, BFC] SHL 12) - [MAC1, MAC2, MAC3]) SAR(sf * 12)
    // [MAC1, MAC2, MAC3] = (([IR1, IR2, IR3] * IR0) + [MAC1, MAC2, MAC3])
    // [MAC1, MAC2, MAC3] = [MAC1, MAC2, MAC3] SAR(sf * 12);< --- for NCDx / NCCx
    // [IR1, IR2, IR3] = [MAC1, MAC2, MAC3]

    // R_MAC1 = (int)(gte_clamp_mac(cpu, 1, ((long)R_RFC << 12) - mac1) >> cpu->gte_sf);
    // R_MAC2 = (int)(gte_clamp_mac(cpu, 2, ((long)R_GFC << 12) - mac2) >> cpu->gte_sf);
    // R_MAC3 = (int)(gte_clamp_mac(cpu, 3, ((long)R_BFC << 12) - mac3) >> cpu->gte_sf);

    // R_IR1 = gte_clamp_ir(cpu, 1, R_MAC1, 0);
    // R_IR2 = gte_clamp_ir(cpu, 2, R_MAC2, 0);
    // R_IR3 = gte_clamp_ir(cpu, 3, R_MAC3, 0);

    // R_MAC1 = (int)(gte_clamp_mac(cpu, 1, ((long)R_IR1 * R_IR0) + mac1) >> cpu->gte_sf);
    // R_MAC2 = (int)(gte_clamp_mac(cpu, 2, ((long)R_IR2 * R_IR0) + mac2) >> cpu->gte_sf);
    // R_MAC3 = (int)(gte_clamp_mac(cpu, 3, ((long)R_IR3 * R_IR0) + mac3) >> cpu->gte_sf);

    // R_IR1 = gte_clamp_ir(cpu, 1, R_MAC1, cpu->gte_lm);
    // R_IR2 = gte_clamp_ir(cpu, 2, R_MAC2, cpu->gte_lm);
    // R_IR3 = gte_clamp_ir(cpu, 3, R_MAC3, cpu->gte_lm);
}

void gte_ncds(psx_cpu_t* cpu, int r) {
    //Normal color depth cue (single vector) //329048 WIP FLAGS
    //In: V0 = Normal vector(for triple variants repeated with V1 and V2),
    //BK = Background color, RGBC = Primary color / code, LLM = Light matrix, LCM = Color matrix, IR0 = Interpolation value.

    // uint16_t vrx = (&cpu->cop2_dr.v0)[(3 * r) + 0];
    // uint16_t vry = (&cpu->cop2_dr.v0)[(3 * r) + 1];
    // uint16_t vrz = (&cpu->cop2_dr.v0)[(3 * r) + 2];

    // // [IR1, IR2, IR3] = [MAC1, MAC2, MAC3] = (LLM * V0) SAR(sf * 12)
    // R_MAC1 = (int)(gte_clamp_mac(cpu, 1, (long)R_L11 * vrx + R_L12 * vry + R_L13 * vrz) >> cpu->gte_sf);
    // R_MAC2 = (int)(gte_clamp_mac(cpu, 2, (long)R_L21 * vrx + R_L22 * vry + R_L23 * vrz) >> cpu->gte_sf);
    // R_MAC3 = (int)(gte_clamp_mac(cpu, 3, (long)R_L31 * vrx + R_L32 * vry + R_L33 * vrz) >> cpu->gte_sf);

    // R_IR1 = gte_clamp_ir(cpu, 1, R_MAC1, cpu->gte_lm);
    // R_IR2 = gte_clamp_ir(cpu, 2, R_MAC2, cpu->gte_lm);
    // R_IR3 = gte_clamp_ir(cpu, 3, R_MAC3, cpu->gte_lm);

    // [IR1, IR2, IR3] = [MAC1, MAC2, MAC3] = (BK * 1000h + LCM * IR) SAR(sf * 12)
    // WARNING each multiplication can trigger mac flags so the check is needed on each op! Somehow this only affects the color matrix and not the light one
    // R_MAC1 = (int)(gte_clamp_mac(cpu, 1, gte_clamp_mac(cpu, 1, gte_clamp_mac(cpu, 1, (long)R_RBK * 0x1000 + R_LM1R * R_IR1) + (long)R_LM1G * R_IR2) + (long)R_LM1B * R_IR3) >> cpu->gte_sf);
    // R_MAC2 = (int)(gte_clamp_mac(cpu, 2, gte_clamp_mac(cpu, 2, gte_clamp_mac(cpu, 2, (long)R_GBK * 0x1000 + R_LM2R * R_IR1) + (long)R_LM2G * R_IR2) + (long)R_LM2B * R_IR3) >> cpu->gte_sf);
    // R_MAC3 = (int)(gte_clamp_mac(cpu, 3, gte_clamp_mac(cpu, 3, gte_clamp_mac(cpu, 3, (long)R_BBK * 0x1000 + R_LM3R * R_IR1) + (long)R_LM3G * R_IR2) + (long)R_LM3B * R_IR3) >> cpu->gte_sf);

    // R_IR1 = gte_clamp_ir(cpu, 1, R_MAC1, cpu->gte_lm);
    // R_IR2 = gte_clamp_ir(cpu, 2, R_MAC2, cpu->gte_lm);
    // R_IR3 = gte_clamp_ir(cpu, 3, R_MAC3, cpu->gte_lm);

    // [MAC1, MAC2, MAC3] = [R * IR1, G * IR2, B * IR3] SHL 4;< --- for NCDx / NCCx
    // R_MAC1 = (int)gte_clamp_mac(cpu, 1, ((long)R_RGBCR * R_IR1) << 4);
    // R_MAC2 = (int)gte_clamp_mac(cpu, 2, ((long)R_RGBCG * R_IR2) << 4);
    // R_MAC3 = (int)gte_clamp_mac(cpu, 3, ((long)R_RGBCB * R_IR3) << 4);

    // gte_interpolate_color(cpu, R_MAC1, R_MAC2, R_MAC3);

    // // Color FIFO = [MAC1 / 16, MAC2 / 16, MAC3 / 16, CODE]
    // R_RGB0 = R_RGB1;
    // R_RGB1 = R_RGB2;

    // uint32_t rgb2;

    // rgb2 = gte_clamp_rgb(cpu, 1, R_MAC1 >> 4);
    // rgb2 = gte_clamp_rgb(cpu, 2, R_MAC2 >> 4) << 8;
    // rgb2 = gte_clamp_rgb(cpu, 3, R_MAC3 >> 4) << 16;
    // rgb2 = R_RGBCC << 24;
}

void psx_cpu_i_gte(psx_cpu_t* cpu) {
    DO_PENDING_LOAD;

    cpu->gte_sf = ((cpu->opcode & 0x80000) != 0) * 12;
    cpu->gte_lm = (cpu->opcode & 0x400) != 0;
    cpu->gte_mmat = (cpu->opcode >> 13) & 3;
    cpu->gte_mvec = (cpu->opcode >> 15) & 3;
    cpu->gte_tvec = (cpu->opcode >> 17) & 3;

    g_psx_gte_table[cpu->opcode & 0x3f](cpu);
}

void psx_gte_i_invalid(psx_cpu_t* cpu) {
    log_fatal("invalid: Unimplemented GTE instruction %02x, %02x", cpu->opcode & 0x3f, cpu->opcode >> 25);
}

#define I64(v) ((int64_t)v)
#define R_TRX cpu->cop2_cr.tr.x
#define R_TRY cpu->cop2_cr.tr.y
#define R_TRZ cpu->cop2_cr.tr.z
#define R_RT11 cpu->cop2_cr.rt.m[0].c[0]
#define R_RT11 cpu->cop2_cr.rt.m[0].c[0]
#define R_RT12 cpu->cop2_cr.rt.m[0].c[1]
#define R_RT13 cpu->cop2_cr.rt.m[1].c[0]
#define R_RT21 cpu->cop2_cr.rt.m[1].c[1]
#define R_RT22 cpu->cop2_cr.rt.m[2].c[0]
#define R_RT23 cpu->cop2_cr.rt.m[2].c[1]
#define R_RT31 cpu->cop2_cr.rt.m[3].c[0]
#define R_RT32 cpu->cop2_cr.rt.m[3].c[1]
#define R_RT33 cpu->cop2_cr.rt.m33
#define R_MAC0 cpu->cop2_dr.mac[0]
#define R_MAC1 cpu->cop2_dr.mac[1]
#define R_MAC2 cpu->cop2_dr.mac[2]
#define R_MAC3 cpu->cop2_dr.mac[3]
#define R_OFX cpu->cop2_cr.ofx
#define R_OFY cpu->cop2_cr.ofy
#define R_IR0 cpu->cop2_dr.ir[0]
#define R_IR1 cpu->cop2_dr.ir[1]
#define R_IR2 cpu->cop2_dr.ir[2]
#define R_IR3 cpu->cop2_dr.ir[3]
#define R_SXY0 cpu->cop2_dr.sxy[0].xy
#define R_SX0 cpu->cop2_dr.sxy[0].p[0]
#define R_SY0 cpu->cop2_dr.sxy[0].p[1]
#define R_SZ0 cpu->cop2_dr.sz[0]
#define R_SXY1 cpu->cop2_dr.sxy[1].xy
#define R_SX1 cpu->cop2_dr.sxy[1].p[0]
#define R_SY1 cpu->cop2_dr.sxy[1].p[1]
#define R_SZ1 cpu->cop2_dr.sz[1]
#define R_SXY2 cpu->cop2_dr.sxy[2].xy
#define R_SX2 cpu->cop2_dr.sxy[2].p[0]
#define R_SY2 cpu->cop2_dr.sxy[2].p[1]
#define R_SZ2 cpu->cop2_dr.sz[2]
#define R_SZ3 cpu->cop2_dr.sz[3]
#define R_DQA cpu->cop2_cr.dqa
#define R_DQB cpu->cop2_cr.dqb
#define R_ZSF3 cpu->cop2_cr.zsf3
#define R_ZSF4 cpu->cop2_cr.zsf4
#define R_OTZ cpu->cop2_dr.otz
#define R_H cpu->cop2_cr.h
#define R_RC cpu->cop2_dr.rgbc.c[0]
#define R_GC cpu->cop2_dr.rgbc.c[1]
#define R_BC cpu->cop2_dr.rgbc.c[2]
#define R_CODE cpu->cop2_dr.rgbc.c[3]
#define R_RGBC cpu->cop2_dr.rgbc.rgbc
#define R_RFC cpu->cop2_cr.fc.x
#define R_GFC cpu->cop2_cr.fc.y
#define R_BFC cpu->cop2_cr.fc.z
#define R_RGB0 cpu->cop2_dr.rgb[0].rgbc
#define R_RGB1 cpu->cop2_dr.rgb[1].rgbc
#define R_RGB2 cpu->cop2_dr.rgb[2].rgbc
#define R_RC0 cpu->cop2_dr.rgb[0].c[0]
#define R_GC0 cpu->cop2_dr.rgb[0].c[1]
#define R_BC0 cpu->cop2_dr.rgb[0].c[2]
#define R_CD0 cpu->cop2_dr.rgb[0].c[3]
#define R_RC1 cpu->cop2_dr.rgb[1].c[0]
#define R_GC1 cpu->cop2_dr.rgb[1].c[1]
#define R_BC1 cpu->cop2_dr.rgb[1].c[2]
#define R_CD1 cpu->cop2_dr.rgb[1].c[3]
#define R_RC2 cpu->cop2_dr.rgb[2].c[0]
#define R_GC2 cpu->cop2_dr.rgb[2].c[1]
#define R_BC2 cpu->cop2_dr.rgb[2].c[2]
#define R_CD2 cpu->cop2_dr.rgb[2].c[3]

// void gte_rtp(psx_cpu_t* cpu, int i, int dq) {
//     R_FLAG = 0;

//     int64_t vx = (int64_t)(int16_t)cpu->cop2_dr.v[i].p[0];
//     int64_t vy = (int64_t)(int16_t)cpu->cop2_dr.v[i].p[1];
//     int64_t vz = (((int64_t)((int32_t)cpu->cop2_dr.v[i].z)) << 32) >> 32;

//     int64_t mac1 = (I64((int32_t)R_TRX) << 12) + (I64((int16_t)R_RT11) * vx) + (I64((int16_t)R_RT12) * vy) + (I64((int16_t)R_RT13) * vz);
//     int64_t mac2 = (I64((int32_t)R_TRY) << 12) + (I64((int16_t)R_RT21) * vx) + (I64((int16_t)R_RT22) * vy) + (I64((int16_t)R_RT23) * vz);
//     int64_t mac3 = (I64((int32_t)R_TRZ) << 12) + (I64((int16_t)R_RT31) * vx) + (I64((int16_t)R_RT32) * vy) + (I64((int16_t)R_RT33) * vz);

//     R_MAC1 = gte_clamp_mac(cpu, 1, mac1);
//     R_MAC2 = gte_clamp_mac(cpu, 2, mac2);
//     R_MAC3 = gte_clamp_mac(cpu, 3, mac3);

//     // log_set_quiet(0);
//     // log_fatal("mac1=%016x (%08x), vx=%016x, vy=%016x, vz=%016x (%08x), trx=%016x, res=%016x",
//     //     mac1, R_MAC1,
//     //     vx, vy, vz, cpu->cop2_dr.v[i].z,
//     //     I64((int32_t)R_TRX) << 12,
//     //     (I64((int32_t)R_TRX) << 12) + I64((int16_t)R_RT11) * vx + I64((int16_t)R_RT12) * vy + I64((int16_t)R_RT13) * vz
//     // );
//     // log_set_quiet(1);

//     R_IR1 = gte_clamp_ir(cpu, 1, I64((int32_t)R_MAC1), cpu->gte_lm);
//     R_IR2 = gte_clamp_ir(cpu, 2, I64((int32_t)R_MAC2), cpu->gte_lm);
//     R_IR3 = gte_clamp_ir_z(cpu, I64((int32_t)R_MAC3), mac3 >> 12, cpu->gte_lm);

//     R_SZ0 = R_SZ1;
//     R_SZ1 = R_SZ2;
//     R_SZ2 = R_SZ3;
//     R_SZ3 = gte_clamp_sz3(cpu, I64(mac3) >> 12);

//     int32_t h_div_sz = gte_divide(cpu, R_H, R_SZ3);

//     int64_t x = gte_clamp_mac0(cpu, I64((int32_t)R_OFX) + (I64((int16_t)R_IR1) * h_div_sz)) >> 16;
//     int64_t y = gte_clamp_mac0(cpu, I64((int32_t)R_OFY) + (I64((int16_t)R_IR2) * h_div_sz)) >> 16;

//     R_SXY0 = R_SXY1;
//     R_SXY1 = R_SXY2;
//     R_SX2 = gte_clamp_sxy(cpu, 1, x);
//     R_SY2 = gte_clamp_sxy(cpu, 2, y);

//     if (dq) {
//         int64_t mac0 = I64(R_DQB) + (h_div_sz * I64(R_DQA));

//         R_MAC0 = gte_clamp_mac0(cpu, mac0);
//         R_IR0 = gte_clamp_ir0(cpu, mac0 >> 12);

//         log_set_quiet(0);
//         log_fatal("mac0=%016x (%08x), dqa=%08x, dqb=%08x, h=%08x, sz=%08x, h/sz=%08x, ir0=%016x (%08x)",
//             mac0, R_MAC0,
//             R_DQA, R_DQB,
//             R_H, R_SZ3,
//             h_div_sz,
//             mac0 >> 12, R_IR0
//         );
//         log_set_quiet(1);
//     }
// }

void gte_rtp(psx_cpu_t* cpu, int i, int dq) {
    R_FLAG = 0;

    int64_t vx = (int64_t)(int16_t)cpu->cop2_dr.v[i].p[0];
    int64_t vy = (int64_t)(int16_t)cpu->cop2_dr.v[i].p[1];
    int64_t vz = (int64_t)(int16_t)cpu->cop2_dr.v[i].z;

    int64_t mac1 = (I64((int32_t)R_TRX) << 12) + (I64((int16_t)R_RT11) * vx) + (I64((int16_t)R_RT12) * vy) + (I64((int16_t)R_RT13) * vz);
    int64_t mac2 = (I64((int32_t)R_TRY) << 12) + (I64((int16_t)R_RT21) * vx) + (I64((int16_t)R_RT22) * vy) + (I64((int16_t)R_RT23) * vz);
    int64_t mac3 = (I64((int32_t)R_TRZ) << 12) + (I64((int16_t)R_RT31) * vx) + (I64((int16_t)R_RT32) * vy) + (I64((int16_t)R_RT33) * vz);

    R_MAC1 = gte_clamp_mac(cpu, 1, mac1);
    R_MAC2 = gte_clamp_mac(cpu, 2, mac2);
    R_MAC3 = gte_clamp_mac(cpu, 3, mac3);

    R_IR1 = gte_clamp_ir(cpu, 1, R_MAC1, cpu->gte_lm);
    R_IR2 = gte_clamp_ir(cpu, 2, R_MAC2, cpu->gte_lm);
    R_IR3 = gte_clamp_ir_z(cpu, cpu->s_mac3, cpu->gte_sf, cpu->gte_lm);

    R_SZ0 = R_SZ1;
    R_SZ1 = R_SZ2;
    R_SZ2 = R_SZ3;
    R_SZ3 = gte_clamp_sz3(cpu, cpu->s_mac3 >> 12);

    int64_t div = (int32_t)gte_divide(cpu, R_H, R_SZ3);

    R_SXY0 = R_SXY1;
    R_SXY1 = R_SXY2;
    R_SX2 = gte_clamp_sxy(cpu, 1, (gte_clamp_mac0(cpu, (int64_t)((int32_t)R_OFX) + ((int64_t)R_IR1 * div)) >> 16));
    R_SY2 = gte_clamp_sxy(cpu, 2, (gte_clamp_mac0(cpu, (int64_t)((int32_t)R_OFY) + ((int64_t)R_IR2 * div)) >> 16));

    if (dq) {
        R_MAC0 = gte_clamp_mac0(cpu, (int64_t)R_DQB + ((int64_t)R_DQA * div));
        R_IR0 = gte_clamp_ir0(cpu, cpu->s_mac0 >> 12);
    }
}

void psx_gte_i_rtps(psx_cpu_t* cpu) {
    // gte_rtp(cpu, 0, 1);

    R_FLAG = 0;

    // Fetch V0 values
    int64_t R_VX0 = (int64_t)((int16_t)cpu->cop2_dr.v[0].p[0]);
    int64_t R_VY0 = (int64_t)((int16_t)cpu->cop2_dr.v[0].p[1]);
    int64_t R_VZ0 = (int64_t)cpu->cop2_dr.v[0].z;

    // Calculate matrix product, checking for 44-bit overflow on the final
    // result:
    // if (value < -0x80000000000) {
    //     R_FLAG |= 0x8000000 >> (i - 1);
    // } else if (value > 0x7ffffffffff) {
    //     R_FLAG |= 0x40000000 >> (i - 1);
    // }
    R_MAC1 = gte_clamp_mac(cpu, 1, ((int64_t)R_TRX << 12) + (I64((int16_t)R_RT11) * R_VX0) + (I64((int16_t)R_RT12) * R_VY0) + (I64((int16_t)R_RT13) * R_VZ0));
    R_MAC2 = gte_clamp_mac(cpu, 2, ((int64_t)R_TRY << 12) + (I64((int16_t)R_RT21) * R_VX0) + (I64((int16_t)R_RT22) * R_VY0) + (I64((int16_t)R_RT23) * R_VZ0));
    R_MAC3 = gte_clamp_mac(cpu, 3, ((int64_t)R_TRZ << 12) + (I64((int16_t)R_RT31) * R_VX0) + (I64((int16_t)R_RT32) * R_VY0) + (I64((int16_t)R_RT33) * R_VZ0));

    // Store on IR1-3, clamping to -0x8000 (or 0) to 0x7fff
    R_IR1 = gte_clamp_ir(cpu, 1, R_MAC1, cpu->gte_lm);
    R_IR2 = gte_clamp_ir(cpu, 2, R_MAC2, cpu->gte_lm);
    R_IR3 = gte_clamp_ir_z(cpu, cpu->s_mac3, cpu->gte_sf, cpu->gte_lm);

    // Push screen Z: The unclamped value of MAC3 shifted right by 12,
    // clamping to 0x0000-0xffff
    R_SZ0 = R_SZ1;
    R_SZ1 = R_SZ2;
    R_SZ2 = R_SZ3;
    R_SZ3 = gte_clamp_sz3(cpu, cpu->s_mac3 >> 12);

    // Divide H by the pushed SZ3 value
    int32_t div = gte_divide(cpu, R_H, R_SZ3);

    // Push screen XY: X = OFX + (IR1 * (H/SZ3))
    //                 Y = OFY + (IR2 * (H/SZ3))
    // Clamping to (-0x400, 0x3ff)
    R_SXY0 = R_SXY1;
    R_SXY1 = R_SXY2;
    R_SX2 = gte_clamp_sxy(cpu, 1, (gte_clamp_mac0(cpu, (int64_t)((int32_t)R_OFX) + ((int64_t)R_IR1 * div)) >> 16));
    R_SY2 = gte_clamp_sxy(cpu, 2, (gte_clamp_mac0(cpu, (int64_t)((int32_t)R_OFY) + ((int64_t)R_IR2 * div)) >> 16));

    // Do depth cueing and store on MAC0, IR0, clamping MAC0 to (-0x80000000, 0x7fffffff)
    // and storing the unclamped MAC0 value to IR0, clamping to (0x0000, 0x1000)
    R_MAC0 = gte_clamp_mac0(cpu, ((int64_t)R_DQB) + (((int64_t)R_DQA) * div));
    R_IR0 = gte_clamp_ir0(cpu, cpu->s_mac0 >> 12);
}

void psx_gte_i_nclip(psx_cpu_t* cpu) {
    R_FLAG = 0;

    int64_t value = I64((int16_t)R_SX0) * (I64((int16_t)R_SY1) - I64((int16_t)R_SY2));
    value += I64((int16_t)R_SX1) * (I64((int16_t)R_SY2) - I64((int16_t)R_SY0));
    value += I64((int16_t)R_SX2) * (I64((int16_t)R_SY0) - I64((int16_t)R_SY1));

    R_MAC0 = (int)gte_clamp_mac0(cpu, value);
}

void psx_gte_i_op(psx_cpu_t* cpu) {
    R_FLAG = 0;

    R_MAC1 = gte_clamp_mac(cpu, 1, I64(I64((int16_t)R_RT22) * I64(R_IR3)) - I64((I64((int16_t)R_RT33) * I64(R_IR2))));
    R_MAC2 = gte_clamp_mac(cpu, 2, I64(I64((int16_t)R_RT33) * I64(R_IR1)) - I64((I64((int16_t)R_RT11) * I64(R_IR3))));
    R_MAC3 = gte_clamp_mac(cpu, 3, I64(I64((int16_t)R_RT11) * I64(R_IR2)) - I64((I64((int16_t)R_RT22) * I64(R_IR1))));

    R_IR1 = gte_clamp_ir(cpu, 1, R_MAC1, cpu->gte_lm);
    R_IR2 = gte_clamp_ir(cpu, 2, R_MAC2, cpu->gte_lm);
    R_IR3 = gte_clamp_ir(cpu, 3, R_MAC3, cpu->gte_lm);
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
    gte_ncds(cpu, 0);
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
    R_FLAG = 0;

    R_MAC1 = gte_clamp_mac(cpu, 1, I64(R_IR1) * I64(R_IR1));
    R_MAC2 = gte_clamp_mac(cpu, 2, I64(R_IR2) * I64(R_IR2));
    R_MAC3 = gte_clamp_mac(cpu, 3, I64(R_IR3) * I64(R_IR3));

    R_IR1 = gte_clamp_ir(cpu, 1, R_MAC1, cpu->gte_lm);
    R_IR2 = gte_clamp_ir(cpu, 2, R_MAC2, cpu->gte_lm);
    R_IR3 = gte_clamp_ir(cpu, 3, R_MAC3, cpu->gte_lm);
}

void psx_gte_i_dcpl(psx_cpu_t* cpu) {
    log_fatal("dpcl: Unimplemented GTE instruction");
}

void psx_gte_i_dpct(psx_cpu_t* cpu) {
    log_fatal("dpct: Unimplemented GTE instruction");
}

void psx_gte_i_avsz3(psx_cpu_t* cpu) {
    R_FLAG = 0;

    int64_t avg = I64(R_ZSF3) * (R_SZ1 + R_SZ2 + R_SZ3);

    R_MAC0 = (int)gte_clamp_mac0(cpu, avg);
    R_OTZ = gte_clamp_sz3(cpu, avg >> 12);
}

void psx_gte_i_avsz4(psx_cpu_t* cpu) {
    R_FLAG = 0;

    int64_t avg = I64(R_ZSF4) * (R_SZ0 + R_SZ1 + R_SZ2 + R_SZ3);

    R_MAC0 = (int)gte_clamp_mac0(cpu, avg);
    R_OTZ = gte_clamp_sz3(cpu, avg >> 12);
}

void psx_gte_i_rtpt(psx_cpu_t* cpu) {
    gte_rtp(cpu, 0, 0);
    gte_rtp(cpu, 1, 0);
    gte_rtp(cpu, 2, 1);
}

void psx_gte_i_gpf(psx_cpu_t* cpu) {
    R_FLAG = 0;

    R_MAC1 = gte_clamp_mac(cpu, 1, R_IR0 * R_IR1);
    R_MAC2 = gte_clamp_mac(cpu, 2, R_IR0 * R_IR2);
    R_MAC3 = gte_clamp_mac(cpu, 3, R_IR0 * R_IR3);
    R_IR1 = gte_clamp_ir(cpu, 1, R_MAC1, cpu->gte_lm);
    R_IR2 = gte_clamp_ir(cpu, 2, R_MAC2, cpu->gte_lm);
    R_IR3 = gte_clamp_ir(cpu, 3, R_MAC3, cpu->gte_lm);
    R_RGB0 = R_RGB1;
    R_RGB1 = R_RGB2;
    R_CD2 = R_CODE;
    R_RC2 = gte_clamp_rgb(cpu, 1, R_MAC1 >> 4);
    R_GC2 = gte_clamp_rgb(cpu, 2, R_MAC2 >> 4);
    R_BC2 = gte_clamp_rgb(cpu, 3, R_MAC3 >> 4);
}

void psx_gte_i_gpl(psx_cpu_t* cpu) {
    R_FLAG = 0;

    R_MAC1 = gte_clamp_mac(cpu, 1, (I64(R_MAC1) << cpu->gte_sf) + (R_IR0 * R_IR1));
    R_MAC2 = gte_clamp_mac(cpu, 2, (I64(R_MAC2) << cpu->gte_sf) + (R_IR0 * R_IR2));
    R_MAC3 = gte_clamp_mac(cpu, 3, (I64(R_MAC3) << cpu->gte_sf) + (R_IR0 * R_IR3));
    R_IR1 = gte_clamp_ir(cpu, 1, R_MAC1, cpu->gte_lm);
    R_IR2 = gte_clamp_ir(cpu, 2, R_MAC2, cpu->gte_lm);
    R_IR3 = gte_clamp_ir(cpu, 3, R_MAC3, cpu->gte_lm);
    R_RGB0 = R_RGB1;
    R_RGB1 = R_RGB2;
    R_CD2 = R_CODE;
    R_RC2 = gte_clamp_rgb(cpu, 1, R_MAC1 >> 4);
    R_GC2 = gte_clamp_rgb(cpu, 2, R_MAC2 >> 4);
    R_BC2 = gte_clamp_rgb(cpu, 3, R_MAC3 >> 4);
}

void psx_gte_i_ncct(psx_cpu_t* cpu) {
    log_fatal("ncct: Unimplemented GTE instruction");
}

#undef R_R0
#undef R_A0
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
#undef TRACE_C2M
#undef TRACE_C2MC
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