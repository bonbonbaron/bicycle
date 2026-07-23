#include <vector>
#include <stdexcept>
#include <iostream>

#define DEBUG(false)
#if DEBUG
#include <iomanip>
#endif

template<typename T>
class Array2D {
private:
    std::vector<T> data;
    size_t rows_;
    size_t cols_;

public:
    // Constructor
    explicit Array2D(size_t rows = 0, size_t cols = 0, T default_value = T{})
        : rows_(rows), cols_(cols), data(rows * cols, default_value) {}

    size_t rows() const noexcept { return rows_; }
    size_t cols() const noexcept { return cols_; }
    size_t size() const noexcept { return data.size(); }

    // Row-major contiguous access
    T& operator()(size_t row, size_t col) {
        return data[row * cols_ + col];
    }

    const T& operator()(size_t row, size_t col) const {
        return data[row * cols_ + col];
    }

    // Bounds-checked access
    T& at(size_t row, size_t col) {
        if (row >= rows_ || col >= cols_) {
            throw std::out_of_range("Array2D::at() - index out of bounds");
        }
        return data[row * cols_ + col];
    }

    const T& at(size_t row, size_t col) const {
        if (row >= rows_ || col >= cols_) {
            throw std::out_of_range("Array2D::at() - index out of bounds");
        }
        return data[row * cols_ + col];
    }

    // Direct access to underlying contiguous storage
    T* data_ptr() noexcept { return data.data(); }
    const T* data_ptr() const noexcept { return data.data(); }

#if DEBUG
    // Print utility  (TODO eliminate this and iomanip when no longer needed)
    void print() const {
        for (size_t i = 0; i < rows_; ++i) {
            for (size_t j = 0; j < cols_; ++j) {
                std::cout << std::setw(4) << (*this)(i, j) << " ";
            }
            std::cout << '\n';
        }
    }
#endif
};
