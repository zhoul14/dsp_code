//#include "DspWordDict.h"
//#include "DspCODEBOOKS.h"
//#include "DSPCmdWordRecg.h"
#include "stdio.h"
#include "string.h"

#include "stdlib.h"
struct DSPWordDict* g_pWordict;
struct DSPGMMCodebookSet* g_pCodeBookSet;
struct DSPCmdWordRecg* g_pCmdWordRec;
struct CSPEECH_DETECTION* g_pSpeechDetector;
int SizeOfLargeCB;
char* g_pLargeCbMem;
char* g_pMemHead;

int MemPosition;
#define SIZE_OF_WORD_DICT 13008
#define SIZE_OF_CODEBOOK 48
#define SIZE_OF_CMDRECG 512
int LargeCb_Position;//大码本应该放的内存地址

char * test;
extern void Call_c_GetCmdBuff(struct DSPWordDict* wd,char* inBuf,int dataSize);
extern void Call_c_GetDictBuff(struct DSPWordDict* wd,char* inBuf,int dataSize);
extern void Call_c_GetCodeSetBuff(struct DSPGMMCodebookSet* cb,char* inBuf);
extern void Call_C_InitVAD(struct CSPEECH_DETECTION* detector);
extern long Call_C_doVAD(struct CSPEECH_DETECTION* detector,char* inBuf);
extern void Call_C_initRec(struct DSPCmdWordRecg* g_pCmdWordRec);
extern int Call_C_doCmdRecg(struct DSPCmdWordRecg* rec,short* sample, int sampleNum);
extern void Call_C_InitCmdRecg(struct DSPCmdWordRecg* rec, struct DSPWordDict* Dict, struct DSPGMMCodebookSet* Set, char* memPool, int* MPidx);

int DspModule(char* inBuf, char* outBuf);

int mergeLargeCb(char* memBegin, int *ptr, char* inBuf, int len);

int mergeLargeCb(char* memBegin, int *ptr, char* inBuf, int len)
{
	if (*ptr + len <= SizeOfLargeCB)
	{
		memcpy(memBegin + *ptr,inBuf,sizeof(char)*len);
		*ptr += len;
		if (*ptr == SizeOfLargeCB)
		{
			Call_c_GetCodeSetBuff(g_pCodeBookSet,g_pLargeCbMem);
			Call_C_InitCmdRecg(g_pCmdWordRec, g_pWordict, g_pCodeBookSet, g_pMemHead, &MemPosition);
		}
		return 1;
	}
	else
	{
		return 0;
	}
}

