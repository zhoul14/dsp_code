#ifndef DSP_WORD_DICT_H
#define DSP_WORD_DICT_H
#include "stdio.h"
#include "Convas.h"
#include "string.h"
class DSPWordDict;
class DSPWordDict
{
public:
	int wordToCVTable[TOTAL_WORD_NUM * 2];
	int CStateTable[INITIAL_WORD_NUM * 2];
	int VStateTable[FINAL_WORD_NUM * 4];
	int VCStateTable[3 * V_CLASS_NUM * C_CLASS_NUM];
	int CClassTable[INITIAL_WORD_NUM];
	int VClassTable[FINAL_WORD_NUM];
	const char* PYBuff[TOTAL_WORD_NUM];
	int CmdSet[MAX_CMD_NUM][MAX_CMD_WORD_NUM];
	char pyBuf[6547];
	//void getCmdSetFromFIle(const char* filename);
	int getCmdSetFromBuf(const char* inBuf, int cmd_len);

	//void WordDictReadFromFile(const char* filename);
	int getWordDictFromBuf(const char* inBuf);
	void getWordCbId(int wordId, int prevWordId, int nextWordId, int* res);
	int textToWord(const char* py);
	int getUsedStateIdInAns(bool* flag, int* ansList, int ansNum);
	DSPWordDict(){

	}
	~DSPWordDict(){

	}
};
extern"C" int Call_c_GetCmdBuff(DSPWordDict* wd,char* inBuf,int dataSize);
extern"C" int Call_c_GetDictBuff(DSPWordDict* wd,char* inBuf,int dataSize);

#endif

