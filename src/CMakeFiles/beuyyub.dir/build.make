# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.28

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/zynt/Documents/Code/C++/beuyyub-git

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/zynt/Documents/Code/C++/beuyyub-git/src

# Include any dependencies generated for this target.
include CMakeFiles/beuyyub.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/beuyyub.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/beuyyub.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/beuyyub.dir/flags.make

CMakeFiles/beuyyub.dir/main.cpp.o: CMakeFiles/beuyyub.dir/flags.make
CMakeFiles/beuyyub.dir/main.cpp.o: main.cpp
CMakeFiles/beuyyub.dir/main.cpp.o: CMakeFiles/beuyyub.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/home/zynt/Documents/Code/C++/beuyyub-git/src/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/beuyyub.dir/main.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/beuyyub.dir/main.cpp.o -MF CMakeFiles/beuyyub.dir/main.cpp.o.d -o CMakeFiles/beuyyub.dir/main.cpp.o -c /home/zynt/Documents/Code/C++/beuyyub-git/src/main.cpp

CMakeFiles/beuyyub.dir/main.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing CXX source to CMakeFiles/beuyyub.dir/main.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/zynt/Documents/Code/C++/beuyyub-git/src/main.cpp > CMakeFiles/beuyyub.dir/main.cpp.i

CMakeFiles/beuyyub.dir/main.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling CXX source to assembly CMakeFiles/beuyyub.dir/main.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/zynt/Documents/Code/C++/beuyyub-git/src/main.cpp -o CMakeFiles/beuyyub.dir/main.cpp.s

CMakeFiles/beuyyub.dir/youtubeapi.cpp.o: CMakeFiles/beuyyub.dir/flags.make
CMakeFiles/beuyyub.dir/youtubeapi.cpp.o: youtubeapi.cpp
CMakeFiles/beuyyub.dir/youtubeapi.cpp.o: CMakeFiles/beuyyub.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/home/zynt/Documents/Code/C++/beuyyub-git/src/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/beuyyub.dir/youtubeapi.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/beuyyub.dir/youtubeapi.cpp.o -MF CMakeFiles/beuyyub.dir/youtubeapi.cpp.o.d -o CMakeFiles/beuyyub.dir/youtubeapi.cpp.o -c /home/zynt/Documents/Code/C++/beuyyub-git/src/youtubeapi.cpp

CMakeFiles/beuyyub.dir/youtubeapi.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing CXX source to CMakeFiles/beuyyub.dir/youtubeapi.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/zynt/Documents/Code/C++/beuyyub-git/src/youtubeapi.cpp > CMakeFiles/beuyyub.dir/youtubeapi.cpp.i

CMakeFiles/beuyyub.dir/youtubeapi.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling CXX source to assembly CMakeFiles/beuyyub.dir/youtubeapi.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/zynt/Documents/Code/C++/beuyyub-git/src/youtubeapi.cpp -o CMakeFiles/beuyyub.dir/youtubeapi.cpp.s

# Object files for target beuyyub
beuyyub_OBJECTS = \
"CMakeFiles/beuyyub.dir/main.cpp.o" \
"CMakeFiles/beuyyub.dir/youtubeapi.cpp.o"

# External object files for target beuyyub
beuyyub_EXTERNAL_OBJECTS =

beuyyub: CMakeFiles/beuyyub.dir/main.cpp.o
beuyyub: CMakeFiles/beuyyub.dir/youtubeapi.cpp.o
beuyyub: CMakeFiles/beuyyub.dir/build.make
beuyyub: /usr/lib/libdpp.so
beuyyub: /usr/lib/libcurl.so
beuyyub: /usr/lib/libmpg123.so
beuyyub: /usr/lib/libopus.so
beuyyub: /usr/lib/libopusfile.so
beuyyub: CMakeFiles/beuyyub.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --bold --progress-dir=/home/zynt/Documents/Code/C++/beuyyub-git/src/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking CXX executable beuyyub"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/beuyyub.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/beuyyub.dir/build: beuyyub
.PHONY : CMakeFiles/beuyyub.dir/build

CMakeFiles/beuyyub.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/beuyyub.dir/cmake_clean.cmake
.PHONY : CMakeFiles/beuyyub.dir/clean

CMakeFiles/beuyyub.dir/depend:
	cd /home/zynt/Documents/Code/C++/beuyyub-git/src && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/zynt/Documents/Code/C++/beuyyub-git /home/zynt/Documents/Code/C++/beuyyub-git /home/zynt/Documents/Code/C++/beuyyub-git/src /home/zynt/Documents/Code/C++/beuyyub-git/src /home/zynt/Documents/Code/C++/beuyyub-git/src/CMakeFiles/beuyyub.dir/DependInfo.cmake "--color=$(COLOR)"
.PHONY : CMakeFiles/beuyyub.dir/depend

