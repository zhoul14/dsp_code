#include "DSPSegmentAlgorithm.h"
#include "Convas.h"

float DspSegmentAlgorithm::constructBinSet()
{
	m_nBinsetLen = 0;
	int noiseId = NOISE_ID;
	if(((unsigned int)(m_pMemPool + *m_pMPidx))%sizeof(DSPStateTraceBin))
		*m_pMPidx += sizeof(DSPStateTraceBin) - ((unsigned int)(m_pMemPool + *m_pMPidx))%sizeof(DSPStateTraceBin);
	DSPStateTraceBin* firstSil = (DSPStateTraceBin*)(m_pMemPool + *m_pMPidx);
	m_binSet = firstSil; 
	firstSil->init(noiseId, m_pMemPool, *m_pMPidx);
	*m_pMPidx += sizeof(DSPStateTraceBin);
	m_nBinsetLen++;//居首静音

	DSPStateTraceBin* lastSil = NULL;
	DSPStateTraceBin* lastF3 = NULL;
	DSPStateTraceBin* lastF3C = NULL;

	int cbInfo[STATE_NUM_IN_WORD];
	for (int i = 0; i < m_nAnsNum; i++)
	{
		int prevWord = i == 0 ? NO_PREVIOUS_WORD : ansList[i - 1];
		int nextWord = i == m_nAnsNum - 1 ? NO_NEXT_WORD : ansList[i + 1];
		int word = ansList[i];
		m_pDict->getWordCbId(word, prevWord, nextWord, cbInfo);
		
		DSPStateTraceBin* I0 = (DSPStateTraceBin*)(m_pMemPool + *m_pMPidx);
		I0->init(cbInfo[INITIAL0], m_pMemPool, *m_pMPidx);
		*m_pMPidx += sizeof(DSPStateTraceBin);
		m_nBinsetLen++;//

		if (i == 0) {
			I0->addPreviousBin(firstSil);
		} else {
			I0->addPreviousBin(lastSil);
			I0->addPreviousBin(lastF3);
		}

		DSPStateTraceBin* I0C = (DSPStateTraceBin*)(m_pMemPool + *m_pMPidx);
		I0C->init(cbInfo[INITIAL0_C], m_pMemPool, *m_pMPidx);
		*m_pMPidx += sizeof(DSPStateTraceBin);
		if (i != 0)
		{
			I0C->addPreviousBin(lastF3C);
		}
		m_nBinsetLen++;//

		DSPStateTraceBin* I1 = (DSPStateTraceBin*)(m_pMemPool + *m_pMPidx);
		I1->init(cbInfo[INITIAL1], m_pMemPool, *m_pMPidx);
		*m_pMPidx += sizeof(DSPStateTraceBin);
		I1->addPreviousBin(I0);
		I1->addPreviousBin(I0C);
		m_nBinsetLen++;//

		DSPStateTraceBin* F0 = (DSPStateTraceBin*)(m_pMemPool + *m_pMPidx);
		F0->init(cbInfo[FINAL0], m_pMemPool, *m_pMPidx);
		*m_pMPidx += sizeof(DSPStateTraceBin);
		F0->addPreviousBin(I1);
		m_nBinsetLen++;//

		DSPStateTraceBin* F1 = (DSPStateTraceBin*)(m_pMemPool + *m_pMPidx);
		F1->init(cbInfo[FINAL1], m_pMemPool, *m_pMPidx);
		*m_pMPidx += sizeof(DSPStateTraceBin);
		F1->addPreviousBin(F0);
		m_nBinsetLen++;//

		DSPStateTraceBin* F2 = (DSPStateTraceBin*)(m_pMemPool + *m_pMPidx);
		F2->init(cbInfo[FINAL2], m_pMemPool, *m_pMPidx);
		*m_pMPidx += sizeof(DSPStateTraceBin);
		F2->addPreviousBin(F1);
		m_nBinsetLen++;//

		DSPStateTraceBin* F2C = (DSPStateTraceBin*)(m_pMemPool + *m_pMPidx);
		F2C->init(cbInfo[FINAL2_C], m_pMemPool, *m_pMPidx);
		*m_pMPidx += sizeof(DSPStateTraceBin);
		F2C->addPreviousBin(F1);
		m_nBinsetLen++;//

		DSPStateTraceBin* F3 = (DSPStateTraceBin*)(m_pMemPool + *m_pMPidx);
		F3->init(cbInfo[FINAL3], m_pMemPool, *m_pMPidx);
		*m_pMPidx += sizeof(DSPStateTraceBin);
		F3->addPreviousBin(F2);
		m_nBinsetLen++;//

		DSPStateTraceBin* F3C = (DSPStateTraceBin*)(m_pMemPool + *m_pMPidx);
		F3C->init(cbInfo[FINAL3_C], m_pMemPool, *m_pMPidx);
		*m_pMPidx += sizeof(DSPStateTraceBin);
		F3C->addPreviousBin(F2C);
		m_nBinsetLen++;//

		DSPStateTraceBin* sil = (DSPStateTraceBin*)(m_pMemPool + *m_pMPidx);
		sil->init(noiseId, m_pMemPool, *m_pMPidx);
		*m_pMPidx += sizeof(DSPStateTraceBin);
		sil->addPreviousBin(F3);
		m_nBinsetLen++;//

		lastSil = sil;
		lastF3 = F3;
		lastF3C = F3C;
	}
	return *m_pMPidx;
}

