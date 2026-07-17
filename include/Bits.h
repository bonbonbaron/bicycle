#include <bitset>

template <size_t N>
size_t find_first(const std::bitset<N>& bs) {
    // Iterate over 64-bit words for high performance
    for (size_t i = 0; i < N; i += 64) {
        uint64_t word = (bs >> i).to_ullong();  // or manual word extraction if needed
        if (word) return i + find_first_set(word);
    }
    return N;  // none set
}
