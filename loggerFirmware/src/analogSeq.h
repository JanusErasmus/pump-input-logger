#ifndef ANALOGSEQ_H_
#define ANALOGSEQ_H_
#include <cyg/hal/hal_arch.h>
#include <time.h>

#define SEQ_BUFFER_SIZE 8

class analogSeq
{

	cyg_uint8 mIndex;
	cyg_uint8 mSequence;

public:

	analogSeq();
	static float nan;
	static bool isNaN(float num);

	struct seqSample
	{
		cyg_uint8 sequence;
		float value;
		time_t time;

		seqSample();
		seqSample(int seq, float val, time_t t){ sequence = seq; value = val; time = t;};

	}__attribute__((__packed__));

	seqSample mBuff[SEQ_BUFFER_SIZE];

	void set(float val);

	seqSample get();
	seqSample get(cyg_uint8 seq);

	seqSample* getBuff();

};

#endif /* ANALOGSEQ_H_ */
