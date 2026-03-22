cmake_minimum_required(VERSION 3.16)
project(qitto VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Define LINUX_PORT for all targets
add_compile_definitions(LINUX_PORT)

find_package(Qt6 REQUIRED COMPONENTS Widgets Sql DBus)
find_package(PkgConfig REQUIRED)
pkg_check_modules(XCB REQUIRED xcb xcb-keysyms)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)

# --------------------------------------------------------------------------
# Adapted Ditto source files (ported via compat.h + #ifdef LINUX_PORT)
# --------------------------------------------------------------------------
set(DITTO_SOURCES
    src/sqlite/CppSQLite3.cpp
    src/Crc32Dynamic.cpp
    src/CF_TextAggregator.cpp
    src/CF_UnicodeTextAggregator.cpp
    src/HTMLFormatAggregator.cpp
    src/RichTextAggregator.cpp
)

# --------------------------------------------------------------------------
# Linux port files (new code + adapted implementations)
# --------------------------------------------------------------------------
set(LINUX_SOURCES
    linux/main.cpp
    linux/QittoApp.cpp
    linux/Settings.cpp
    linux/DatabaseUtilities_linux.cpp
    linux/Misc_linux.cpp
    linux/Clip_linux.cpp
    linux/ClipboardMonitor.cpp
    linux/FormatMapper.cpp
    linux/MainWindow.cpp
    linux/ClipListModel.cpp
    linux/ClipDelegate.cpp
    linux/SearchBox.cpp
    linux/GlobalHotkey.cpp
    linux/HotkeyManager.cpp
    linux/PasteInjector.cpp
    linux/PortalInputInjector.cpp
    linux/SettingsDialog.cpp
)

add_executable(qitto ${DITTO_SOURCES} ${LINUX_SOURCES})

target_include_directories(qitto PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/linux
    ${CMAKE_SOURCE_DIR}/Shared
    ${CMAKE_SOURCE_DIR}
    ${SQLITE3_INCLUDE_DIRS}
    ${XCB_INCLUDE_DIRS}
)

target_link_libraries(qitto PRIVATE
    Qt6::Widgets
    Qt6::Sql
    Qt6::DBus
    ${SQLITE3_LIBRARIES}
    ${XCB_LIBRARIES}
)

install(TARGETS qitto RUNTIME DESTINATION bin)

# --------------------------------------------------------------------------
# Tests
# --------------------------------------------------------------------------
enable_testing()

find_package(Qt6 REQUIRED COMPONENTS Test)

add_executable(test_crc32
    tests/test_crc32.cpp
    src/Crc32Dynamic.cpp
)
target_include_directories(test_crc32 PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/linux
    ${CMAKE_SOURCE_DIR}
)
target_link_libraries(test_crc32 PRIVATE Qt6::Test Qt6::Core)
add_test(NAME test_crc32 COMMAND test_crc32)
