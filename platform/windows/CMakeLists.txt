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

# Bộ gõ TSF cho Windows 7 SP1 → Windows 11 (x86 và x64)
add_library(DhammaKey SHARED
  DllMain.cpp
  TextService.cpp
  LangBar.cpp
  MenuArt.cpp
  Conflict.cpp
  Guard.cpp
  DhammaKey.def
  DhammaKey.rc)
target_link_libraries(DhammaKey PRIVATE dkcore ole32 oleaut32 uuid advapi32 user32 gdi32 shell32 gdiplus oleacc uxtheme)
target_include_directories(DhammaKey PRIVATE ${CMAKE_SOURCE_DIR}/core/include)
set_target_properties(DhammaKey PROPERTIES OUTPUT_NAME "DhammaKey")
# Số phiên bản cho thông tin tệp (DhammaKey.rc) lấy từ project VERSION ở CMakeLists.txt gốc
target_compile_definitions(DhammaKey PRIVATE DK_VERSION="${PROJECT_VERSION}"
  DK_VER_MAJOR=${PROJECT_VERSION_MAJOR} DK_VER_MINOR=${PROJECT_VERSION_MINOR} DK_VER_PATCH=${PROJECT_VERSION_PATCH})
