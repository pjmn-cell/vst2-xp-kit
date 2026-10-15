// Maths sans runtime C : exp, sin/cos par séries, racine carrée par fsqrt inline.
#pragma once
namespace bf {
static const double kPI = 3.14159265358979323846;

static inline double mexp(double x) {
    if (x < -700.0) return 0.0;
    if (x > 700.0) x = 700.0;
    const double LN2 = 0.69314718055994530942, INV = 1.4426950408889634074;
    double k = x * INV;
    long n = (long)(k + (k >= 0 ? 0.5 : -0.5));
    double r = x - (double)n * LN2;
    double p = 1.0 + r*(1.0 + r*(1.0/2 + r*(1.0/6 + r*(1.0/24 + r*(1.0/120 + r*(1.0/720 +
               r*(1.0/5040 + r*(1.0/40320 + r*(1.0/362880 + r*(1.0/3628800 + r/39916800))))))))));
    union { double d; unsigned long long u; } s;
    s.u = (unsigned long long)(n + 1023) << 52;
    return p * s.d;
}
// w dans [0, pi]
static inline void msincos(double w, double& s, double& c) {
    bool flip = false; double a = w;
    if (a > kPI/2) { a = kPI - a; flip = true; }
    double a2 = a*a;
    s = a*(1 - a2/6*(1 - a2/20*(1 - a2/42*(1 - a2/72*(1 - a2/110*(1 - a2/156*(1 - a2/210)))))));
    c = 1 - a2/2*(1 - a2/12*(1 - a2/30*(1 - a2/56*(1 - a2/90*(1 - a2/132*(1 - a2/182))))));
    if (flip) c = -c;
}
// ln(x), x > 0 (plage float) : exposant + série atanh sur la mantisse
static inline double mlog(double x) {
    union { float f; unsigned u; } b; b.f = (float)x;
    int e = (int)((b.u >> 23) & 255) - 127;
    b.u = (b.u & 0x7fffffu) | 0x3f800000u;
    double m = b.f, z = (m - 1.0) / (m + 1.0), z2 = z * z;
    double s = z*(1.0 + z2*(1.0/3 + z2*(1.0/5 + z2*(1.0/7 + z2*(1.0/9 + z2*(1.0/11 + z2/13.0))))));
    return e * 0.69314718055994530942 + 2.0 * s;
}
static inline float msqrt(float x) { return __builtin_sqrtf(x); }
static inline float mfabs(float x) { return __builtin_fabsf(x); }
static inline float mmin(float a, float b) { return a < b ? a : b; }
} // namespace bf