int main()//test
{
	FILE* fDict,*fCmd,*FCB;

	char* mempool = (char*)malloc(50000000);
	unsigned int head = 0;
	char* out = (char*)malloc(1000);char* buf_dict,*buf_cb,*buf_cb_ALL,*buf_cmd,*fbuf;
	int interVal = 200000;
	int num;
	int i;
	int buffSize;int*temp;
	g_pWordict =(struct DSPWordDict* ) (mempool + head);
	head += SIZE_OF_WORD_DICT;

	g_pCodeBookSet = (struct DSPGMMCodebookSet* ) (mempool + head);
	head += SIZE_OF_CODEBOOK;

	g_pCmdWordRec = (struct DSPCmdWordRecg* ) (mempool + head);
	head += SIZE_OF_CMDRECG;

	g_pMemHead = mempool;
	MemPosition = head;

	fDict = fopen("small_model","rb");
	fseek(fDict, 0, SEEK_END);
	buffSize = ftell(fDict);
	fseek(fDict, 0, SEEK_SET);
	buf_dict = (char*)malloc(buffSize + 6);
	buf_dict[0] = 1;
	buf_dict[1] = 'D';
	temp = (int*)(buf_dict + 2) ;
	*temp = buffSize;
	fread(buf_dict + 6, sizeof(char), buffSize, fDict);
	fclose(fDict);
	DspModule(buf_dict,out);

	fCmd = fopen("cmd_word.txt","rb");
	fseek(fCmd, 0, SEEK_END);
	buffSize = ftell(fCmd);
	fseek(fCmd, 0, SEEK_SET);
	buf_cmd = (char*)malloc(buffSize + 1 + 6);
	buf_cmd[0] = 1;
	buf_cmd[1] = 'C';
	buf_cmd[buffSize + 6] = 0;
	temp = (int*)(buf_cmd + 2) ;
	*temp = buffSize;
	fread(buf_cmd + 6, sizeof(char), buffSize, fCmd);
	fclose(fCmd);
	DspModule(buf_cmd,out);



	FCB = fopen("large_model_diag_4","rb");
	fseek(FCB, 0, SEEK_END);
	buffSize = ftell(FCB);
	fseek(FCB, 0, SEEK_SET);
	num = buffSize / interVal + 1;
	buf_cb = (char*)malloc(10);
	buf_cb[0] = 1;
	buf_cb[1] = 'E';
	DspModule(buf_cb,out);

	buf_cb[0] = 1;
	buf_cb[1] = 'F';
	temp = (int*)(buf_cb + 2);
	*temp = 10;
	temp = (int*)(buf_cb + 6);
	*temp = buffSize;
	DspModule(buf_cb,out);

	buf_cb_ALL = (char*)malloc(buffSize);
	test = buf_cb_ALL;
	fread(buf_cb_ALL, sizeof(char), buffSize, FCB);
	fclose(FCB);
	buf_cb = (char*)malloc(interVal);
	buf_cb[0] = 1;
	buf_cb[1] = 'G';
	*(int*)(buf_cb + 2) = interVal;
	g_pLargeCbMem = (char*)malloc(buffSize);
	for (i = 0; i < num; i++)
	{
		if (i!= num - 1)
		{
			memcpy(buf_cb + 6, buf_cb_ALL + i * interVal,interVal); 
		}
		else
		{
			memcpy(buf_cb + 6, buf_cb_ALL + i * interVal, buffSize - interVal * i);
			*(int*)(buf_cb + 2) = buffSize%interVal;

		}
		DspModule(buf_cb,out);
	}




	FCB = fopen("test.fm","rb");
	fseek(FCB, 0, SEEK_END);
	buffSize = ftell(FCB);
	fseek(FCB, 0, SEEK_SET);
	fbuf = (char*)malloc(buffSize + 6);
	fbuf[0] = 1;
	fbuf[1] = 'L';
	*(int*)(fbuf +2) = buffSize/sizeof(short);
	fread(fbuf + 6,sizeof(short),buffSize/sizeof(short),FCB);
	DspModule(fbuf,out);
	fclose(FCB);

	


	return 1;
}


int DspModule(char* inBuf, char* outBuf)
{
	int* len = (int*)(inBuf + 2);
	int ll = *len;
	int res = -1;
	switch (inBuf[1])
	{
	case 'C'://ARM cmd word
		Call_c_GetCmdBuff(g_pWordict, inBuf + 6, *len);
		outBuf = 0;
		return 0;
		break;
	case 'D'://
		Call_c_GetDictBuff(g_pWordict, inBuf + 6, *len);
		outBuf = NULL;
		break;
	case 'E':
		if (!g_pLargeCbMem)
		{
			return;
		}
		break;
	case 'F':
		SizeOfLargeCB = *(int*)(inBuf + 6);
		LargeCb_Position = 0;
		break;
	case 'G':
		mergeLargeCb(g_pLargeCbMem,&LargeCb_Position,inBuf + 6,*len);
		break;
	case 'L':
		res = Call_C_doCmdRecg(g_pCmdWordRec, (short*)(inBuf + 6), *len);
		break;
	case 'H':
		break;
	case 'Z':
		Call_C_InitVAD(g_pSpeechDetector);
		Call_C_doVAD(g_pSpeechDetector,inBuf + 6);
		res = Call_C_doCmdRecg(g_pCmdWordRec, (short*)(inBuf + 6), *len);
		break;
	default:
		break;
	} 

	if (res != -1)
	{
		outBuf[1] = 'R';
		outBuf[2] = res;
	}
	return 0;
}