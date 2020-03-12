/*
 * dsp_fileaccess.h
 *
 *  Created on: 12 janv. 2020
 *      Author: Fabriceo
 */


#ifndef DSP_FILEACCESS_H_
#define DSP_FILEACCESS_H_
#include <stdio.h>

extern char* dspFileName;
extern long   dspFileSize;
extern char* dspFileNameDump;
extern FILE* dspFileDump;
#define dumpprintf(...) fprintf(dspFileDump, __VA_ARGS__)


// open a file for reading, either in binary "rb" or text "r"
extern int dspfopenRead(char * mode);
// read a predefined quantity of bytes into the provide buffer adress
extern int dspfreadData(char * dataPtr, int size);
// creat a file in write mode binary with "wb" or text with "w"
extern int dspfopenWrite(char * mode);
// write a quantity of bytes in the file, return -1 in case of error
extern int dspfwriteData(char * dataPtr, int size);
// close the file (read or write)
extern void dspfclose();
// check if the file is open otherwise return -1
extern int dspfileIsOpen();
// read a simple float from the text file (1 per line)
extern int dspfreadFloat(float * value);
// read a predefined number of float from the text file into the provided buffer.
//return the number of float effectively loaded.
extern int dspfreadImpulse(float * coefPtr, int sizeMax);

extern int dspCreateBuffer(char * name, int * buff, int size);
extern int dspCreateIntFile(char * name, int * buff, int size, char * begin, char * end);
int dspCreateAssemblyFile(char * name, int * buff, int size);
extern int dspReadBuffer(char * name, int * buff, int size);

extern int  dumpFileIsOpen();
extern int  dumpFileCreate();
extern void dumpFileClose();
extern int  dumpFileInit(char * name);


#endif /* DSP_FILEACCESS_H_ */
