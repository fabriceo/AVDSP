
#include "../runtime/dsp_header.h"  //expected in the runtime folder at same level as encoder folder

static const unsigned dispatch = 4;

static const unsigned tableMipsXS2[DSP_MAX_OPCODE] = {
    0,  //DSP_END_OF_CODE = 0,// this opcode value is 0 and its length is 0 as a convention
    0,  //DSP_HEADER,         // contain summary information about the program.
    0,  //DSP_PARAM,          // define an area of data (or parameters), like a sine wave or biquad coefs or any kind of data in fact
    0,  //DSP_PARAM_NUM,      // same as PARAM but the data area is indexed and each param_num section can be accessed separately
    0,  //DSP_NOP,            // sometime used to align opcode adress start to a 8byte cell
    0,  //DSP_CORE,           // used to separate each dsp code trunck and distribute opcodes on multiple tasks.
    0,  //DSP_SECTION,        //conditional section.

/* IO engine */
    5,  //DSP_LOAD = 7,           //load a sample from the sample array location Z into the ALU "X" without conversion in s.31 format
    6,  //DSP_STORE,          // store the LSB of ALU "X" into the sample aray location Z without conversion. sat0db expected upfront
    5,  //DSP_LOAD_STORE,     // move many samples from location X to Y without conversion (int32 or float) for N entries
    6+7,//DSP_STORE_TPDF,     // apply a gain and store sum in an output
    11, //DSP_STORE_GAIN,     // apply a gain and store sum in an output
    10, //DSP_LOAD_GAIN,      // load a sample from the sample array location Z into the ALU "X" and apply a Qnm gain. sum is double precision
    6,  //DSP_LOAD_MUX,       // combine many inputs samples into a value, same as summing many 2,  
    10, //DSP_MIXER,          // load all inputs with their respective gain, couples stores below opcode

    4,  //DSP_LOAD_X_MEM,       // load a memory location 64bits into the ALU "X" without any conversion.
    4,  //DSP_STORE_X_MEM,      // store the ALU "X" into a memory location without conversion (raw  64bits)
    4,  //DSP_LOAD_Y_MEM,       // load a memory location 64bits into the ALU "X" without any conversion.
    4,  //DSP_STORE_Y_MEM,      // store the ALU "X" into a memory location without conversion (raw  64bits)

    5,  //DSP_LOAD_MEM_DATA,  // load the double precision value stored in data space at an absolute adress

/* math engine */

    3,  //DSP_CLRXY = 20,          //clear both ALU register
    3,  //DSP_SWAPXY,         // exchange ALU X with second one "Y".
    1,  //DSP_COPYXY,         // copy ALU X in a second "Y" register.
    1,  //DSP_COPYYX,         // copy ALU Y to ALU X
    3,  //DSP_ADDXY,          // perform X = X + Y, 64 bits
    3,  //DSP_ADDYX,          // perform Y = X + Y
    3,  //DSP_SUBXY,          // perform X = X - Y
    3,  //DSP_SUBYX,          // perform Y = Y - X
    13, //DSP_MULXY,         // perform X = X * Y
    13, //DSP_MULYX,         // perform X = X * Y
    25, //DSP_DIVXY,         // perform X = X / Y
    25, //DSP_DIVYX,         // perform Y = Y / X
    6,  //DSP_AVGXY,          // perform X = X/2 + Y/2;
    6,  //DSP_AVGYX,          // perform Y = X/2 + Y/2;
    3,  //DSP_NEGX,           // perform X = -X
    3,  //DSP_NEGY,           // perform Y = -Y
    6,  //DSP_SHIFT,          // perform shift left or right if param is negative
    5,  //DSP_VALUEX,         // load an imediate value qnm or float
    5,  //DSP_VALUEY,         // load an imediate int32 value without conversion


/* gains */
    10,  //DSP_GAIN = 39,      // apply a fixed gain (qnm or float) on the ALU , if a ALU was a sample s.31 then it becomes s4.59
    10,  //DSP_CLIP,           // check wether the sample is reaching the thresold given and maximize/minize it accordingly.

    11,  //DSP_SAT0DB,         // verify boundaries -1/+1. and set a flag in case of saturation
    11,  //DSP_SAT0DB_VOL,     // apply volume and then verify boundaries -1/+1.
    7+3, //DSP_STORE_VOL,     // store the accumulator in the target location and apply digital volume
    20,  //DSP_SAT0DB_GAIN,    // apply a gain and then check boundaries as above
    2+3, //DSP_STORE_VOL_SAT, // store the accumulator in the target location and apply digital volume and eventually apply saturation gain and detect extra saturation
    2,   //DSP_SERIAL,          // if not equal to product serial number, then DSP will reduce its output by 24db !

/* delays */
    6,  //DSP_DELAY_1 = 47,    // equivalent to a delay of 1 sample (Z-1), used to synchronize multi-core output.
    15, //DSP_DELAY,          // 48 execute a delay line (32 bits ONLY). to be used just before a 2,  //DSP_STORE for example.
    16, //DSP_DELAY_DP,       // 49 same as 2,  //DSP_DELAY but 64bits (twice data space required ofcourse)


/* filters */
    14, //DSP_BIQUADS,        // 50 execute N biquad cell. ALU is expected s4.59 and will be return as s4.59
    0,  //DSP_DCBLOCK,         // remove any DC offset. equivalent to first order high pass with noise reinjection

/* specials */
    0,  //DSP_DATA_TABLE,      // extract one sample of a data block. typically used for wave generation
    14, //DSP_TPDF_CALC,      // generate radom number (white & triangular) and prepare for dithering bit Nth
    0,  //DSP_TPDF,            // prepare for dithering at bit N as parameter
    8,  //DSP_WHITE,           // load the random int32 number that was generated for the tpdf
    0,  //DSP_DITHER,          // add dithering on bit x and noise shapping
    0,  //DSP_DITHER_NS2,      // same with custom noise shapping factors
    0,  //DSP_DISTRIB,         // experimental : distribute the value of ALU towards a table[N] and provide table[i] as outcome. Used to show the noise distibution on a scope view
    0,  //DSP_DIRAC,           // generate a single sample pulse at a given frequency. frequency and pulse amplitude depends on provided float number
    0,  //DSP_SQUAREWAVE,      // generate a square waved (zero symetrical) at a given frequency and amplitude
    25, //DSP_SINE,           // generate a sine wave (zero symetrical) at a given frequency using modified coupled form oscillator.
    2,  //DSP_SQRTX,           // perfomr X = sqrt(x) where x is int64 or float
    2,  //DSP_RMS,             // compute sum of square during a given period then compute moving overage with sqrt (64bits->32bits)
    16,  //DSP_FIR,            // execute a fir filter with many possible impulse depending on frequency
    20,  //DSP_WFIR,           // execute a warped fir filter with many possible impulse depending on frequency

    29,  //DSP_DELAY_FB_MIX = 66,
    14,  //DSP_INTEGRATOR,
    28,  //DSP_CICUS,
    26,  //DSP_CICN,
    16,  //DSP_EXPMA,
    19,  //DSP_THDCOMP,
    2,  //DSP_BIQUADS_FS,      //accept cascaded biquads parameters. single raw of coefficicients expected to be computed at each fs change
    2,  //DSP_BIQUADS_FS_FAST, //same, using VPU capabilities, increasing THD+N at lowfrequency and high sampling rate
    2,  //DSP_BIQUADS_FS_FAST8,   //same using specific VPU capability to handle 8 biquad section as fast as possible
    2,  //DSP_SEND,            //send data to other tiles
    2,  //DSP_RECEIVE,         //receive data from other tiles
    2,  //DSP_DRC_ENV_PEAK,    //compute peak enveloppe, given an attack+release alpha coeficient
    2,  //DSP_DRC_ENV_RMS,     //compute rms enveloppe, , given an attack+release alpha coeficient
    2,  //DSP_DRC_LIM_PEAK,    //gain limiter based on peak enveloppe
    2,  //DSP_DRC_LIM_RMS,     //gain limiter based on rms enveloppe
    2,  //DSP_DRC_LIM_PEAK_CLIP,  //gain limiter based on peak enveloppe, and hard clipping anyway
    2,  //DSP_DRC_COMPRESSOR,  // compressor based on rms enveloppe, with threshold, gain and slope
    2,  //DSP_DRC_EXPANDER,    // expander based on rms enveloppe, with threshold, gain and slope
    2,  //DSP_DRC_NOISE_GATE,  //remove low level signal based on rms enveloppe, with threshold, gain

    4,  //DSP_MEMCLR = 85,     //these functions extends the possibilities already offered with Y accumulator
    6,  //DSP_SWAPMEM,
    2,  //DSP_ADDMEM,
    7,  //DSP_MEMADD,
    2,  //DSP_SUBMEM,
    7,  //DSP_MEMSUB,
    18, //DSP_MULMEM,
    29, //DSP_DIVMEM,
    8,  //DSP_AVGMEM,
    9,  //DSP_MEMAVG,
    7,  //DSP_MEMNEG,
    4,  //DSP_SAVEMEM,
    4,  //DSP_LOADMEM,
    5,  //DSP_MEMSAVEXY,
    5,  //DSP_LOADMEMXY,
    7,  //DSP_MEMVALUE,
    0,  //DSP_MIXERMEM,
    0,  //DSP_MEMGAIN,
    0,  //DSP_MEMINPUT,

    0,  //DSP_CORE_AES,
    0,  //DSP_FULL_LOAD,
    2,  //DSP_PRIO_ON,
    2,  //DSP_PRIO_OFF,
    6,  //DSP_TRANSFER2
    14, //DSP_TRANSFER8
    
    5,  //DSP_LOADY,
    8,  //DSP_LOADXY,
    7,  //DSP_STOREY,
    9,  //DSP_STOREXY,
    10, //DSP_GAINY,
    17, //DSP_GAINXY,
    19, //DSP_GAIN_X_Y,
    15, //DSP_DELAYY,
    20, //DSP_SAT0DBXY,
    20, //DSP_SAT0DBXY_VOL,
    15, //DSP_BIQUADSXY,
    1,  //DSP_BIQUADSXY_FS,

    11,  //DSP_FLOAD,
    7,   //DSP_FSTORE,
    10,  //DSP_FVALUEX,
    19,  //DSP_FGAIN,
    2,   //DSP_FBIQUADS,  
    // new opcodes should come here below
};

