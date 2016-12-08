#pragma once

#if defined(_WIN32) || defined(WIN32) || defined(OS_WINDOWS) || defined(_WINDOWS)
#  if !defined(_WIN32_WINNT) || (_WIN32_WINNT < 0x0501)
#    define _WIN32_WINNT	0x0601
#  endif
#endif

#if defined(_MSC_VER)
  #define BOOST_ASIO_MSVC   _MSC_VER
#endif

#if defined(BOOST_ASIO_MSVC)
#  if (_MSC_VER >= 1900)
#    define BOOST_ASIO_ERROR_CATEGORY_NOEXCEPT noexcept(true)
#  endif // (_MSC_VER >= 1900)
#endif // defined(BOOST_ASIO_MSVC)
