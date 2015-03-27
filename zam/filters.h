#include <inttypes.h>
#include <math.h>

typedef struct {
	double x[3];
	double y[3];
	double a[3];
	double b[3];
} FilterStateZam;

void init_highpass_filter_zam(FilterStateZam *hpf, float fc, float fs);
void init_lowpass_filter_zam(FilterStateZam *lpf, float fc, float fs);
int run_filter_zam(FilterStateZam* fil, float* data, int length);
int run_saturator_zam(float *data, int length);

typedef struct {
	float rate;
	float semitone;
	float drylevel;
	float wetlevel;
	float latency;
	float * input;
	float * output;

	float * ringbuffer;
	unsigned long buflen;
	unsigned long pos;
	float phase;

	unsigned long sample_rate;
	float run_adding_gain;
} VoiceBox;

VoiceBox * instantiate_voicebox(unsigned long sample_rate);
void activate_voicebox(VoiceBox * Instance);
void select_robot(VoiceBox * Instance);
void select_alien(VoiceBox * Instance);
void select_girl(VoiceBox * Instance);
void run_voicebox(VoiceBox * Instance, float *in, float *out, unsigned long SampleCount);
void cleanup_voicebox(VoiceBox * Instance);
