# MSVC x64 Toolchain File
# This ensures 64-bit compilation on Windows with MSVC

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR AMD64)

# Note: CMAKE_GENERATOR_PLATFORM is only for Visual Studio generators, not Ninja
# Visual Studio's CMake Settings already specifies x64 via inheritEnvironments

# Ensure we use 64-bit MSVC compiler
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
	message(FATAL_ERROR "32-bit compiler detected! This project requires 64-bit compilation.")
endif()
