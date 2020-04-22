#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libusb.h"
// document available at http://libusb.sourceforge.net/api-1.0/modules.html

//to be compiled with : make -f Makefile.OSX all


#if defined(__APPLE__)
#define SLEEP(n) system("sleep " #n) //osx
#elif defined(__linux__)
#define SLEEP(n) system("sleep " #n) //osx
#else // aka windows :)
#include "windows.h"
#define SLEEP(n) Sleep(1000*n) //windows
#endif


/* the device's vendor and product id */
#define XMOS_VID 0x20b1

#define XMOS_XCORE_AUDIO_AUDIO2_PID 0x3066
#define XMOS_DXIO                   0x2009
#define XMOS_L1_AUDIO2_PID          0x0002
#define XMOS_L1_AUDIO1_PID          0x0003
#define XMOS_L2_AUDIO2_PID          0x0004
#define XMOS_SU1_AUDIO2_PID         0x0008
#define XMOS_U8_MFA_AUDIO2_PID      0x000A

unsigned short pidList[] = {XMOS_XCORE_AUDIO_AUDIO2_PID, 
						    XMOS_DXIO,
                            XMOS_L1_AUDIO2_PID,
                            XMOS_L1_AUDIO1_PID,
                            XMOS_L2_AUDIO2_PID,
                            XMOS_SU1_AUDIO2_PID, 
                            XMOS_U8_MFA_AUDIO2_PID}; 



#define DFU_REQUEST_TO_DEV      0x21
#define DFU_REQUEST_FROM_DEV    0xa1

#define VENDOR_REQUEST_TO_DEV    (LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE)
#define VENDOR_REQUEST_FROM_DEV  (LIBUSB_ENDPOINT_IN  | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE)

// Standard DFU requests (not used as the DFU mode is now bridged to the Audio Control  commands)
#if 0
#define DFU_DETACH 0
#define DFU_DNLOAD 1
#define DFU_UPLOAD 2
#define DFU_GETSTATUS 3
#define DFU_CLRSTATUS 4
#define DFU_GETSTATE 5
#define DFU_ABORT 6
#endif

#define XMOS_DFU_RESETDEVICE   0xf0 //final command after downloading new image to imediately reboot
#define XMOS_DFU_REVERTFACTORY 0xf1 //not used
#define XMOS_DFU_RESETINTODFU  0xf2 // very first command to send for using DNLOAD and GET_STATUS
#define XMOS_DFU_RESETFROMDFU  0xf3 //final command to re-enter in the standard Audio mode without rebooting
#define XMOS_DFU_SELECTIMAGE   0xf4 //not used
#define XMOS_DFU_SAVESTATE     0xf5 //not used
#define XMOS_DFU_RESTORESTATE  0xf6 //not used
#define XMOS_DFU_DNLOAD        0xf7 //same as original DFU_DNLOAD
#define XMOS_DFU_GETSTATUS     0xf8 //same as original DFU_GETSTATUS

enum VendorCommands {
/*D2H*/    VENDOR_AUDIO_MUTE  = 0xA0,  // force the DAC to mute by sending Zeros to the DAC I2S lines
/*D2H*/    VENDOR_AUDIO_UNMUTE,        // restore the normal mode
/*D2H*/    VENDOR_AUDIO_STOP,          // stop any data flow between I2S, USB, SPDIF : ready for flash access or dsp program changes
/*D2H*/    VENDOR_AUDIO_START,         // restore the normal mode
/*H2D*/    VENDOR_GET_DEVICE_INFO,     // return a set of bytes representing the DAC status and modes.
/*D2H*/    VENDOR_SET_MODE,            // force the DAC in the given Mode. Front panel is informed and might save this value
/*D2H*/    VENDOR_SET_DSP_PROG,        // force the DAC to switch to the given dsp program by loading it from flash. Front panel is informed and might save this value
/*D2H*/    VENDOR_LOAD_DSP,            // load a 64bytes page in the dsp program buffer area. page number in wValue, 0 = start, then end
/*D2H*/    VENDOR_WRITE_DSP_FLASH,     // write the dsp program being in dsp program buffer, to the FLASH location N, overwrting exsting one and erasing nexts if too large to fit
/*D2H*/    VENDOR_READ_DSP_FLASH,      // read dsp program from flash memory
           VENDOR_WRITE_DSP_MEM,       // read up to 16 words in the dsp data area.
           VENDOR_READ_DSP_MEM,        // write up to 16words in the DSP data area.
/*D2H*/    VENDOR_OPEN_FLASH,          // open the flash memory for read/write
/*D2H*/    VENDOR_CLOSE_FLASH,          // close flash accees
/*H2D*/    VENDOR_READ_FLASH,          // read 64 bytes of flash memory in data partition at addres wValue, wValue representing the page index, one page bieng 64bytes
/*D2H*/    VENDOR_WRITE_FLASH,         // write 64bytes of data in a 4096bytes buffer. then write a full sector with these 4096 bytes
/*H2D*/    VENDOR_ERASE_FLASH,         // erase sector n passed in wValue, sectorsize = 4096 bytes
/*D2H*/    VENDOR_ERASE_FLASH_ALL,     // erase all data partition

/*D2H*/    VENDOR_AUDIO_HANDLER,       // stop the exchanges between USB and DAC and SPDIF receivers/transmitters and treat specific calls
/*D2H*/    VENDOR_AUDIO_FINISH,        // restart normal exchanges
};

