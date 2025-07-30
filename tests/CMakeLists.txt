# Include test headers
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# SDL and emu includes already come from top-level
# Now add Check manually since find_package won't work

if(WIN32)
    set(CHECK_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/../windows_deps/check/include")
    set(CHECK_LIBRARY "${PROJECT_SOURCE_DIR}/../windows_deps/check/lib/x64/libcheck.a")

    include_directories(${CHECK_INCLUDE_DIR})
endif()

# Test sources
set(TEST_SOURCES
    check_gbe.c
)

# Executable
add_executable(check_gbe ${TEST_SOURCES})

# Link dependencies
target_link_libraries(check_gbe
    emu
    ${SDL2_LIBRARIES}
    ${SDL2_TTF_LIBRARIES}
    ${CHECK_LIBRARY}
)

# Project includes (emu headers etc.)
target_include_directories(check_gbe PRIVATE ${PROJECT_SOURCE_DIR}/include)
