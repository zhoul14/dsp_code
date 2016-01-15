#include "DSPCmdWordRecg.h"
#define MINUS_INF -9999999
void DSPCmdWordRecg::quiksort(float a[],int low,int high)
{
	int i = low;
	int j = high;  
	float temp = a[i]; 

	if( low < high)
	{          
		while(i < j) 
		{
			while((a[j] >= temp) && (i < j))
			{ 
				j--; 
			}
			a[i] = a[j];
			while((a[i] <= temp) && (i < j))
			{
				i++; 
			}  
			a[j]= a[i];
		}
		a[i] = temp;
		quiksort(a,low,i-1);
		quiksort(a,j+1,high);
	}
	else
	{
		return;
	}
}

DSPCmdWordRecg::DSPCmdWordRecg(const char* cmdWordBuf, int cmd_len, const char* codebookBuf, const char* dictBuf, char* memPool, int& MPidx)
{
	m_nCmdLen = 0;
	m_pMempool = memPool;
	m_pMPidx = &MPidx;

	m_pDict = (DSPWordDict*)(m_pMempool + MPidx);
	MPidx += sizeof(DSPWordDict);

	m_pDict->getWordDictFromBuf(dictBuf);
	m_pDict->getCmdSetFromBuf(cmdWordBuf, cmd_len);

	DSPGMMCodebookSet* m_pSet = (DSPGMMCodebookSet*)(m_pMempool + MPidx);
	*m_pMPidx += sizeof(DSPGMMCodebookSet);
	m_pSet->init(codebookBuf);

	allCmdMask = (bool*)m_pMempool + MPidx;
	MPidx += ALL_CBNUM * sizeof(bool);
	for (int i = 0; i < ALL_CBNUM; i++) {
		allCmdMask[i] = 0;
	}
	for (int i = 0; i < MAX_CMD_NUM; i++)
	{
		if (m_pDict->CmdSet[i][0] == -2)
		{
			m_nCmdLen = i;
			break;
		}
		int j = 0;
		int ansList[MAX_CMD_WORD_NUM];
		while (m_pDict->CmdSet[i][j] != -2)
		{
			ansList[j] = m_pDict->CmdSet[i][j];
			j++;
		}
		m_nListCmdNum[i] = j;
		m_pDict->getUsedStateIdInAns(allCmdMask, ansList, j);
	}
	if(!m_nCmdLen)m_nCmdLen = MAX_CMD_NUM;

	DSPGMMCalc*	gbc = (DSPGMMCalc*)(m_pMempool + MPidx);
	*m_pMPidx += sizeof(DSPGMMCalc);
	gbc->init(m_pSet,m_pMempool,*m_pMPidx);
	gbc->setMask(allCmdMask);

	u.bc = gbc;
	u.dict = m_pDict;	
}

