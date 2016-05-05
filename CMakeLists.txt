CMAKE_MINIMUM_REQUIRED(VERSION 2.8.12)
project(netlib_test)

set(CMAKE_VERBOSE_MAKEFILE ON)

# Single thread mode /ML and /MLd have be deprecated from VS 2003.
OPTION(BoostOption_USE_MULTITHREADED "Enable Boost's library use multithread" ON)

if (WIN32)
    option(BoostOption_USE_STATIC_LIBS "Enable Boost use static library" ON)
    option(BoostOption_USE_STATIC_RUNTIME "Enable Boost use static runtime library" OFF)
else()
    option(BoostOption_USE_STATIC_LIBS "Enable Boost use static library" OFF)
    option(BoostOption_USE_STATIC_RUNTIME "Enable Boost use static runtime library" OFF)
endif()

set(Boost_USE_STATIC_LIBS        BoostOption_USE_STATIC_LIBS)      # only find static libs
set(Boost_USE_MULTITHREADED      BoostOption_USE_MULTITHREADED)
set(Boost_USE_STATIC_RUNTIME     BoostOption_USE_STATIC_RUNTIME)

find_package(Boost 1.54 COMPONENTS coroutine context system thread chrono REQUIRED)

if (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    set(EXTRA_LIBS ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
endif()

find_package (Threads)
Include_directories(include)

if (CMAKE_BUILD_TYPE)
else()
    set(CMAKE_BUILD_TYPE RELEASE)
endif()

message("------------ Options -------------")
message("  CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

message("----------------------------------")

message("-------------- Env ---------------")
message("  CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")
message("  CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")
message("----------------------------------")

if (NOT MSVC)
    add_compile_options("-std=c++11")
    add_compile_options("-Wall")
    add_compile_options("-Wextra")
endif()

if (WIN32)
    add_compile_options("-D_WIN32_WINNT=0x0501")
    add_compile_options("-D_CRT_SECURE_NO_WARNINGS")
    set(EXTRA_LIBS ${EXTRA_LIBS} ws2_32 mswsock)
    if (Boost_USE_STATIC_RUNTIME)
        #
        # Replace all "/MD" flags to "/MT"
        # See: http://stackoverflow.com/questions/14172856/cmake-compile-with-mt-instead-of-md
        #
        set(CompilerFlags
            CMAKE_CXX_FLAGS
            CMAKE_CXX_FLAGS_DEBUG
            CMAKE_CXX_FLAGS_RELEASE
            CMAKE_CXX_FLAGS_MINSIZEREL
            CMAKE_CXX_FLAGS_RELWITHDEBINFO
            CMAKE_C_FLAGS
            CMAKE_C_FLAGS_DEBUG
            CMAKE_C_FLAGS_RELEASE
            CMAKE_C_FLAGS_MINSIZEREL
            CMAKE_C_FLAGS_RELWITHDEBINFO
            )
        foreach(CompilerFlag ${CompilerFlags})
            if (${CompilerFlags} MATCHES "/MD")
                string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
            endif()
        endforeach()
    endif()
endif()

if (UNIX)
    set(EXTRA_LIBS ${EXTRA_LIBS} pthread)
endif()

include_directories(
    "${PROJECT_BINARY_DIR}/src/asio/asio_echo_serv"
    )

set(SOURCE_FILES
    src/asio/asio_echo_serv/asio_echo_serv.cpp
    )

subdirs(
    test
)

add_custom_target(debug
    COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Debug ${CMAKE_SOURCE_DIR}
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target all
    COMMENT "Switch CMAKE_BUILD_TYPE to Debug"
)
add_custom_target(release
    COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Release ${CMAKE_SOURCE_DIR}
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target all
    COMMENT "Switch CMAKE_BUILD_TYPE to Release"
)

add_executable(asio_echo_serv
    src/asio/asio_echo_serv/asio_echo_serv.cpp)
target_link_libraries(asio_echo_serv ${EXTRA_LIBS})

add_executable(asio_echo_client
    src/asio/asio_echo_client/asio_echo_client.cpp)
target_link_libraries(asio_echo_client ${EXTRA_LIBS})

include (InstallRequiredSystemLibraries)
