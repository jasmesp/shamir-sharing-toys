cmake_minimum_required(VERSION 3.5.0)
project(secret-sharing VERSION 0.1.0 LANGUAGES C)

add_executable(secret-sharing main.c)
set(CMAKE_TOOLCHAIN_FILE ${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
