extern int initAlsaIO(char *indevname, int nbchin, char* outdevname, int nbchout, int fs);
extern int readAlsa(int *buffer , const int frames) ;
extern int writeAlsa(const int *buffer , const int frames) ;

