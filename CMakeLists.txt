# SPDX-FileCopyrightText: 2026 Ujudhammo
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
#
# DhammaKey - a DhammaWare project
# Homepage: https://dhammaware.org
#
# Copyright (C) 2026 Ujudhammo.
#
# TERMS OF USE AND NONCOMMERCIAL COMMITMENT:
# This software is offered as dana (a free gift) to serve the community, to
# support study and research, and to help spread the Dhamma. You may use, read,
# study, modify and contribute improvements to this source code for
# noncommercial purposes.
#
# PROHIBITED: copying, packaging, redistributing or using this source code or
# any derivative work for commercial purposes, for sale, or for any fee.
#
# The full, legally binding terms are in the LICENSE file at the project root.
# Third-party components keep their own licenses; see THIRD-PARTY-NOTICES.txt.
#
# THIS SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND. IN NO EVENT
# SHALL THE AUTHOR BE LIABLE FOR ANY DAMAGES ARISING FROM ITS USE.

cmake_minimum_required(VERSION 3.16)
project(DhammaKey VERSION 0.3.34 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
  # Chỉ áp cho C/C++ — trình biên dịch tài nguyên (rc.exe) không nhận các tuỳ chọn này
  add_compile_options($<$<COMPILE_LANGUAGE:C,CXX>:/utf-8> $<$<COMPILE_LANGUAGE:C,CXX>:/W4>)
  # Chạy được từ Windows 7 SP1
  add_compile_definitions(_WIN32_WINNT=0x0601 WINVER=0x0601 UNICODE _UNICODE)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>") # không cần cài VC++ runtime
else()
  add_compile_options(-Wall -Wextra)
endif()

# ---- Lõi ----
add_library(dkcore STATIC
  core/src/vn.cpp
  core/src/pali.cpp
  core/src/engine.cpp
  core/src/charset.cpp
  core/src/capi.cpp)
target_include_directories(dkcore PUBLIC core/include)
set_target_properties(dkcore PROPERTIES POSITION_INDEPENDENT_CODE ON)

# ---- Kiểm thử ----
enable_testing()
add_executable(dk_tests core/tests/test_main.cpp)
target_link_libraries(dk_tests PRIVATE dkcore)
add_test(NAME core COMMAND dk_tests)

# ---- Công cụ gõ thử trên dòng lệnh ----
add_executable(dkdemo tools/dkdemo.cpp)
target_link_libraries(dkdemo PRIVATE dkcore)

# ---- Windows: bộ gõ TSF ----
if(WIN32)
  add_subdirectory(platform/windows)
endif()

# ---- macOS: bộ gõ Input Method Kit (khung đầu tiên, chưa thử trên máy thật) ----
if(APPLE)
  add_subdirectory(platform/macos)
endif()

# ---- Cửa sổ cài đặt (Qt 5.15) — chỉ biên dịch khi tìm thấy Qt ----
#   cmake -S . -B build-x64 -A x64 -DCMAKE_PREFIX_PATH=C:/Qt/5.15.2/msvc2019_64
find_package(Qt5 5.15 COMPONENTS Widgets QUIET)
if(Qt5Widgets_FOUND)
  add_compile_definitions(NOMINMAX)
  add_subdirectory(settings)
else()
  message(STATUS "Khong thay Qt 5.15: bo qua cua so cai dat (them -DCMAKE_PREFIX_PATH=C:/Qt/5.15.2/msvc2019_64)")
endif()
