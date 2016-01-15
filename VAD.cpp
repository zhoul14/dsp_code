#include <stdio.h>
#include "VAD.h"



CSPEECH_DETECTION::CSPEECH_DETECTION( )
{

}


void	CSPEECH_DETECTION::IniClass()
{
	m_LowpassFilter.IniClass();

	m_FrameLen							= DEF_FRAME_LEN;
	m_SpeechFrameCircularBufferFrameNum	= DEF_SPEECH_FRAME_CIRCULAR_BUFFER_NUM;
	m_FrameEnergyCircularBufferFrameNum	= DEF_FRAME_ENERGY_CIRCULAR_BUFFER_NUM;
	m_MaxDetectedSpeechFrameNum			= DEF_MAX_DETECT_SPEECH_FRAME_NUM;

	if( m_MaxDetectedSpeechFrameNum < REWINDED_FRAME_NUM + REVERSE_SEARCH_FRAME + MIN_SPEECH_DURATION + POST_EXTENED_FRAME_NUM )
		m_MaxDetectedSpeechFrameNum = REWINDED_FRAME_NUM + REVERSE_SEARCH_FRAME + MIN_SPEECH_DURATION + POST_EXTENED_FRAME_NUM;

	m_SpeechCircularBufferLen		= m_SpeechFrameCircularBufferFrameNum*m_FrameLen;

	ResetDetector();

}


void CSPEECH_DETECTION::ResetDetector(void)
{
	m_LowpassFilter.ResetFilter();
	m_PreSpeechData							= 0;
	m_SpeechCircularBufferWritePtr			= 0;
	m_SpeechDetectedWritePtr				= 0;
	m_FrameEnergyCircularWritePtr			= 0;
	m_SpeechFrameNumCachedInCircualrBuffer	= 0;
	m_EnergyFrameNumCachedInCircualrBuffer	= 0;
	m_SpeechFrameNum						= 0;
	m_SkipFrame								= 0;
	m_MinEn.dB								= 200;
	m_MinEn.Duration						= 0;
	m_SpeechStatus							= WAIT_SPEECH;

	for( long i = 0 ; i < m_SpeechCircularBufferLen; i++ )
	{
		m_SpeechCircularBuffer[i] = 0;
	}

	for( long i = 0 ; i < m_FrameEnergyCircularBufferFrameNum; i++ )
	{
		m_FrameEnergyCircularBuf[i] = -1000;
	}

}

CSPEECH_DETECTION::~CSPEECH_DETECTION()
{
}

