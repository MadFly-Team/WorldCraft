cmake_minimum_required(VERSION 3.27.0)

project(CoreLibraryDemo VERSION 1.0.0)

# Create executable
add_executable(CoreLibraryDemo
	main.cpp
)

# Link CoreLibrary
target_link_libraries(CoreLibraryDemo PRIVATE
	CoreLibrary
	SDL2main
)

# Set C++20 standard
set_property(TARGET CoreLibraryDemo PROPERTY CXX_STANDARD 20)
set_property(TARGET CoreLibraryDemo PROPERTY CXX_STANDARD_REQUIRED ON)

# Copy SDL2.dll to output directory on Windows (if using shared SDL2)
if(WIN32)
	add_custom_command(TARGET CoreLibraryDemo POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
		$<TARGET_FILE:SDL2-static>
		$<TARGET_FILE_DIR:CoreLibraryDemo>
		COMMENT "Copying SDL2 dependencies to output directory"
	)
endif()

message(STATUS "CoreLibraryDemo configured successfully")
