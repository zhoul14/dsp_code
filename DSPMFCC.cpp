#include "DSPMFCC.h"
#include <math.h>
#include "stdio.h"
#define PREE        0.98f
#define MELFLOOR    1.0f

float CMFCC::Mel(int k)
{
	return (float)(1127 * log(1 + k*fres));
}

void CMFCC::init(float SampleF,char* mempool, int &mpidx,long DefFFTLen,long DefFrameLen,long DefSubBandNum, long DefCepstrumNum)
{
	long	M, SubBandNo, i, j;
	float	ms,pi_factor, mfnorm, a, melk, t;
	MPidx = &mpidx;
	m_pMemPool = mempool;
	historyMPidx = mpidx;

	FrameLen	= DefFrameLen;
	SubBandNum	= DefSubBandNum;
	CepstrumNum	= DefCepstrumNum;
	initFFT(m_pMemPool,mpidx,DefFFTLen);
	FFT_LEN = GetFFTAnalyseLen();
	//	std::cout <<"FFT_LEN = "<<FFT_LEN<<std::endl;

	if(((unsigned int)(m_pMemPool + mpidx))%4)
		mpidx += 4 - ((unsigned int)(m_pMemPool + mpidx))%4;

	cosTab = (float*)(m_pMemPool + mpidx);
	mpidx += sizeof(float) * (SubBandNum+1)*(SubBandNum+1);	//DCT�任ϵ��

	hamWin = (float*)(m_pMemPool + mpidx);
	mpidx += sizeof(float) * FrameLen;	//hamming ��ϵ��

	cepWin = (float*)(m_pMemPool + mpidx);
	mpidx += sizeof(float) * (SubBandNum+1);	

	MelBandBoundary = (float*)(m_pMemPool + mpidx);
	mpidx += sizeof(float) * (SubBandNum+2);

	SubBandWeight = (float*)(m_pMemPool + mpidx);
	mpidx += sizeof(float) * FFT_LEN/2;

	SubBandIndex = (long*)(m_pMemPool + mpidx);
	mpidx += sizeof(long) * FFT_LEN/2;


	SubBandEnergy = (float*)(m_pMemPool + mpidx); 
	mpidx += sizeof(float) * (SubBandNum+2);		//for accumulating the corresponding 

	FFTFrame = (float*)(m_pMemPool + mpidx); 
	mpidx += sizeof(float) * (FFT_LEN);		//for accumulating the corresponding 

	Cepstrum = (float*)(m_pMemPool + mpidx); 
	mpidx += sizeof(float) * (SubBandNum+1);		//for accumulating the corresponding 

	fres = SampleF/(FFT_LEN*700.0f);
	//caculating the mel scale sub-band boundary
	//�����Ӵ�0��ϵ����ֱ��������������MFCC�������㣬�����Ӵ�����Ҫ��һ��
	M	 = SubBandNum+1;	
	ms	 = Mel(FFT_LEN/2);		//����1/2����������Ӧ��MEL�̶�
	//Note that the sub-band 0 is not used for cepstrum caculating
	for ( SubBandNo = 0; SubBandNo <= M; SubBandNo++ )
	{	//����ÿ���Ӵ�����ʼMEL�̶�
		MelBandBoundary[SubBandNo] = ( (float)SubBandNo/(float)M )*ms;
	}

	//mapping the FFT frequence component into the corresponding sub-band
	for ( i=0,SubBandNo=1; i< FFT_LEN/2; i++)
	{
		melk = Mel(i);
		while( MelBandBoundary[SubBandNo] < melk ) SubBandNo++;
		SubBandIndex[i] = SubBandNo-1;
	}
	//caculating the weighting coefficients for each FFT frequence components	
	for(i=0; i< FFT_LEN/2; i++)
	{	//���Ӵ�����ʼMELƵ��Ϊ���ģ��������Ǵ���Ȩϵ��
		SubBandNo = SubBandIndex[i];
		SubBandWeight[i] = (MelBandBoundary[SubBandNo+1]-Mel(i))/(MelBandBoundary[SubBandNo+1]-MelBandBoundary[SubBandNo]);
	}

	pi_factor = (float)( asin(1.0)*2.0/(float)SubBandNum );
	mfnorm	  = (float)sqrt(2.0f/(float)SubBandNum);
	for( i=1; i<= CepstrumNum; i++ )
	{
		t = (float)i*pi_factor;
		for(j=1; j<=SubBandNum; j++)
			cosTab[i*(SubBandNum+1)+j] = (float)cos(t*(j-0.5f))*mfnorm;
	}

	a =(float)( asin(1.0)*4/(FrameLen-1) );
	for(i=0;i<FrameLen;i++)
		hamWin[i] = 0.54f - 0.46f * (float)cos(a*i);

	for(i=1;i<=CepstrumNum;i++)
		cepWin[i-1] = (float)i * (float)exp(-(float)i*2.0/(float)CepstrumNum);
}

