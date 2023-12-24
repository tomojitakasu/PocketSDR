//
//  Pocket SDR C AP - Generate FFTW Wisdom
//
//  Author:
//  T.TAKASU
//
//  History:
//  2022-07-20  1.0  port fftw_wisdom.py to C.
//
#include "pocket_sdr.h"

// constants --------------------------------------------------------------------
#define FFTW_WISDOM "./fftw_wisdom.txt"

//-------------------------------------------------------------------------------
//
//   Synopsis
//
//     fftw_wisdowm [-n size] [file]
//
//   Description
//
//     Generate FFTW wisdom. FFTW wisdom is used to optimize FFT and IFFT
//     performance by FFTW in target environment.
// 
//   Options ([]: default)
// 
//     -n size
//         FFT and IFFT size. [48000]
//
//     file
//         Output FFTW wisdom file. [fftw_wisdom.txt]
//
int main(int argc, char **argv)
{
    int N = 48000;
    char *file = FFTW_WISDOM;
    
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-n") && i + 1 < argc) {
            N = atoi(argv[++i]);
        }
        else {
            file = argv[i];
        }
    }
    uint32_t tick = sdr_get_tick();
    
    if (sdr_gen_fftw_wisdom(file, N)) {
        printf("FFTW wisdom generated as %s (N=%d).\n", file, N);
    }
    else {
        printf("FFTW wisdom generation error.\n");
    }
    printf("  TIME(s) = %.3f\n", (sdr_get_tick() - tick) * 1e-3);
    return 0;
}
