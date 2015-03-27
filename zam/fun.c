#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include "filters.h"

/* push a sample into a ringbuffer and return the sample falling out */
static inline float push_buffer(float insample, float * buffer,
            unsigned long buflen, unsigned long * pos) {

        float outsample;

        outsample = buffer[*pos];
        buffer[(*pos)++] = insample;

        if (*pos >= buflen)
                *pos = 0;

        return outsample;
}

/* read a value from a ringbuffer.
 * n == 0 returns the oldest sample from the buffer.
 * n == buflen-1 returns the sample written to the buffer
 *      at the last push_buffer call.
 * n must not exceed buflen-1, or your computer will explode.
 */
static inline float read_buffer(float * buffer, unsigned long buflen,
            unsigned long pos, unsigned long n) {

        while (n + pos >= buflen)
                n -= buflen;
        return buffer[n + pos];
}

/* overwrites a value in a ringbuffer, but pos stays the same.
 * n == 0 overwrites the oldest sample pushed in the buffer.
 * n == buflen-1 overwrites the sample written to the buffer
 *      at the last push_buffer call.
 * n must not exceed buflen-1, or your computer... you know.
 */
static inline void write_buffer(float insample, float * buffer, 
             unsigned long buflen, unsigned long pos, unsigned long n) {

        while (n + pos >= buflen)
                n -= buflen;
        buffer[n + pos] = insample;
}

#define db2lin(x) ((x) > -90.0f ? powf(10.0f, (x) * 0.05f) : 0.0f)
#define ABS(x)  (x)>0.0f?(x):-1.0f*(x)
#define LN_2_2 0.34657359f
#define LIMIT(v,l,u) ((v)<(l)?(l):((v)>(u)?(u):(v)))
#define PM_DEPTH 3681.0f
#define PM_BUFLEN 16027
#define PM_FREQ 6.0f
#define ROOT_12_2  1.059463094f
#define COS_TABLE_SIZE 1024
float cos_table[COS_TABLE_SIZE];

VoiceBox * instantiate_voicebox(unsigned long sample_rate) {

	VoiceBox * ptr;

	if ((ptr = malloc(sizeof(VoiceBox))) != NULL) {
		((VoiceBox *)ptr)->sample_rate = sample_rate;
		((VoiceBox *)ptr)->run_adding_gain = 1.0f;

		if ((((VoiceBox *)ptr)->ringbuffer = 
		     calloc(2 * PM_BUFLEN, sizeof(float))) == NULL)
			return NULL;
		((VoiceBox *)ptr)->buflen = 2 * PM_BUFLEN * sample_rate / 192000;
		((VoiceBox *)ptr)->pos = 0;

		return ptr;
	}
	return NULL;
}

void activate_voicebox(VoiceBox * Instance) {
	VoiceBox * ptr = (VoiceBox *)Instance;
	unsigned long i;
	for (i = 0; i < ptr->buflen; i++)
		ptr->ringbuffer[i] = 0.0f;
	ptr->phase = 0.0f;
	for (i = 0; i < COS_TABLE_SIZE; i++)
		cos_table[i] = cosf(i * 2.0f * M_PI / COS_TABLE_SIZE);

}

void select_robot(VoiceBox * Instance) {
	VoiceBox * ptr = (VoiceBox *)Instance;
	ptr->semitone = -3.f;
	ptr->drylevel = 1.f;
	ptr->wetlevel = 1.f;
}

void select_alien(VoiceBox * Instance) {
	VoiceBox * ptr = (VoiceBox *)Instance;
	ptr->semitone = -9.f;
	ptr->drylevel = 0.f;
	ptr->wetlevel = 1.f;
}

void select_girl(VoiceBox * Instance) {
	VoiceBox * ptr = (VoiceBox *)Instance;
	ptr->semitone = 3.f;
	ptr->drylevel = 0.f;
	ptr->wetlevel = 1.f;
}