static const int tableFreq[8] = {
        44100, 48000,
        88200, 96000,
        176400,192000,
        352800,384000 };

static libusb_device_handle *devh = NULL;   // current usb device found and opened
unsigned XMOS_DFU_IF = 0;

static char Manufacturer[64] = "";
static char Product[64] = "";
static char SerialNumber[16] = "";

static int find_xmos_device(unsigned int id, unsigned int list) // list !=0 means printing device information
{
    libusb_device *dev;
    libusb_device **devs;
    int currentId = 0;
    char string[256];
    int result;
    XMOS_DFU_IF = 0;

    libusb_get_device_list(NULL, &devs);
    devh = NULL;
    int i = 0;
    while ((dev = devs[i++]) != NULL) 
    {
        struct libusb_device_descriptor desc;
        libusb_get_device_descriptor(dev, &desc); 

        int foundDev = 0;
        if(desc.idVendor == XMOS_VID) {
            for(int j = 0; j < sizeof(pidList)/sizeof(unsigned short); j++) {
                if(desc.idProduct == pidList[j] /* && !list */) {
                    foundDev = 1;
                    //printf("device identified\n");
                    break; // for loop
                }
            }
        } // id = xmos_vid
        if ((list || foundDev)) {
            if (foundDev) printf("[%d] > ",currentId);
            else printf("      ");
            printf("VID = 0x%04x, PID = 0x%04x, BCDDevice: 0x%04x\n", desc.idVendor, desc.idProduct, desc.bcdDevice);
        }

        if (foundDev) {
            if (currentId == id)  {
                if ((result=libusb_open(dev, &devh)) < 0)  {
                    printf("     USB problem in acessing this device (error %d)\n",result);
                    libusb_free_device_list(devs, 1);
                    return -1;
                }  else {

                    libusb_config_descriptor *config_desc = NULL;
                    libusb_get_active_config_descriptor(dev, &config_desc); 
                    if (config_desc != NULL)  {
                        if (desc.iManufacturer) {
                            int ret = libusb_get_string_descriptor_ascii(devh, desc.iManufacturer, (unsigned char*)string, sizeof(string));
                            if (list & (ret > 0))
                                printf("      %s\n", string); }

                        if (desc.iProduct) {
                            int ret = libusb_get_string_descriptor_ascii(devh, desc.iProduct, (unsigned char*)string, sizeof(string));
                            if (list & (ret > 0))
                                printf("      %s\n", string); }

                        if (desc.iSerialNumber) {
                            int ret = libusb_get_string_descriptor_ascii(devh, desc.iSerialNumber, (unsigned char*)string, sizeof(string));
                            if (list & (ret > 0))
                                printf("      %s\n", string); }

                        for (int j = 0; j < config_desc->bNumInterfaces; j++) {
                            const libusb_interface_descriptor *inter_desc = ((libusb_interface *)&config_desc->interface[j])->altsetting;

                            if (inter_desc->bInterfaceClass == 0xFE && // DFU class
                                inter_desc->bInterfaceSubClass == 0x1)  {
                                       XMOS_DFU_IF = j;
                                       if (list)
                                       printf("          (%d)  usb DFU\n",j); }

                            if (inter_desc->bInterfaceClass == LIBUSB_CLASS_AUDIO &&
                                inter_desc->bInterfaceSubClass == 0x01)  {
                                if (list)
                                       printf("          (%d)  usb Audio Control\n",j); }

                            if (inter_desc->bInterfaceClass == LIBUSB_CLASS_AUDIO &&
                                inter_desc->bInterfaceSubClass == 0x02)  {

                                if (list)
                                       printf("          (%d)  usb Audio Streaming\n",j); }

                            if (inter_desc->bInterfaceClass == LIBUSB_CLASS_AUDIO &&
                                inter_desc->bInterfaceSubClass == 0x03)  {

                                if (list)
                                       printf("          (%d)  usb Midi Streaming\n",j); }

                            if (inter_desc->bInterfaceClass == LIBUSB_CLASS_HID &&
                                inter_desc->bInterfaceSubClass == 0x00)  {

                                if (list)
                                       printf("          (%d)  usb HID\n",j); }

                            if (inter_desc->bInterfaceClass == LIBUSB_CLASS_COMM &&
                                inter_desc->bInterfaceSubClass == 0x00)  {

                                if (list)
                                       printf("          (%d)  usb Communication\n",j); }
                            if (inter_desc->bInterfaceClass == LIBUSB_CLASS_DATA &&
                                inter_desc->bInterfaceSubClass == 0x00)  {

                                if (list)
                                       printf("          (%d)  usb CDC Serial\n",j); }
                           }
                    } else {
                        printf("     NO config descriptor ...?\n"); }

                } // libusb_open
                if (!list) break;  // device selected : leave the loop

            } // currentId == id
            currentId++;

        } // foundDev

    } // while 1

    libusb_free_device_list(devs, 1);

    return devh ? 0 : -1;
}


