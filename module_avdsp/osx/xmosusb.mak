#make -f Makefile.OSX all
all:
	g++ -o xmosusb xmosusb.cpp -I. -IOSX libusb-1.0.0-x86_64.dylib -m64