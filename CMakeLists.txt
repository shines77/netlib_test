CMAKE_MINIMUM_REQUIRED(VERSION 2.8.12)
project(netlib_test)

set(CMAKE_VERBOSE_MAKEFILE ON)

# Single thread mode /ML and /MLd have be deprecated from VS 2003.
option(BoostOption_USE_MULTITHREADED "Enable Boost's library use multithread" ON)

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

find_package(Boost 1.55 COMPONENTS coroutine context system thread chrono program_options REQUIRED)

if (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    set(EXTRA_LIBS ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
endif()

find_package (Threads)
Include_directories(include)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

message("------------ Options -------------")
message("  CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

message("----------------------------------")

message("-------------- Env ---------------")
message("  CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")
message("  CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")
message("  PROJECT_SOURCE_DIR: ${PROJECT_SOURCE_DIR}")
message("  PROJECT_BINARY_DIR: ${PROJECT_BINARY_DIR}")
message("----------------------------------")

if (NOT MSVC)
    add_compile_options("-std=c++11")
    add_compile_options("-Wall")
    add_compile_options("-Wno-unused-function")
    add_compile_options("-Wno-deprecated-declarations")
    add_compile_options("-Wno-unused-variable")
    ##add_compile_options("-Wextra")
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
            if (${CompilerFlag} MATCHES "/MD")
                string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
            endif()
        endforeach()
    endif()
endif()

if (UNIX)
    set(EXTRA_LIBS ${EXTRA_LIBS} pthread)
endif()

include_directories(
    "${PROJECT_SOURCE_DIR}/src"
    )

set(SOURCE_FILES
    src/common/padding_atomic.hpp
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

#
# See: http://stackoverflow.com/questions/7988297/cmake-to-add-vs2010-project-custom-build-events
#
#add_custom_command(TARGET asio_echo_serv
#    POST_BUILD
#    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:asio_echo_serv> ${SDK_DIR}
#    COMMAND ${CMAKE_COMMAND} -E $<$<CONFIG:Release>:copy_if_different> $<$<NOT:$<CONFIG:Release>>:echo> $<TARGET_FILE:asio_echo_serv> ${REMOTE_DIR}
#    COMMENT "Copy SDK files ..."
#)

#add_custom_command(TARGET asio_echo_serv
#    POST_BUILD
#    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:asio_echo_serv> ${SDK_DIR}
#    COMMENT "Copy SDK files ..."
#)

#add_custom_command(TARGET asio_echo_serv
#    POST_BUILD
#    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:asio_echo_serv> "$<$<CONFIG:Release>:${REMOTE_DIR}>$<$<NOT:$<CONFIG:Release>>:$<TARGET_FILE:asio_echo_serv>>"
#    COMMENT "Copy update files ..."
#)

include (InstallRequiredSystemLibraries)