int dspOpcodeMips(unsigned opcode) {
    int inst = tableMipsXS2[opcode];
    return inst;
}

//ptr = begining of core program, just after header and normally on a core op code
//condition correspond to the pattern given when launching runtimeinit (see dac8pro)
//minfreq 44k = 4; maxfreq 384 = 11
int getMipsEstimate(opcode_t * ptr, unsigned cond, unsigned minfreq, unsigned maxfreq) {
    int sum = 0, summax = 0;
    int endsectionsum = 0;
    int firstcode = 0;
    int coreseen = 0;
    int lastsectionsum =0;
    int pos = 0;
    dspprintf3("estimating instructions...\n");
    opcode_t * elseskipptr = 0;
    while (ptr) {
        if (sum > summax) summax = sum;
        unsigned code = ptr->op.opcode;
        int skip = ptr->op.skip;
        //dspprintf3("<%s>\n",dspOpcodeText(code))
        if ((code == DSP_END_OF_CODE)||(skip == 0)) return ((sum > endsectionsum) ? sum : endsectionsum);
        if (code == DSP_CORE_EXTERN) return ((sum>endsectionsum) ? sum : endsectionsum);
        if (code == DSP_CORE) { 
            if (coreseen) return ((sum>endsectionsum) ? sum : endsectionsum);    //begining of a new core
            coreseen = 1;
            if (cond) {
                int n = (skip-6)/2; //number of conditions seen, 2 words each
                unsigned res=0;
                for (int i=0; i<n ; i++) {
                    res = (cond & ptr[6+i+i].u32) && ((cond & ptr[7+i+i].u32)==0);
                    if (res) break;
                }
                if (res == 0) return 0;
                dspprintf4("%4d %16s validated\n",pos,dspOpcodeText(code));
            } else 
                dspprintf4("%4d %16s valid with COND = 0\n",pos,dspOpcodeText(code));
            ptr += skip; pos += skip; continue;
        }
        if (firstcode == 0) { //no opcode seen yet, first time
            if ((code == DSP_NOP) || (code == DSP_PARAM) || (code == DSP_PARAM_NUM) ) 
                 firstcode = 0;
            else firstcode = 1;
        } 
        if (firstcode == 1) {
            int param1 = ptr[1].u32;
            int inst = 0;
            if (sum > endsectionsum) endsectionsum = sum;

            if (ptr == elseskipptr) {
                //at the end of the section else. count inst and check max.
                if (endsectionsum>sum) { 
                    sum = endsectionsum;
                    dspprintf4("end of sectionelse detected, sum adjusted max %d\n",sum);
                } else
                    dspprintf4("end of sectionelse detected, sum %d\n",sum);
                elseskipptr=0;
            }
            switch (code) {

                case DSP_SECTION : {

                    unsigned elsecode = ptr[-1].op.opcode;
                    int elseskip = ptr[-1].op.skip;
                    if ((elsecode == DSP_NOP) && (elseskip > 1)) {
                        elseskipptr = ptr-1+elseskip;
                        //sectionelse with condition
                        sum = lastsectionsum;
                        dspprintf4("sectionelse (after NOP) with condition\n");
                        dspprintf4("restart estimation with %d, max %d\n",sum,endsectionsum);
                    } else {
                        elseskipptr = 0;
                        //totaly new section.
                        if (endsectionsum > sum) { 
                            sum = endsectionsum;
                            dspprintf4("new section, sum max adjusted %d\n",sum);
                        } else 
                            dspprintf4("new section, sum %d\n",sum);
                    }
                    lastsectionsum = -1;    //this will memorize where we are later
                    int n = (skip-4); //number of conditions seen (multiple of 2) 
                    int i=0;
                    unsigned res = 0;
                    inst = 1;
                    while(1) {
                        inst += 3;
                        if (cond & ptr[2+i].u32) {
                            inst++;
                            if ((cond & ptr[3+i].u32)==0) { inst++; res=1; break; }
                        }
                        inst++;
                        if (n==0) { 
                            if (cond == 0) res=1;
                            inst +=3; break; 
                        }
                        n-=2;
                    }
                    if (res == 0) {
                        dspprintf4("section not validated, COND = %X\n",cond);
                        skip = param1;
                    } else 
                        dspprintf4("section validated, COND = %X\n",cond);
                    break;}

                case DSP_NOP : {
                    if (skip > 1) {
                        dspprintf4("sectionelse (NOP) identified\n");
                        if (cond == 0) {
                            //as we dont know if the previous one was to be done, then we continue here
                            sum += dispatch;
                            if (sum > endsectionsum) endsectionsum = sum;
                            sum = lastsectionsum;
                            dspprintf4("restart estimation with %d, sum %d\n",sum,endsectionsum);
                            ptr++; pos++;
                            continue;
                        } 
                    }
                    break; }
                case DSP_LOAD_STORE : { 
                    inst = ((skip-1)/2-1) * 6; 
                    if (inst==0) inst++;
                    break; }
                case DSP_MIXER :      { 
                    inst = ((skip-1)/2-1) * 6; break; }
                case DSP_LOAD_MUX : {
                    short num = ptr[param1].s16.low;
                    inst = num * 6;
                    break; }
                case DSP_ADDMEM : 
                case DSP_SUBMEM : 
                case DSP_MIXERMEM : { 
                    inst = (skip-1) * 6; break; }
                case DSP_STORE : //fallthrough
                case DSP_STOREY:
                case DSP_STORE_VOL : 
                case DSP_STORE_VOL_SAT : 
                case DSP_STORE_TPDF : {
                    if (param1 & 0xFF000000) inst = 8;
                    else if (param1 & 0xFF0000) inst = 6;
                    else if (param1 & 0xFF00) inst = 3;
                    break; }
                case DSP_BIQUADS : {
                    int param2 = ptr[2].i32;
                    short sections = ptr[param2].s16.low;
                    inst = sections * 18;
                    break; }
                case DSP_BIQUADSXY : {
                    int param2 = ptr[2].i32;
                    short sections = ptr[param2].s16.low;
                    inst = sections * 33;
                    break; }
                case DSP_FIR : 
                case DSP_WFIR : {
                    int mul = (code == DSP_FIR) ? 1 : 2;
                    int maxtaps = 0;
                    for (int freq = minfreq; freq <= maxfreq; freq++) {
                        if ( (((freq==4)||(freq==5))  && ((cond & 0x100) || (cond == 0))) ||
                             (((freq==6)||(freq==7))  && ((cond & 0x200) || (cond == 0))) ||
                             (((freq==8)||(freq==9))  && ((cond & 0x400) || (cond == 0))) ||
                             (((freq==10)||(freq==11))&& ((cond & 0x800) || (cond == 0))) ) {
                                int taps = ptr[2+1+(freq-minfreq)*(mul+1)].i32;                                
                                if (taps > maxtaps) maxtaps = taps;
                                dspprintf4("freq %d, taps %d, max %d\n",freq,taps,maxtaps);
                             }
                    } //for
                    if (code == DSP_FIR) {
                        int mul16 = maxtaps / 16;
                        int min16 = maxtaps % 16;
                        const unsigned table[16] = { 0, 5, 7, 12, 14, 19,21,26,28, 33,35,40,42, 47,49,52 };
                        inst = 44 * mul16 + table[min16];
                        if (mul16) inst++;
                    } else if (code == DSP_WFIR) {
                        int mul8 = maxtaps / 8;
                        int min8 = maxtaps % 8;
                        const unsigned table[8] = {  0, 7, 12, 18, 23, 29, 34, 39 }; 
                        inst = 39 * mul8 + table[min8] + (mul8==0?1:0);
                    }
                    break;}
                case DSP_FULL_LOAD : { 
                    if (param1) inst = param1;
                    else return ((sum>endsectionsum) ? sum : endsectionsum);
                    break;}
            } //switch
            int total = dispatch + inst + tableMipsXS2[code];
            sum += total;
            if (lastsectionsum == -1) lastsectionsum = sum;
            dspprintf3("%4d %16s: base %2d, inst %3d, result %3d, sum %d\n",pos,dspOpcodeText(code),tableMipsXS2[code],inst,total,sum);
        }
        ptr += skip ; pos += skip;
    } //while ptr;
    return ((sum>endsectionsum) ? sum : endsectionsum);
}
