#ifndef DSP_MFCC_H
#define DSP_MFCC_H
#include "DSPCFFT.h"
#define DIM     45                  /* feature vector dimemsion */
#define D       14

#define	FRAME_STEP	160
#define	FRAME_LEN	320

#define	DEFAULT_SAMPLE_RATE		16000		// 采样率
#define	DEFAULT_FFT_LEN			512			// FFT长度
#define	DEFAULT_FRAME_LEN		FRAME_LEN	// 语音的帧长
#define	DEFAULT_SUB_BAND_NUM	24			// 子带个数
#define	DEFAULT_CEP_COEF_NUM	14			// 倒谱系数个数
class CMFCC;
bool get20dBEnergyGeometryAveragMfcc(short *sentData,float (*ftrVect)[DIM],long FrameNum, CMFCC& mfcc);

class CMFCC:public CFFTanalyser
{
private:
	float	fres;
	long	FFT_LEN,FrameLen, SubBandNum, CepstrumNum;
	float	*cosTab;			//cosine table for DCT transformation 
	float	*hamWin;			//hamming window coefficients
	float	*cepWin;			//倒谱窗加权系数
	float	*MelBandBoundary;	//记录每个子带的起始MEL刻度
	float	*SubBandWeight;		//weighting coefficients for 
	//energy of each FFT frequence component
	long	*SubBandIndex;		//mapping of the FFT frequence component to sub-band No.
	float	*SubBandEnergy;		//for accumulating the corresponding subband energy
	float	*FFTFrame,*Cepstrum;
	char	*m_pMemPool;
	int		historyMPidx;
	int		*MPidx;
public:
	CMFCC(){

	}
	void init(
		float	sampleF,			//数据的采样率（Hz）
		char	*mempool,
		int		&mpidx,
		long	DefFFTLen=512,		//默认的FFT长度为512点
		long	DefFrameLen=320,	//默认的帧长为320点
		long	DefSubBandNum=24,	//默认的子带的个数为24
		long	DefCepstrumNum=14	//默认的倒谱系数个数为14
		);
	~CMFCC();
	float	Mel(int k);
	float	DoMFCC(short *inData, float *outVect);	//返回当前帧的能量

};	
#endif