void run_voicebox(VoiceBox * Instance, float *inf, float *outf, unsigned long SampleCount) {
  
	VoiceBox * ptr = (VoiceBox *)Instance;
	float * input = inf;
	float * output = outf;
	float drylevel = db2lin(LIMIT(ptr->drylevel,-90.0f,20.0f));
	float wetlevel = 0.333333f * db2lin(LIMIT(ptr->wetlevel,-90.0f,20.0f));
	float buflen = ptr->buflen / 2.0f;
	float semitone = LIMIT(ptr->semitone,-12.0f,12.0f);
	float rate; 
	float r;
	float depth;

	unsigned long sample_index;
	unsigned long sample_count = SampleCount;
	
	float in = 0.0f;
	float sign = 1.0f;
	float phase_0 = 0.0f;
	float phase_am_0 = 0.0f;
	float phase_1 = 0.0f;
	float phase_am_1 = 0.0f;
	float phase_2 = 0.0f;
	float phase_am_2 = 0.0f;
	float fpos_0 = 0.0f, fpos_1 = 0.0f, fpos_2 = 0.0f;
	float n_0 = 0.0f, n_1 = 0.0f, n_2 = 0.0f;
	float rem_0 = 0.0f, rem_1 = 0.0f, rem_2 = 0.0f;
	float sa_0, sb_0, sa_1, sb_1, sa_2, sb_2;

	if (semitone == 0.0f)
		rate = LIMIT(ptr->rate,-50.0f,100.0f);
	else
		rate = 100.0f * (powf(ROOT_12_2,semitone) - 1.0f);
	
	r = -1.0f * ABS(rate);
	depth = buflen * LIMIT(ABS(r) / 100.0f, 0.0f, 1.0f);
	
	if (rate > 0.0f)
		sign = -1.0f;

	for (sample_index = 0; sample_index < sample_count; sample_index++) {

		in = *(input++);

		phase_0 = COS_TABLE_SIZE * PM_FREQ * sample_index / ptr->sample_rate + ptr->phase;
		while (phase_0 >= COS_TABLE_SIZE)
		        phase_0 -= COS_TABLE_SIZE;
		phase_am_0 = phase_0 + COS_TABLE_SIZE/2;
		while (phase_am_0 >= COS_TABLE_SIZE)
			phase_am_0 -= COS_TABLE_SIZE;

		phase_1 = phase_0 + COS_TABLE_SIZE/3.0f;
		while (phase_1 >= COS_TABLE_SIZE)
		        phase_1 -= COS_TABLE_SIZE;
		phase_am_1 = phase_1 + COS_TABLE_SIZE/2;
		while (phase_am_1 >= COS_TABLE_SIZE)
			phase_am_1 -= COS_TABLE_SIZE;

		phase_2 = phase_0 + 2.0f*COS_TABLE_SIZE/3.0f;
		while (phase_2 >= COS_TABLE_SIZE)
		        phase_2 -= COS_TABLE_SIZE;
		phase_am_2 = phase_2 + COS_TABLE_SIZE/2;
		while (phase_am_2 >= COS_TABLE_SIZE)
			phase_am_2 -= COS_TABLE_SIZE;

		push_buffer(in, ptr->ringbuffer, ptr->buflen, &(ptr->pos));

		fpos_0 = depth * (1.0f - sign * (2.0f * phase_0 / COS_TABLE_SIZE - 1.0f));
		n_0 = floorf(fpos_0);
		rem_0 = fpos_0 - n_0;

		fpos_1 = depth * (1.0f - sign * (2.0f * phase_1 / COS_TABLE_SIZE - 1.0f));
		n_1 = floorf(fpos_1);
		rem_1 = fpos_1 - n_1;

		fpos_2 = depth * (1.0f - sign * (2.0f * phase_2 / COS_TABLE_SIZE - 1.0f));
		n_2 = floorf(fpos_2);
		rem_2 = fpos_2 - n_2;

		sa_0 = read_buffer(ptr->ringbuffer, ptr->buflen, ptr->pos, (unsigned long) n_0);
		sb_0 = read_buffer(ptr->ringbuffer, ptr->buflen, ptr->pos, (unsigned long) n_0 + 1);

		sa_1 = read_buffer(ptr->ringbuffer, ptr->buflen, ptr->pos, (unsigned long) n_1);
		sb_1 = read_buffer(ptr->ringbuffer, ptr->buflen, ptr->pos, (unsigned long) n_1 + 1);

		sa_2 = read_buffer(ptr->ringbuffer, ptr->buflen, ptr->pos, (unsigned long) n_2);
		sb_2 = read_buffer(ptr->ringbuffer, ptr->buflen, ptr->pos, (unsigned long) n_2 + 1);

		*(output++) = 
			wetlevel *
			((1.0f + cos_table[(unsigned long) phase_am_0]) *
			 ((1 - rem_0) * sa_0 + rem_0 * sb_0) +
			 (1.0f + cos_table[(unsigned long) phase_am_1]) *
			 ((1 - rem_1) * sa_1 + rem_1 * sb_1) +
			 (1.0f + cos_table[(unsigned long) phase_am_2]) *
			 ((1 - rem_2) * sa_2 + rem_2 * sb_2)) +
			drylevel *
			read_buffer(ptr->ringbuffer, ptr->buflen, ptr->pos, (unsigned long) depth);

	}

	ptr->phase += COS_TABLE_SIZE * PM_FREQ * sample_index / ptr->sample_rate;
	while (ptr->phase >= COS_TABLE_SIZE)
		ptr->phase -= COS_TABLE_SIZE;
}

void cleanup_voicebox(VoiceBox * Instance) {

  	VoiceBox * ptr = (VoiceBox *)Instance;
	free(ptr->ringbuffer);
	free(Instance);
}
