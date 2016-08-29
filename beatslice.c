#include <stdio.h>
#include <sndfile.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#define FLT float

typedef struct {
    FLT b0, b1, b2, a0, a1, a2;
} biquad_coef;

typedef struct {
    biquad_coef *coef;
    FLT x1, x2, y1, y2;
} biquad_filter;

static FLT filter(biquad_filter *filt, FLT x)
{
    biquad_coef *c = filt->coef;
    FLT out = 
        c->b0*x + 
        c->b1*filt->x1 + 
        c->b2*filt->x2 -
        c->a1*filt->y1 -
        c->a2*filt->y2;

    filt->x2 = filt->x1;
    filt->x1 = x;

    filt->y2 = filt->y1;
    filt->y1 = out;
    return out;
}

static void normalize(FLT *buf, long size)
{
    long i;
    FLT max = 0;
    FLT smp = 0;
    for(i = 0; i < size; i++) {
        smp = fabs(buf[i]);
        if(smp > max) max = smp;
    }

    if(max == 0) return; 

    for(i = 0; i < size; i++) {
        buf[i] /= max;
    }

    
}

static void filter_buf(biquad_filter *filt, FLT *buf, long size) 
{
    long i;
    for(i = 0; i < size; i ++) {
        buf[i] = filter(filt, buf[i]);
    }
}

static void log_thresh(FLT *buf, long size, FLT thresh)
{
    long i;
    for(i = 0; i < size; i++) {
        buf[i] = 20 * log10(fabs(buf[i]));
        if(buf[i] < thresh) buf[i] = thresh;
    }
}

static void print_peaks(FLT *buf, long size, long skip) 
{
    long i;
    long counter = skip + 3;
    FLT prev = 0;
    for(i = 0; i < size; i += 2) {
        if((buf[i] - prev) > 0.8 && counter > skip)  {
            counter = 0;
            printf("%d ", i);
            fflush(stdout);
        }
        counter++; 
        prev = buf[i];
    }
}

int main(int argc, char *argv[])
{
    if(argc == 1) {
        printf("Usage: %s loop.wav\n", argv[0]);
        return 1;
    }
    char *str = argv[1];
    SF_INFO sfinfo;
    memset(&sfinfo, 0, sizeof(SF_INFO));
    SNDFILE *snd = sf_open(str, SFM_READ, &sfinfo);

    if(sfinfo.channels != 1) {
        printf("Error: File must be mono!");
        goto close;
    }

    biquad_coef c = {0.13111, -0.26221, 0.13111, 
        1.00000, 0.74779, 0.27221};
    biquad_filter filt;
    filt.coef = &c;
    filt.x1 = 0;
    filt.x2 = 0;
    filt.y1 = 0;
    filt.y2 = 0;
    FLT *buf = malloc(sizeof(FLT) * sfinfo.frames);    

    sf_read_float(snd, buf, sfinfo.frames);

    normalize(buf, sfinfo.frames);
    filter_buf(&filt, buf, sfinfo.frames);
    normalize(buf, sfinfo.frames);

    log_thresh(buf, sfinfo.frames, -15);

    print_peaks(buf, sfinfo.frames, sfinfo.samplerate * 0.1);  

    free(buf);
    close:
    sf_close(snd); 
    return 0;
}
