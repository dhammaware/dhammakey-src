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

# Cửa sổ cài đặt DhammaKey (Qt 5.15 Widgets)
# Chỉ biên dịch khi tìm thấy Qt:  cmake ... -DCMAKE_PREFIX_PATH=C:\Qt\5.15.2\msvc2019_64
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(DK_SETTINGS_SOURCES main.cpp net.cpp settings.qrc)
if(WIN32)
  list(APPEND DK_SETTINGS_SOURCES
       ${CMAKE_SOURCE_DIR}/platform/windows/Conflict.cpp
       settings.rc)
endif()

add_executable(DhammaKeySettings WIN32 ${DK_SETTINGS_SOURCES})
target_link_libraries(DhammaKeySettings PRIVATE Qt5::Widgets)
target_include_directories(DhammaKeySettings PRIVATE
  ${CMAKE_SOURCE_DIR}/core/include
  ${CMAKE_SOURCE_DIR}/platform/windows)
target_compile_definitions(DhammaKeySettings PRIVATE DK_VERSION="${PROJECT_VERSION}"
  DK_VER_MAJOR=${PROJECT_VERSION_MAJOR} DK_VER_MINOR=${PROJECT_VERSION_MINOR} DK_VER_PATCH=${PROJECT_VERSION_PATCH})

if(WIN32)
  target_link_libraries(DhammaKeySettings PRIVATE advapi32 user32 shell32 dwmapi winhttp)
  # Qt dùng thư viện C++ dạng DLL → chương trình này cũng phải dùng /MD (khác DLL bộ gõ dùng /MT)
  set_property(TARGET DhammaKeySettings PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

  # Sau khi biên dịch: chép thư viện Qt và thư viện C++ (vcruntime140.dll…) vào cạnh tệp .exe,
  # để thư mục này đem đi cài là chạy được ngay, không cần cài thêm gì.
  get_target_property(_qmake Qt5::qmake IMPORTED_LOCATION)
  get_filename_component(_qtbin "${_qmake}" DIRECTORY)
  set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
  include(InstallRequiredSystemLibraries)
  add_custom_command(TARGET DhammaKeySettings POST_BUILD
    COMMAND "${_qtbin}/windeployqt.exe" --release --no-translations --no-system-d3d-compiler
            --no-opengl-sw --no-angle --no-compiler-runtime "$<TARGET_FILE:DhammaKeySettings>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}
            "$<TARGET_FILE_DIR:DhammaKeySettings>"
    COMMENT "Deploy Qt + VC++ runtime next to DhammaKeySettings.exe")

  # Giấy phép đi theo bộ cài (bản 0.3.18): bộ cài WiX gom cả thư mục này nên tự có các tệp dưới.
  # Cửa sổ Về chúng tôi mở LICENSE.vi.txt và THIRD-PARTY-NOTICES.txt; LGPL/GPL đi kèm vì có thư viện Qt.
  add_custom_command(TARGET DhammaKeySettings POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/LICENSE"
            "$<TARGET_FILE_DIR:DhammaKeySettings>/LICENSE.txt"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/LICENSE.vi.md"
            "$<TARGET_FILE_DIR:DhammaKeySettings>/LICENSE.vi.txt"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/THIRD-PARTY-NOTICES.txt"
            "$<TARGET_FILE_DIR:DhammaKeySettings>/THIRD-PARTY-NOTICES.txt"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:DhammaKeySettings>/licenses"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/LICENSES/LGPL-3.0-only.txt"
            "${CMAKE_SOURCE_DIR}/LICENSES/GPL-3.0-only.txt"
            "${CMAKE_SOURCE_DIR}/LICENSES/OFL-1.1.txt"
            "$<TARGET_FILE_DIR:DhammaKeySettings>/licenses"
    COMMENT "Copy license files next to DhammaKeySettings.exe")
endif()
