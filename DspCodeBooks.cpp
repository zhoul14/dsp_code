#include "DspCODEBOOKS.h"



float DSPGMMCodebookSet::init(const char* initBuf)
{
	const char* p = initBuf;
	memcpy(&cbType, p, sizeof(int));
	p += sizeof(int);

	memcpy(&MixNum, p, sizeof(int));
	p += sizeof(int);
	FDim = 45;
	CodebookNum = 3206;


	int L = SigmaL();
	Alpha = (float*)p;
	Mu = Alpha + MixNum * CodebookNum;
	InvSigma = Mu + MixNum * CodebookNum * FDim;
	LogDetSigma = InvSigma + CodebookNum * L;
	
	return 1;
}

DSPGMMCodebook DSPGMMCodebookSet:: getCodebook(int num)
{
	int L = SigmaL();
	DSPGMMCodebook cb(MixNum, FDim, cbType);
	cb.Mu = Mu + num * MixNum * FDim;
	cb.InvSigma = InvSigma + num * L;
	cb.Alpha = Alpha + num * MixNum; 
	return cb;
}
extern "C"float Call_c_GetCodeSetBuff(DSPGMMCodebookSet* cb,char* inBuf)
{
	return	cb->init(inBuf);
}
