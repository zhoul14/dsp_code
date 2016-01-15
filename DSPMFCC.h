#ifndef DSP_MFCC_H
#define DSP_MFCC_H
#include "DSPCFFT.h"
#define DIM     45                  /* feature vector dimemsion */
#define D       14

#define	FRAME_STEP	160
#define	FRAME_LEN	320

#define	DEFAULT_SAMPLE_RATE		16000		// ������
#define	DEFAULT_FFT_LEN			512			// FFT����
#define	DEFAULT_FRAME_LEN		FRAME_LEN	// ������֡��
#define	DEFAULT_SUB_BAND_NUM	24			// �Ӵ�����
#define	DEFAULT_CEP_COEF_NUM	14			// ����ϵ������
class CMFCC;
bool get20dBEnergyGeometryAveragMfcc(short *sentData,float (*ftrVect)[DIM],long FrameNum, CMFCC& mfcc);

class CMFCC:public CFFTanalyser
{
private:
	float	fres;
	long	FFT_LEN,FrameLen, SubBandNum, CepstrumNum;
	float	*cosTab;			//cosine table for DCT transformation 
	float	*hamWin;			//hamming window coefficients
	float	*cepWin;			//���״���Ȩϵ��
	float	*MelBandBoundary;	//��¼ÿ���Ӵ�����ʼMEL�̶�
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
		float	sampleF,			//���ݵĲ����ʣ�Hz��
		char	*mempool,
		int		&mpidx,
		long	DefFFTLen=512,		//Ĭ�ϵ�FFT����Ϊ512��
		long	DefFrameLen=320,	//Ĭ�ϵ�֡��Ϊ320��
		long	DefSubBandNum=24,	//Ĭ�ϵ��Ӵ��ĸ���Ϊ24
		long	DefCepstrumNum=14	//Ĭ�ϵĵ���ϵ������Ϊ14
		);
	~CMFCC();
	float	Mel(int k);
	float	DoMFCC(short *inData, float *outVect);	//���ص�ǰ֡������

};	
#endif