float DSPCmdWordRecg::init(DSPWordDict* Dict,DSPGMMCodebookSet* Set,DSPGMMCodebookSet* pSet, char* memPool, int* MPidx, char* daram, void* DMA_handle)
{
	m_pDict = Dict;
	m_pSet = Set;
	m_pPreSet = pSet;
	m_pMPidx = MPidx;
	m_pMempool= memPool;
	m_pDaram = daram;
	if(((unsigned int)(m_pMempool + *m_pMPidx))%4)
		*m_pMPidx += 4 - ((unsigned int)(m_pMempool + *m_pMPidx))%4;
	allCmdMask = (bool*)(m_pMempool + *m_pMPidx);
	*MPidx += ALL_CBNUM * sizeof(bool) + 2;

	for (int i = 0; i < ALL_CBNUM; i++) {
		allCmdMask[i] = false;
	}
	for (int i = 0; i < MAX_CMD_NUM; i++)
	{
		if (m_pDict->CmdSet[i][0] == -2)
		{
			m_nCmdLen = i;
			break;
		}
		int j = 0;
		int ansList[MAX_CMD_WORD_NUM];
		while (m_pDict->CmdSet[i][j] != -2)
		{
			ansList[j] = m_pDict->CmdSet[i][j];
			j++;
		}
		m_nListCmdNum[i] = j;
		m_pDict->getUsedStateIdInAns(allCmdMask, ansList, j);
	}

	if(!m_nCmdLen)m_nCmdLen = MAX_CMD_NUM;

	if(((unsigned int)(m_pMempool + *m_pMPidx))%4)
		*m_pMPidx += 4 - ((unsigned int)(m_pMempool + *m_pMPidx))%4;

	sa = (DspSegmentAlgorithm*)(m_pMempool + *MPidx);
	*m_pMPidx += (sizeof(DspSegmentAlgorithm)+ 4 - sizeof(DspSegmentAlgorithm)%4);


	cmfcc = (CMFCC*)(m_pMempool + *m_pMPidx);
	*m_pMPidx += (sizeof(CMFCC)+ 4 - sizeof(CMFCC)%4);
	
/*
	cmfcc->init(DEFAULT_SAMPLE_RATE,
		m_pMempool,
		*m_pMPidx,
		DEFAULT_FFT_LEN,
		DEFAULT_FRAME_LEN,
		DEFAULT_SUB_BAND_NUM,
		DEFAULT_CEP_COEF_NUM);*/


	/*if(((unsigned int)(m_pMempool + *m_pMPidx))%sizeof(DSPGMMCalc))
		*m_pMPidx += sizeof(DSPGMMCalc) - ((unsigned int)(m_pMempool + *m_pMPidx))%sizeof(DSPGMMCalc);*/
//
	DSPGMMCalc*	gbc = (DSPGMMCalc*)(m_pMempool + *MPidx);
	*m_pMPidx += 4 - *MPidx%4 +(sizeof(DSPGMMCalc) + 4 - sizeof(DSPGMMCalc)%4);
	gbc->init(m_pSet,m_pMempool,*m_pMPidx);
	u.bc = gbc;
	u.dict = m_pDict;	

	gbc->m_handle = (IDMA3_Handle)DMA_handle;
	//gbc->setMask(allCmdMask);

	gbc = (DSPGMMCalc*)(m_pMempool + *MPidx);
	*m_pMPidx +=4 - *MPidx%4 + (sizeof(DSPGMMCalc) + 4 - sizeof(DSPGMMCalc)%4);
	gbc->init(m_pPreSet,m_pMempool,*m_pMPidx);
	u.pbc = gbc;

	gbc->m_handle = (IDMA3_Handle)DMA_handle;
	return gbc->setMask(allCmdMask);
	 
}

