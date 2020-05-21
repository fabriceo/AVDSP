

extern void dspPresetWriteTextfile(dspPreset_t * p, int numberOfPresets, char * filename);
extern int  dspPresetReadTextfile(dspPreset_t * p, int numberOfPresets, char * filename);
extern int  miniParseFile(dspPreset_t * p, int preset, char * filename);
