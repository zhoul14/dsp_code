#include <math.h>
#include "Filter.h"

CFIRFilter::CFIRFilter()
{
}

CFIRFilter::~CFIRFilter()
{
}

void CFIRFilter::IniClass(float LowFreqCutoff , float HighFreqCutoff,float SampleRate)
{
	FilterLowCutoffFreq		= LowFreqCutoff;
	FilterHighCutoffFreq	= HighFreqCutoff;
	FilterSampleRate		= SampleRate;
	FirFilterLen			= FIR_FILTER_LEN;	//should be an old number
	CircularBufLen			= CIR_BUFFER_LEN;	//ѭ���������ĳ��ȣ���С�Ĵ���FIR������2���ݴη���
	CircularMask			= CircularBufLen-1;	//CircularMask=2**n - 1
	CirPtr					= 0;
	GetFirFilter(m_FirFilter, FirFilterLen, FilterSampleRate, FilterLowCutoffFreq, FilterHighCutoffFreq); //Setup filter coefficience
	ResetFilter();
}

void CFIRFilter::ResetFilter(void)
{
unsigned long	i;

	for(i=0;i<CircularBufLen;i++)
		m_CircularBuf[i]=0;
	
	CirPtr = 0;
}

long CFIRFilter::GetFirFilterLen(void)
{
	return(FirFilterLen);
}

long CFIRFilter::GetDelayTime(void)
{
	return((FirFilterLen-1)/2);
}

void CFIRFilter::DoFirFilter(short *InBuffer,short *OutBuffer,long DataNum)
{
long	DataNo,TmpPtr,i;
double	Sum;
	
	for(DataNo=0;DataNo<DataNum;DataNo++)
	{
		CirPtr=(CirPtr+1)&CircularMask;
		m_CircularBuf[CirPtr]=InBuffer[DataNo];
		TmpPtr=CirPtr;
		Sum=0.0f;
		for(i=0;i<FirFilterLen;i++)
		{
			Sum=Sum+m_FirFilter[i]*m_CircularBuf[TmpPtr];
			TmpPtr=(TmpPtr-1)&CircularMask;
		}
		if( Sum >0 )	Sum+=0.5;		//��������
		else			Sum-=0.5;
		OutBuffer[DataNo]=(short)Sum;
	}
}

void CFIRFilter::DoFirFilter(double *InBuffer,double *OutBuffer,long DataNum)
{
long	DataNo,TmpPtr,i;
double	Sum;
	
	for(DataNo=0;DataNo<DataNum;DataNo++)
	{
		CirPtr=(CirPtr+1)&CircularMask;
		m_CircularBuf[CirPtr]=InBuffer[DataNo];
		TmpPtr=CirPtr;
		Sum=0.0f;
		for(i=0;i<FirFilterLen;i++)
		{
			Sum=Sum+m_FirFilter[i]*m_CircularBuf[TmpPtr];
			TmpPtr=(TmpPtr-1)&CircularMask;
		}
		OutBuffer[DataNo]=Sum;
	}
}


void CFIRFilter::GetFirFilter(double *h,long FirLen,float SampleRate,float LowCutoffFreq,float HighCutoffFreq)
{
double	PI,w;
double midTag;
//double	sum,Re,Im;
long	i,k;

	PI=asin(1.0)*2;
	for(i=0;i<FirLen;i++)	h[i]=0.0;

	k=(FirLen-1)/2;
	
	midTag=2*HighCutoffFreq/SampleRate;
	h[k]=midTag;
	w=midTag*PI;
	for(i=1;i<=k;i++)
	{
		h[k+i]=(midTag*sin(w*i)/(w*i));
		h[k-i]=h[k+i];
	}

	if(LowCutoffFreq > 0 )
	{
		midTag=2*LowCutoffFreq/SampleRate;
		h[k]=h[k]-midTag;
		w=midTag*PI;
		for(i=1;i<=k;i++)
		{
			h[k+i]=h[k+i]-(midTag*sin(w*i)/(w*i));
			h[k-i]=h[k+i];
		}
	}
/*
	if(0)
	{
		sum=0.0f;
		for(i=0;i<FirLen;i++)	sum+=h[i];
		std::cout<<"sum="<<sum<<std::endl;
		Re=0.0f;
		Im=0.0f;
		for(w=0.0f;w<PI;w=w+0.001f)
		{
			Re=0.0f;
			Im=0.0f;
			for(i=0;i<FirLen;i++)
			{
				Re=Re+(h[i]*cos(w*i));
				Im=Im-(h[i]*sin(w*i));
			}
			sum=sqrt(Re*Re+Im*Im);
			std::cout<<"f="<<w*SampleRate/(2*PI)<<"(Hz)"<<"\t Am="<<sum<<std::endl;
		}
	}
*/
}
