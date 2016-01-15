#ifndef DSO_GMM_PROB_BATCH_CALC_H
#define DSO_GMM_PROB_BATCH_CALC_H

#include "CalCGaussLh.h"
#include "DspCODEBOOKS.h"
#include "Convas.h"
#include "DspWordDict.h"

/*#include <ti/xdais/idma3.h>
#include <ti/sdo/fc/acpy3/acpy3.h>*/

extern "C"{
	typedef struct IDMA3_Obj;
	typedef struct IDMA3_Obj *IDMA3_Handle;
	typedef struct ACPY3_Params ACPY3_Params;
	void ACPY3_activate(IDMA3_Handle handle);
	void ACPY3_deactivate(IDMA3_Handle handle);
	void ACPY3_init(void);
	bool ACPY3_complete(IDMA3_Handle handle);
	void ACPY3_wait(IDMA3_Handle handle);
	void ACPY3_start(IDMA3_Handle handle);
	void DMA_start(void* handle);
	void DMA_wait(void* handle);
	int DMA_complete(void* handle);
	int DMA_setParams(void* handle,void* Params, void* src, void* dst, long bufferSize);

}

 class DSPGMMCalc
 {
 //private:
 public:

	 //float alphaWeightedStateLh[ALL_CBNUM];
	 float* m_pAlphaWeightedStateLh;
	 bool* m_pMask;//[ALL_CBNUM];
	 int* m_pMaskIdx;//[ALL_CBNUM];
	 int m_nLhBufLen;
	 float* m_pCst;
	 char* m_pMempool;
	 float* m_pinvSigmaMuHost;
	 float* m_pAlpha;
	 float* m_pInvSigma;
	 float* m_pInvSigmaMu;
	 float* m_pMu;
	 float* m_pRes;
	 int* m_nPtIdx;
	 int  m_nHistory;
	 char* myTestMem;
	 CCalcGaussLh * m_cp_f;
	 CCalcDiagGaussLh* m_cp_d;
	 IDMA3_Handle m_handle;
 public:
	 const DSPGMMCodebookSet* codebooks;
 int m_maskedCodebookNum;
	 DSPGMMCalc(){

	 }
	 void init(const DSPGMMCodebookSet* cb, char* memPool, int& ptn);

	 ~DSPGMMCalc()
	 {
	 }
	 float testDma(float* features, int fnum, float* InvSigma, float* InvSigmaMu, float* mu, float* Cst, float* alpha,char* mem,int* idx);
	 float test(float* features, int fnum);
	 float mergeCBmem();
	 float setMask(bool* mask);
	 float prepare(float* features, int fnum);
	 float prepareDMA(float* features, int fnum, char* mem, int* idx);
	 inline float getStateLh(int cbidx, int frameidx){
		float p = m_pAlphaWeightedStateLh[frameidx * ALL_CBNUM + cbidx];
		 return p;
	 }
	 inline long calcCodeBookSize()
	{
		return 	(codebooks->SigmaL() + 45 * codebooks->MixNum + (codebooks->MixNum) * 2 + ((codebooks->cbType == DSPGMMCodebookSet::CB_TYPE_FULL_RANK)? (45 * codebooks->MixNum ): 0))*4;
	}
 };

extern "C" float Call_C_doTest(DSPGMMCalc* gbc,float* features, int fnum);
extern "C" float Call_C_doTestDMA(DSPGMMCalc* gbc, float* features, int fnum,char* mem);

#endif // !DSO_GMM_PROB_BATCH_CALC_H

