# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.6

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
CMAKE_SOURCE_DIR = /home/volumio/AVDSP/module_avdsp

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/volumio/AVDSP/module_avdsp/build

# Include any dependencies generated for this target.
include CMakeFiles/asound_module_pcm_avdsp.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/asound_module_pcm_avdsp.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/asound_module_pcm_avdsp.dir/flags.make

CMakeFiles/asound_module_pcm_avdsp.dir/runtime/dsp_runtime.c.o: CMakeFiles/asound_module_pcm_avdsp.dir/flags.make
CMakeFiles/asound_module_pcm_avdsp.dir/runtime/dsp_runtime.c.o: ../runtime/dsp_runtime.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/volumio/AVDSP/module_avdsp/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/asound_module_pcm_avdsp.dir/runtime/dsp_runtime.c.o"
	/usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/asound_module_pcm_avdsp.dir/runtime/dsp_runtime.c.o   -c /home/volumio/AVDSP/module_avdsp/runtime/dsp_runtime.c

CMakeFiles/asound_module_pcm_avdsp.dir/runtime/dsp_runtime.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/asound_module_pcm_avdsp.dir/runtime/dsp_runtime.c.i"
	/usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/volumio/AVDSP/module_avdsp/runtime/dsp_runtime.c > CMakeFiles/asound_module_pcm_avdsp.dir/runtime/dsp_runtime.c.i

CMakeFiles/asound_module_pcm_avdsp.dir/runtime/dsp_runtime.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/asound_module_pcm_avdsp.dir/runtime/dsp_runtime.c.s"
	/usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/volumio/AVDSP/module_avdsp/runtime/dsp_runtime.c -o CMakeFiles/asound_module_pcm_avdsp.dir/runtime/dsp_runtime.c.s

CMakeFiles/asound_module_pcm_avdsp.dir/runtime/dsp_runtime.c.o.requires:

.PHONY : CMakeFiles/asound_module_pcm_avdsp.dir/runtime/dsp_runtime.c.o.requires

CMakeFiles/asound_module_pcm_avdsp.dir/runtime/dsp_runtime.c.o.provides: CMakeFiles/asound_module_pcm_avdsp.dir/runtime/dsp_runtime.c.o.requires
	$(MAKE) -f CMakeFiles/asound_module_pcm_avdsp.dir/build.make CMakeFiles/asound_module_pcm_avdsp.dir/runtime/dsp_runtime.c.o.provides.build
.PHONY : CMakeFiles/asound_module_pcm_avdsp.dir/runtime/dsp_runtime.c.o.provides

CMakeFiles/asound_module_pcm_avdsp.dir/runtime/dsp_runtime.c.o.provides.build: CMakeFiles/asound_module_pcm_avdsp.dir/runtime/dsp_runtime.c.o


CMakeFiles/asound_module_pcm_avdsp.dir/encoder/dsp_fileaccess.c.o: CMakeFiles/asound_module_pcm_avdsp.dir/flags.make
CMakeFiles/asound_module_pcm_avdsp.dir/encoder/dsp_fileaccess.c.o: ../encoder/dsp_fileaccess.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/volumio/AVDSP/module_avdsp/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object CMakeFiles/asound_module_pcm_avdsp.dir/encoder/dsp_fileaccess.c.o"
	/usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/asound_module_pcm_avdsp.dir/encoder/dsp_fileaccess.c.o   -c /home/volumio/AVDSP/module_avdsp/encoder/dsp_fileaccess.c

CMakeFiles/asound_module_pcm_avdsp.dir/encoder/dsp_fileaccess.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/asound_module_pcm_avdsp.dir/encoder/dsp_fileaccess.c.i"
	/usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/volumio/AVDSP/module_avdsp/encoder/dsp_fileaccess.c > CMakeFiles/asound_module_pcm_avdsp.dir/encoder/dsp_fileaccess.c.i

CMakeFiles/asound_module_pcm_avdsp.dir/encoder/dsp_fileaccess.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/asound_module_pcm_avdsp.dir/encoder/dsp_fileaccess.c.s"
	/usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/volumio/AVDSP/module_avdsp/encoder/dsp_fileaccess.c -o CMakeFiles/asound_module_pcm_avdsp.dir/encoder/dsp_fileaccess.c.s

CMakeFiles/asound_module_pcm_avdsp.dir/encoder/dsp_fileaccess.c.o.requires:

.PHONY : CMakeFiles/asound_module_pcm_avdsp.dir/encoder/dsp_fileaccess.c.o.requires

CMakeFiles/asound_module_pcm_avdsp.dir/encoder/dsp_fileaccess.c.o.provides: CMakeFiles/asound_module_pcm_avdsp.dir/encoder/dsp_fileaccess.c.o.requires
	$(MAKE) -f CMakeFiles/asound_module_pcm_avdsp.dir/build.make CMakeFiles/asound_module_pcm_avdsp.dir/encoder/dsp_fileaccess.c.o.provides.build
.PHONY : CMakeFiles/asound_module_pcm_avdsp.dir/encoder/dsp_fileaccess.c.o.provides

CMakeFiles/asound_module_pcm_avdsp.dir/encoder/dsp_fileaccess.c.o.provides.build: CMakeFiles/asound_module_pcm_avdsp.dir/encoder/dsp_fileaccess.c.o


