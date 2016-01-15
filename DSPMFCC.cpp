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
	mpidx += sizeof(float) * (SubBandNum+1)*(SubBandNum+1);	//DCT变换系数

	hamWin = (float*)(m_pMemPool + mpidx);
	mpidx += sizeof(float) * FrameLen;	//hamming 窗系数

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
	//由于子带0的系数（直流分量）不参与MFCC特征计算，所以子带个数要多一个
	M	 = SubBandNum+1;	
	ms	 = Mel(FFT_LEN/2);		//计算1/2采样率所对应的MEL刻度
	//Note that the sub-band 0 is not used for cepstrum caculating
	for ( SubBandNo = 0; SubBandNo <= M; SubBandNo++ )
	{	//计算每个子带的起始MEL刻度
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
	{	//以子带的起始MEL频率为中心，计算三角窗加权系数
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
	for( i= FrameLen-1; i > 0; i-- )	//计算语音信号差分并进行加窗
	{
		FFTFrame[i] -= FFTFrame[i-1]*PREE;
		FFTFrame[i] *= hamWin[i];
	}
	FFTFrame[0] *= (1.0f-PREE);
	FFTFrame[0] *= hamWin[0];
	for(i=FrameLen;i<FFT_LEN;i++) FFTFrame[i]=0.0f;

	// 进行FFT变换
	DoRealFFT(FFTFrame);
	for( SubBandNo = 1 ; SubBandNo <= SubBandNum; SubBandNo++ )
	{
		SubBandEnergy[SubBandNo]=0.0f;
	}
	for( i = 1; i < FFT_LEN/2; i++ )	//the component at zero frequence is discarded
	{
		temp1 = FFTFrame[2*i]; temp2 = FFTFrame[2*i+1];
		ek = temp1*temp1+temp2*temp2;	// 计算第k点频率处的能量
		SubBandNo = SubBandIndex[i];	// 查表求得第k点频率所属的子带号
		temp1 = SubBandWeight[i]*ek;
		if(SubBandNo>0) SubBandEnergy[SubBandNo] += temp1;
		if(SubBandNo<SubBandNum) SubBandEnergy[SubBandNo+1] += ek-temp1;
	}
	// 计算每组滤波器输出能量的对数值
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
		outVect[i-1] = Cepstrum[i];	//输出当前帧的倒谱系数
	*MPidx = historyMPidx;			//drawback
	return(FrameEnergy);			//返回当前帧的能量
}

CMFCC::~CMFCC(){

}

//CMFCC	CMfcc(DEFAULT_SAMPLE_RATE,
//			  DEFAULT_FFT_LEN,
//			  DEFAULT_FRAME_LEN,
//			  DEFAULT_SUB_BAND_NUM,
//			  DEFAULT_CEP_COEF_NUM);

float	aa[5] = {-0.75f, -0.375f, 0.0f, 0.375f, 0.75f};

/*-----------------------------------------------------------------------———————-----*/
/*   此乃修改后的特征程序，根据语音信号的元音部分能量来计算语音的信号能量的几何平均值       */ 
/*   元音信号的门限值是最大语音帧能量的-20dB                                                */ 
/*---------------------------------------------------------------------------———————-*/
bool get20dBEnergyGeometryAveragMfcc(short *SpeecBuffer,float (*FeatureBuffer)[DIM],long FrameNum, CMFCC& CMfcc)
{
	long	i,FrameCounter,FrameNo;
	float	FrameEnergy,MaxEn,AveEn;

	//第 0 ~ 13 维存放倒谱，第 14 ~ 27 维存放一阶倒谱
	for( FrameNo = 0 ; FrameNo < FrameNum; FrameNo++ )
	{
		//第 0 ~ 13 维存放倒谱
		FrameEnergy = CMfcc.DoMFCC( &SpeecBuffer[FrameNo * FRAME_STEP], FeatureBuffer[FrameNo] );	
		FeatureBuffer[FrameNo][3*D] = (float)(10.0*log10(FrameEnergy));
	}

	//搜索能量最大的语音帧 ------ 2001,6,11修订
	MaxEn = -1000;
	for( FrameNo = 0; FrameNo < FrameNum; FrameNo++ )
	{
		if( FeatureBuffer[FrameNo][3*D] > MaxEn )
			MaxEn = FeatureBuffer[FrameNo][3*D];
	}
	//计算语音的平均能量	
	AveEn		 = 0.0;
	FrameCounter = 0;
	for( FrameNo = 0; FrameNo < FrameNum; FrameNo++ )
	{	//以最大能量的-20dB为元音信号的门限值计算语音的平均能量
		if( FeatureBuffer[FrameNo][3*D] > MaxEn -20 )
		{
			AveEn += FeatureBuffer[FrameNo][3*D];
			FrameCounter++;
		}
	}
	AveEn = AveEn/FrameCounter;
	// 归一化语音帧能量
	for( FrameNo =0 ; FrameNo < FrameNum; FrameNo++ )
		FeatureBuffer[FrameNo][3*D] = FeatureBuffer[FrameNo][3*D]-AveEn;

	//计算头两帧和最后两帧的一阶差分倒谱
	for( i = 0; i < D; i++ )
	{
		FeatureBuffer[0][D+i]=(float)(2.0*(aa[1]*FeatureBuffer[0][i]+aa[3]*FeatureBuffer[1][i]));		//第0帧
		FeatureBuffer[1][D+i]=(float)(2.0*(aa[1]*FeatureBuffer[0][i]+aa[3]*FeatureBuffer[2][i]));		//第1帧
		FeatureBuffer[FrameNum-2][D+i]=(float)(2.0*(aa[1]*FeatureBuffer[FrameNum-3][i]+aa[3]*FeatureBuffer[FrameNum-1][i]) );
		FeatureBuffer[FrameNum-1][D+i]=(float)(2.0*(aa[1]*FeatureBuffer[FrameNum-2][i]+aa[3]*FeatureBuffer[FrameNum-1][i]) );
	}
	//计算中间帧的一阶差分倒谱
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
	//计算头两帧和最后两帧的二阶差分倒谱
	for( i = 0; i < D; i++ )
	{
		FeatureBuffer[0][2*D+i]=(float)(2.0*(aa[1]*FeatureBuffer[0][D+i]+aa[3]*FeatureBuffer[1][D+i]));
		FeatureBuffer[1][2*D+i]=(float)(2.0*(aa[1]*FeatureBuffer[0][D+i]+aa[3]*FeatureBuffer[2][D+i]));
		FeatureBuffer[FrameNum-2][2*D+i]=(float)(2.0*(aa[1]*FeatureBuffer[FrameNum-3][D+i]+aa[3]*FeatureBuffer[FrameNum-1][D+i]) );
		FeatureBuffer[FrameNum-1][2*D+i]=(float)(2.0*(aa[1]*FeatureBuffer[FrameNum-2][D+i]+aa[3]*FeatureBuffer[FrameNum-1][D+i]) );
	}
	//计算第 FrameNo 帧倒谱的二阶差分
	for( FrameNo = 2; FrameNo < FrameNum-2; FrameNo++ ) 
	{
		for( i = 0; i < D; i++ )
			FeatureBuffer[FrameNo][2*D+i] = 
			aa[0]*FeatureBuffer[FrameNo-2][D+i] +
			aa[1]*FeatureBuffer[FrameNo-1][D+i] +
			aa[3]*FeatureBuffer[FrameNo+1][D+i] +
			aa[4]*FeatureBuffer[FrameNo+2][D+i];
	}

	//计算头两帧和最后两帧的一阶能量差分
	FeatureBuffer[0][3*D+1]	= (float)(0.8*(aa[1]*FeatureBuffer[0][3*D] + aa[3]*FeatureBuffer[1][3*D]));
	FeatureBuffer[1][3*D+1]	= (float)(0.8*(aa[1]*FeatureBuffer[0][3*D] + aa[3]*FeatureBuffer[2][3*D]));
	FeatureBuffer[FrameNum-2][3*D+1] = (float)(0.8*(aa[1]*FeatureBuffer[FrameNum-3][3*D] + aa[3]*FeatureBuffer[FrameNum-1][3*D]));
	FeatureBuffer[FrameNum-1][3*D+1] = (float)(0.8*(aa[1]*FeatureBuffer[FrameNum-2][3*D] + aa[3]*FeatureBuffer[FrameNum-1][3*D]));
	//计算中间帧的一阶能量差分
	for( FrameNo = 2; FrameNo < FrameNum-2; FrameNo++ )
		FeatureBuffer[FrameNo][3*D+1] = 
		(float)(0.4*(aa[0]*FeatureBuffer[FrameNo-2][3*D]
	+ aa[1]*FeatureBuffer[FrameNo-1][3*D]
	+ aa[3]*FeatureBuffer[FrameNo+1][3*D]
	+ aa[4]*FeatureBuffer[FrameNo+2][3*D]));

	//计算头两帧和最后两帧的二阶能量差分
	FeatureBuffer[0][3*D+2] = (float)(1.5*(FeatureBuffer[1][3*D+1]-FeatureBuffer[0][3*D+1]));
	FeatureBuffer[FrameNum-1][3*D+2] = (float)(1.5*(FeatureBuffer[FrameNum-1][3*D+1] - FeatureBuffer[FrameNum-2][3*D+1]));
	//计算中间帧的二阶能量差分
	for(FrameNo=1;FrameNo<FrameNum-1;FrameNo++)
		FeatureBuffer[FrameNo][3*D+2] = (float)(1.5*(FeatureBuffer[FrameNo+1][3*D+1] - FeatureBuffer[FrameNo-1][3*D+1]));

	return true;
}