int testvendor(){
    unsigned char data[64];
    libusb_control_transfer(devh, VENDOR_REQUEST_TO_DEV,
            0x47, 4, 0, NULL, 0, 0);

    data[0]=0x22;data[1]=0x33;data[2]=0x44;data[3]=0x55;
    libusb_control_transfer(devh, VENDOR_REQUEST_TO_DEV,
            0x48, 5, 0, data, 4, 0);

    libusb_control_transfer(devh, VENDOR_REQUEST_FROM_DEV,
            0x49, 6, 0, data, 2, 0);
    printf("data 0-1 = 0x%X 0x%X\n",data[0],data[1]);
    return 0;
}


int xmos_resetdevice(unsigned int interface) {
  libusb_control_transfer(devh, DFU_REQUEST_TO_DEV, XMOS_DFU_RESETDEVICE, 0, interface, NULL, 0, 0);
  return 0;
}

int xmos_enterdfu(unsigned int interface) {
  libusb_control_transfer(devh, DFU_REQUEST_TO_DEV, XMOS_DFU_RESETINTODFU, 0, interface, NULL, 0, 0);
  return 0;
}

int xmos_leavedfu(unsigned int interface) {
  libusb_control_transfer(devh, DFU_REQUEST_TO_DEV, XMOS_DFU_RESETFROMDFU, 0, interface, NULL, 0, 0);
  return 0;
}



int dfu_getStatus(unsigned int interface, unsigned char *state, unsigned int *timeout,
                  unsigned char *nextState, unsigned char *strIndex) {
  unsigned int data[2];
  libusb_control_transfer(devh, DFU_REQUEST_TO_DEV, XMOS_DFU_GETSTATUS, 0, interface, (unsigned char *)data, 6, 0);
  
  *state = data[0] & 0xff;
  *timeout = (data[0] >> 8) & 0xffffff;
  *nextState = data[1] & 0xff;
  *strIndex = (data[1] >> 8) & 0xff;
  return 0;
}


int dfu_download(unsigned int interface, unsigned int block_num, unsigned int size, unsigned char *data) {
  //printf("... Downloading block number %d size %d\r", block_num, size);
    unsigned int numBytes = 0;
    numBytes = libusb_control_transfer(devh, DFU_REQUEST_TO_DEV, XMOS_DFU_DNLOAD, block_num, interface, data, size, 0);
    return numBytes;
}


