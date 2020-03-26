cmake_minimum_required(VERSION 3.10)

# set the project name
project(AVDSP C)

include_directories(encoder runtime)

add_executable(dsprun runtime/dsp_biquadSTD.c  runtime/dsp_firSTD.c  runtime/dsp_runtime.c encoder/dsp_fileaccess.c linux/alsa.c  linux/dsprun.c )
target_link_libraries(dsprun asound)
target_link_libraries(dsprun sndfile)
target_link_libraries(dsprun m)

add_executable(dspcreate encoder/dspcreate.c encoder/dsp_encoder.c encoder/dsp_filters.c encoder/dsp_fileaccess.c encoder/dsp_nanosharcxml.c )
target_link_libraries(dspcreate m)
target_link_libraries(dspcreate dl)
set_target_properties(dspcreate PROPERTIES ENABLE_EXPORTS TRUE)

add_custom_target(dspprogs ALL COMMAND make WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/dspprogs)

