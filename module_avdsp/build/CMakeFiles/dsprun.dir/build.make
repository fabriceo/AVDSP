# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.10

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
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
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/fabriceo/AVDSP/module_avdsp

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/fabriceo/AVDSP/module_avdsp/build

# Include any dependencies generated for this target.
include CMakeFiles/dsprun.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/dsprun.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/dsprun.dir/flags.make

CMakeFiles/dsprun.dir/runtime/dsp_runtime.c.o: CMakeFiles/dsprun.dir/flags.make
CMakeFiles/dsprun.dir/runtime/dsp_runtime.c.o: ../runtime/dsp_runtime.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/fabriceo/AVDSP/module_avdsp/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/dsprun.dir/runtime/dsp_runtime.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/dsprun.dir/runtime/dsp_runtime.c.o   -c /home/fabriceo/AVDSP/module_avdsp/runtime/dsp_runtime.c

CMakeFiles/dsprun.dir/runtime/dsp_runtime.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/dsprun.dir/runtime/dsp_runtime.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/fabriceo/AVDSP/module_avdsp/runtime/dsp_runtime.c > CMakeFiles/dsprun.dir/runtime/dsp_runtime.c.i

CMakeFiles/dsprun.dir/runtime/dsp_runtime.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/dsprun.dir/runtime/dsp_runtime.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/fabriceo/AVDSP/module_avdsp/runtime/dsp_runtime.c -o CMakeFiles/dsprun.dir/runtime/dsp_runtime.c.s

CMakeFiles/dsprun.dir/runtime/dsp_runtime.c.o.requires:

.PHONY : CMakeFiles/dsprun.dir/runtime/dsp_runtime.c.o.requires

CMakeFiles/dsprun.dir/runtime/dsp_runtime.c.o.provides: CMakeFiles/dsprun.dir/runtime/dsp_runtime.c.o.requires
	$(MAKE) -f CMakeFiles/dsprun.dir/build.make CMakeFiles/dsprun.dir/runtime/dsp_runtime.c.o.provides.build
.PHONY : CMakeFiles/dsprun.dir/runtime/dsp_runtime.c.o.provides

CMakeFiles/dsprun.dir/runtime/dsp_runtime.c.o.provides.build: CMakeFiles/dsprun.dir/runtime/dsp_runtime.c.o


CMakeFiles/dsprun.dir/encoder/dsp_fileaccess.c.o: CMakeFiles/dsprun.dir/flags.make
CMakeFiles/dsprun.dir/encoder/dsp_fileaccess.c.o: ../encoder/dsp_fileaccess.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/fabriceo/AVDSP/module_avdsp/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object CMakeFiles/dsprun.dir/encoder/dsp_fileaccess.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/dsprun.dir/encoder/dsp_fileaccess.c.o   -c /home/fabriceo/AVDSP/module_avdsp/encoder/dsp_fileaccess.c

CMakeFiles/dsprun.dir/encoder/dsp_fileaccess.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/dsprun.dir/encoder/dsp_fileaccess.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/fabriceo/AVDSP/module_avdsp/encoder/dsp_fileaccess.c > CMakeFiles/dsprun.dir/encoder/dsp_fileaccess.c.i

CMakeFiles/dsprun.dir/encoder/dsp_fileaccess.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/dsprun.dir/encoder/dsp_fileaccess.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/fabriceo/AVDSP/module_avdsp/encoder/dsp_fileaccess.c -o CMakeFiles/dsprun.dir/encoder/dsp_fileaccess.c.s

CMakeFiles/dsprun.dir/encoder/dsp_fileaccess.c.o.requires:

.PHONY : CMakeFiles/dsprun.dir/encoder/dsp_fileaccess.c.o.requires

CMakeFiles/dsprun.dir/encoder/dsp_fileaccess.c.o.provides: CMakeFiles/dsprun.dir/encoder/dsp_fileaccess.c.o.requires
	$(MAKE) -f CMakeFiles/dsprun.dir/build.make CMakeFiles/dsprun.dir/encoder/dsp_fileaccess.c.o.provides.build