CMakeFiles/asound_module_pcm_avdsp.dir/linux/avdsp_plugin.c.o: CMakeFiles/asound_module_pcm_avdsp.dir/flags.make
CMakeFiles/asound_module_pcm_avdsp.dir/linux/avdsp_plugin.c.o: ../linux/avdsp_plugin.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/volumio/AVDSP/module_avdsp/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building C object CMakeFiles/asound_module_pcm_avdsp.dir/linux/avdsp_plugin.c.o"
	/usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/asound_module_pcm_avdsp.dir/linux/avdsp_plugin.c.o   -c /home/volumio/AVDSP/module_avdsp/linux/avdsp_plugin.c

CMakeFiles/asound_module_pcm_avdsp.dir/linux/avdsp_plugin.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/asound_module_pcm_avdsp.dir/linux/avdsp_plugin.c.i"
	/usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/volumio/AVDSP/module_avdsp/linux/avdsp_plugin.c > CMakeFiles/asound_module_pcm_avdsp.dir/linux/avdsp_plugin.c.i

CMakeFiles/asound_module_pcm_avdsp.dir/linux/avdsp_plugin.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/asound_module_pcm_avdsp.dir/linux/avdsp_plugin.c.s"
	/usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/volumio/AVDSP/module_avdsp/linux/avdsp_plugin.c -o CMakeFiles/asound_module_pcm_avdsp.dir/linux/avdsp_plugin.c.s

CMakeFiles/asound_module_pcm_avdsp.dir/linux/avdsp_plugin.c.o.requires:

.PHONY : CMakeFiles/asound_module_pcm_avdsp.dir/linux/avdsp_plugin.c.o.requires

CMakeFiles/asound_module_pcm_avdsp.dir/linux/avdsp_plugin.c.o.provides: CMakeFiles/asound_module_pcm_avdsp.dir/linux/avdsp_plugin.c.o.requires
	$(MAKE) -f CMakeFiles/asound_module_pcm_avdsp.dir/build.make CMakeFiles/asound_module_pcm_avdsp.dir/linux/avdsp_plugin.c.o.provides.build
.PHONY : CMakeFiles/asound_module_pcm_avdsp.dir/linux/avdsp_plugin.c.o.provides

CMakeFiles/asound_module_pcm_avdsp.dir/linux/avdsp_plugin.c.o.provides.build: CMakeFiles/asound_module_pcm_avdsp.dir/linux/avdsp_plugin.c.o


# Object files for target asound_module_pcm_avdsp
asound_module_pcm_avdsp_OBJECTS = \
"CMakeFiles/asound_module_pcm_avdsp.dir/runtime/dsp_runtime.c.o" \
"CMakeFiles/asound_module_pcm_avdsp.dir/encoder/dsp_fileaccess.c.o" \
"CMakeFiles/asound_module_pcm_avdsp.dir/linux/avdsp_plugin.c.o"

# External object files for target asound_module_pcm_avdsp
asound_module_pcm_avdsp_EXTERNAL_OBJECTS =

libasound_module_pcm_avdsp.so: CMakeFiles/asound_module_pcm_avdsp.dir/runtime/dsp_runtime.c.o
libasound_module_pcm_avdsp.so: CMakeFiles/asound_module_pcm_avdsp.dir/encoder/dsp_fileaccess.c.o
libasound_module_pcm_avdsp.so: CMakeFiles/asound_module_pcm_avdsp.dir/linux/avdsp_plugin.c.o
libasound_module_pcm_avdsp.so: CMakeFiles/asound_module_pcm_avdsp.dir/build.make
libasound_module_pcm_avdsp.so: CMakeFiles/asound_module_pcm_avdsp.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/volumio/AVDSP/module_avdsp/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Linking C shared library libasound_module_pcm_avdsp.so"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/asound_module_pcm_avdsp.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/asound_module_pcm_avdsp.dir/build: libasound_module_pcm_avdsp.so

.PHONY : CMakeFiles/asound_module_pcm_avdsp.dir/build

CMakeFiles/asound_module_pcm_avdsp.dir/requires: CMakeFiles/asound_module_pcm_avdsp.dir/runtime/dsp_runtime.c.o.requires
CMakeFiles/asound_module_pcm_avdsp.dir/requires: CMakeFiles/asound_module_pcm_avdsp.dir/encoder/dsp_fileaccess.c.o.requires
CMakeFiles/asound_module_pcm_avdsp.dir/requires: CMakeFiles/asound_module_pcm_avdsp.dir/linux/avdsp_plugin.c.o.requires

.PHONY : CMakeFiles/asound_module_pcm_avdsp.dir/requires

CMakeFiles/asound_module_pcm_avdsp.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/asound_module_pcm_avdsp.dir/cmake_clean.cmake
.PHONY : CMakeFiles/asound_module_pcm_avdsp.dir/clean

CMakeFiles/asound_module_pcm_avdsp.dir/depend:
	cd /home/volumio/AVDSP/module_avdsp/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/volumio/AVDSP/module_avdsp /home/volumio/AVDSP/module_avdsp /home/volumio/AVDSP/module_avdsp/build /home/volumio/AVDSP/module_avdsp/build /home/volumio/AVDSP/module_avdsp/build/CMakeFiles/asound_module_pcm_avdsp.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/asound_module_pcm_avdsp.dir/depend
