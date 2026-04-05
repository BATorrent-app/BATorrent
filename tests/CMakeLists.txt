# =============================================================================
#  BATorrent Test Suite
#  Framework: Catch2 v3
#  Tools:     ASan, MSVC /analyze, CppCheck, CRT Debug Heap, Dr. Memory
# =============================================================================
enable_testing()

find_package(Qt6 REQUIRED COMPONENTS Widgets Network Svg Test)
find_package(LibtorrentRasterbar REQUIRED)

# --- Fetch Catch2 v3 ---
include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.7.1
)
FetchContent_MakeAvailable(Catch2)
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(CTest)
include(Catch)

# --- Common sources (everything except main.cpp) ---
set(COMMON_SOURCES
    ${CMAKE_SOURCE_DIR}/src/torrent/sessionmanager.cpp
    ${CMAKE_SOURCE_DIR}/src/app/translator.cpp
    ${CMAKE_SOURCE_DIR}/src/webui/webserver.cpp
    ${CMAKE_SOURCE_DIR}/src/app/rssmanager.cpp
    ${CMAKE_SOURCE_DIR}/src/app/addonmanager.cpp
    ${CMAKE_SOURCE_DIR}/src/gui/torrentmodel.cpp
    ${CMAKE_SOURCE_DIR}/src/gui/torrentfilter.cpp
)

set(COMMON_LIBS
    Qt6::Widgets Qt6::Network Qt6::Svg Qt6::Test
    LibtorrentRasterbar::torrent-rasterbar
    Catch2::Catch2WithMain
)

set(COMMON_DEFS APP_VERSION="test")
if(WIN32)
    list(APPEND COMMON_DEFS _WIN32_WINNT=0x0601)
endif()

# =============================================================================
#  OPTIONS
# =============================================================================
option(BAT_ASAN       "Enable AddressSanitizer"              OFF)
option(BAT_CPPCHECK   "Enable CppCheck static analysis"      OFF)
option(BAT_MSVC_ANALYZE "Enable MSVC /analyze"               OFF)
option(BAT_CRT_DEBUG  "Enable Windows CRT Debug Heap"        OFF)
option(BAT_DRMEMORY   "Add Dr. Memory wrapper targets"       OFF)

# =============================================================================
#  TOOL: AddressSanitizer (ASan)
# =============================================================================
if(BAT_ASAN)
    if(MSVC)
        add_compile_options(/fsanitize=address)
        # MSVC ASan doesn't need a link flag, but incompatible with /RTC
        string(REGEX REPLACE "/RTC[^ ]*" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
        string(REGEX REPLACE "/RTC[^ ]*" "" CMAKE_C_FLAGS_DEBUG   "${CMAKE_C_FLAGS_DEBUG}")
    else()
        add_compile_options(-fsanitize=address -fno-omit-frame-pointer -g)
        add_link_options(-fsanitize=address)
    endif()
    message(STATUS "[BAT] AddressSanitizer ENABLED")
endif()

# =============================================================================
#  TOOL: MSVC Code Analysis (/analyze)
# =============================================================================
if(BAT_MSVC_ANALYZE AND MSVC)
    add_compile_options(/analyze /analyze:external- /W4)
    message(STATUS "[BAT] MSVC /analyze ENABLED")
endif()

# =============================================================================
#  TOOL: CppCheck
# =============================================================================
if(BAT_CPPCHECK)
    find_program(CPPCHECK_EXE cppcheck)
    if(CPPCHECK_EXE)
        set(CMAKE_CXX_CPPCHECK
            ${CPPCHECK_EXE}
            --enable=warning,performance,portability
            --suppress=missingIncludeSystem
            --inline-suppr
            --quiet
        )
        message(STATUS "[BAT] CppCheck ENABLED: ${CPPCHECK_EXE}")
    else()
        message(WARNING "[BAT] CppCheck requested but not found in PATH")
    endif()
endif()

# =============================================================================
#  TOOL: Windows CRT Debug Heap
# =============================================================================
if(BAT_CRT_DEBUG AND WIN32)
    list(APPEND COMMON_DEFS BAT_CRT_DEBUG_HEAP)
    message(STATUS "[BAT] CRT Debug Heap ENABLED")
endif()

# =============================================================================
#  Helper: create a test target
# =============================================================================
function(bat_add_test NAME SOURCE)
    # Qt MOC needs AUTOMOC
    set(CMAKE_AUTOMOC ON)
    qt_add_executable(${NAME} ${SOURCE} ${COMMON_SOURCES})
    target_compile_definitions(${NAME} PRIVATE ${COMMON_DEFS})
    target_link_libraries(${NAME} PRIVATE ${COMMON_LIBS})
    target_include_directories(${NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)
    catch_discover_tests(${NAME})

    # --- Dr. Memory wrapper target ---
    if(BAT_DRMEMORY AND WIN32)
        find_program(DRMEMORY_EXE drmemory HINTS "C:/Program Files (x86)/Dr. Memory/bin")
        if(DRMEMORY_EXE)
            add_custom_target(${NAME}_drmemory
                COMMAND ${DRMEMORY_EXE}
                    -batch
                    -brief
                    -no_follow_children
                    -suppress ${CMAKE_CURRENT_SOURCE_DIR}/drmemory_suppress.txt
                    -- $<TARGET_FILE:${NAME}>
                DEPENDS ${NAME}
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMENT "Running Dr. Memory on ${NAME}..."
            )
        endif()
    endif()
endfunction()

# =============================================================================
#  TEST TARGETS
# =============================================================================
bat_add_test(test_security test_security.cpp)
bat_add_test(test_memory   test_memory.cpp)
bat_add_test(test_unit     test_unit.cpp)

# =============================================================================
#  Convenience: run all analysis tools
# =============================================================================
add_custom_target(check
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -C $<CONFIG>
    DEPENDS test_security test_memory test_unit
    COMMENT "Running all BATorrent tests..."
)