int write_dfu_image(unsigned int interface, char *file) {
  int i = 0;
  FILE* inFile = NULL;
  int image_size = 0;
  unsigned int num_blocks = 0;
  unsigned int block_size = 64;
  unsigned int remainder = 0;
  unsigned char block_data[256];

  unsigned char dfuState = 0;
  unsigned char nextDfuState = 0;
  unsigned int timeout = 0;
  unsigned char strIndex = 0;
  unsigned int dfuBlockCount = 0;

  inFile = fopen( file, "rb" );
  if( inFile == NULL ) {
    fprintf(stderr,"Error: Failed to open input data file.\n");
    return -1;
  }

  /* Discover the size of the image. */
  if( 0 != fseek( inFile, 0, SEEK_END ) ) {
    fprintf(stderr,"Error: Failed to discover input data file size.\n");
    return -1;
  }

  image_size = (int)ftell( inFile );

  if( 0 != fseek( inFile, 0, SEEK_SET ) ) {
    fprintf(stderr,"Error: Failed to input file pointer.\n");
   return -1; 
  }

  num_blocks = image_size/block_size;
  remainder = image_size - (num_blocks * block_size);

  printf("... Downloading image (%s) to device (%dbytes)\n", file,image_size);
 
  dfuBlockCount = 0; 

  for (i = 0; i < num_blocks; i++) {
    memset(block_data, 0x0, block_size);
    fread(block_data, 1, block_size, inFile);
    if (i == 0) printf("first block download includes time to erase all flash\n");
    int numbytes = dfu_download(interface, dfuBlockCount, block_size, block_data);
    if (i == 0) {
        printf("result = %d bytes\n",numbytes);
        if (numbytes != 64) {
            fprintf(stderr,"Error: dfudownload returned an error %d\n",numbytes);
           return -1; }
    }
    dfu_getStatus(interface, &dfuState, &timeout, &nextDfuState, &strIndex);
    dfuBlockCount++;
    if ((dfuBlockCount & 127) == 0)
        printf("%dko\n",dfuBlockCount >> 4);
  }

  if (remainder) {
    memset(block_data, 0x0, block_size);
    fread(block_data, 1, remainder, inFile);
    dfu_download(interface, dfuBlockCount, block_size, block_data);
    dfu_getStatus(interface, &dfuState, &timeout, &nextDfuState, &strIndex);
  }
   printf("downloaded %d bytes\n",image_size);

  dfu_download(interface, 0, 0, NULL);
  dfu_getStatus(interface, &dfuState, &timeout, &nextDfuState, &strIndex);

  printf("... Download completed\n");

  return 0;
}

#if 0   // not supported anymore, just kept for futur code example
int read_dfu_image(unsigned int interface, char *file) {
  FILE *outFile = NULL;
  unsigned int block_count = 0;
  unsigned int block_size = 64;
  unsigned char block_data[64];

  outFile = fopen( file, "wb" );
  if( outFile == NULL ) {
    fprintf(stderr,"Error: Failed to open output data file.\n");
    return -1;
  }

  printf("... Uploading image (%s) from device\n", file);

  while (1) {
    unsigned int numBytes = 0;
    numBytes = dfu_upload(interface, block_count, 64, block_data);
    if (numBytes == 0) 
      break;
    fwrite(block_data, 1, block_size, outFile);
    block_count++;
    printf(".");
  }
  printf("\n");

  fclose(outFile);
  return 0;
}
#endif

int bin2hex(char *file){
    FILE* inFile = NULL;

    int bin_size = 0;
    unsigned int num_blocks = 0;
    unsigned int block_size = 4096;
    unsigned int remainder = 0;
    unsigned char block_data[0x40000];  // max 256ko

    inFile = fopen( file, "rb" );
    if( inFile == NULL ) {
      fprintf(stderr,"Error: Failed to open binary.\n");
      return -1;
    }

    /* Discover the size of the file. */
    if( 0 != fseek( inFile, 0, SEEK_END ) ) {
      fprintf(stderr,"Error: Failed to discover binary file size.\n");
      return -1;
    }

    bin_size = (int)ftell( inFile );

    if( 0 != fseek( inFile, 0, SEEK_SET ) ) {
      fprintf(stderr,"Error: Failed to input file pointer.\n");
     return -1;
    }

    num_blocks = bin_size/block_size;
    remainder = bin_size - (num_blocks * block_size);

    printf("... converting binary (%s) to .h file (%dbytes)\n", file, bin_size);

    for (int i = 0; i < num_blocks; i++) {
      memset(&block_data[i*block_size], 0x0, block_size);
      fread(&block_data[i*block_size], 1, block_size, inFile);
      }


    if (remainder) {
      memset(&block_data[num_blocks*block_size], 0x0, block_size);
      fread(&block_data[num_blocks*block_size], 1, remainder, inFile);
    }
     printf("loaded %d bytes\n",bin_size);
     fclose(inFile);

    FILE *outFile = NULL;

    outFile = fopen( strcat(file, ".h"), "w" );
    if( outFile == NULL ) {
      fprintf(stderr,"Error: Failed to open output .h file.\n");
      return -1;
    }

    num_blocks = bin_size / 64;
    bin_size = (num_blocks+1)*16;
    fprintf(outFile,"//#define BIN2C_SIZE %d\n",bin_size);
    fprintf(outFile,"//const unsigned int bin2c[BIN2C_SIZE] = { // hex in little-endian\n");
    for (int i = 0; i < num_blocks; i++) {
        for (int j=0; j<16;j++) {
            unsigned char * pch = &block_data[i*64+j*4];
            unsigned int* p = (unsigned int*)(pch);
            if ((j==15)&&(i==(num_blocks-1)))
                fprintf(outFile,"0x%X",*p);
            else
                fprintf(outFile,"0x%X, ",*p);
        }
        fprintf(outFile,"\n");
    }
    fprintf(outFile,"\n");

    fclose(outFile);
    return 0;
  }


