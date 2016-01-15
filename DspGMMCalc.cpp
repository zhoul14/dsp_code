#include "DspGMMCalc.h"


float DSPGMMCalc::setMask(bool* mask) 
{
	memcpy(this->m_pMask, mask, ALL_CBNUM);

	int mixNum = codebooks->MixNum;
	int fDim = 45;
	int cbType = codebooks->cbType;
	int codebookNum = ALL_CBNUM;


	*m_nPtIdx += 4 - (*m_nPtIdx)%4;

	m_pMaskIdx = (int*) (m_pMempool + *m_nPtIdx);
	*m_nPtIdx += sizeof(int) * codebookNum;
	for(int i = 0; i < codebookNum; i++)
	{
		m_pMaskIdx[i] = -1;
	}


	m_maskedCodebookNum = 0;
	for (int i = 0; i < codebookNum; i++) {
		if (m_pMask[i]) {
			m_pMaskIdx[m_maskedCodebookNum] = i;
			m_maskedCodebookNum++;
		}
	}
	

	m_pCst = (float*) (m_pMempool + *m_nPtIdx);
	*m_nPtIdx += sizeof(float) * mixNum * m_maskedCodebookNum;

	float c0 = (float)(fDim / 2.0 * log(2 * (float)PI));
	for (int i = 0; i < m_maskedCodebookNum; i++) {
		for (int j = 0; j < mixNum; j++) {
			int p = (cbType == DSPGMMCodebookSet::CB_TYPE_FULL_RANK_SHARE )? m_pMaskIdx[i] : (m_pMaskIdx[i] * mixNum + j);

			m_pCst[i * mixNum + j] = c0 + 0.5 * codebooks->LogDetSigma[p];
		}
	}

	m_pAlpha = (float*) (m_pMempool + *m_nPtIdx);
	*m_nPtIdx += sizeof(float) * mixNum * m_maskedCodebookNum;

	for (int i = 0; i < m_maskedCodebookNum; i++)
		memcpy(m_pAlpha + i * mixNum, codebooks->Alpha + m_pMaskIdx[i] * mixNum, mixNum * sizeof(float));

	int L = mixNum * 45;
	m_pMu = (float*) (m_pMempool + *m_nPtIdx);
	*m_nPtIdx += sizeof(float) * L * m_maskedCodebookNum;
	for (int i = 0; i < m_maskedCodebookNum; i++)
		memcpy(m_pMu + i * L, codebooks->Mu + m_pMaskIdx[i] * L, L * sizeof(float));

	L = codebooks->SigmaL();
	m_pInvSigma = (float*) (m_pMempool + *m_nPtIdx);
	*m_nPtIdx += sizeof(float) * L * m_maskedCodebookNum;
	for (int i = 0; i < m_maskedCodebookNum; i++)
		memcpy(m_pInvSigma + i * L, codebooks->InvSigma + m_pMaskIdx[i] * L, L * sizeof(float));
	

	if (cbType == DSPGMMCodebookSet::CB_TYPE_FULL_RANK) {

		m_pInvSigmaMu = (float*) (m_pMempool + *m_nPtIdx);
		*m_nPtIdx += sizeof(float) * fDim * m_maskedCodebookNum * mixNum;

		m_cp_f = (CCalcGaussLh *)(m_pMempool + *m_nPtIdx);
		m_cp_f->initFirst((mixNum*m_maskedCodebookNum), fDim, m_pInvSigma, m_pInvSigmaMu, m_pMu, m_pCst, m_pAlpha, mixNum, m_pMempool, *m_nPtIdx);

	} else if (cbType == DSPGMMCodebookSet::CB_TYPE_DIAG) {
	
		this->m_pInvSigmaMu = NULL;
	}
	

	*m_nPtIdx += 4 - (*m_nPtIdx)%4;
	return mergeCBmem();
	
	float fRes = (float)m_maskedCodebookNum;
	//memcpy(&fRes, myTestMem + m_maskedCodebookNum * calcCodeBookSize()-4,4);
	return fRes;
}
float DSPGMMCalc::mergeCBmem()
{
	myTestMem = m_pMempool + *m_nPtIdx;
	*m_nPtIdx +=  m_maskedCodebookNum * calcCodeBookSize();//
	int idx = 0;
	float* ft = (float*) myTestMem;
	for(int i = 0; i < m_maskedCodebookNum; i++)
	{
		for(int j = 0; j < codebooks->MixNum; j++)
		{
			ft[idx++] = m_pAlpha[i * codebooks->MixNum + j];
		}
		for(int j = 0; j < codebooks->MixNum; j++)
		{
			ft[idx++] = m_pCst[i * codebooks->MixNum + j];
		}
		for(int j = 0; j < codebooks->MixNum; j++)
		{
			for(int k = 0; k < 45; k++)
			{
				ft[idx++] = m_pMu[i * codebooks->MixNum * 45 + j * 45 + k];
			}
		}
		for(int j = 0; j < codebooks->MixNum; j++)
		{
			for(int k = 0; k < codebooks->SigmaL() / codebooks->MixNum; k++)
			{
				ft[idx++] = m_pInvSigma[i * codebooks->SigmaL() + j * codebooks->SigmaL() / codebooks->MixNum  + k];
			}
		}

		if (m_pInvSigmaMu)
		{
			for(int j = 0; j < codebooks->MixNum; j++)
			{
				for(int k = 0; k < 45; k++)
				{
					ft[idx++] = m_pInvSigmaMu[i * codebooks->MixNum * 45 + j * 45 + k];
				}
			}

		}		 
	}
	return ft[idx-1];
	/*
	myTestMem = m_pMempool + *m_nPtIdx;
	*m_nPtIdx +=  m_maskedCodebookNum * calcCodeBookSize();//
	int idx = 0;
	float* ft = (float*) myTestMem;
	for(int i = 0; i < m_maskedCodebookNum; i++)
	{
		for(int j = 0; j < codebooks->MixNum; j++)
		{
			ft[idx++] = m_pAlpha[i * codebooks->MixNum + j];
		}
		for(int j = 0; j < codebooks->MixNum; j++)
		{
			ft[idx++] = m_pCst[i * codebooks->MixNum + j];
		}
		for(int j = 0; j < codebooks->MixNum; j++)
		{
			for(int k = 0; k < 45; k++)
			{
				ft[idx++] = m_pMu[i * codebooks->MixNum * 45 + j * 45 + k];
			}
		}
		for(int j = 0; j < codebooks->MixNum; j++)
		{
			for(int k = 0; k < codebooks->SigmaL() / codebooks->MixNum; k++)
			{
				ft[idx++] = m_pInvSigma[i * codebooks->SigmaL() + j * codebooks->SigmaL() / codebooks->MixNum  + k];
			}
		}
		if (m_pInvSigmaMu)
		{
			for(int j = 0; j < codebooks->MixNum; j++)
			{
				for(int k = 0; k < 45; k++)
				{
					ft[idx++] = m_pInvSigmaMu[i * codebooks->MixNum * 45 + j * 45 + k];
				}
			}

		}		 
	}*/
	/*if(idx-1 != m_maskedCodebookNum * calcCodeBookSize())
		return 444444;
	int i =0,temp =0,y=0;
	for(i =0 ;i < m_maskedCodebookNum * calcCodeBookSize()/4;i++)
	{
		memcpy(&y,&myTestMem[i*4],4);
		temp = temp ^ y;
	}
	return (float)(temp/10000000.0);*/

}

