#ifndef expect_fallback_HXX
#define expect_fallback_HXX

#include "maroontress/lighter/Flint.hxx"

#define expect(x) maroontress::lighter::Flint {(x), #x, [](auto r) { \
        auto fileName = __FILE__; \
        auto line = __LINE__; \
        std::ostringstream out; \
        out << fileName << ":" << line << ": error:" << std::endl \
            << "  Expected: " << r.getExpected() << std::endl \
            << "  Actual:   " << r.getActual() << std::endl; \
        throw std::runtime_error(out.str()); \
    }}

#endif
