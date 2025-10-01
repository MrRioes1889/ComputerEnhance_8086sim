cmake_minimum_required(VERSION 3.10)
set(APP_NAME 8086sim)
set(PROJ_DIR ${CMAKE_SOURCE_DIR}/8086sim)
set(SOURCE_DIR ${PROJ_DIR}/sauce)

project(${APP_NAME} C)

set(CMAKE_CONFIGURATION_TYPES Debug Release ODebug)
set(CMAKE_C_STANDARD 17)

if(NOT MSVC)
    set(CMAKE_C_FLAGS_DEBUG "-g -O0")
    set(CMAKE_C_FLAGS_ODEBUG "-g -O3")
    set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")

    # Provide a default build type if none was specified
    if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE Debug)
    endif()
else()
    set(CMAKE_C_FLAGS_ODEBUG "/O2 /Zi /DODEBUG /DDEBUG")
endif()

file(GLOB_RECURSE SRC_FILES "${SOURCE_DIR}/*.c")
file(GLOB_RECURSE HEADER_FILES "${SOURCE_DIR}/*.h")

# Set output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

# Create executable from all .c files
add_executable(${APP_NAME} ${SRC_FILES} ${HEADER_FILES})

# Make sure object files go into bin-int
set_target_properties(
    ${APP_NAME} PROPERTIES
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS ON
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
    OUTPUT_NAME "${APP_NAME}"
)

if (MSVC)
    target_compile_options(${APP_NAME} PRIVATE /W4 /WX /wd4100)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(${APP_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()


target_include_directories(
    ${APP_NAME}
    PRIVATE
        ${SOURCE_DIR}
)

# Make Visual Studio solution a tad less obnoxious
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${APP_NAME})
source_group(TREE "${PROJ_DIR}" FILES ${SRC_FILES})
source_group(TREE "${PROJ_DIR}" FILES ${HEADER_FILES})