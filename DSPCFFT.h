#ifndef DSP_FFT_H
#define DSP_FFT_H

#include "math.h"




class CFFTanalyser {
private:
	float	*cosTable,*sinTable;
	long	FFT_LEN,ButterFlyDepth;
	char	*m_pMemPool;
	int *m_pMPidx;
public:
	void initFFT(char* mempool,int &pidx, long FFT_AnalysePointNum=512);
	CFFTanalyser(){

	}
	~CFFTanalyser(){

	}
	void SetupSinCosTable(void);
	void DoFFT(float *fr, float *fi, short flag );	
	void DoRealFFT(float *Buf);
	long GetFFTAnalyseLen(void);
};

#endif
