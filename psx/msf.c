#include "msf.h"
#include "log.h"

uint8_t msf_btoi(uint8_t b) {
    return ((b >> 4) * 10) + (b & 0xf);
}

uint8_t msf_itob(int i) {
    return i + (6 * (i / 10));
}

void msf_copy(msf_t* dst, msf_t src) {
    dst->m = src.m;
    dst->s = src.s;
    dst->f = src.f;
}

void msf_adjust(msf_t* msf) {
    if (msf->f > 75) {
        int s = msf->f / CD_SECTORS_PS;

        msf->s += s;
        msf->f -= CD_SECTORS_PS * s;
    }

    if (msf->s > 60) {
        int m = msf->s / 60;

        msf->m += m;
        msf->s -= 60 * m;
    }
}

void msf_adjust_sub(msf_t* msf) {
    if ((int)msf->f < 0) {
        int f = ((int)msf->f) * -1;
        int s = (f / 60) + 1;

        msf->s -= s;
        msf->f += CD_SECTORS_PS * f;
    }

    if ((int)msf->s < 0) {
        int s = ((int)msf->s) * -1;
        int m = (s / 60) + 1;

        msf->m -= m;
        msf->s += 60 * m;
    }
}

void msf_to_bcd(msf_t* msf) {
    msf->m = ITOB(msf->m);
    msf->s = ITOB(msf->s);
    msf->f = ITOB(msf->f);
}

void msf_from_bcd(msf_t* msf) {
    msf->m = BTOI(msf->m);
    msf->s = BTOI(msf->s);
    msf->f = BTOI(msf->f);
}

uint32_t msf_to_address(msf_t msf) {
    return (((msf.m * 60) * CD_SECTORS_PS) + (msf.s * 75) + msf.f) * CD_SECTOR_SIZE;
}

void msf_from_address(msf_t* msf, uint32_t addr) {
    msf->f = addr / CD_SECTOR_SIZE;
    msf->s = msf->f / CD_SECTORS_PS;
    msf->m = msf->s / 60;
    msf->s -= msf->m * 60;
    msf->f -= ((msf->m * 60) + msf->s) * CD_SECTORS_PS;
}

void msf_add_f(msf_t* msf, int f) {
    msf->f += f;

    msf_adjust(msf);
}

void msf_add_s(msf_t* msf, int s) {
    msf->s += s;

    msf_adjust(msf);
}