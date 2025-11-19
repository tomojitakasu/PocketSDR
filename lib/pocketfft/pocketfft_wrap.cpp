//
// PocketFFT wrapper functions
//
#include <cstdlib>
#include <cstring>
#include <new>
#include <complex>

#include "pocketfft_hdronly.h"
#include "pocketfft.h"

struct pocketfftwf_plan_s {
    int n, sign;
};

extern "C" {

fftwf_plan fftwf_plan_dft_1d(int n, fftwf_complex *in, fftwf_complex *out,
    int sign, unsigned flags)
{
    if (n <= 0 || !(sign == FFTW_FORWARD || sign == FFTW_BACKWARD)) {
        return nullptr;
    }
    pocketfftwf_plan_s* p = nullptr;
    try {
        p = new pocketfftwf_plan_s();
    } catch (...) {
        return nullptr;
    }
    p->n = n;
    p->sign = sign;
    return p;
}

void fftwf_execute_dft(const fftwf_plan p, fftwf_complex* in,
    fftwf_complex* out)
{
    if (!p || !in || !out) return;
    const size_t N = static_cast<size_t>(p->n);
    
    auto* in_c = reinterpret_cast<std::complex<float>*>(in);
    auto* out_c = reinterpret_cast<std::complex<float>*>(out);
    
    using namespace pocketfft; // for detail
    pocketfft::detail::shape_t shape{N};
    pocketfft::detail::stride_t stride_in{static_cast<ptrdiff_t>(sizeof(std::complex<float>))};
    pocketfft::detail::stride_t stride_out{static_cast<ptrdiff_t>(sizeof(std::complex<float>))};
    pocketfft::detail::shape_t axes{0};
    
    const bool dir = (p->sign == FFTW_FORWARD) ?
        pocketfft::detail::FORWARD : pocketfft::detail::BACKWARD;
    
    pocketfft::detail::c2c<float>(shape, stride_in, stride_out, axes, dir, in_c,
        out_c, 1.0f, 1);
}

void fftwf_destroy_plan(fftwf_plan p)
{
    delete p;
}

void* fftwf_malloc(size_t nbytes)
{
#if defined(_WIN32)
    // 32-byte alignment for SIMD friendliness
    void* p = _aligned_malloc(nbytes, 32);
    return p;
#else
    void* p = nullptr;
    if (posix_memalign(&p, 32, nbytes) != 0) return nullptr;
    return p;
#endif
}

void fftwf_free(void* p)
{
#if defined(_WIN32)
    _aligned_free(p);
#else
    free(p);
#endif
}

int fftwf_import_wisdom_from_filename(const char *filename)
{
    return 1;
}

int fftwf_export_wisdom_to_filename(const char *filename)
{
    return 1;
}

void fftwf_cleanup(void)
{
}

} // extern "C"
