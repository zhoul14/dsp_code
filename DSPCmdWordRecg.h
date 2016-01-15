#ifndef DSP_DSP_CMD_WORD_RECG_H
#define DSP_DSP_CMD_WORD_RECG_H
#include "DSPStateTrace.h"
#include "DspCODEBOOKS.h"
#include "DSPStateTraceBin.h"
#include "DSPSegmentAlgorithm.h"
#include "DspWordDict.h"
#include "Convas.h"
#include "DspGMMCalc.h"
#include "DSPMFCC.h"



class DSPCmdWordRecg{
private:
	SegmentUtility u;

	DSPWordDict* m_pDict;

	DSPGMMCodebookSet* m_pSet;

	DSPGMMCodebookSet* m_pPreSet;

	DspSegmentAlgorithm* sa;
	
	CMFCC * cmfcc;

	bool* allCmdMask;

	char* m_pMempool;

	char* m_pDaram;

	int* m_pMPidx;

	int m_nCmdLen;

	int m_nListCmdNum[MAX_CMD_NUM];

	float resList[MAX_CMD_NUM];

public:
	float cmdRecg(short* sample, int sampleNum);

	void quiksort(float a[],int low,int high);

	DSPCmdWordRecg(const char* cmdWordBuf, int cmd_len, const char* codebookBuf, const char* dictBuf, char* memPool, int& MPidx);
	float init(DSPWordDict* m_pDict,DSPGMMCodebookSet* m_pSet, DSPGMMCodebookSet* pSet, char* memPool, int* MPidx, char* daram, void* DMA_handle);
	DSPCmdWordRecg(){}
	
	~DSPCmdWordRecg(){}
};

extern "C" float Call_C_doCmdRecg(DSPCmdWordRecg* rec,short* sample, int sampleNum);
extern "C" float Call_C_InitCmdRecg(DSPCmdWordRecg* rec,DSPWordDict* m_pDict,DSPGMMCodebookSet* m_pSet,DSPGMMCodebookSet* pSet, char* memPool, int* MPidx, char* daram, void* DMA_handle);
extern "C"long Call_C_doCmdRecg2(DSPCmdWordRecg* rec,short* sample, int sampleNum);

#endif
