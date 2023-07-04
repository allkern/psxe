#include <stdio.h>

#include "exe.h"
#include "log.h"

void psx_exe_load(psx_cpu_t* cpu, const char* path) {
    FILE* file = fopen(path, "rb");

    if (!file) {
        log_error("Couldn't open PS-X EXE file \"%s\"", path);

        exit(1);
    }

    // Read header
    psx_exe_hdr_t hdr;
    
    fread((char*)&hdr, 1, sizeof(psx_exe_hdr_t), file);

    // Seek to program start 
    fseek(file, 0x800, SEEK_SET);

    // Read to RAM directly
    uint32_t offset = hdr.ramdest & 0x7fffffff;

    fread(cpu->bus->ram->buf + offset, 1, hdr.filesz, file);

    // Load initial register values
    cpu->pc = hdr.ipc;
    cpu->next_pc = cpu->pc + 4;
    cpu->r[28] = hdr.igp;
    cpu->r[29] = hdr.ispb + hdr.ispoff;
    cpu->r[30] = cpu->r[29];

    psx_cpu_fetch(cpu);

    log_info("Loaded PS-X EXE file \"%s\"", path);

    fclose(file);
}