#include <math.h>
//#include <stdio.h>
#include "filters.h"

static inline float from_dB(float gdb) {
    return (exp(gdb/20.f*log(10.f)));
}

static inline float to_dB(float g) {
    return (20.f*log10(g));
}

static void pushsample(Gate *gate, float sample, float fs) {
    int gatemax = (fs > 48000.f) ? MAX_GATE : (int) 8 * fs / 100;
    gate->pos++;
    if (gate->pos >= gatemax)
        gate->pos = 0;
    gate->samples[gate->pos] = sample;
}

static float averageabs(Gate *gate, float fs) {
    int i;
    int gatemax = (fs > 48000.f) ? MAX_GATE : (int) 8 * fs / 100;
    float average = 0.f;

    for (i = 0; i < gatemax; i++) {
        average += fabsf(gate->samples[i]);
    }
    average /= (float) gatemax;
    return average;
}

void run_gate(Gate *gate, float *playsignal, float *micsignal, float *outsignal, int frames, float fs) {
    float absample;
    float attack;
    float release;
    int i;
    float gain;
    gain = gate->gain;

    attack = 1.f / (0.05 * fs);
    release = 1.f / (0.6 * fs);

    for(i = 0; i < frames; i++) {
        gain = sanitize_denormal(gain);
        pushsample(gate, playsignal[i], fs);
        absample = averageabs(gate, fs);
        if (absample > from_dB(-45.0)) {
            gain -= attack;
            if (gain < 0.f)
                gain = 0.f;
        } else {
            gain += release;
            if (gain > 1.f)
                gain = 1.f;
        }
        outsignal[i] = gain*micsignal[i];
        gate->gain = gain;
    }
//    printf("level(dB)=%f gain=%f playsignal(dB)=%f\n", to_dB(absample), gain, to_dB(playsignal[i-1]));
}