DSPStateTrace* DspSegmentAlgorithm::bestPreviousTrace(const DSPStateTraceBin* bin) const
{
	if (bin->previousSize == 0) {
		return NULL;
	}

	DSPStateTrace* best = NULL;
	for (int i = 0; i < bin->previousSize; i++)
	{
		DSPStateTrace* t = bin->previous[i]->getBestTrace();
		if (t == NULL)
		{
			continue;
		}
		if (best == NULL)
		{
			best = t;
		}
		else
		{
			if (t->lh > best->lh)
			{
				best = t;
			}
		}
	}

	return best;
}


DspSegmentAlgorithm::DspSegmentAlgorithm(){

}

DspSegmentAlgorithm::~DspSegmentAlgorithm(){

}
float DspSegmentAlgorithm::prune(DSPStateTraceBin* bin) {
	DSPStateTrace** list = bin->content;
	if (bin->contentSize < 2)
		return 0.0;

	int bestIdx = 0;
	float bestLh = list[0]->lh;
	float x = bestLh;
	for (int i = 1; i < bin->contentSize; i++) {
		if (list[i]->lh > bestLh) {
			bestIdx = i;
			bestLh = list[i]->lh;
		}
	}
	for (int i = 0; i < bin->contentSize; i++) {
		if (i == bestIdx)
			continue;
		destoryST(list + i);
	}
	DSPStateTrace* bestToken = list[bestIdx];
	//list.clear();
	bin->contentSize = 1;
	bin->content[0] = bestToken;
	//list.push_back(bestToken);
	return 1;
}