void DSPGMMCalc::init(const DSPGMMCodebookSet* cb, char* memPool, int& ptIdx)
{
	this->codebooks = cb;
	this->m_pMempool = memPool;
	this->m_nPtIdx = &ptIdx;
	this->m_handle = NULL;
	this->m_cp_f = NULL;
	this->m_pMask = (bool*)(memPool + ptIdx);
	ptIdx += (ALL_CBNUM * sizeof(bool) + 2);
}


float DSPGMMCalc::prepare(float* features, int fnum)
{

	m_nLhBufLen = fnum * ALL_CBNUM;
	int cbType = codebooks->cbType;
	int mixNum = codebooks->MixNum;
	int fDim = 45;
	
	int flag;
	if(((unsigned int)(m_pMempool + *m_nPtIdx))%4)
		*m_nPtIdx += 4 - ((unsigned int)(m_pMempool + *m_nPtIdx))%4;


	m_pAlphaWeightedStateLh = (float*)(m_pMempool + *m_nPtIdx);
	*m_nPtIdx += sizeof(float) * m_nLhBufLen;
	m_pRes = (float*) (m_pMempool + *m_nPtIdx);
	*m_nPtIdx += sizeof(float) * fnum * m_maskedCodebookNum;

	if (cbType == DSPGMMCodebookSet::CB_TYPE_FULL_RANK) {

		CCalcGaussLh *cp = (CCalcGaussLh *)(m_pMempool + *m_nPtIdx);/* m_cp_f;*/
		*m_nPtIdx += sizeof(CCalcGaussLh);
		cp->init((mixNum*m_maskedCodebookNum), fDim, m_pInvSigma, m_pInvSigmaMu, m_pMu, m_pCst, m_pAlpha, mixNum, m_pMempool, *m_nPtIdx);
		cp->runWeightedCalc(features, fnum, m_pRes);

	} else if (cbType == DSPGMMCodebookSet::CB_TYPE_DIAG) {
		CCalcDiagGaussLh *cp = /*m_cp_d;*/(CCalcDiagGaussLh *)(m_pMempool + *m_nPtIdx);
		*m_nPtIdx += sizeof(CCalcDiagGaussLh);
		cp->init((mixNum*m_maskedCodebookNum), fDim, m_pInvSigma, m_pMu, m_pCst, m_pAlpha, mixNum, m_pMempool, *m_nPtIdx);
		cp->runWeightedCalc(features, fnum, m_pRes);		
	}

	for (int i = 0; i < fnum; i++) 
	{
		for (int j = 0; j < m_maskedCodebookNum; j++) 
		{
			m_pAlphaWeightedStateLh[i * ALL_CBNUM + m_pMaskIdx[j]] = m_pRes[i * m_maskedCodebookNum + j];
		}
	}	
	
	return 555;
}

