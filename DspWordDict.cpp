#include "DspWordDict.h"

char * g_pWordDictBuf;
char * g_pCmdBuf;

int DSPWordDict::getWordDictFromBuf(const char* inbuf){

	DSPWordDict* wd = this;
	unsigned int offset = 0;

	memcpy(wordToCVTable,&inbuf[offset],TOTAL_WORD_NUM * 2*4);
	offset = offset + 4 * 1254 * 2;

	memcpy(CStateTable, &inbuf[offset], INITIAL_WORD_NUM * 2 * 4);	
	offset = offset + 4 * INITIAL_WORD_NUM * 2; 

	memcpy(VStateTable, &inbuf[offset], FINAL_WORD_NUM * 4 * 4);
	offset = offset + 4 * FINAL_WORD_NUM * 4;


	memcpy(VCStateTable, &inbuf[offset], 4 * 3 * V_CLASS_NUM * C_CLASS_NUM);
	offset = offset + 4 * 3 * V_CLASS_NUM * C_CLASS_NUM;

	memcpy(CClassTable, &inbuf[offset], 4 * INITIAL_WORD_NUM);
	offset = offset + 4 * INITIAL_WORD_NUM;

	memcpy(VClassTable, &inbuf[offset], FINAL_WORD_NUM * 4);
	offset = offset + 4 * FINAL_WORD_NUM;

	memcpy(pyBuf,&inbuf[offset],6547);
	offset = 0;
	for (int i = 0; i < TOTAL_WORD_NUM; i++)
	{
		wd->PYBuff[i] = pyBuf + offset;
		while (pyBuf[offset++] != 0)
		{
		}
	}
	return PYBuff[1253][0];
}

int DSPWordDict::textToWord(const char * py)
{
	DSPWordDict* wd = this;
	int idx = 0;
	while (idx < TOTAL_WORD_NUM && strcmp(py,wd->PYBuff[idx++]) != 0)
	{
	}
	if (idx > TOTAL_WORD_NUM)
	{
		return -1;
	}
	else
	{
		return idx - 1;
	}
}

void DSPWordDict::getWordCbId(int wordId, int prevWordId, int nextWordId,int* res)
{

	int cid = *(wordToCVTable + wordId *2);
	int vid = *(wordToCVTable + wordId *2 + 1);

	res[INITIAL0] = *(CStateTable + cid *2);
	res[INITIAL1] = *(CStateTable + cid *2 +1);
	res[FINAL0] = *(VStateTable + vid *4);
	res[FINAL1] = *(VStateTable + vid *4 + 1);
	res[FINAL2] = *(VStateTable + vid *4 + 2);
	res[FINAL3] = *(VStateTable + vid *4 + 3);

	int CClass = CClassTable[cid];
	int VClass = VClassTable[vid];

	if (prevWordId >= 0) {
		int prevVId = *(wordToCVTable + prevWordId * 2 + 1);
		int prevVClass = VClassTable[prevVId];
		res[INITIAL0_C] = *(VCStateTable + prevVClass * 27 * 3 + CClass *3 + 2);
	} else {	//若是第一字
		res[INITIAL0_C] = -1;
	}

	if (nextWordId >= 0) {
		int nextCId = *(wordToCVTable + nextWordId * 2);
		int nextCClass = CClassTable[nextCId];
		res[FINAL2_C] = *(VCStateTable + VClass * 27 * 3 + nextCClass *3);
		res[FINAL3_C] = *(VCStateTable + VClass * 27 * 3 + nextCClass *3 +1);
	} else {	//若是最后一字
		res[FINAL2_C] = -1;
		res[FINAL3_C] = -1;
	}

}

int DSPWordDict::getUsedStateIdInAns(bool* flag, int* ansList, int ansNum)
{
	int cbInfo[STATE_NUM_IN_WORD];
	int cnt = 0;
	for (int i = 0; i < ansNum; i++)
	{
		int prevWord = i == 0 ? NO_PREVIOUS_WORD : ansList[i - 1];
		int nextWord = i == ansNum - 1 ? NO_NEXT_WORD : ansList[i + 1];
		int word = ansList[i];
		getWordCbId(word, prevWord, nextWord, cbInfo);
		for (int j = 0; j < STATE_NUM_IN_WORD; j++) {
			if (cbInfo[j] != -1) {
				flag[cbInfo[j]] = true;
			}
		}
	}
	flag[NOISE_ID] = 1;
	cnt++;
	return 2000;
}

int DSPWordDict::getCmdSetFromBuf(const char* inBuf, int cmd_len)
{
	DSPWordDict* wd = this;
	int j = 0, k = 0, cnt = 0, k0,sum = 0;
	char buff[20];

	for (int i = 0 ; i < MAX_CMD_NUM; i++)
	{
		for(int j = 0; j < MAX_CMD_WORD_NUM ;j++)
			wd->CmdSet[i][j] = -2;
	}
	for (int i = 0; i < MAX_CMD_NUM; i++)
	{
		j = 0;
		while (inBuf[k] != '\n')
		{

			k0 = k;
			cnt = 0;
			while (inBuf[k++] != ' ')
			{
				if (k >= cmd_len) 
				{
					return sum;
				}
				cnt++;
				if (inBuf[k] == '\r')
				{
					k++;
					break;
				}
			}
			memcpy(buff, inBuf + k0, cnt);
			buff[cnt] = 0;
			wd->CmdSet[i][j++] = textToWord(buff);
			sum++;
		}
		k++;
	}
	return wd->CmdSet[52][3];
}


extern"C" int Call_c_GetCmdBuff(DSPWordDict* wd,char* inBuf,int dataSize)
{
	return wd->getCmdSetFromBuf(inBuf,dataSize);
}
extern"C" int Call_c_GetDictBuff(DSPWordDict* wd,char* inBuf,int dataSize)
{
	return wd->getWordDictFromBuf(inBuf);
}
