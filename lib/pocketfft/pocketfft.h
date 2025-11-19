//
// PocketFFT wrapper function header
//
#ifndef POCKETFFT_H
#define POCKETFFT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#define FFTW_FORWARD (-1)
#define FFTW_BACKWARD (+1)
#define FFTW_ESTIMATE (0U)
#define FFTW_MEASURE (1U)
#define FFTW_PATIENT (2U)

typedef float fftwf_complex[2];
typedef struct pocketfftwf_plan_s* fftwf_plan;

void *fftwf_malloc(size_t nbytes);
void fftwf_free(void* p);
fftwf_plan fftwf_plan_dft_1d(int n, fftwf_complex* in, fftwf_complex* out,
    int sign, unsigned flags);
void fftwf_execute_dft(const fftwf_plan p, fftwf_complex* in,
    fftwf_complex* out);
void fftwf_destroy_plan(fftwf_plan p);
int fftwf_import_wisdom_from_filename(const char* filename);
int fftwf_export_wisdom_to_filename(const char* filename);
void fftwf_cleanup(void);

#ifdef __cplusplus
}
#endif
#endif // POCKETFFT_H