float DSPGMMCalc::prepareDMA(float* features, int fnum, char* mem, int* idx)
{

	m_nLhBufLen = fnum * ALL_CBNUM;
	int cbType = codebooks->cbType;
	int mixNum = codebooks->MixNum;
	char* memList[2];
	char* memTemp;
	memList[0] = mem + *idx;
	memList[1] = mem + *idx + 60 *1024;
	*idx += 60 *1024 * 2;
  	int nHistory = *idx;
	int chunkSize = calcCodeBookSize();
	int PerCBNum = 60 * 1024 / chunkSize;
	int thisCBNum = PerCBNum;
	int nextCBNum = PerCBNum;


	if(((unsigned int)(m_pMempool + *m_nPtIdx))%4)
		*m_nPtIdx += 4 - ((unsigned int)(m_pMempool + *m_nPtIdx))%4;

	char* Params = (char*)(m_pMempool + *m_nPtIdx);
	*m_nPtIdx += sizeof(char) * 64;// default size

	m_pAlphaWeightedStateLh = (float*)(m_pMempool + *m_nPtIdx);
	*m_nPtIdx += sizeof(float) * m_nLhBufLen;
	m_pRes = (float*) (m_pMempool + *m_nPtIdx);
	*m_nPtIdx += sizeof(float) * fnum * m_maskedCodebookNum;
	
	float fRes;
	int nRes;

	ACPY3_activate(m_handle);
	nRes = DMA_setParams(m_handle, Params, myTestMem, memList[0], PerCBNum * chunkSize);
	ACPY3_start(m_handle);

	if(m_maskedCodebookNum/PerCBNum == 0)
		nextCBNum = m_maskedCodebookNum % PerCBNum;		
		
	ACPY3_wait(m_handle);

	//float fRes;
	//while(!ACPY3_complete(m_handle)){int i = 0; i++;}	
	//if(ACPY3_complete(m_handle))	
	//memcpy(&fRes, memList[0] + PerCBNum * chunkSize - 4, 4);
	//return PerCBNum;//*(float*)(myTestMem + chunkSize * PerCBNum - 4);//test dma
	//return *(float*)(memList[0] + chunkSize * PerCBNum - 4);//test dma


	for(int i = 0; i<= m_maskedCodebookNum/PerCBNum; i++)//=
	{
		thisCBNum = nextCBNum;
		if(i == m_maskedCodebookNum/PerCBNum - 1)
		{
			nextCBNum = m_maskedCodebookNum % PerCBNum;
		}
		
		if( i != m_maskedCodebookNum/PerCBNum)
		{
/*************************************DMA******************************************/
		DMA_setParams(m_handle, Params, myTestMem + i * chunkSize * PerCBNum + PerCBNum * chunkSize, memList[(i+1)%2], chunkSize * nextCBNum);
		ACPY3_start(m_handle);
		}
/**********************************Cacl likehood************************************/
		if (cbType == DSPGMMCodebookSet::CB_TYPE_FULL_RANK) 
		{
			CCalcGaussLh *cp = (CCalcGaussLh *)(mem + *idx);
			*idx += sizeof(CCalcGaussLh);
			cp->init((mixNum * thisCBNum), 45, NULL, NULL, NULL, NULL, NULL, mixNum, mem, *idx);
			cp->runWeightedCalcDMA(features, fnum, m_pRes + i * PerCBNum* fnum, memList[i%2], chunkSize);
			*idx = nHistory;	
		}
		else if (cbType == DSPGMMCodebookSet::CB_TYPE_DIAG)
		{
			CCalcDiagGaussLh *cp = (CCalcDiagGaussLh*)(mem + *idx);
			*idx += sizeof(CCalcDiagGaussLh);
			cp->init((mixNum * thisCBNum), 45, NULL, NULL, NULL, NULL, mixNum, mem, *idx);
			cp->runWeightedCalcDMA(features, fnum, m_pRes + i * PerCBNum * fnum,/*myTestMem + i * chunkSize * PerCBNum*/memList[i%2], chunkSize);
			*idx = nHistory;	
		}	

		/***************************************************calc over**************/
		if( i != m_maskedCodebookNum/PerCBNum)	
			DMA_wait(m_handle);
	}

	for (int i = 0; i < fnum; i++) {
		for (int j = 0; j < m_maskedCodebookNum; j++) {
			m_pAlphaWeightedStateLh[i * ALL_CBNUM + m_pMaskIdx[j]] = m_pRes[i + j * fnum];
		}
	}	
	ACPY3_deactivate(m_handle);

	

	return  m_pRes[-1 + m_maskedCodebookNum * fnum];
}


