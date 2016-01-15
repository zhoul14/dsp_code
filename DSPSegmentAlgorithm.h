#ifndef DSP_SEG_ALGORITHM_H
#define DSP_SEG_ALGORITHM_H
#include "DSPStateTrace.h"
#include "DSPStateTraceBin.h"
#include "DspWordDict.h"
#include "DspGMMCalc.h"

struct SegmentResult {
	float lh;

	SegmentResult() {
		lh = 0;
	}
};

struct SegmentUtility {
	DSPWordDict* dict;
	DSPGMMCalc* bc;
	DSPGMMCalc* pbc;
};
class DspSegmentAlgorithm
{
	friend class DSPStateTrace;
	friend class DSPStateTraceBin;
	DSPStateTraceBin* m_binSet;

	int m_nBinsetLen;

	int m_nFDim;

	char* m_pMemPool;

	int* m_pMPidx;

	int m_nAnsNum;

	const int* ansList;

	DSPGMMCalc* m_pBC;

	DSPWordDict* m_pDict;

	int m_nHistory; 

	int time;

	float constructBinSet();
	
	DSPStateTrace* bestPreviousTrace(const DSPStateTraceBin* bin)const;

	float frameSegmentation();

	void resetBinSet(){
		*m_pMPidx = m_nHistory;
	}

	float prune(DSPStateTraceBin* bin);

public:
float segmentSpeech(int fnum, int fDim, int ansNum, const int* ansList, SegmentUtility util, DSPGMMCalc* bc, float& res, char* memPool, int& MPidx);

	DspSegmentAlgorithm();

	~DspSegmentAlgorithm();

};

#endif