.PHONY : CMakeFiles/dsprun.dir/encoder/dsp_fileaccess.c.o.provides

CMakeFiles/dsprun.dir/encoder/dsp_fileaccess.c.o.provides.build: CMakeFiles/dsprun.dir/encoder/dsp_fileaccess.c.o


CMakeFiles/dsprun.dir/linux/dsprun.c.o: CMakeFiles/dsprun.dir/flags.make
CMakeFiles/dsprun.dir/linux/dsprun.c.o: ../linux/dsprun.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/fabriceo/AVDSP/module_avdsp/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building C object CMakeFiles/dsprun.dir/linux/dsprun.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/dsprun.dir/linux/dsprun.c.o   -c /home/fabriceo/AVDSP/module_avdsp/linux/dsprun.c

CMakeFiles/dsprun.dir/linux/dsprun.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/dsprun.dir/linux/dsprun.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/fabriceo/AVDSP/module_avdsp/linux/dsprun.c > CMakeFiles/dsprun.dir/linux/dsprun.c.i

CMakeFiles/dsprun.dir/linux/dsprun.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/dsprun.dir/linux/dsprun.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/fabriceo/AVDSP/module_avdsp/linux/dsprun.c -o CMakeFiles/dsprun.dir/linux/dsprun.c.s

CMakeFiles/dsprun.dir/linux/dsprun.c.o.requires:

.PHONY : CMakeFiles/dsprun.dir/linux/dsprun.c.o.requires

CMakeFiles/dsprun.dir/linux/dsprun.c.o.provides: CMakeFiles/dsprun.dir/linux/dsprun.c.o.requires
	$(MAKE) -f CMakeFiles/dsprun.dir/build.make CMakeFiles/dsprun.dir/linux/dsprun.c.o.provides.build
.PHONY : CMakeFiles/dsprun.dir/linux/dsprun.c.o.provides

CMakeFiles/dsprun.dir/linux/dsprun.c.o.provides.build: CMakeFiles/dsprun.dir/linux/dsprun.c.o


# Object files for target dsprun
dsprun_OBJECTS = \
"CMakeFiles/dsprun.dir/runtime/dsp_runtime.c.o" \
"CMakeFiles/dsprun.dir/encoder/dsp_fileaccess.c.o" \
"CMakeFiles/dsprun.dir/linux/dsprun.c.o"

# External object files for target dsprun
dsprun_EXTERNAL_OBJECTS =

dsprun: CMakeFiles/dsprun.dir/runtime/dsp_runtime.c.o
dsprun: CMakeFiles/dsprun.dir/encoder/dsp_fileaccess.c.o
dsprun: CMakeFiles/dsprun.dir/linux/dsprun.c.o
dsprun: CMakeFiles/dsprun.dir/build.make
dsprun: CMakeFiles/dsprun.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/fabriceo/AVDSP/module_avdsp/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Linking C executable dsprun"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/dsprun.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/dsprun.dir/build: dsprun

.PHONY : CMakeFiles/dsprun.dir/build

CMakeFiles/dsprun.dir/requires: CMakeFiles/dsprun.dir/runtime/dsp_runtime.c.o.requires
CMakeFiles/dsprun.dir/requires: CMakeFiles/dsprun.dir/encoder/dsp_fileaccess.c.o.requires
CMakeFiles/dsprun.dir/requires: CMakeFiles/dsprun.dir/linux/dsprun.c.o.requires

.PHONY : CMakeFiles/dsprun.dir/requires

CMakeFiles/dsprun.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/dsprun.dir/cmake_clean.cmake
.PHONY : CMakeFiles/dsprun.dir/clean

CMakeFiles/dsprun.dir/depend:
	cd /home/fabriceo/AVDSP/module_avdsp/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/fabriceo/AVDSP/module_avdsp /home/fabriceo/AVDSP/module_avdsp /home/fabriceo/AVDSP/module_avdsp/build /home/fabriceo/AVDSP/module_avdsp/build /home/fabriceo/AVDSP/module_avdsp/build/CMakeFiles/dsprun.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/dsprun.dir/depend