int vendor_from_dev(int cmd, unsigned value, unsigned index, unsigned char *data, unsigned int size ){

    int result = libusb_control_transfer(devh, VENDOR_REQUEST_FROM_DEV, cmd, value, index, data, size, 0);
    return result;
}

int vendor_to_dev(int cmd, unsigned value, unsigned index){
    unsigned char dummy[64];
    int result = libusb_control_transfer(devh, VENDOR_REQUEST_TO_DEV, cmd, value, index, dummy, 0, 0);
    return result;
}

int vendor_dsp_load_page(unsigned int block_num, unsigned int size, unsigned char *data) {
    int result = libusb_control_transfer(devh, VENDOR_REQUEST_TO_DEV, VENDOR_LOAD_DSP, block_num, size, data, size, 0);
    return result;
}



int load_dsp_prog(char *file) {
  int i = 0;
  FILE* inFile = NULL;
  int bin_size = 0;
  unsigned int num_blocks = 0;
  unsigned int block_size = 64;
  unsigned char block_data[64];
  int result = 0;

  unsigned int timeout = 0;
  unsigned int blockCount = 0;

  inFile = fopen( file, "rb" );
  if( inFile == NULL ) {
    fprintf(stderr,"Error: Failed to open dsp program file.\n");
    return -1;
  }

  /* Discover the size of the image. */
  if( 0 != fseek( inFile, 0, SEEK_END ) ) {
    fprintf(stderr,"Error: Failed to discover dsp program file size.\n");
    return -1;
  }

  bin_size = (int)ftell( inFile );
  bin_size += 63;   //roundup
  bin_size &= ~63;

  if( 0 != fseek( inFile, 0, SEEK_SET ) ) {
    fprintf(stderr,"Error: Failed to input dsp file pointer.\n");
   return -1;
  }

  num_blocks = bin_size/block_size;

  printf("... loading opcodes (%s) to device (%dbytes)\n", file, bin_size);

  blockCount = 0;

  for (i = 0; i < num_blocks; i++) {
    memset(block_data, 0x0, block_size);
    fread(block_data, 1, block_size, inFile);
    result = vendor_dsp_load_page(blockCount, block_size, block_data);
    if (0) {
            fprintf(stderr,"Error: dsp_load_page returned an error %d\n",result);
           return -1; }
    blockCount++;
    if ((blockCount & 63) == 0)
        printf("%dko\n", blockCount / 16);
  }

  result = vendor_dsp_load_page(0, 0, NULL);    // this is the formal command for ending the loading process
  if (0) {
          fprintf(stderr,"Error: dsp_load_page final step returned an error %d\n",result);
         return -1; }
   printf("... dsp load completed %d bytes\n",bin_size);

  return 0;
}

void getDacStatus(){
    unsigned char data[64];
    libusb_control_transfer(devh, VENDOR_REQUEST_FROM_DEV,
            VENDOR_GET_DEVICE_INFO, 0, 0, data, 64, 0);
    printf("DAC mode            = 0x%X\n", data[0]);
    printf("I2S Audio config    = 0x%2X (%d)\n", data[1], tableFreq[data[1] & 7]);
    printf("USB Audio config    = 0x%2X (%d)\n", data[2], tableFreq[data[2] & 7]);
    printf("DSP program number  = %d\n",    data[3]);
    printf("front panel version = %d\n",    data[4]);
    printf("Sound presence      = 0x%2X\n", data[5]);
    printf("trigger             = 0x%2X\n", data[6]);
    short vol= (signed char)data[7];
    if (vol>0)
        printf("usb volume          = muted (%ddB)\n", -vol-1 );
    else
        printf("usb volume          = %ddB\n", vol );
    vol= (signed char)data[8];
    if (vol>0)
        printf("front panel volume  = muted (%ddB)\n", -vol-1 );
    else
        printf("front panel volume  = %ddB\n", vol );
    printf("xmos BCD version    = 0x%4X\n", (data[9]+(data[10]<<8)) );
    printf("usb vendor ID       = 0x%4X\n", (data[11]+(data[12]<<8)) );
    printf("usb product ID      = 0x%4X\n", (data[13]+(data[14]<<8)) );
    printf("maximum frequency   = %d\n",    (data[15]+(data[16]<<8)+(data[17]<<16)+(data[18]<<24)) );
    printf("maximum dsp tasks   = %d\n",    (data[19]) );
    for (int i=0; i<= data[19]; i++)
        if (i != data[19])
             printf("dsp %d: instructions = %d\n", i+1, (data[20+i+i]+(data[20+i+i]<<8)) );
        else printf("maxi   instructions = %d\n", (data[20+i+i]+(data[20+i+i]<<8)) );
}