float DSPGMMCalc::testDma(float* features, int fnum, float* InvSigma, float* InvSigmaMu,float* mu, float* Cst, float* alpha, char* mem, int* idx)
{
	m_nHistory = *m_nPtIdx;
	m_nLhBufLen = fnum * ALL_CBNUM;
	int cbType = codebooks->cbType;
	int mixNum = codebooks->MixNum;
	int fDim = 45;
	

	if(((unsigned int)(m_pMempool + *m_nPtIdx))%4)
		*m_nPtIdx += 4 - ((unsigned int)(m_pMempool + *m_nPtIdx))%4;


	m_pAlphaWeightedStateLh = (float*)(m_pMempool + *m_nPtIdx);
	*m_nPtIdx += sizeof(float) * m_nLhBufLen;
	m_pRes = (float*) (m_pMempool + *m_nPtIdx);
	*m_nPtIdx += sizeof(float) * fnum * 30;

	if (cbType == DSPGMMCodebookSet::CB_TYPE_FULL_RANK) {

		CCalcGaussLh *cp = (CCalcGaussLh *)(m_pMempool + *m_nPtIdx);
		*m_nPtIdx += sizeof(CCalcGaussLh);
		cp->init((mixNum*30), fDim, InvSigma, InvSigmaMu, mu, Cst, alpha, mixNum, mem, *idx);
		cp->runWeightedCalc(features, fnum, m_pRes);


	} else if (cbType == DSPGMMCodebookSet::CB_TYPE_DIAG) {
		CCalcDiagGaussLh *cp = (CCalcDiagGaussLh*)(mem + *idx);
		*idx += sizeof(CCalcDiagGaussLh);
		cp->init((mixNum*30), fDim, InvSigma, mu, Cst, alpha, mixNum, mem, *idx);
		cp->runWeightedCalc(features, fnum, m_pRes);		

	}

	for (int i = 0; i < fnum; i++) {
		for (int j = 0; j < 30; j++) {
			m_pAlphaWeightedStateLh[i * ALL_CBNUM + m_pMaskIdx[j]] = m_pRes[i * 30 + j];
		}
	}	
	
	return  m_pAlphaWeightedStateLh[(fnum - 1) * ALL_CBNUM + m_pMaskIdx[0]];
}