float DspSegmentAlgorithm::frameSegmentation() {
	time++;

	//0时刻放入初值
	if (time == 0) {
		
		if((unsigned int)(m_pMemPool + *m_pMPidx)%sizeof(DSPStateTrace))
			*m_pMPidx += sizeof(DSPStateTrace) - (unsigned int)(m_pMemPool + *m_pMPidx)%sizeof(DSPStateTrace);

		DSPStateTrace* s0 = (DSPStateTrace* )(m_pMemPool + *m_pMPidx);
		*m_pMPidx += sizeof(DSPStateTrace);

		int cb[STATE_NUM_IN_WORD];
		m_pDict->getWordCbId(ansList[0], NO_PREVIOUS_WORD, NO_NEXT_WORD, cb);

		//种子放入第一字的I0状态
		int cbidx0 = cb[INITIAL0];
		float stateLh0 = m_pBC->getStateLh(cbidx0, time);

		stateLh0;	
		s0->set(time, cbidx0, stateLh0, 0, NULL, -1);
		(m_binSet + 1)->addStateTrace(s0);


		//种子放入句首的sil
		DSPStateTrace* s1 = (DSPStateTrace* )(m_pMemPool + *m_pMPidx);
		*m_pMPidx += sizeof(DSPStateTrace);

		int cbidx1 = NOISE_ID;
		float stateLh1 = m_pBC->getStateLh(cbidx1, time);
		s1->set(time, cbidx1, stateLh1, 0, NULL, -1);
		m_binSet->addStateTrace(s1);

		/*return 99;*/
		//test
	return 99;
	}

	//time != 0

	for (int i = m_nBinsetLen - 1; i >= 0; i--) {
		DSPStateTraceBin* b = m_binSet + i;
		
		int cbidx = b->cbidx;
		
		
		//在一句话的最后一字可能发生这样的情况，因为不存在下一字，无法协同发音，协同发音的状态ID的值为INVALID_STATE_ID
		if (cbidx == -1) 
			continue;

		float stateLh = m_pBC->getStateLh(cbidx, time);

		/*if(stateLh < -50000000 || stateLh > 0)
			return 88000 + cbidx;*/
		//处理状态内跳转
		for (int j = 0; j < b->contentSize; j++) {
			DSPStateTrace* st = b->content[j];
		/*if(st->lh < -5000000 || st->lh > 0)
			return 101;*/
			float newLh = st->lh + stateLh;
			st->lh = newLh;
		}

		//状态剪枝
		//b->prune(factory);
		/*if(stateLh < -50000000 || stateLh > 0)
			return 77000 + cbidx;*/
		//处理状态间跳转		
		DSPStateTrace* previousBest = bestPreviousTrace(b);
		if (previousBest == NULL)
			continue;

		/*if(stateLh < -50000000 || stateLh > 0)
			return 66000 + cbidx;*/

		if((unsigned int)(m_pMemPool + *m_pMPidx)%sizeof(DSPStateTrace))
			*m_pMPidx += sizeof(DSPStateTrace) - (unsigned int)(m_pMemPool + *m_pMPidx)%sizeof(DSPStateTrace);

		DSPStateTrace* copy = (DSPStateTrace* )(m_pMemPool + *m_pMPidx);
		*m_pMPidx += sizeof(DSPStateTrace);

		copy->set(previousBest->enterTime, previousBest->cbidx, previousBest->lh, 0, previousBest->prev, -1);
		if (previousBest->prev != NULL)
			previousBest->prev->refcnt++;
		
		/*if(copy->lh < -50000000 || copy->lh > 0)
			return 3303;
		if(stateLh < -50000000 || stateLh > 0)
			return 44000 + cbidx;*/
		float newLh = stateLh + previousBest->lh;
		DSPStateTrace* newTrace = (DSPStateTrace* )(m_pMemPool + *m_pMPidx);
		*m_pMPidx += sizeof(DSPStateTrace);
		newTrace->set(time, cbidx, newLh, 0, copy, -1);
		copy->refcnt++;
		b->addStateTrace(newTrace);

		/*if(newTrace->lh < -50000000 || newTrace->lh > 0)
			return cbidx;*/


		//状态剪枝
		prune(b);
		/*if(test == 5)
			return test;*/
	}
	return 99;
}

float DspSegmentAlgorithm::segmentSpeech(int fnum, int fDim, int ansNum, const int* ansList, SegmentUtility util, DSPGMMCalc* bc,float& res, char* memPool, int& MPidx)
{
	if (fnum < ansNum * HMM_STATE_DUR * 2) {
		return -1.0;
	}


	this->m_pBC = bc;
	this->m_nAnsNum = ansNum;
	this->ansList = ansList;
	m_pMemPool = memPool;
	m_pMPidx = &MPidx;
	m_nHistory = MPidx;
	this->m_nFDim = fDim;
	this->m_pDict = util.dict;

	time = -1;	float aa;
	/*if(m_pBC->getStateLh(1476,1)<-500000||m_pBC->getStateLh(1476,1)>0)
		return 430;*/
	constructBinSet();


	/*if(m_pBC->getStateLh(1476,1)<-500000||m_pBC->getStateLh(1476,1)>0)
		return 438;*/
	for (int i = 0; i < fnum; i++) {
		/*aa =*/ frameSegmentation();// test;
		/*if(aa != 99)
			return aa;		*/
	}


	int len = m_nBinsetLen;
	DSPStateTrace* lastSil = (m_binSet + len - 1)->getBestTrace();
	DSPStateTrace* lastF3 = (m_binSet + len - 3)->getBestTrace();

	if (lastSil == NULL) {
		printf("the sil bin of last word is empty");
		//exit(-1);
	}
	if (lastF3 == NULL) {
		printf("the f3 bin of last word is empty");
		//exit(-1);
	}
	DSPStateTrace* best = lastSil->lh > lastF3->lh ? lastSil : lastF3;
	res = best->lh;


	//test
	//int t[1000];
	//float tlh[1000];
	//int resLen = 0;
	//DSPStateTrace* i = best;
	//int rtime = fnum;
	//while (i != NULL) {
	//	rtime--;
	//	t[resLen] = i->cbidx;
	//	tlh[resLen] = i->lh;
	//	resLen++;
	//	if (rtime ==  i->enterTime) {
	//		i = i->prev;
	//	}
	//}

	resetBinSet();
	return res;
}
