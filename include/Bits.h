#include <bitset>

#include <cstdint>

#if defined(__GNUC__) || defined(__clang__)
    #define PORTABLE_CTZ(x) __builtin_ctzll(x)
#elif defined(_MSC_VER)
    #include <intrin.h>
    #pragma intrinsic(_BitScanForward64)
    inline unsigned long portable_ctz(uint64_t x) {
        unsigned long index;
        _BitScanForward64(&index, x);
        return index;
    }
    #define PORTABLE_CTZ(x) portable_ctz(x)
#else
    // Slow but correct fallback
    inline int portable_ctz(uint64_t x) {
        if (x == 0) return 64;
        int n = 0;
        while ((x & 1) == 0) { x >>= 1; ++n; }
        return n;
    }
    #define PORTABLE_CTZ(x) portable_ctz(x)
#endif

// Usage (mirrors _Find_first behavior)
inline size_t find_first_set(uint64_t word) noexcept {
    if (word == 0) return ~size_t(0);  // or appropriate sentinel like bitset size
    return PORTABLE_CTZ(word);
}

template <size_t N>
size_t find_first(const std::bitset<N>& bs) {
    // Iterate over 64-bit words for high performance
    for (size_t i = 0; i < N; i += 64) {
        uint64_t word = (bs >> i).to_ullong();  // or manual word extraction if needed
        if (word) return i + find_first_set(word);
    }
    return N;  // none set
}
