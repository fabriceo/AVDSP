cmake_minimum_required(VERSION 3.5)

# set the project name
project(AVDSP C)

add_definitions(-DDSP_FORMAT=2)
add_compile_options(-Ofast -std=gnu99)

include_directories(encoder runtime)

add_executable(dsprun runtime/dsp_runtime.c encoder/dsp_fileaccess.c linux/dsprun.c )
target_link_libraries(dsprun sndfile)
target_link_libraries(dsprun m)

add_library(asound_module_pcm_avdsp SHARED runtime/dsp_runtime.c encoder/dsp_fileaccess.c linux/avdsp_plugin.c)
target_compile_definitions(asound_module_pcm_avdsp PRIVATE -DPIC)
target_link_libraries(asound_module_pcm_avdsp asound)

add_executable(dspcreate encoder/dspcreate.c encoder/dsp_encoder.c encoder/dsp_filters.c encoder/dsp_fileaccess.c encoder/dsp_nanosharcxml.c )
target_link_libraries(dspcreate m)
target_link_libraries(dspcreate dl)
set_target_properties(dspcreate PROPERTIES ENABLE_EXPORTS TRUE)

add_custom_target(dspprogs ALL COMMAND make WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/dspprogs)

