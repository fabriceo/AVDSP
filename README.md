# AVDSP
Audio Virtual DSP framework multi platform

Release: 
V1.0, May 1st 2020, tested on linux with alsa plugin and on OKTO RESEARCH DAC8PRO for XMOS platform.

remarks : 
- dsp_FIR instruction implemented but not tested yet
- a nanosharc xml parser is provided as a proff of concept but not fully integrated to generate proper dsp program

please read the 2 pages brief introduction in the AVDSP_intro.pdf
and the readme.md in linux folder.

folders:
encoder : all files and helpers functions needed to generate a dsp program file as binary or hex file.
dspprog : a collection of user examples and crossovers
runtime : all files needed to execute an encoded binary dsp program file.
osx     : some files used by the author to test or generate dsp files for the OktoDac. 
          xmosusb is to upload these programs into the dac trough the usb connection.
          unfortunatelly, and by opposite to linux, no CoreAudio driver provided yet...