float CMFCC::DoMFCC(short *inData, float *outVect)
{
	int		i, SubBandNo;
	float	FrameEnergy,ek,temp1,temp2;
	float	*pCosTable;

	FrameEnergy = 0.0f;
	for(i=0;i<FrameLen;i++)
	{
		FFTFrame[i] = (float)inData[i];
		FrameEnergy += (FFTFrame[i]*FFTFrame[i]);
	}
	for( i= FrameLen-1; i > 0; i-- )	//���������źŲ�ֲ����мӴ�
	{
		FFTFrame[i] -= FFTFrame[i-1]*PREE;
		FFTFrame[i] *= hamWin[i];
	}
	FFTFrame[0] *= (1.0f-PREE);
	FFTFrame[0] *= hamWin[0];
	for(i=FrameLen;i<FFT_LEN;i++) FFTFrame[i]=0.0f;

	// ����FFT�任
	DoRealFFT(FFTFrame);
	for( SubBandNo = 1 ; SubBandNo <= SubBandNum; SubBandNo++ )
	{
		SubBandEnergy[SubBandNo]=0.0f;
	}
	for( i = 1; i < FFT_LEN/2; i++ )	//the component at zero frequence is discarded
	{
		temp1 = FFTFrame[2*i]; temp2 = FFTFrame[2*i+1];
		ek = temp1*temp1+temp2*temp2;	// �����k��Ƶ�ʴ�������
		SubBandNo = SubBandIndex[i];	// �����õ�k��Ƶ���������Ӵ���
		temp1 = SubBandWeight[i]*ek;
		if(SubBandNo>0) SubBandEnergy[SubBandNo] += temp1;
		if(SubBandNo<SubBandNum) SubBandEnergy[SubBandNo+1] += ek-temp1;
	}
	// ����ÿ���˲�����������Ķ���ֵ
	for( SubBandNo=1; SubBandNo<= SubBandNum; SubBandNo++ )
	{
		temp1 = SubBandEnergy[SubBandNo];
		if(temp1<MELFLOOR)	temp1 = MELFLOOR;		// clipping 
		SubBandEnergy[SubBandNo] = (float)(0.5*log(temp1));    
	}

	pCosTable = cosTab + SubBandNum + 1;
	for(i=1;i<=CepstrumNum;i++)
	{
		Cepstrum[i]=0.0f; 
		for(SubBandNo=1;SubBandNo<=SubBandNum;SubBandNo++)
			Cepstrum[i] += SubBandEnergy[SubBandNo]*pCosTable[SubBandNo];
		Cepstrum[i] *= cepWin[i-1];
		//		cout<<"["<<i<<"]"<<Cepstrum[i]<<endl;
		pCosTable = pCosTable + SubBandNum + 1; 
	}
	// Output the cepstrum and energy
	for( i = 1; i <= CepstrumNum; i++ )
		outVect[i-1] = Cepstrum[i];	//�����ǰ֡�ĵ���ϵ��
	*MPidx = historyMPidx;			//drawback
	return(FrameEnergy);			//���ص�ǰ֡������
}

