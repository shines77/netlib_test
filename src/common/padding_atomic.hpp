
#pragma once

#include <atomic>
#include <type_traits>

#define CACHELINE_SIZE  64

#pragma pack(push)
#pragma pack(1)

namespace asio_test {

struct base_padding_data {};

template <typename T>
class is_padding_data
{
    enum { value = std::is_base_of<T, base_padding_data>::value };
};

template <typename T>
struct alignas(CACHELINE_SIZE)
    padding_data : public std::atomic<T>, public base_padding_data
{
    typedef typename std::remove_volatile<T>::type _T;
    typedef _T value_type;

    static const std::size_t kSizeofData = sizeof(value_type);
    static const std::size_t kPaddingSize =
        (CACHELINE_SIZE - kSizeofData) > 0 ? (CACHELINE_SIZE - kSizeofData) : 1;

    // Cacheline padding
    char padding[kPaddingSize];

    padding_data(_T val) : std::atomic<_T>(val) {};
    ~padding_data() {};
};

template <typename T>
struct alignas(CACHELINE_SIZE)
    padding_volatile : public std::atomic<T>, public base_padding_data
{
    typedef typename std::remove_volatile<T>::type _T;
    typedef volatile _T value_type;

    static const std::size_t kSizeofData = sizeof(value_type);
    static const std::size_t kPaddingSize =
        (CACHELINE_SIZE - kSizeofData) > 0 ? (CACHELINE_SIZE - kSizeofData) : 1;

    // Cacheline padding
    char padding[kPaddingSize];

    padding_volatile(_T val) : std::atomic<_T>(val) {};
    ~padding_volatile() {};
};

template <typename T>
struct alignas(CACHELINE_SIZE)
    padding_atomic : public std::atomic<T>, public base_padding_data
{
    typedef typename std::remove_volatile<T>::type _T;
    typedef std::atomic<_T> value_type;

    static const std::size_t kSizeofData = sizeof(value_type);
    static const std::size_t kPaddingSize =
        (CACHELINE_SIZE - kSizeofData) > 0 ? (CACHELINE_SIZE - kSizeofData) : 1;

    // Cacheline padding
    char padding[kPaddingSize];

    padding_atomic(_T val) : std::atomic<_T>(val) {};
    ~padding_atomic() {};
};

/*
template <typename T>
struct padding_data : public base_padding_data
{
    typedef typename std::remove_volatile<T>::type _T;
    typedef _T value_type;

    static const std::size_t kSizeofData = sizeof(value_type);
    static const std::size_t kPaddingSize =
        (CACHELINE_SIZE - kSizeofData) > 0 ? (CACHELINE_SIZE - kSizeofData) : 1;

    // T aligned to cacheline size
    alignas(CACHELINE_SIZE) value_type value;

    // Cacheline padding
    char padding[kPaddingSize];

    void padding_data(_T val) : value(val) {}
    void ~padding_data(_T val) {}
};

template <typename T>
struct padding_volatile : public base_padding_data
{
    typedef typename std::remove_volatile<T>::type _T;
    typedef volatile _T value_type;

    static const std::size_t kSizeofData = sizeof(value_type);
    static const std::size_t kPaddingSize =
        (CACHELINE_SIZE - kSizeofData) > 0 ? (CACHELINE_SIZE - kSizeofData) : 1;

    // (volatile T) aligned to cacheline size
    alignas(CACHELINE_SIZE) value_type value;

    // Cacheline padding
    char padding[kPaddingSize];

    void padding_volatile(_T val) : value(val) {}
    void ~padding_volatile(_T val) {}
};

template <typename T>
struct padding_atomic : public base_padding_data
{
    typedef typename std::remove_volatile<T>::type _T;
    typedef std::atomic<_T> value_type;

    static const std::size_t kSizeofData = sizeof(value_type);
    static const std::size_t kPaddingSize =
        (CACHELINE_SIZE - kSizeofData) > 0 ? (CACHELINE_SIZE - kSizeofData) : 1;

    // std::atomic<T> aligned to cacheline size
    alignas(CACHELINE_SIZE) value_type value;

    // Cacheline padding
    char padding[kPaddingSize];

    void padding_atomic(_T val) : value(val) {}
    void ~padding_atomic(_T val) {}
};
//*/

} // namespace asio_test

#pragma pack(pop)

#undef CACHELINE_SIZE