#define F28(x) ((double)(x)/(double)(1<<28))
#define F(x,y) ((double)(x)/(double)(1<<y))
void dspReadMem(int addr){
    unsigned data[16];
    unsigned char * p = (unsigned char*)&data[0];
    libusb_control_transfer(devh, VENDOR_REQUEST_FROM_DEV,
            VENDOR_READ_DSP_MEM, addr, 0, p, 64, 0);
    for (int i=0; i<16; i++) {
        int x = data[i];
        float f = (float)data[i];
        printf("0x%4X : %8X %d %f %f\n",i,x,x,F(x,24),f);
    }
}

static const int dspTableFreq[] = {
        8000, 16000,
        24000, 32000,
        44100, 48000,
        88200, 96000,
        176400,192000,
        352800,384000,
        705600, 768000 };

typedef struct dspHeader_s {    // 11 words
/* 0 */     int head;
/* 1 */     int   totalLength;  // the total length of the dsp program (in 32 bits words), rounded to upper 8bytes
/* 2 */     int   dataSize;     // required data space for executing the dsp program (in 32 bits words)
/* 3 */     unsigned checkSum;  // basic calculated value representing the sum of all opcodes used in the program
/* 4 */     int   numCores;     // number of cores/tasks declared in the dsp program
/* 5 */     int   version;      // version of the encoder used MAJOR, MINOR,BUGFIX
/* 6 */     unsigned short   format;       // contains DSP_MANT used by encoder or 1 for float or 2 for double
/*   */     unsigned short   maxOpcode;    // last op code number used in this program (to check compatibility with runtime)
/* 7 */     int   freqMin;      // minimum frequency possible for this program, in raw format eg 44100
/* 8 */     int   freqMax;      // maximum frequency possible for this program, in raw format eg 192000
/* 9 */     unsigned usedInputs;   //bit mapping of all used inputs
/* 10 */    unsigned usedOutputs;  //bit mapping of all used outputs
        } dspHeader_t;



void dspReadHeader(){
    dspHeader_t header;
    unsigned char * p = (unsigned char*)&header;
    libusb_control_transfer(devh, VENDOR_REQUEST_FROM_DEV,
            VENDOR_READ_DSP_MEM, 0, 0, p, sizeof(dspHeader_t), 0);
    printf("total length = %d\n",header.totalLength);
    if (header.totalLength) {
        printf("data size    = %d\n",header.dataSize);
        printf("checksum     = 0x%X\n",header.checkSum);
        printf("num cores    = %d\n",header.numCores);
        printf("version      = %X\n",header.version);
        if (header.format)
             printf("encoded int    %d.%d\n",(32-header.format),header.format);
        else printf("encoded float\n");
        printf("freq min     = %d (%d)\n",header.freqMin,dspTableFreq[header.freqMin]);
        printf("freq max     = %d (%d)\n",header.freqMax,dspTableFreq[header.freqMax]);
    }
}

