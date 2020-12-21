#include "dsp_encoder.h"

// true example of 3 way cross over + LFE for DAC8PRO,
// used by the forumer CDGG for his Home Therater HC-Cocoon


extern int fcross;  // default crossover frequency for the demo
extern int subdelay;

#define DACOUT(x) (x)               // DAC outputs are stored at the begining of the samples table
#define ADCIN(x)  (8  + x )         // SPDIF receiver stored with an offset of 8
#define USBOUT(x) (16 + x)          // the samples sent by USB Host are offseted by 16
#define USBIN(x)  (16 + 8 + x)      // the samples going to the USB Host are offseted by 24
#define modeREW 1                   // used during simulation to send back result to USB Host, no DAC output
#define OUT(x) ((modeREW) ? USBIN(x) : DACOUT(x))

//#define dB(x) (POW(10.0,(x/20.0))

static void crossOver3ways(int in, int outlow, int outmid, int outhigh,
                    int Flow, int Flowmid, int Fmidhigh,
                    float Glow, float Gmid, float Ghigh,
                    int Dlow, int Dmid, int Dhigh) {

    dsp_PARAM();

    int BQlow = dspBiquad_Sections( 9 );
        dsp_HP_BUT8(Flow);      // 4cells
        dsp_LP_BUT6(Flowmid);   // 3cells
        dsp_Filter2ndOrder(FPEAK,160, 1.3, 0.82224);//dB(-1.7));
        dsp_Filter2ndOrder(FPEAK,475, 7,   0.74989);//dB(-2.5));

    int BQmid = dspBiquad_Sections( 10 );
        dsp_HP_BUT3(Flowmid);   //2cells
        dsp_LP_BUT8(Fmidhigh);  //4cells
        dsp_Filter2ndOrder(FPEAK, 1400,  2.8, 0.84140);//dB(-1.5));
        dsp_Filter2ndOrder(FPEAK, 2000,  7,   1.1885);//dB( 1.5));
        dsp_Filter2ndOrder(FPEAK, 8180, 10,   0.84140);//dB(-1.5));
        dsp_Filter1stOrder(FHS1, 11800,    2.81838);//dB( 9.0));

    int BQhigh = dspBiquad_Sections( 5 );
        dsp_HP_BUT8(Fmidhigh);  // 4 cells
        dsp_Filter2ndOrder(FPEAK,  9500,  10, 0.50119);//dB(-6.0));

    dsp_LOAD_GAIN_Fixed( USBOUT(in), Glow);
    dsp_BIQUADS(BQlow);
    dsp_SAT0DB_TPDF();
    if (Dlow) dsp_DELAY_FixedMicroSec(Dlow);
    dsp_STORE( OUT(outlow) );

    dsp_LOAD_GAIN_Fixed( USBOUT(in), Gmid);
    dsp_BIQUADS(BQmid);
    dsp_SAT0DB_TPDF();
    dsp_DELAY_FixedMicroSec(Dmid);
    dsp_STORE( OUT(outmid) );

    dsp_LOAD_GAIN( USBOUT(in), Ghigh);
    dsp_BIQUADS(BQhigh);
    dsp_SAT0DB_TPDF();
    dsp_DELAY_FixedMicroSec(Dhigh);
    dsp_STORE( OUT(outhigh));
}

static void LFEChannel(int in1, int in2, int out,
                       int Flfe, int Glfe, int Dlfe){
    dsp_PARAM();
    int filterlfe = dspBiquad_Sections(6);
        dsp_Filter2ndOrder(FPEAK, 30,  4.0,  0.7);
        dsp_Filter2ndOrder(FPEAK, 31, 10.0,  0.25);
        dsp_Filter2ndOrder(FPEAK, 71,  8.0,  0.53);
        dsp_LP_BUT6(Flfe);   // 3cells

    int mux1 = dspLoadMux_Inputs(2);
        dspLoadMux_Data(USBOUT(in1), 0.5);
        dspLoadMux_Data(USBOUT(in2), 0.5);

    dsp_LOAD_MUX(mux1);
    dsp_BIQUADS(filterlfe);
    dsp_SAT0DB_TPDF();
    dsp_DELAY_FixedMicroSec(Dlfe);
    dsp_STORE( OUT(out) );
}

static int StereoCrossOver(){

    const int Flow  =    45;
    const int Fmid  =   580;
    const int Fhigh = 10000;

    const float Glow  = 1.0;
    const float Gmid  = 0.85114;
    const float Ghigh = 0.74131;

    const int Dlow  =  150;
    const int Dmid  =    0;
    const int Dhigh = 1320;

    dsp_CORE();

    crossOver3ways(0, 2, 3, 4, Flow, Fmid, Fhigh,Glow, Gmid, Ghigh, Dlow, Dmid, Dhigh);

    dsp_CORE();

    crossOver3ways(0, 5, 6, 7, Flow, Fmid, Fhigh,Glow, Gmid, Ghigh, Dlow, Dmid, Dhigh);

    dsp_CORE();

    dsp_TPDF(24);

    const int Flfe  =    50;
    const float Glfe  = 0.335; //-9.5db
    const int Dlfe  =  7600;

    if (modeREW) {
        dsp_LOAD_STORE();   // loop back for minimum delay time reference for testing with REW
        dspLoadStore_Data( USBOUT(1), USBIN(1) );
    }
    LFEChannel(0,(modeREW?0:1),0,Flfe, Glfe, Dlfe);

    return dsp_END_OF_CODE();
}

int dspProg_HCcocoon(){

    return StereoCrossOver();

}