float DSPGMMCalc::test(float* features, int fnum)
{
	m_nHistory = *m_nPtIdx;
	m_nLhBufLen = fnum * ALL_CBNUM;
	int cbType = codebooks->cbType;
	int mixNum = codebooks->MixNum;
	int fDim = 45;
	

	if(((unsigned int)(m_pMempool + *m_nPtIdx))%4)
		*m_nPtIdx += 4 - ((unsigned int)(m_pMempool + *m_nPtIdx))%4;


	m_pAlphaWeightedStateLh = (float*)(m_pMempool + *m_nPtIdx);
	*m_nPtIdx += sizeof(float) * m_nLhBufLen;
	m_pRes = (float*) (m_pMempool + *m_nPtIdx);
	*m_nPtIdx += sizeof(float) * fnum * 30;

	if (cbType == DSPGMMCodebookSet::CB_TYPE_FULL_RANK) {

		CCalcGaussLh *cp = (CCalcGaussLh *)(m_pMempool + *m_nPtIdx);
		*m_nPtIdx += sizeof(CCalcGaussLh);
		cp->init((mixNum*30), fDim, m_pInvSigma, m_pInvSigmaMu, m_pMu, m_pCst, m_pAlpha, mixNum, m_pMempool, *m_nPtIdx);
		cp->runWeightedCalc(features, fnum, m_pRes);

	} else if (cbType == DSPGMMCodebookSet::CB_TYPE_DIAG) {
		CCalcDiagGaussLh *cp = (CCalcDiagGaussLh*)(m_pMempool + *m_nPtIdx);
		*m_nPtIdx += sizeof(CCalcDiagGaussLh);
		cp->init((mixNum*30), fDim, m_pInvSigma, m_pMu, m_pCst, m_pAlpha, mixNum, m_pMempool, *m_nPtIdx);
		cp->runWeightedCalc(features, fnum, m_pRes);		
	}

	for (int i = 0; i < fnum; i++) {
		for (int j = 0; j < 30; j++) {
			m_pAlphaWeightedStateLh[i * ALL_CBNUM + m_pMaskIdx[j]] = m_pRes[i * 30 + j];
		}
	}	
	
	return  m_pAlphaWeightedStateLh[(fnum - 1) * ALL_CBNUM + m_pMaskIdx[0]];
}


extern "C" float Call_C_doTest(DSPGMMCalc* gbc,float* features, int fnum)
{
	int i =0;
	float res = 0.0;

	for(i = 0; i <100;i++)
	{
		res = gbc->test(features, fnum);
		*gbc->m_nPtIdx =gbc->m_nHistory;
	}
	return res;
}
extern "C" float Call_C_doTestDMA(DSPGMMCalc* gbc, float* features, int fnum,char* mem)
{
	
	int i =0,L=0,j = 0,mixNum;
	mixNum = gbc->codebooks->MixNum;
	float res = 0.0;
	float* InvSigma, *mu, *Cst, *alpha, *InvSigmaMu;
	alpha = (float*)(mem + i);
	i += sizeof(float) * 30 * mixNum;	
	memcpy(alpha,gbc->m_pAlpha,sizeof(float) * 30 * mixNum);
	
	mu = (float*)(mem + i);
	i += sizeof(float) * 30 * mixNum * 45;
	memcpy(mu,gbc->m_pMu,sizeof(float) * 30 * mixNum * 45);

	Cst = (float*)(mem + i);
	i += sizeof(float) * 30 * mixNum;
	memcpy(Cst,gbc->m_pCst,sizeof(float) * 30 * mixNum);
	
	
	L = gbc->codebooks->SigmaL();
	InvSigma = (float*)(mem + i);
	i += sizeof(float) * 30 *L;
	memcpy(InvSigma,gbc->m_pInvSigma,sizeof(float) * 30 * L);

	if(gbc->m_pInvSigmaMu != NULL)
	{
		InvSigmaMu = (float*)(mem + i);
		i += sizeof(float) * 30 *mixNum * 45;
		memcpy(InvSigmaMu,gbc->m_pInvSigmaMu,sizeof(float) * 30 * L);
	}
	L = i;
	for(j = 0; j <100;j++)
	{
		i = L;
		res = gbc->testDma(features, fnum, InvSigma, InvSigmaMu, mu, Cst, alpha, mem, &i);
		*gbc->m_nPtIdx =gbc->m_nHistory;
	}
	return res;
}