float DSPCmdWordRecg::cmdRecg(short* samples, int sampleNum)
{
	if(((unsigned int)(m_pMempool + *m_pMPidx))%4)
		*m_pMPidx += 4 - ((unsigned int)(m_pMempool + *m_pMPidx))%4;

	int historyIdx = *m_pMPidx;
	int frameNum = (sampleNum - FRAME_LEN + FRAME_STEP) / FRAME_STEP;
	//auto featureBuf = (float(*)[DIM])malloc(frameNum * DIM * sizeof(float));
	//if (featureBuf == NULL ) {
	//	printf("cannot malloc memory for FeatureBuf\n");
	//	exit(-1);
	//}
	float(*featureBuf)[DIM] = (float(*)[DIM])(m_pMempool + *m_pMPidx);
	*m_pMPidx += frameNum * DIM * sizeof(float);

	
	/*CMFCC *cmfcc = (CMFCC*)(m_pMempool + *m_pMPidx);
	*m_pMPidx += (sizeof(CMFCC)/4+1)*4;*/
	

	cmfcc->init(DEFAULT_SAMPLE_RATE,
		m_pMempool,
		*m_pMPidx,
		DEFAULT_FFT_LEN,
		DEFAULT_FRAME_LEN,
		DEFAULT_SUB_BAND_NUM,
		DEFAULT_CEP_COEF_NUM);


	get20dBEnergyGeometryAveragMfcc(samples, featureBuf, frameNum, *cmfcc);	

	float* features = (float*)featureBuf;//featureBuf

	DSPGMMCalc* gbc = (DSPGMMCalc*)u.pbc;
	
	int nDMAidx = 0;
	float fres = gbc->prepareDMA(features, frameNum, m_pDaram, &nDMAidx);

	for (int i = 0; i < m_nCmdLen; i++) 
	{

		int ansNum = m_nListCmdNum[i];

		 sa->segmentSpeech(frameNum, DIM, ansNum, m_pDict->CmdSet[i], u, gbc, resList[i], m_pMempool, *m_pMPidx);		
		
	}
	
	float resLh = MINUS_INF;
	int idx = -1;

/*	if (m_nCmdLen > 0)
	{
		for (int i = 0; i < m_nCmdLen; i++)
		{

			if (resLh <= resList[i])
			{
				idx = i;
				resLh = resList[i];
			}
		}
	}
	*m_pMPidx = historyIdx;
	return idx;
*/


	float *sortedList  = (float *)(m_pMempool + *m_pMPidx);
	*m_pMPidx += 4 * m_nCmdLen;
	int* minList = (int *)(m_pMempool + *m_pMPidx);
	*m_pMPidx += sizeof(int) * m_nCmdLen;

	
	memcpy(sortedList, resList, sizeof(float) * m_nCmdLen);
	for(int i =0;i< m_nCmdLen;i++)
	{
		minList[i] = 0;
	}
	quiksort(sortedList, 0, m_nCmdLen - 1);
		

	for (int i = m_nCmdLen - 20 ; i < m_nCmdLen; i++)
	{
		int j = 0;
		while (j != m_nCmdLen)
		{
			if (sortedList[i] == resList[j])
			{
				minList[j] = 1;
			}
			j++;
		}
	}

	for (int i = 0; i < ALL_CBNUM; i++) {
		allCmdMask[i] = false;
	}

	for (int i = 0; i < m_nCmdLen; i++)
	{
		if(!minList[i])
			continue;

		m_pDict->getUsedStateIdInAns(allCmdMask, m_pDict->CmdSet[i], m_nListCmdNum[i]);
	}
	gbc = (DSPGMMCalc*)u.bc;
	nDMAidx = 0;
	gbc->setMask(allCmdMask);
	//*m_pMPidx = historyIdx;


	gbc->prepareDMA(features, frameNum, m_pDaram, &nDMAidx);

	for (int i = 0; i < m_nCmdLen; i++) 
	{
		if(!minList[i])
			continue;
		int ansNum = m_nListCmdNum[i];

 		sa->segmentSpeech(frameNum, DIM, ansNum, m_pDict->CmdSet[i], u, gbc, resList[i], m_pMempool, *m_pMPidx);		
	}


	
	if (m_nCmdLen > 0)
	{
		for (int i = 0; i < m_nCmdLen; i++)
		{
			if(!minList[i])
				continue;
			if (resLh <= resList[i])
			{
				idx = i;
				resLh = resList[i];
			}
		}
	}

	*m_pMPidx = historyIdx;
	
	return idx;
}
extern "C" float Call_C_doCmdRecg(DSPCmdWordRecg* rec,short* sample, int sampleNum)
{
	//return 998;
	return rec->cmdRecg(sample, sampleNum);
}
extern "C"long Call_C_doCmdRecg2(DSPCmdWordRecg* rec,short* sample, int sampleNum)
{
	return (long)(rec->cmdRecg(sample, sampleNum));
}

extern "C" float Call_C_InitCmdRecg(DSPCmdWordRecg* rec,DSPWordDict* Dict,DSPGMMCodebookSet* Set,DSPGMMCodebookSet* pSet, char* memPool, int* MPidx, char* daram, void * DMA_handle)
{
	return rec->init(Dict, Set, pSet, memPool, MPidx, daram, DMA_handle);
}
