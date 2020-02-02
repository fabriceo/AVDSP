#include <stdlib.h>

#define ALSA_PCM_NEW_HW_PARAMS_API
#define ALSA_PCM_NEW_SW_PARAMS_API
#include <alsa/asoundlib.h>

static snd_pcm_t *inhandle,*outhandle;

int initAlsaIO(char *indevname, int nbchin, char* outdevname, int nbchout, int fs)
{
   int err;

    if ((err = snd_pcm_open(&inhandle, indevname, SND_PCM_STREAM_CAPTURE , 0 )) < 0) {
	fprintf(stderr, "ALSA I/O: Could not open audio input \"%s\": %s.\n", indevname, snd_strerror(err));
	return -1;
    }

    if ((err = snd_pcm_set_params(inhandle, SND_PCM_FORMAT_S32_LE, SND_PCM_ACCESS_RW_INTERLEAVED,
				nbchin, fs, 1, 500000)) < 0) {
	fprintf(stderr, "ALSA I/O: Could not set params  \"%s\": %s.\n", indevname, snd_strerror(err));
	return -1;
    }
    snd_pcm_prepare(inhandle);

    if ((err = snd_pcm_open(&outhandle, outdevname, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
	fprintf(stderr, "ALSA I/O: Could not open audio output \"%s\": %s.\n", outdevname, snd_strerror(err));
	return -1;
    }

    if ((err = snd_pcm_set_params(outhandle, SND_PCM_FORMAT_S32_LE, SND_PCM_ACCESS_RW_INTERLEAVED,
				nbchout, fs, 0, 500000)) < 0) {
	fprintf(stderr, "ALSA I/O: Could not set params  \"%s\": %s.\n", indevname, snd_strerror(err));
	return -1;
    }
    snd_pcm_prepare(outhandle);

    snd_pcm_start(outhandle);
    snd_pcm_start(inhandle);

    return 0;
}

int readAlsa(int *buffer , const int frames) {

    snd_pcm_sframes_t iframes;

    iframes = snd_pcm_readi(inhandle, buffer, frames);
    if (iframes < 0) {
	fprintf(stderr, "read error %s", snd_strerror(iframes));
               snd_pcm_recover(inhandle, iframes, 0);
		return -1;
    }
    return iframes;
 
}

int writeAlsa(const int *buffer , const int frames) {

    snd_pcm_sframes_t iframes,oframes;

    iframes=frames;
    oframes = snd_pcm_writei(outhandle, buffer, iframes);
    if (oframes < 0) {
              	snd_pcm_recover(outhandle, oframes, 0);
		return -1;
	    	}
    if (oframes !=iframes) {
		fprintf(stderr, "read diff  %ld %ld", iframes,oframes);
	    	}
    return 0;
}

