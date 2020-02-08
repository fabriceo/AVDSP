/*
 * dsp_fileaccess.c
 *
 *  Created on: 12 janv. 2020
 *      Author: Fabrice
 */

#include "dsp_fileaccess.h"


// very basic file access for R/W ops and to read list of float e.g. for FIR impulse

static FILE* dspFile = NULL;
FILE* dspFileDump = NULL;
char* dspFileName;
long   dspFileSize;
char* dspFileNameDump;

int dumpFileIsOpen(){
    return (dspFileDump != NULL);
}
int dumpFileCreate(){
    dspFileDump = fopen( dspFileNameDump, "w" );
    if (dspFileDump == NULL) return -1;
    return 0;
}

void dumpFileClose(){
    fclose(dspFileDump);
}

int dumpFileInit(char * name){
    dspFileNameDump = name;
    if (name != NULL)
        if (*name != 0)
            return dumpFileCreate();
    return 0;
}

int dspfileIsOpen(){
    if( dspFile == NULL ) {
      fprintf(stderr,"Error: File is not open.\n");
      return -1; }
    return 0;
}

int dspfopenRead(char * mode){ //mode = "r" or "rb"
    dspFile = NULL;
    dspFile = fopen( dspFileName, mode );
    if( dspFile == NULL ) {
      fprintf(stderr,"Error: Failed to open input data file.\n");
      return -1;
    }
    if( 0 != fseek( dspFile, 0, SEEK_END ) ) {
      fprintf(stderr,"Error: Failed to discover input data file size.\n");
      fclose(dspFile);
      return -1;
    }

    dspFileSize = ftell( dspFile );

    if( 0 != fseek( dspFile, 0, SEEK_SET ) ) {
      fprintf(stderr,"Error: Failed to input file pointer.\n");
     return -1;
    }
    return 0;
}

int dspfreadData(char * dataPtr, int size){
    if (-1 == dspfileIsOpen() ) return -1;
    return fread(dataPtr, 1, size, dspFile);
}

int dspfopenWrite(char * mode){ // mode = "w" or "wb"
    dspFile = fopen( dspFileName, mode );
    if( dspFile == NULL ) {
      fprintf(stderr,"Error: Failed to open output data file.\n");
      return -1;
    }
    return 0;
}
int dspfwriteData(char * dataPtr, int size){
    if (-1 == dspfileIsOpen() ) return -1;
    fwrite(dataPtr, 1, size, dspFile);
    return 0;
}

void dspfclose(){
    fclose(dspFile);
}

int dspfreadFloat(float * value) {
    if (-1 == dspfileIsOpen() ) return -1;
    if (feof(dspFile)) {
        dspfclose();
        return -1; }
    if (1 != fscanf(dspFile, "%f", value)) return -1;
    return 0;
}


int dspfreadImpulse(float * coefPtr, int sizeMax){
    if(-1 == dspfileIsOpen()) return -1;
    int length = 0;
    while (1) {
        if (length >= sizeMax) return length;
        float val;
        if ( -1 == dspfreadFloat(&val) ) return length;
        *coefPtr++ = val;
        length++;
    }
}

int dspCreateBuffer(char * name, int * buff, int size){
    dspFileName = name;
    if (0 != dspfopenWrite("wb")) return -1;
    if (0 != dspfwriteData((char*)buff, size*sizeof(int))) return -1;
    dspfclose();
    return size;
}

int dspCreateIntFile(char * name, int * buff, int size, const char *begin, const char *end){
    dspFileName = name;
    if (0 != dspfopenWrite("w")) return -1;
    fprintf(dspFile,"%s\n",begin);
    for (int i=0; i<size; i++) {
        if (i!=(size-1)) fprintf(dspFile,"0x%X, ",*(buff+i));
        else fprintf(dspFile,"0x%X\n",*(buff+i));
        if ((i % 16) == 15) fprintf(dspFile,"\n");
    }
    fprintf(dspFile,"%s\n",end);
    dspfclose();
    return size;
}

int dspCreateAssemblyFile(char * name, int * buff, int size){
    dspFileName = name;
    if (0 != dspfopenWrite("w")) return -1;
    for (int i=0; i<size; i++) {
        fprintf(dspFile,".long %10d\n",*(buff+i));
    }
    dspfclose();
    return size;
}


int dspReadBuffer(char * name, int * buff, int size){
    dspFileName = name;
    if (0 != dspfopenRead("rb")) return -1;
    printf("file %s opened in read binary , taille = %ld\n",name, dspFileSize);
    if (dspFileSize > (size*sizeof(int))) return -1;
    if (-1 == dspfreadData((char*)buff, size*sizeof(int))) return -1;
    dspfclose();
    return size;
}
