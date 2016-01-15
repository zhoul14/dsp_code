#ifndef DSP_STATE_TRACE_H
#define DSP_STATE_TRACE_H
#include "stdio.h"
class DSPStateTrace {
	friend class DspSegmentAlgorithm;
	friend class DSPStateTraceBin;
protected:
	int enterTime;

	int cbidx;

	float lh;

	DSPStateTrace* prev;

	int wordId;

	void set(int enterTime, int cbidx, float lh, int refcnt, DSPStateTrace* prev, int wordId);

public:
	int getEnterTime() const;

	int refcnt; 

	int getCodebookIndex() const;

	inline float getLikelihood() const {
		return this->lh;
	}

	DSPStateTrace* getPrevious() const;

	int getWordId() const;
	
	
};
float destoryST(DSPStateTrace**);

#endif