CMFCC::~CMFCC(){

}

//CMFCC	CMfcc(DEFAULT_SAMPLE_RATE,
//			  DEFAULT_FFT_LEN,
//			  DEFAULT_FRAME_LEN,
//			  DEFAULT_SUB_BAND_NUM,
//			  DEFAULT_CEP_COEF_NUM);

float	aa[5] = {-0.75f, -0.375f, 0.0f, 0.375f, 0.75f};

/*-----------------------------------------------------------------------��������������-----*/
/*   �����޸ĺ���������򣬸��������źŵ�Ԫ�����������������������ź������ļ���ƽ��ֵ       */ 
/*   Ԫ���źŵ�����ֵ���������֡������-20dB                                                */ 
/*---------------------------------------------------------------------------��������������-*/
bool get20dBEnergyGeometryAveragMfcc(short *SpeecBuffer,float (*FeatureBuffer)[DIM],long FrameNum, CMFCC& CMfcc)
{
	long	i,FrameCounter,FrameNo;
	float	FrameEnergy,MaxEn,AveEn;

	//�� 0 ~ 13 ά��ŵ��ף��� 14 ~ 27 ά���һ�׵���
	for( FrameNo = 0 ; FrameNo < FrameNum; FrameNo++ )
	{
		//�� 0 ~ 13 ά��ŵ���
		FrameEnergy = CMfcc.DoMFCC( &SpeecBuffer[FrameNo * FRAME_STEP], FeatureBuffer[FrameNo] );	
		FeatureBuffer[FrameNo][3*D] = (float)(10.0*log10(FrameEnergy));
	}

	//����������������֡ ------ 2001,6,11�޶�
	MaxEn = -1000;
	for( FrameNo = 0; FrameNo < FrameNum; FrameNo++ )
	{
		if( FeatureBuffer[FrameNo][3*D] > MaxEn )
			MaxEn = FeatureBuffer[FrameNo][3*D];
	}
	//����������ƽ������	
	AveEn		 = 0.0;
	FrameCounter = 0;
	for( FrameNo = 0; FrameNo < FrameNum; FrameNo++ )
	{	//�����������-20dBΪԪ���źŵ�����ֵ����������ƽ������
		if( FeatureBuffer[FrameNo][3*D] > MaxEn -20 )
		{
			AveEn += FeatureBuffer[FrameNo][3*D];
			FrameCounter++;
		}
	}
	AveEn = AveEn/FrameCounter;
	// ��һ������֡����
	for( FrameNo =0 ; FrameNo < FrameNum; FrameNo++ )
		FeatureBuffer[FrameNo][3*D] = FeatureBuffer[FrameNo][3*D]-AveEn;

	//����ͷ��֡�������֡��һ�ײ�ֵ���
	for( i = 0; i < D; i++ )
	{
		FeatureBuffer[0][D+i]=(float)(2.0*(aa[1]*FeatureBuffer[0][i]+aa[3]*FeatureBuffer[1][i]));		//��0֡
		FeatureBuffer[1][D+i]=(float)(2.0*(aa[1]*FeatureBuffer[0][i]+aa[3]*FeatureBuffer[2][i]));		//��1֡
		FeatureBuffer[FrameNum-2][D+i]=(float)(2.0*(aa[1]*FeatureBuffer[FrameNum-3][i]+aa[3]*FeatureBuffer[FrameNum-1][i]) );
		FeatureBuffer[FrameNum-1][D+i]=(float)(2.0*(aa[1]*FeatureBuffer[FrameNum-2][i]+aa[3]*FeatureBuffer[FrameNum-1][i]) );
	}
	//�����м�֡��һ�ײ�ֵ���
	for( FrameNo = 2; FrameNo < FrameNum-2; FrameNo++ ) 
	{
		for( i = 0; i < D; i++ )
			FeatureBuffer[FrameNo][D+i] = 
			aa[0]*FeatureBuffer[FrameNo-2][i] +
			aa[1]*FeatureBuffer[FrameNo-1][i] +
			aa[3]*FeatureBuffer[FrameNo+1][i] +
			aa[4]*FeatureBuffer[FrameNo+2][i];
	}

	/************************************/
	//����ͷ��֡�������֡�Ķ��ײ�ֵ���
	for( i = 0; i < D; i++ )
	{
		FeatureBuffer[0][2*D+i]=(float)(2.0*(aa[1]*FeatureBuffer[0][D+i]+aa[3]*FeatureBuffer[1][D+i]));
		FeatureBuffer[1][2*D+i]=(float)(2.0*(aa[1]*FeatureBuffer[0][D+i]+aa[3]*FeatureBuffer[2][D+i]));
		FeatureBuffer[FrameNum-2][2*D+i]=(float)(2.0*(aa[1]*FeatureBuffer[FrameNum-3][D+i]+aa[3]*FeatureBuffer[FrameNum-1][D+i]) );
		FeatureBuffer[FrameNum-1][2*D+i]=(float)(2.0*(aa[1]*FeatureBuffer[FrameNum-2][D+i]+aa[3]*FeatureBuffer[FrameNum-1][D+i]) );
	}
	//����� FrameNo ֡���׵Ķ��ײ��
	for( FrameNo = 2; FrameNo < FrameNum-2; FrameNo++ ) 
	{
		for( i = 0; i < D; i++ )
			FeatureBuffer[FrameNo][2*D+i] = 
			aa[0]*FeatureBuffer[FrameNo-2][D+i] +
			aa[1]*FeatureBuffer[FrameNo-1][D+i] +
			aa[3]*FeatureBuffer[FrameNo+1][D+i] +
			aa[4]*FeatureBuffer[FrameNo+2][D+i];
	}

	//����ͷ��֡�������֡��һ���������
	FeatureBuffer[0][3*D+1]	= (float)(0.8*(aa[1]*FeatureBuffer[0][3*D] + aa[3]*FeatureBuffer[1][3*D]));
	FeatureBuffer[1][3*D+1]	= (float)(0.8*(aa[1]*FeatureBuffer[0][3*D] + aa[3]*FeatureBuffer[2][3*D]));
	FeatureBuffer[FrameNum-2][3*D+1] = (float)(0.8*(aa[1]*FeatureBuffer[FrameNum-3][3*D] + aa[3]*FeatureBuffer[FrameNum-1][3*D]));
	FeatureBuffer[FrameNum-1][3*D+1] = (float)(0.8*(aa[1]*FeatureBuffer[FrameNum-2][3*D] + aa[3]*FeatureBuffer[FrameNum-1][3*D]));
	//�����м�֡��һ���������
	for( FrameNo = 2; FrameNo < FrameNum-2; FrameNo++ )
		FeatureBuffer[FrameNo][3*D+1] = 
		(float)(0.4*(aa[0]*FeatureBuffer[FrameNo-2][3*D]
	+ aa[1]*FeatureBuffer[FrameNo-1][3*D]
	+ aa[3]*FeatureBuffer[FrameNo+1][3*D]
	+ aa[4]*FeatureBuffer[FrameNo+2][3*D]));

	//����ͷ��֡�������֡�Ķ����������
	FeatureBuffer[0][3*D+2] = (float)(1.5*(FeatureBuffer[1][3*D+1]-FeatureBuffer[0][3*D+1]));
	FeatureBuffer[FrameNum-1][3*D+2] = (float)(1.5*(FeatureBuffer[FrameNum-1][3*D+1] - FeatureBuffer[FrameNum-2][3*D+1]));
	//�����м�֡�Ķ����������
	for(FrameNo=1;FrameNo<FrameNum-1;FrameNo++)
		FeatureBuffer[FrameNo][3*D+2] = (float)(1.5*(FeatureBuffer[FrameNo+1][3*D+1] - FeatureBuffer[FrameNo-1][3*D+1]));

	return true;
}
