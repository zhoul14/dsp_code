#ifndef DSP_STATE_TRACE_BIN_H
#define DSP_STATE_TRACE_BIN_H

//#include <vector>
//#include <iostream>

#include "DSPStateTrace.h"
#include "DspWordDict.h"
#include "Convas.h"

//typedef DSPStateTrace** StateTraceList;
//typedef DSPStateTraceBin** BinList;
class DSPStateTrace;

class DSPStateTraceBin;
class DSPStateTraceBin {
	friend class DspSegmentAlgorithm;
public:

	void init(int cbidx, char* pMempool, int& Mpidx);

	void addPreviousBin(DSPStateTraceBin* b);

	void addStateTrace(DSPStateTrace* s);

private:
	int cbidx;

	DSPStateTrace* content[50];

	int contentSize;

	int previousSize;

	char* m_pMemPool;

	int* m_pMpIdx;

	DSPStateTraceBin* previous[5];

	DSPStateTraceBin(int cbidx, char* pMempool, int& Mpidx);

	DSPStateTraceBin(){

	}


	DSPStateTrace* getBestTrace();

	//void prune(StateTraceFactory* factory);

	int stateTraceNum();

};


#endif