int main(int argc, char **argv) {
  int r = 1;
  unsigned int deviceID = 0;

  unsigned int xmosload = 0;
  unsigned int samdload = 0;
  unsigned int listdev  = 0;
  unsigned int enterdfu = 0;
  unsigned int leavedfu = 0;
  unsigned int modetest = 0;
  unsigned int dspload  = 0;
  unsigned int dspprog  = 0;
  unsigned int dspwrite = 0;
  unsigned int dspread  = 0;
  unsigned int param1   = 0;
  unsigned int dacstatus= 0;
  unsigned int dacmute  = 0;
  unsigned int dacunmute= 0;
  unsigned int dacmode  = 0;
  unsigned int readflash= 0;
  unsigned int eraseflash=0;
  unsigned int dspreadmem = 0;
  unsigned int dspreadheader = 0;
  unsigned int resetdevice = 0;
  unsigned argi = 1;
  unsigned char data[64];

  char *filename = NULL;

  if (argc < 2) {
    fprintf(stderr, "No options passed to usb application\n");
    fprintf(stderr, "Available options (optional deviceID as first option):\n");
    fprintf(stderr, "--listdevices\n");      // list all the USB devices and point the one with xmos vendor ID
    fprintf(stderr, "--resetdevice\n");      // send a DFU command for reseting the device
    fprintf(stderr, "--bin2hex  file\n");    // convert a binary file into a text file with hexadecimal presentation grouped by 4 bytes, for C/C++ include
    fprintf(stderr, "--xmosload file\n");    // load a new firmware into the xmos flash boot partition
    fprintf(stderr, "--samdload file\n");    // load the front panel flash with a samd binary program and reboot
    fprintf(stderr, "--dspload  file\n");    // send the file binary content (dsp opcode) to the xmos dsp memory
    fprintf(stderr, "--dspwrite slot\n");    // save xmos dsp memory content to flash slot N (1..15)
    fprintf(stderr, "--dspread  slot\n");    // load xmos dsp memory content with flash slot 1..15
    fprintf(stderr, "--dspreadmem addr\n");  // read 16 word of data in the dsp data area
    fprintf(stderr, "--dspheader\n");    // read dsp header of dsp program in memory
    fprintf(stderr, "--dspprog  val\n");     // set the dsp program number in the front panel menu settings and load it from flash
    fprintf(stderr, "--dacstatus\n");        // return the main registers providing informations on the dac and data stream status
    fprintf(stderr, "--dacmode  val\n");     // set the dac mode with the value given. front panel informed
    fprintf(stderr, "--dacmute\n");          // set the dac in mute till unmute
    fprintf(stderr, "--dacunmute\n");        // unmute the dac
    fprintf(stderr, "--flashread page\n");   // read 64 bytes of flash in data partition at adress page*64
    fprintf(stderr, "--flasherase sector\n");// erase a sector (4096bytes=64pages)  in data partition (starting 0)
    return -1; }

  if (strcmp(argv[1], "--bin2hex") == 0) {
    if (argv[2]) {
      bin2hex(argv[2]);
      exit(1); }
    }

  if (strlen(argv[argi]) == 1){
      deviceID = atoi(argv[argi]);  // extract deviceID forced by user
      argi++;
  }
  if (strcmp(argv[argi], "--xmosload") == 0) {
    if (argv[argi+1]) {
      filename = argv[argi+1];
    } else {
      fprintf(stderr, "No filename specified for xmosload option\n");
      return -1; }
    xmosload = 1;
  } else
  if (strcmp(argv[argi], "--samdload") == 0) {
    if (argv[argi+1]) {
      filename = argv[argi+1];
    } else {
      fprintf(stderr, "No filename specified for samdload option\n");
      return -1; }
    samdload = 1; }
  else
  if(strcmp(argv[argi], "--listdevices") == 0) {
          listdev = 1; }
  else
  if(strcmp(argv[argi], "--resetdevice") == 0) {
          listdev = 1;
          resetdevice = 1; }
  else
  if(strcmp(argv[argi], "--test") == 0) {
          listdev = 1;
          modetest = 1; }
  else
  if (strcmp(argv[argi], "--dspload") == 0) {
      if (argv[argi+1]) {
        filename = argv[argi+1];
      } else {
        fprintf(stderr, "No filename specified for dspload option\n");
        return -1; }
      dspload = 1; }
  else
  if (strcmp(argv[argi], "--dspreadmem") == 0) {
      if (argv[argi+1]) {
          param1 = atoi(argv[argi+1]);
      } else {
        fprintf(stderr, "No value specified for dspreadmem option\n");
        return -1; }
      dspreadmem = 1; }
  else
  if (strcmp(argv[argi], "--dspheader") == 0) {
      dspreadheader = 1; }
  else
  if (strcmp(argv[argi], "--dspprog") == 0) {
      if (argv[argi+1]) {
          param1 = atoi(argv[argi+1]);
      } else {
        fprintf(stderr, "No value specified for dspprog option\n");
        return -1; }
      dspprog = 1; }
  else
  if (strcmp(argv[argi], "--dspwrite") == 0) {
      if (argv[argi+1]) {
          param1 = atoi(argv[argi+1]);
      } else {
        fprintf(stderr, "No value specified for dspwrite option\n");
        return -1; }
      dspwrite = 1; }
  else
  if (strcmp(argv[argi], "--dspread") == 0) {
      if (argv[argi+1]) {
          param1 = atoi(argv[argi+1]);
      } else {
        fprintf(stderr, "No value specified for dspwrite option\n");
        return -1; }
      dspread = 1; }
  else
  if (strcmp(argv[argi], "--dacmode") == 0) {
      if (argv[argi+1]) {
          param1 = atoi(argv[argi+1]);
      } else {
        fprintf(stderr, "No value specified for dacmode option\n");
        return -1; }
      dacmode = 1; }
  else
  if(strcmp(argv[argi], "--dacstatus") == 0) {
          dacstatus = 1; }
  else
  if(strcmp(argv[argi], "--dacmute") == 0) {
          dacmute = 1; }
  else
  if(strcmp(argv[argi], "--dacunmute") == 0) {
          dacunmute = 1; }
  else
  if (strcmp(argv[argi], "--flashread") == 0) {
      if (argv[argi+1]) {
          param1 = atoi(argv[argi+1]);
      } else {
        fprintf(stderr, "No page specified for readflash option\n");
        return -1; }
      readflash = 1; }
  else
  if (strcmp(argv[argi], "--flasherase") == 0) {
      if (argv[argi+1]) {
          param1 = atoi(argv[argi+1]);
      } else {
        fprintf(stderr, "No sector specified for eraseflash option\n");
        return -1; }
      eraseflash = 1; }
  else {
    fprintf(stderr, "Invalid option passed to usb application\n");
    return -1; }

 
  r = libusb_init(NULL);
  if (r < 0) {
    fprintf(stderr, "failed to initialise libusb...\n");
    return -1; }

  r = find_xmos_device(0, listdev);
  if (r < 0)  {
      if(!listdev) {
          fprintf(stderr, "Could not find xmos usb device\n");
          return -1; }
  }

   if(resetdevice)  {
      printf("sending DFU command : XMOS_DFU_RESETDEVICE\n");
        xmos_resetdevice(XMOS_DFU_IF);
        printf("done.\n");
    }
   else
   if(modetest)  {
      printf("test\n");
      testvendor();
      printf("done.\n");
  } else
  if (listdev == 0) {

      if (xmosload) {
          xmos_enterdfu(XMOS_DFU_IF);
          write_dfu_image(XMOS_DFU_IF, filename);
          xmos_resetdevice(XMOS_DFU_IF);
      }
      else if (samdload) {
          xmos_enterdfu(XMOS_DFU_IF);
          //load samd prog
          xmos_resetdevice(XMOS_DFU_IF);
      }
      else if (dspload) {
          //vendor_to_dev(VENDOR_AUDIO_MUTE,0,0); // can be an option, then user need to unmute
          vendor_to_dev(VENDOR_AUDIO_STOP,0,0);
          load_dsp_prog(filename);
          vendor_to_dev(VENDOR_AUDIO_START,0,0);
      }
      else if (dspwrite) {
          vendor_to_dev(VENDOR_AUDIO_STOP,0,0);
          data[0] = param1; // no need for VENDOR_OPEN_FLASH nor VENDOR_CLOSE_FLASH
          vendor_from_dev(VENDOR_WRITE_DSP_FLASH, param1, 0, data, 1);
          if (data[0]) printf("Write to flash error num %d\n",data[0]);
          vendor_to_dev(VENDOR_AUDIO_START,0,0);
      }
      else if (dspread) {
          vendor_to_dev(VENDOR_AUDIO_STOP, 0, 0);
          data[0] = param1; // no need for VENDOR_OPEN_FLASH nor VENDOR_CLOSE_FLASH
          vendor_from_dev(VENDOR_READ_DSP_FLASH, param1, 0, data, 1);
          if (data[0]) printf("Read from flash error num %d\n",data[0]);
          vendor_to_dev(VENDOR_AUDIO_START, 0, 0);
      }
      else if (readflash) {
          vendor_to_dev(VENDOR_AUDIO_STOP, 0, 0);
          vendor_to_dev(VENDOR_OPEN_FLASH,0,0);
          vendor_from_dev(VENDOR_READ_FLASH, param1, 0, data, 64);
          for (int i = 0; i<4; i++) {
              printf("%6X : ",param1*64+i*16);
              for (int j=0; j<16; j++) {
                  printf("%2X ",data[i*16+j]); }
              printf("\n"); }
          vendor_to_dev(VENDOR_CLOSE_FLASH,0,0);
          vendor_to_dev(VENDOR_AUDIO_START, 0, 0);
      }
      else if (eraseflash) {
          vendor_to_dev(VENDOR_AUDIO_STOP, 0, 0);
          vendor_to_dev(VENDOR_OPEN_FLASH,0,0);
          vendor_from_dev(VENDOR_ERASE_FLASH, param1, 1, data, 1);
          if (data[0]) printf("erase flash error num %d\n",data[0]);
          vendor_to_dev(VENDOR_CLOSE_FLASH,0,0);
          vendor_to_dev(VENDOR_AUDIO_START, 0, 0);
      }
      else if (dspprog) {
          vendor_to_dev(VENDOR_SET_DSP_PROG, param1, 0);
      }
      else if (dspreadmem) {
          dspReadMem(param1);
      }
      else if (dspreadheader) {
          dspReadHeader();
      }
      else if (dacstatus) {
          getDacStatus();
      }
      else if (dacmode) {
          vendor_to_dev(VENDOR_SET_MODE,param1,0);
      }
      else if (dacmute) {
          vendor_to_dev(VENDOR_AUDIO_MUTE,0,0);
      } else if (dacunmute) {
          vendor_to_dev(VENDOR_AUDIO_UNMUTE,0,0);
      }

}
  libusb_close(devh);
  libusb_exit(NULL);
  return 1;
}
