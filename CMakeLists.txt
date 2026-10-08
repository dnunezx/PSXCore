cmake_minimum_required(VERSION 3.16)
project(PSXCore VERSION 0.1.0 LANGUAGES C)

add_library(psxcore STATIC src/vcd.c src/request.c src/elf.c src/boot_elf.c src/boot_sha256.c
    src/compat.c src/launch.c src/file.c src/file_patch.c src/patch.c src/runtime_corrections.c src/discmap.c src/sha1.c)
add_library(PSXCore::core ALIAS psxcore)
target_include_directories(psxcore PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_compile_features(psxcore PUBLIC c_std_11)
