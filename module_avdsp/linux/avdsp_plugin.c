/*
 * Avdsp DSP plugin
 *
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <alsa/asoundlib.h>
#include <alsa/pcm_external.h>
#include "dsp_fileaccess.h" // for loading the opcodes in memory
#include "dsp_runtime.h"
#include <time.h>

#define opcodesMax 2000 // = 8kbytes
#define nbCoreMax 8

// ouput from 0 to 7, input from 8 to 15
#define inputOutputMax 16
#define OUTOFFSET 0
#define INOFFSET 8

typedef struct {
    int		nbchin, nbchout;
    int 	inputMap[inputOutputMax];
    int 	outputMap[inputOutputMax];
} coreio_t;


typedef struct {
    snd_pcm_extplug_t ext;

    int nbcores;
    int	dither;
    int nbchin,nbchout;
    coreio_t coreio[nbCoreMax];

    opcode_t opcodes[opcodesMax];
    opcode_t *codestart[nbCoreMax];
    int *dataPtr;

    int status;         // 0 not loaded, 1 loaded, 2 inserted, 3 ready for transfer, 4 some transfer done

    int timestat;       // periodic prinitng of CPU usage
    double timespenttotal;
    double samplestotal;
    double samplesmax;
    int tagoutput;      // tag the LSB, accepted value : 24 or 32 if not 0
    unsigned tagmask;
    int previoussample;
} snd_pcm_avdsp_t;


static inline void *area_addr(const snd_pcm_channel_area_t *area, snd_pcm_uframes_t offset)
{
	unsigned int bitofs = area->first + area->step * offset;
	return (char *) area->addr + bitofs / 8;
}

static snd_pcm_sframes_t
dsp_transfer(snd_pcm_extplug_t *ext,
	     const snd_pcm_channel_area_t *dst_areas,
	     snd_pcm_uframes_t dst_offset,
	     const snd_pcm_channel_area_t *src_areas,
	     snd_pcm_uframes_t src_offset,
	     snd_pcm_uframes_t size)
{
	snd_pcm_avdsp_t *dsp = ext->private_data;

	void *src = area_addr(src_areas, src_offset);
	int  *dst = area_addr(dst_areas, dst_offset);

	int nc,n,ch;
	clock_t start,stop;
	double timespent;

	if (dsp->status == 3) {
	    //printf("AVDSP first data transfer received.\n");
        dsp->status = 4;
	}
    start = clock();

	// for each core
	for(nc = 0; nc < dsp->nbcores; nc++)  {

	  // for each samples provided
	    for (n=0;n < size ; n++) {

	        dspSample_t inputOutput[inputOutputMax];   // temporary buffer for treating 1 sample only

	        // for each channel used by the dspcore
	        for( ch = 0; ch < dsp->coreio[nc].nbchin; ch++) {

                int in = dsp->coreio[nc].inputMap[ch];
                int samplepos = n * dsp->nbchin + (in - INOFFSET);
                dspSample_t sample;

                switch (ext->format) {
                    case SND_PCM_FORMAT_S32 :
                        sample = ((int*)src)[ samplepos ];
                        break;
                    case SND_PCM_FORMAT_S24_3LE : {
                        unsigned char * ptr = (unsigned char *)src;
                        samplepos *= 3; // 3 bytes per sample. not 32bits alligned...
                        sample = (ptr[samplepos] <<8) | (ptr[samplepos+1] <<16 ) | (ptr[samplepos+2] << 24 );
                        break; }
                    case SND_PCM_FORMAT_S16 :
                        sample = (int)(((short*)src)[ samplepos ]) << 16;
                        break;
                }
                inputOutput[ in ] = sample;
	        }
	
	        // execute one dsp core
	        DSP_RUNTIME_FORMAT(dspRuntime)(dsp->codestart[nc], dsp->dataPtr, inputOutput);

	        // extract results of the job done by the dspruntime and put it out as S32_LE stream
	        for(ch = 0; ch < dsp->coreio[nc].nbchout; ch++) {
	            int out = dsp->coreio[nc].outputMap[ch];
	            int sample = inputOutput[ out ];
	            // add a tag that can be recognized easily to verify bitperfect, kind of dithering approach
	            if ((ch == 0) && (dsp->tagoutput)) {
	                int newsample = sample & dsp->tagmask;
	                sample = newsample | (((unsigned)dsp->previoussample) >> dsp->tagoutput);
	                dsp->previoussample = newsample;
	            }
                dst[ n*dsp->nbchout + ( out - OUTOFFSET ) ] = sample;

	        }

	    } // for each samples
     } // for each channels

    // printing time spent in dspruntime, averaged. period depends on "timestat" parameter.
    if (dsp->samplesmax != 0.0) {
        stop = clock();
        timespent = ((double)(stop - start)) / CLOCKS_PER_SEC ; // result is in seconds
        timespent *= 1000000.0;     // now in micro sec
        dsp->timespenttotal += timespent;
        dsp->samplestotal += size;

        if (dsp->samplestotal > dsp->samplesmax) {
            timespent = dsp->timespenttotal / dsp->samplestotal;  // averaged time spent by samples
            double timesample = 1000000.0 / (double)ext->rate; // sample duration in micro sec
            double percent = 100.0 * timespent / timesample;
            printf("AVDSP time spent per samples = %f uSec = %f percents at %ld hz\n", timespent, percent, ext->rate);
            dsp->timespenttotal = 0; dsp->samplestotal = 0; // reset avg
            if (dsp->timestat == 1) dsp->samplesmax = 0.0;   // check for unic printing
        }
    }

	return size;
}

static int dsp_close(snd_pcm_extplug_t *ext) {
    snd_pcm_avdsp_t *dsp = ext->private_data;
    if ((dsp) && (dsp->status)) free(dsp);
    printf("AVDSP closed\n");
    return 0;
}

static int dsp_init(snd_pcm_extplug_t *ext)
{
	snd_pcm_avdsp_t *dsp = ext->private_data;
	int dither;
	int fs = ext->rate;
	printf("AVDSP dspRuntimeReset(%ld)\n",fs);
	if(dspRuntimeReset(fs, 0, dsp->dither)) {
		SNDERR("avdsp filter not supported  sample freq : %d\n",fs);
		return -EINVAL;
	}
	dsp->status = 3;    // ready for transfer
	dsp->timespenttotal = 0.0;
	dsp->samplestotal   = 0.0;
	if (dsp->timestat) dsp->samplesmax = (double)fs * (double)(dsp->timestat);    // stats will be printed every x sec
	else dsp->samplesmax = 0.0;
	dsp->previoussample = 0;
	return 0;
}



static const snd_pcm_extplug_callback_t avdsp_callback = {
	.transfer = dsp_transfer,
	.init     = dsp_init,
	.close    = dsp_close,
};

SND_PCM_PLUGIN_DEFINE_FUNC(avdsp)
{
	snd_config_iterator_t i, next;
	snd_pcm_avdsp_t *dsp;
	snd_config_t *sconf = NULL;
	int err;
	char *dspprogname = NULL;
        const int format_list[] =  { SND_PCM_FORMAT_S16, SND_PCM_FORMAT_S32, SND_PCM_FORMAT_S24_3LE };

    	int size,result;
    	int n,ch;

	dsp = calloc(1, sizeof(*dsp));
	if (!dsp)
		return -ENOMEM;

	dsp->ext.version = SND_PCM_EXTPLUG_VERSION;
	dsp->ext.name = "Avdsp Plugin";
	dsp->ext.callback = &avdsp_callback;
	dsp->ext.private_data = dsp;
	dsp->dither=31;

	dsp->status = 1;
	dsp->timestat = 0;
	dsp->tagoutput = 0;
	dsp->tagmask = 0;

	printf("AVDSP opening plugin...\n");

	snd_config_for_each(i, next, conf) {
		snd_config_t *n = snd_config_iterator_entry(i);
		const char *id;
		if (snd_config_get_id(n, &id) < 0)
			continue;
		if (strcmp(id, "comment") == 0 || strcmp(id, "type") == 0 ||
		    strcmp(id, "hint") == 0)
			continue;
		if (strcmp(id, "slave") == 0) {
			sconf = n;
			continue;
		}
		if (strcmp(id, "dspprog") == 0) {
			if(snd_config_get_string(n,(const char **)&dspprogname)==0) 
				continue;
			SNDERR("Invalid dspprog name");
            goto bad;
		}

		if (strcmp(id, "dither") == 0) {
			long val;
			if(snd_config_get_integer(n,&val)==0) {
			    if ((val == 0) || ((val >=7) && (val <=31))) {
			        dsp->dither=val;
			        continue;
			    }
			}
			SNDERR("Invalid dither value");
            goto bad;
		}
        if (strcmp(id, "timestat") == 0) {
            long val;
            if(snd_config_get_integer(n,&val)==0) {
                if ((val >= 0) && (val <= 60)) {
                    dsp->timestat = val;
                    continue;
                }
            }
            SNDERR("Invalid timestat value");
            goto bad;

        }
        if (strcmp(id, "tagoutput") == 0) {
            long val;
            if(snd_config_get_integer(n,&val)==0) {
                if ((val >= 15) && (val <= 31)) {
                    dsp->tagoutput = val;
                    dsp->tagmask = 0xFFFFFFFF << (32-val);
                    continue;
                }
            }
            SNDERR("Invalid tagoutput value");
            goto bad;
        }

		SNDERR("Unknown field %s", id);
    bad:
		err = -EINVAL;
	ok:
		if (err < 0)
			return err;
	}

	if (!sconf) {
		SNDERR("No slave configuration defined for avdsp pcm");
		return -EINVAL;
	}

	if (!dspprogname) {
		SNDERR("No dspprog file defined for avdsp pcm");
		return -EINVAL;
	}

	err = snd_pcm_extplug_create(&dsp->ext, name, root, sconf, stream, mode);
	if (err < 0) {
		free(dsp);
		return err;
	}


   // load dsp prog
    size = dspReadBuffer(dspprogname, (int*)(dsp->opcodes), opcodesMax);
    if (size < 0) {
        SNDERR("FATAL ERROR trying to load opcode.\n");
        return -EINVAL;
    }

    // verify frequency compatibility with header, and at least 1 core is defined, and checksum ok
    result = dspRuntimeInit(dsp->opcodes, size, 0, 0, 0);
    if (result < 0) {
        SNDERR("FATAL ERROR: problem with opcode header or compatibility\n");
        return -EINVAL;
    }
    // runing data area just after the program code
    dsp->dataPtr = (int*)(dsp->opcodes) + result;

    // find cores and compute iomaps
    dsp->nbchin=0, dsp->nbchout=0;
    for(dsp->nbcores=0; dsp->nbcores<nbCoreMax; dsp->nbcores++) {

    	   opcode_t *corePtr = dspFindCore(dsp->opcodes, dsp->nbcores+1);  // find the DSP_CORE instruction
           if (corePtr) {
               unsigned int usedInputs,usedOutputs;
               int * IOPtr = (int *)corePtr+1;             // point on DSP_CORE parameters

               usedInputs  = *IOPtr++;
               usedOutputs = *IOPtr;

               corePtr = dspFindCoreBegin(corePtr);    // skip any useless opcode to reach the real core begin

               // compute nbchin / nbchout and input/output map by core
               dsp->coreio[dsp->nbcores].nbchin = dsp->coreio[dsp->nbcores].nbchout = 0;

                for(ch=0;ch<inputOutputMax;ch++) {
                    if(usedInputs & (1<<ch)) {
                        dsp->coreio[dsp->nbcores].inputMap[dsp->coreio[dsp->nbcores].nbchin] = ch;
                        dsp->coreio[dsp->nbcores].nbchin++;
                        if((ch-INOFFSET+1)>dsp->nbchin) dsp->nbchin = ch-INOFFSET+1;
                    }
                    if(usedOutputs & (1<<ch)) {
                        dsp->coreio[dsp->nbcores].outputMap[dsp->coreio[dsp->nbcores].nbchout] = ch;
                        dsp->coreio[dsp->nbcores].nbchout++;
                        if((ch-OUTOFFSET+1)>dsp->nbchout) dsp->nbchout = ch-OUTOFFSET+1;
                    }
                }
            }
            dsp->codestart[dsp->nbcores] = corePtr;
            if (corePtr == 0) break;
    }

    printf("AVDSP nbcores %d, nbchanin %d, nbchanout %d\n",dsp->nbcores, dsp->nbchin,dsp->nbchout);

    snd_pcm_extplug_set_param(&dsp->ext, SND_PCM_EXTPLUG_HW_CHANNELS, dsp->nbchin);
    snd_pcm_extplug_set_slave_param(&dsp->ext,SND_PCM_EXTPLUG_HW_CHANNELS, dsp->nbchout);

    snd_pcm_extplug_set_param_list(&dsp->ext, SND_PCM_EXTPLUG_HW_FORMAT,3,format_list);
    snd_pcm_extplug_set_slave_param(&dsp->ext, SND_PCM_EXTPLUG_HW_FORMAT,SND_PCM_FORMAT_S32 );

    dsp->status = 2;
    *pcmp = dsp->ext.pcm;

	return 0;
}

SND_PCM_PLUGIN_SYMBOL(avdsp);
