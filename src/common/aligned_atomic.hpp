
#pragma once

#include <atomic>
#include <type_traits>

#define CACHE_LINE_SIZE  64

#pragma pack(push)
#pragma pack(1)

namespace asio_test {

////////////////////////////////////////////////////////////////////////////////////////

namespace detail {

//
// See: http://en.cppreference.com/w/cpp/experimental/type_trait_variable_templates
// See: http://en.cppreference.com/w/cpp/types/is_arithmetic
// See: http://en.cppreference.com/w/cpp/types/is_class
//
template <typename T, bool isArithmetic = false>
struct is_inheritable : std::integral_constant<bool,
                        std::is_class<T>::value  &&
#if (defined(__cplusplus) && (__cplusplus >= 201300L)) || (defined(_MSC_VER) && (_MSC_VER >= 1900L))
                        !std::is_final<T>::value &&
#endif // std::is_final<T> only can use in C++ 14.
                        !std::is_volatile<T>::value> {};

} // namespace detail

////////////////////////////////////////////////////////////////////////////////////////

struct base_aligned_data {};

template <typename T>
struct is_aligned_data
{
    enum { value = std::is_base_of<T, base_aligned_data>::value };
};

////////////////////////////////////////////////////////////////////////////////////////

template <typename T, std::size_t CacheLineSize, bool isInheritable = true>
struct alignas(CacheLineSize)
aligned_data_impl : public T, public base_aligned_data
{
    typedef T value_type;

    static const std::size_t kCacheLineSize = CacheLineSize;
    static const std::size_t kSizeofData = sizeof(value_type);
    static const std::size_t kPaddingBytes =
        (kCacheLineSize - kSizeofData) > 0 ? (kCacheLineSize - kSizeofData) : 1;

    // Cacheline padding
    char padding[kPaddingBytes];

    aligned_data_impl(value_type value) : T(value) {};
    ~aligned_data_impl() {};
};

template <typename T, std::size_t CacheLineSize>
struct aligned_data_impl<T, CacheLineSize, false> : public base_aligned_data
{
    typedef typename std::remove_volatile<T>::type _T;
    typedef _T value_type;

    static const std::size_t kCacheLineSize = CacheLineSize;
    static const std::size_t kSizeofData = sizeof(value_type);
    static const std::size_t kPaddingBytes =
        (kCacheLineSize - kSizeofData) > 0 ? (kCacheLineSize - kSizeofData) : 1;

    // T aligned to cacheline size
    alignas(CacheLineSize) value_type data;

    // Cacheline padding
    char padding[kPaddingBytes];

    aligned_data_impl(value_type value) : data(value) {};
    ~aligned_data_impl() {};
};

template <typename T, std::size_t CacheLineSize = CACHE_LINE_SIZE>
struct aligned_data : public aligned_data_impl<T, CacheLineSize, detail::is_inheritable<T>::value> {};

////////////////////////////////////////////////////////////////////////////////////////

template <typename T, std::size_t CacheLineSize, bool isInheritable = true>
struct alignas(CacheLineSize)
volatile_aligned_data_impl : public T, public base_aligned_data
{
    typedef typename std::remove_volatile<T>::type _T;
    typedef volatile _T value_type;

    static const std::size_t kCacheLineSize = CacheLineSize;
    static const std::size_t kSizeofData = sizeof(value_type);
    static const std::size_t kPaddingBytes =
        (kCacheLineSize - kSizeofData) > 0 ? (kCacheLineSize - kSizeofData) : 1;

    // Cacheline padding
    char padding[kPaddingBytes];

    volatile_aligned_data_impl(_T value) : T(value) {};
    ~volatile_aligned_data_impl() {};
};

template <typename T, std::size_t CacheLineSize>
struct volatile_aligned_data_impl<T, CacheLineSize, false> : public base_aligned_data
{
    typedef typename std::remove_volatile<T>::type _T;
    typedef volatile _T value_type;

    static const std::size_t kCacheLineSize = CacheLineSize;
    static const std::size_t kSizeofData = sizeof(value_type);
    static const std::size_t kPaddingBytes =
        (kCacheLineSize - kSizeofData) > 0 ? (kCacheLineSize - kSizeofData) : 1;

    // (volatile T) aligned to cacheline size
    alignas(CacheLineSize) value_type data;

    // Cacheline padding
    char padding[kPaddingBytes];

    volatile_aligned_data_impl(value_type value) : data(value) {};
    ~volatile_aligned_data_impl() {};
};

template <typename T, std::size_t CacheLineSize = CACHE_LINE_SIZE>
struct volatile_aligned_data : public volatile_aligned_data_impl<T, CacheLineSize, detail::is_inheritable<T>::value> {};

////////////////////////////////////////////////////////////////////////////////////////

template <typename T, std::size_t CacheLineSize = CACHE_LINE_SIZE>
struct alignas(CacheLineSize)
aligned_atomic : public std::atomic<typename std::remove_volatile<T>::type>,
                 public base_aligned_data
{
    typedef typename std::remove_volatile<T>::type _T;
    typedef std::atomic<_T> value_type;

    static const std::size_t kCacheLineSize = CacheLineSize;
    static const std::size_t kSizeofData = sizeof(value_type);
    static const std::size_t kPaddingBytes =
        (kCacheLineSize - kSizeofData) > 0 ? (kCacheLineSize - kSizeofData) : 1;

    // Cacheline padding
    char padding[kPaddingBytes];

    aligned_atomic(_T val) : std::atomic<typename std::remove_volatile<T>::type>(val) {};
    ~aligned_atomic() {};
};

template <typename T, std::size_t CacheLineSize = CACHE_LINE_SIZE>
struct aligned_atomic_wrapper : public base_aligned_data
{
    typedef typename std::remove_volatile<T>::type _T;
    typedef std::atomic<_T> value_type;

    static const std::size_t kCacheLineSize = CacheLineSize;
    static const std::size_t kSizeofData = sizeof(value_type);
    static const std::size_t kPaddingBytes =
        (kCacheLineSize - kSizeofData) > 0 ? (kCacheLineSize - kSizeofData) : 1;

    // std::atomic<T> aligned to cacheline size
    alignas(CacheLineSize) value_type data;

    // Cacheline padding
    char padding[kPaddingBytes];

    aligned_atomic_wrapper(_T value) : data(value) {};
    ~aligned_atomic_wrapper() {};
};

////////////////////////////////////////////////////////////////////////////////////////

} // namespace asio_test

#pragma pack(pop)

#undef CACHE_LINE_SIZE
