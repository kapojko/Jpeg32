cmake_minimum_required(VERSION 3.15)
project(Jpeg32 VERSION 1.0.0 LANGUAGES C)

# Set C17 standard
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Options
option(JPEG_BUILD_TESTS "Build test executables" ON)
option(JPEG_BUILD_TOOLS "Build utility tools" ON)

# Detect ARM build
if(CMAKE_SYSTEM_PROCESSOR MATCHES "ARM|Cortex")
    set(JPEG_USE_ARM 1)
endif()

# Compiler-specific settings
if(MSVC)
    # MSVC warnings
    add_compile_options(/W4 /WX)
    # Disable warnings for system headers
    add_compile_options(/wd4820 /wd5045)
else()
    # GCC/Clang warnings
    add_compile_options(-Wall -Wextra -Wpedantic)
    # Additional strict warnings
    add_compile_options(-Wconversion -Wsign-conversion)
endif()

# Include directories
target_include_directories(${PROJECT_NAME} INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Create library target
add_library(${PROJECT_NAME} STATIC
    src/jpeg_encoder.c
    src/jpeg_tables.c
    src/jpeg_block_copy.c
    src/jpeg_dct.c
    src/jpeg_huffman.c
    src/jpeg_bitstream.c
)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Set public compile definitions
target_compile_definitions(${PROJECT_NAME} PUBLIC
    JPEG_QUALITY_DEFAULT=75
    JPEG_QUALITY_MIN=1
    JPEG_QUALITY_MAX=100
)

# Add ARM-specific definitions if needed
if(JPEG_USE_ARM)
    target_compile_definitions(${PROJECT_NAME} PUBLIC
        JPEG_USE_CMSIS_DSP=1
    )
endif()

# Tests
if(JPEG_BUILD_TESTS)
    enable_testing()
    
    # Unit tests
    add_executable(test_unit 
        test/test_unit.c
        test/test_images/test_images.c
    )
    target_link_libraries(test_unit PRIVATE ${PROJECT_NAME} m)
    target_compile_definitions(test_unit PRIVATE JPEG_USE_CMSIS_DSP=0)
    
    # End-to-end tests
    add_executable(test_e2e
        test/test_e2e.c
    )
    target_link_libraries(test_e2e PRIVATE ${PROJECT_NAME} m)
    target_compile_definitions(test_e2e PRIVATE JPEG_USE_CMSIS_DSP=0)
    
    add_test(NAME unit_tests COMMAND test_unit)
    add_test(NAME e2e_tests COMMAND test_e2e)
endif()

# Tools
if(JPEG_BUILD_TOOLS)
    add_executable(jpeg_verify
        tools/verify_jpeg.c
    )
    target_link_libraries(jpeg_verify PRIVATE ${PROJECT_NAME})
    
    add_executable(jpeg_profile
        tools/profile_jpeg.c
    )
    target_link_libraries(jpeg_profile PRIVATE ${PROJECT_NAME} m)
endif()

# Install rules
install(TARGETS ${PROJECT_NAME}
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

# Export project
export(PACKAGE ${PROJECT_NAME})

# Create package config
configure_file(cmake/${PROJECT_NAME}Config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    @ONLY
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    DESTINATION lib/cmake/${PROJECT_NAME}
)