//��������Ǹ��ݲ�������ı仯�����о���
long CSPEECH_DETECTION::DetectSpeeeh(short *Buffer)
{
	long	i,j,tmpCirPtr,FrameNumRewinded,BackwardFrameNum;
	double	Sum,tmp;

	if( m_SkipFrame < 2 )
	{
		//�ڶ����������������˲�ʱ���õ���������λ�˲�����FIR�ĳ�����2*m_FrameLen+1���˲�����������һ֡���ӳ�
		//��Ҫ��֡�����ݣ�20mS�����������˲����еĻ�����
		m_SkipFrame++;
		for( i = m_FrameLen-1 ; i> 0; i-- )
		{	//����������
			m_SpeechEnBuffer[i] = (double)(Buffer[i]-Buffer[i-1]);
			m_SpeechEnBuffer[i] = m_SpeechEnBuffer[i]*m_SpeechEnBuffer[i];
		}
		m_SpeechEnBuffer[0] = (double)(Buffer[0]-m_PreSpeechData);
		m_SpeechEnBuffer[0] = m_SpeechEnBuffer[0]*m_SpeechEnBuffer[0];
		m_PreSpeechData = Buffer[m_FrameLen-1]; 
		m_LowpassFilter.DoFirFilter(m_SpeechEnBuffer,m_LowpassSpeechEnBuffer,m_FrameLen);

		for( i = 0; i< m_FrameLen; i++ )
		{	//������������ӵ���������
			m_SpeechCircularBuffer[m_SpeechCircularBufferWritePtr++] = Buffer[i];
		}

		if( m_SpeechCircularBufferWritePtr >= m_SpeechCircularBufferLen ) m_SpeechCircularBufferWritePtr -=m_SpeechCircularBufferLen;
		m_SpeechFrameNumCachedInCircualrBuffer++;												//�������л����֡����1
		if( m_SpeechFrameNumCachedInCircualrBuffer > m_SpeechFrameCircularBufferFrameNum )
			m_SpeechFrameNumCachedInCircualrBuffer = m_SpeechFrameCircularBufferFrameNum;		//���ݻ�������


		return(WAIT_SPEECH);
	}

	for( i = 0; i< m_FrameLen; i++ )
	{	//������������ӵ���������
		m_SpeechCircularBuffer[m_SpeechCircularBufferWritePtr++] = Buffer[i];
	}

	for( i = m_FrameLen-1 ; i> 0; i-- )
	{	//����������
		m_SpeechEnBuffer[i] = (double)(Buffer[i]-Buffer[i-1]);
		m_SpeechEnBuffer[i] = m_SpeechEnBuffer[i]*m_SpeechEnBuffer[i];
	}
	m_SpeechEnBuffer[0] = (double)(Buffer[0]-m_PreSpeechData);
	m_SpeechEnBuffer[0] = m_SpeechEnBuffer[0]*m_SpeechEnBuffer[0];
	m_PreSpeechData = Buffer[m_FrameLen-1]; 

	if( m_SpeechCircularBufferWritePtr >= m_SpeechCircularBufferLen )
	{
		m_SpeechCircularBufferWritePtr -=m_SpeechCircularBufferLen;
	}

	m_SpeechFrameNumCachedInCircualrBuffer++;												//�������л����֡����1
	if( m_SpeechFrameNumCachedInCircualrBuffer > m_SpeechFrameCircularBufferFrameNum )
		m_SpeechFrameNumCachedInCircualrBuffer = m_SpeechFrameCircularBufferFrameNum;		//���ݻ�������

	m_LowpassFilter.DoFirFilter(m_SpeechEnBuffer,m_LowpassSpeechEnBuffer,m_FrameLen);
	//���㵱ǰ֡��ƽ����������
	Sum = 0;
	for( i = 0; i< m_FrameLen; i++ )
	{
		tmp = m_LowpassSpeechEnBuffer[i];
		if( tmp < 0.01 ) tmp =0.01;
		Sum += 10*log10(tmp);
	}
	Sum = Sum/m_FrameLen;
	m_FrameEnergyCircularBuf[m_FrameEnergyCircularWritePtr] = Sum;	//��ǰ֡��ƽ������

	//�����������е���С����
	m_MinEn.Duration++;
	if( m_MinEn.Duration < m_FrameEnergyCircularBufferFrameNum ) 
	{	//��С����ֵδ��������Ч��
		if( Sum < m_MinEn.dB )
		{	m_MinEn.dB		= Sum;
		m_MinEn.Position	= m_FrameEnergyCircularWritePtr;
		m_MinEn.Duration	= 0;
		}
	}
	else
	{	//��С����ֵ��������Ч�ڣ�ȫ����Ѱ����С�ı�����������
		m_MinEn.dB = 200;
		for( i = 0 ; i < m_FrameEnergyCircularBufferFrameNum; i++ )
		{
			if( m_FrameEnergyCircularBuf[i] < m_MinEn.dB )
			{
				m_MinEn.dB		= m_FrameEnergyCircularBuf[i];
				m_MinEn.Position	= i;
			}
		}
		// ע�⣺�����m_MinEn.Duration��ֵ����Ӧ���Ǹ���m_MinEn.Position�뵱
		//       ǰ��m_FrameEnergyCircularWritePtr��ֵ�������������Ϊ�˼��㷨����
		//       ����ֱ������m_MinEn.Duration = 0��������һ�ε�ȫ��������ֻ��
		//       ����m_SpeechFrameCircularBufferNum֡�����Ժ󣬼����˼�������������
		//       ����Ӱ����С������ʵʱ���£���������ƽ����ʱ���־���һ����
		//       �ĺ�Ч�ԣ��ӳ�m_SpeechFrameCircularBufferNum֡���£���

		m_MinEn.Duration = 0;
	}

	m_EnergyFrameNumCachedInCircualrBuffer++;		//����֡���� 1 
	if( m_EnergyFrameNumCachedInCircualrBuffer > m_FrameEnergyCircularBufferFrameNum )
		m_EnergyFrameNumCachedInCircualrBuffer = m_FrameEnergyCircularBufferFrameNum;		//�������ݻ�������

	m_FrameEnergyCircularWritePtr++;	//ѭ��ָ���1
	if( m_FrameEnergyCircularWritePtr >= m_FrameEnergyCircularBufferFrameNum ) m_FrameEnergyCircularWritePtr -= m_FrameEnergyCircularBufferFrameNum;

	switch(m_SpeechStatus)
	{
	case WAIT_SPEECH:
		if( Sum > m_MinEn.dB + THRESH_HOLD_VOWEL_DETECTED )			//������������Ԫ����������ֵ
		{
			//���������źŵ����λ��
			BackwardFrameNum = REVERSE_SEARCH_FRAME;
			if( BackwardFrameNum > m_EnergyFrameNumCachedInCircualrBuffer )
				BackwardFrameNum = m_EnergyFrameNumCachedInCircualrBuffer;

			FrameNumRewinded = 1;		//��ͨ�˲���Ⱥʱ��Ϊ1֡
			tmpCirPtr = m_FrameEnergyCircularWritePtr;
			for( i = 0; i < BackwardFrameNum; i++ )
			{
				FrameNumRewinded++;
				tmpCirPtr--;
				if( tmpCirPtr < 0 ) tmpCirPtr += m_FrameEnergyCircularBufferFrameNum;
				if( m_FrameEnergyCircularBuf[tmpCirPtr] < m_MinEn.dB + THRESH_HOLD_CONSONANT_DETECTED )	//���ݽ����ĸ�������
					break;
			}
			FrameNumRewinded += REWINDED_FRAME_NUM;
			if( FrameNumRewinded > m_SpeechFrameNumCachedInCircualrBuffer ) 
			{
				FrameNumRewinded = m_SpeechFrameNumCachedInCircualrBuffer;
			}

			tmpCirPtr = m_SpeechCircularBufferWritePtr-m_FrameLen*FrameNumRewinded;
			if( tmpCirPtr < 0 ) tmpCirPtr += m_SpeechCircularBufferLen;

			//���ݼ�⵽����㣬�����������ݵ���������
			m_SpeechDetectedWritePtr = 0;
			for( i = 0; i < FrameNumRewinded; i++ )
			{
				for( j = 0; j < m_FrameLen; j++ )
				{
					m_SpeechBuffer[m_SpeechDetectedWritePtr++] = m_SpeechCircularBuffer[tmpCirPtr++];
				}
				if( tmpCirPtr >= m_SpeechCircularBufferLen ) tmpCirPtr -= m_SpeechCircularBufferLen;
			}
			m_SpeechFrameNum		= FrameNumRewinded;
			m_VowelSpeechFrameNum	= 0;
			m_SilenceInterval		= 0;
			m_SpeechStatus			= SPEECH_START;
		}
		break;
	case SPEECH_START:
		if( m_SpeechFrameNum < m_MaxDetectedSpeechFrameNum )
		{
			for( i = 0; i< m_FrameLen; i++ )
			{
				m_SpeechBuffer[m_SpeechDetectedWritePtr++] = Buffer[i];
			}
			m_SpeechFrameNum++;
			m_VowelSpeechFrameNum++;
		}
		// �쳣������⴦��
		if( m_SpeechFrameNum >= m_MaxDetectedSpeechFrameNum )
		{
			m_SpeechFrameNum  = m_SpeechFrameNum - m_SilenceInterval;
			if( m_SpeechFrameNum > MIN_SPEECH_DURATION )
				m_SpeechStatus = SPEECH_DETECTED;		//ֻ���������ȴ�������ֵʱ������Ϊ������������
			else
				m_SpeechStatus = WAIT_SPEECH;			//��������С������ֵʱ����Ϊ�Ǹ����ź�
			break;
		}

		if( Sum < m_MinEn.dB + THRESH_HOLD_VOWEL_TO_SILENCE )
		{
			//��Ԫ������3֡ʱ��Ϊ�Ǿ����Ŀ�ʼ��������Ϊ�������ĸ���
			if( m_VowelSpeechFrameNum > 3 )
			{
				m_SpeechStatus		= SPEECH_VOWEL_TO_SILENCE;	//�����������������ּ�϶
				m_SilenceInterval	= 0;
			}
			else
			{	//�������ţ����¼����������ʼ��
				m_SpeechStatus = WAIT_SPEECH;
				//printf("Vowel too short\n");
			}
		}
		break;
	case SPEECH_VOWEL_TO_SILENCE:
		if( m_SpeechFrameNum < m_MaxDetectedSpeechFrameNum )
		{
			for( i = 0 ; i < m_FrameLen; i++ )
			{
				m_SpeechBuffer[m_SpeechDetectedWritePtr++] = Buffer[i];
			}
			m_SpeechFrameNum++;
		}
		// �쳣������⴦��
		if( m_SpeechFrameNum >= m_MaxDetectedSpeechFrameNum )
		{
			m_SpeechFrameNum  = m_SpeechFrameNum - m_SilenceInterval;
			if( m_SpeechFrameNum > MIN_SPEECH_DURATION )
				m_SpeechStatus = SPEECH_DETECTED;		//ֻ���������ȴ�������ֵʱ������Ϊ������������
			else
				m_SpeechStatus = WAIT_SPEECH;			//��������С������ֵʱ����Ϊ�Ǹ����ź�
			break;
		}

		if( Sum < m_MinEn.dB + THRESH_HOLD_VOWEL_TO_SILENCE )		//����С��25dBʱ�п���Ϊ����
		{
			m_SilenceInterval++;
			if( m_SilenceInterval > SILENCE_INTERVAL )
			{
				m_SpeechFrameNum -= SILENCE_INTERVAL;
				if( m_SpeechFrameNum > MIN_SPEECH_DURATION )
				{
					m_SpeechFrameNum += POST_EXTENED_FRAME_NUM;		//�����������޵���󸽼ӵ�����֡��
					m_SpeechStatus = SPEECH_DETECTED;				//ֻ���������ȴ�������ֵʱ������Ϊ������������
				}
				else
					m_SpeechStatus = WAIT_SPEECH;					//��������С������ֵʱ����Ϊ�Ǹ����ź�
			}
		}
		else
		{
			m_SilenceInterval		= 0;
			//	m_VowelSpeechFrameNum = 0;					//��������ε��󣬷�����������������
			m_SpeechStatus		= SPEECH_START;
		}
		break;
	}
	printf("\r[%7.2f%7.2f]",m_MinEn.dB,Sum);
	return(m_SpeechStatus);
}
 

extern"C" void Call_C_InitVAD(CSPEECH_DETECTION* detector)
{
	detector->IniClass();
}
extern"C" long Call_C_doVAD(CSPEECH_DETECTION* detector,char* inBuf, int len,long (*pfDoRecg)(void *pInfo,short *SpeechBuf,int DataNum),void *recClass)
{
	long FrameLen = detector->m_FrameLen;
	long recres;	
	if(len != FrameLen)
		return len;
	recres = detector->DetectSpeeeh((short*)inBuf); 
	if(recres == 3)
	{

		short *pVADSpeechBuf	= detector->m_SpeechBuffer;
		int VADDataNum			= detector->m_SpeechFrameNum * detector->m_FrameLen;
		long res = pfDoRecg(recClass, pVADSpeechBuf, VADDataNum);
		detector->ResetDetector();
		return res;

	}
	else
	{
			return -3;
	}
	return -4;
}
