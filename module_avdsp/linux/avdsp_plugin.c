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

#define opcodesMax 1000
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
} snd_pcm_avdsp_t;


static inline void *area_addr(const snd_pcm_channel_area_t *area,
			      snd_pcm_uframes_t offset)
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
	int *dst = area_addr(dst_areas, dst_offset);
	int nc,n,ch;

	for(nc=0;nc<dsp->nbcores;nc++)  {
	  for (n=0;n < size ; n++) {

             dspSample_t inputOutput[inputOutputMax];

	     if(ext->format == SND_PCM_FORMAT_S32 ) {
	      for(ch=0;ch<dsp->coreio[nc].nbchin;ch++) 
    		inputOutput[dsp->coreio[nc].inputMap[ch]] = ((int*)src)[n*dsp->nbchin+(dsp->coreio[nc].inputMap[ch]-INOFFSET)];
	     } else {
	      for(ch=0;ch<dsp->coreio[nc].nbchin;ch++) 
    		inputOutput[dsp->coreio[nc].inputMap[ch]] = (int)(((short*)src)[n*dsp->nbchin+(dsp->coreio[nc].inputMap[ch]-INOFFSET)])<<16;
	     }
	
              DSP_RUNTIME_FORMAT(dspRuntime)(dsp->codestart[nc], dsp->dataPtr, inputOutput); 

              for(ch=0;ch<dsp->coreio[nc].nbchout;ch++) 
    		dst[n*dsp->nbchout+(dsp->coreio[nc].outputMap[ch]-OUTOFFSET)]=inputOutput[dsp->coreio[nc].outputMap[ch]] ;

	  }
         }

	return size;
}

static int dsp_init(snd_pcm_extplug_t *ext)
{
	snd_pcm_avdsp_t *dsp = ext->private_data;
	int dither;

	if(dspRuntimeReset(ext->rate,0,dsp->dither)) {
		SNDERR("avdsp filter not supported  sample freq : %d\n",ext->rate);
		return -EINVAL;
	}

	return 0;
}

static const snd_pcm_extplug_callback_t avdsp_callback = {
	.transfer = dsp_transfer,
	.init = dsp_init,
};

SND_PCM_PLUGIN_DEFINE_FUNC(avdsp)
{
	snd_config_iterator_t i, next;
	snd_pcm_avdsp_t *dsp;
	snd_config_t *sconf = NULL;
	int err;
	char *dspprogname = NULL;
        const int format_list[] =  { SND_PCM_FORMAT_S16, SND_PCM_FORMAT_S32 };

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
		}

		if (strcmp(id, "dither") == 0) {
			long val;
			if(snd_config_get_integer(n,&val)==0) {
				dsp->dither=val;
				continue;
			}
			SNDERR("Invalid dspprog name");
		}

		SNDERR("Unknown field %s", id);
		err = -EINVAL;
	ok:
		if (err < 0)
			return err;
	}

	if (!sconf) {
		SNDERR("No slave configuration for avdsp pcm");
		return -EINVAL;
	}

	if (!dspprogname) {
		SNDERR("No dspprog file for  avdsp pcm");
		return -EINVAL;
	}

	err = snd_pcm_extplug_create(&dsp->ext, name, root, sconf,
				     stream, mode);
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
    dsp->nbchin=0,dsp->nbchout=0;
    for(dsp->nbcores=0;dsp->nbcores<nbCoreMax; dsp->nbcores++) {

    	   opcode_t *corePtr = dspFindCore(dsp->opcodes, dsp->nbcores+1);  // find the DSP_CORE instruction
           if (corePtr) {
	       unsigned int usedInputs,usedOutputs;
               int * IOPtr = (int *)corePtr+1;             // point on DSP_CORE parameters

               usedInputs  = *IOPtr++;
               usedOutputs = *IOPtr;

               corePtr = dspFindCoreBegin(corePtr);    // skip any useless opcode to reach the real core begin

    		// compute nbchin / nbchout and input/output map by core
		dsp->coreio[dsp->nbcores].nbchin=dsp->coreio[dsp->nbcores].nbchout=0;
    		for(ch=0;ch<inputOutputMax;ch++) {
			if(usedInputs & (1<<ch)) {
				dsp->coreio[dsp->nbcores].inputMap[dsp->coreio[dsp->nbcores].nbchin]=ch;
				dsp->coreio[dsp->nbcores].nbchin++;
				if((ch-INOFFSET+1)>dsp->nbchin) dsp->nbchin=ch-INOFFSET+1;
			}
			if(usedOutputs & (1<<ch)) {
				dsp->coreio[dsp->nbcores].outputMap[dsp->coreio[dsp->nbcores].nbchout]=ch;
				dsp->coreio[dsp->nbcores].nbchout++;
				if((ch-OUTOFFSET+1)>dsp->nbchout) dsp->nbchout=ch-OUTOFFSET+1;
			}
    		}
    
            }
            dsp->codestart[dsp->nbcores] = corePtr;
            if (corePtr == 0) break;
    }

	snd_pcm_extplug_set_param(&dsp->ext, SND_PCM_EXTPLUG_HW_CHANNELS, dsp->nbchin);
	snd_pcm_extplug_set_slave_param(&dsp->ext,SND_PCM_EXTPLUG_HW_CHANNELS, dsp->nbchout);

        snd_pcm_extplug_set_param_list(&dsp->ext, SND_PCM_EXTPLUG_HW_FORMAT,2,format_list);
        snd_pcm_extplug_set_slave_param(&dsp->ext, SND_PCM_EXTPLUG_HW_FORMAT,SND_PCM_FORMAT_S32);

	*pcmp = dsp->ext.pcm;
	return 0;
}

SND_PCM_PLUGIN_SYMBOL(avdsp);
