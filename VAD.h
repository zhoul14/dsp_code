#ifndef	_XIAOXI_CSPEECH_DETECTION_H_
#define	_XIAOXI_CSPEECH_DETECTION_H_

#include	<math.h>
#include "Filter.h"

typedef struct tagSPEECH_EN{
	double	dB;
	long	Position;
	long	Duration;
} SPEECH_EN;

#define	WAIT_SPEECH				0
#define	SPEECH_START			1
#define SPEECH_VOWEL_TO_SILENCE	2
#define SPEECH_DETECTED			3


class	CSPEECH_DETECTION{
private:
	enum{
		THRESH_HOLD_CONSONANT_DETECTED   = 9,							//�������������
		THRESH_HOLD_VOWEL_DETECTED   = 25,								//Ԫ���������
		//			THRESH_HOLD_VOWEL_TO_SILENCE = THRESH_HOLD_VOWEL_DETECTED-3,	//Ԫ����������
		THRESH_HOLD_VOWEL_TO_SILENCE = THRESH_HOLD_VOWEL_DETECTED-6,	//Ԫ����������

		REVERSE_SEARCH_FRAME	= 20,		//��Ԫ��λ�ÿ�ʼ���ݣ�������150���루�����������Ϊ0.15�룩
		SILENCE_INTERVAL		= 50,		//�ּ�϶���Ϊ0.5��
		MIN_SPEECH_DURATION		= 20,		//�����������Ϊ0.2��
		POST_EXTENED_FRAME_NUM	= 30,		//�����������޵���֮�󸽼ӵľ�������֡��
		REWINDED_FRAME_NUM		= 15,		//������ʼ����֮ǰ���ӵľ���Ԥ��֡��

	};
	enum{
		DEF_FRAME_LEN	= 160,						//�������ķֱ��ʣ�ÿ������ĵ�����10mS��
		DEF_SAMPLE_RATE = 16000,					//�������ݲ�����
		DEF_SPEECH_FRAME_CIRCULAR_BUFFER_NUM = 200,	//����ѭ����������֡����2�룩
		DEF_FRAME_ENERGY_CIRCULAR_BUFFER_NUM = 400, //����֡������ѭ��������֡����4�룩�����ڼ�ⱳ�������ͻ�����������ʼ��
		DEF_MAX_DETECT_SPEECH_FRAME_NUM		 = 2000, //���ܼ�����������֡����10�룩
	};
	CFIRFilter	m_LowpassFilter;
	long	m_FrameEnergyCircularBufferFrameNum;
	long	m_SpeechFrameCircularBufferFrameNum;
	long	m_MaxDetectedSpeechFrameNum;

	long	m_SpeechCircularBufferLen;									//����ѭ�����������ܳ���
	long	m_SpeechFrameNumCachedInCircualrBuffer;						//��¼������ѭ����������������֡��
	long	m_EnergyFrameNumCachedInCircualrBuffer;						//��¼������ѭ����������������֡��

	long	m_SpeechCircularBufferWritePtr,m_FrameEnergyCircularWritePtr;	//ѭ����������дָ��
	long	m_SpeechDetectedWritePtr;

	long	m_SkipFrame,m_VowelSpeechFrameNum;
	long	m_SpeechStatus,m_SilenceInterval;
	short	m_PreSpeechData;
	SPEECH_EN	m_MinEn;

	short	m_SpeechCircularBuffer[DEF_SPEECH_FRAME_CIRCULAR_BUFFER_NUM*DEF_FRAME_LEN];
	double	m_FrameEnergyCircularBuf[DEF_FRAME_ENERGY_CIRCULAR_BUFFER_NUM];	//������������֡������ѭ��������
	double	m_SpeechEnBuffer[DEF_FRAME_LEN];
	double	m_LowpassSpeechEnBuffer[DEF_FRAME_LEN];
public:
	short	m_SpeechBuffer[DEF_MAX_DETECT_SPEECH_FRAME_NUM*DEF_FRAME_LEN];
	long	m_FrameLen,m_SpeechFrameNum;

	CSPEECH_DETECTION();
	~CSPEECH_DETECTION( );
	void	IniClass();
	long DetectSpeeeh(short *Buffer);
	void ResetDetector(void);
};

extern"C" void Call_C_InitVAD(CSPEECH_DETECTION* detector);
extern"C" long Call_C_doVAD(CSPEECH_DETECTION* detector,char* inBuf, int len,long (*pfDoRecg)(void *pInfo,short *SpeechBuf,int DataNum),void *recClass);

#endif
