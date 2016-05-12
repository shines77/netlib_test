
#pragma once

#include <atomic>
#include <type_traits>

#define CACHELINE_SIZE  64

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
                        !std::is_final<T>::value &&
                        !std::is_volatile<T>::value> {};

} // namespace detail

////////////////////////////////////////////////////////////////////////////////////////

struct base_padding_data {};

template <typename T>
struct is_padding_data
{
    enum { value = std::is_base_of<T, base_padding_data>::value };
};

////////////////////////////////////////////////////////////////////////////////////////

template <typename T, std::size_t CacheLineSize, bool isInheritable = true>
struct alignas(CacheLineSize)
padding_data_impl : public T, public base_padding_data
{
    typedef T value_type;

    static const std::size_t kCacheLineSize = CacheLineSize;
    static const std::size_t kSizeofData = sizeof(value_type);
    static const std::size_t kPaddingSize =
        (kCacheLineSize - kSizeofData) > 0 ? (kCacheLineSize - kSizeofData) : 1;

    // Cacheline padding
    char padding[kPaddingSize];

    padding_data_impl(value_type value) : T(value) {};
    ~padding_data_impl() {};
};

template <typename T, std::size_t CacheLineSize>
struct padding_data_impl<T, CacheLineSize, false> : public base_padding_data
{
    typedef typename std::remove_volatile<T>::type _T;
    typedef _T value_type;

    static const std::size_t kCacheLineSize = CacheLineSize;
    static const std::size_t kSizeofData = sizeof(value_type);
    static const std::size_t kPaddingSize =
        (kCacheLineSize - kSizeofData) > 0 ? (kCacheLineSize - kSizeofData) : 1;

    // T aligned to cacheline size
    alignas(CacheLineSize) value_type data;

    // Cacheline padding
    char padding[kPaddingSize];

    padding_data_impl(value_type value) : data(value) {};
    ~padding_data_impl() {};
};

template <typename T, std::size_t CacheLineSize = CACHELINE_SIZE>
struct padding_data : public padding_data_impl<
                             T, CacheLineSize, detail::is_inheritable<T>::value> {};

////////////////////////////////////////////////////////////////////////////////////////

template <typename T, std::size_t CacheLineSize, bool isInheritable = true>
struct alignas(CacheLineSize)
volatile_padding_data_impl : public T, public base_padding_data
{
    typedef typename std::remove_volatile<T>::type _T;
    typedef volatile _T value_type;

    static const std::size_t kCacheLineSize = CacheLineSize;
    static const std::size_t kSizeofData = sizeof(value_type);
    static const std::size_t kPaddingSize =
        (kCacheLineSize - kSizeofData) > 0 ? (kCacheLineSize - kSizeofData) : 1;

    // Cacheline padding
    char padding[kPaddingSize];

    volatile_padding_data_impl(_T value) : T(value) {};
    ~volatile_padding_data_impl() {};
};

template <typename T, std::size_t CacheLineSize>
struct volatile_padding_data_impl<T, CacheLineSize, false> : public base_padding_data
{
    typedef typename std::remove_volatile<T>::type _T;
    typedef volatile _T value_type;

    static const std::size_t kCacheLineSize = CacheLineSize;
    static const std::size_t kSizeofData = sizeof(value_type);
    static const std::size_t kPaddingSize =
        (kCacheLineSize - kSizeofData) > 0 ? (kCacheLineSize - kSizeofData) : 1;

    // (volatile T) aligned to cacheline size
    alignas(CacheLineSize) value_type data;

    // Cacheline padding
    char padding[kPaddingSize];

    volatile_padding_data_impl(value_type value) : data(value) {};
    ~volatile_padding_data_impl() {};
};

template <typename T, std::size_t CacheLineSize = CACHELINE_SIZE>
struct volatile_padding_data : public volatile_padding_data_impl<
                                      T, CacheLineSize, detail::is_inheritable<T>::value> {};

////////////////////////////////////////////////////////////////////////////////////////

template <typename T, std::size_t CacheLineSize = CACHELINE_SIZE>
struct alignas(CacheLineSize)
padding_atomic : public std::atomic<typename std::remove_volatile<T>::type>,
                 public base_padding_data
{
    typedef typename std::remove_volatile<T>::type _T;
    typedef std::atomic<_T> value_type;

    static const std::size_t kCacheLineSize = CacheLineSize;
    static const std::size_t kSizeofData = sizeof(value_type);
    static const std::size_t kPaddingSize =
        (kCacheLineSize - kSizeofData) > 0 ? (kCacheLineSize - kSizeofData) : 1;

    // Cacheline padding
    char padding[kPaddingSize];

    padding_atomic(_T val) : std::atomic<typename std::remove_volatile<T>::type>(val) {};
    ~padding_atomic() {};
};

template <typename T, std::size_t CacheLineSize = CACHELINE_SIZE>
struct padding_atomic_wrapper : public base_padding_data
{
    typedef typename std::remove_volatile<T>::type _T;
    typedef std::atomic<_T> value_type;

    static const std::size_t kCacheLineSize = CacheLineSize;
    static const std::size_t kSizeofData = sizeof(value_type);
    static const std::size_t kPaddingSize =
        (kCacheLineSize - kSizeofData) > 0 ? (kCacheLineSize - kSizeofData) : 1;

    // std::atomic<T> aligned to cacheline size
    alignas(CacheLineSize) value_type data;

    // Cacheline padding
    char padding[kPaddingSize];

    padding_atomic_wrapper(_T value) : data(value) {};
    ~padding_atomic_wrapper() {};
};

////////////////////////////////////////////////////////////////////////////////////////

} // namespace asio_test

#pragma pack(pop)

#undef CACHELINE_SIZE
