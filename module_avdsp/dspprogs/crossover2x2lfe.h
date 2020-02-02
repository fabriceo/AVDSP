
extern int fcross;  // default crossover frequency for the demo
extern int distance; // defaut distance between low and high. (positive means high in front)
extern int flfe;
extern int subdelay;

extern int StereoCrossOverLFE(int left, int right, int outs, int fx, int dist, int flfe);


static inline int dspProg(){

    return StereoCrossOverLFE(0,1,0,fcross,distance,flfe);


}
