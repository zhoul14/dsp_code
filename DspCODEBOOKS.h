#ifndef DSP_GMM_CODEBOOKS_H
#define DSP_GMM_CODEBOOKS_H
#include "stdio.h"
#include "string.h"
#include "Convas.h"

struct DSPGMMCodebook {

	int FDim, MixNum;
	float*	Mu;//[FEATURE_DIM * MIXTURE_NUM];
	float*	InvSigma;//[FEATURE_DIM * FEATURE_DIM * MIXTURE_NUM];
	float* Alpha; 
	int cbType;
	DSPGMMCodebook(const DSPGMMCodebook& g)
	{

		cbType = g.cbType;
		FDim = g.FDim;
		MixNum = g.MixNum;
		Mu = g.Mu;
		InvSigma = g.InvSigma;
		Alpha = g.Alpha;

	}
	DSPGMMCodebook(int& mixnum, int& fdim, int& cbtype)
	{
		cbType = cbtype;
		MixNum = mixnum;
		FDim =fdim;
	}
	~DSPGMMCodebook()
	{
	}
};

class DSPGMMCodebookSet
{
public:
	int CodebookNum;

	int FDim;

	int MixNum;

	int cbType;

	float* Alpha;

	float* Mu;

	float* InvSigma;

	float* LogDetSigma;	//det(Sigma)

	static const int CB_TYPE_DIAG = 0;

	static const int CB_TYPE_FULL_RANK = 1;

	static const int CB_TYPE_FULL_RANK_SHARE = 2;

	DSPGMMCodebookSet(){}

	float init(const char* initFile);


	int getFDim() const{
		return FEATURE_DIM;
	}

	int getCodebookNum() const
	{
		return CodebookNum;
	}

	int getMixNum() const{
		return MixNum;
	}

	bool isFullRank() const{
		return cbType != CB_TYPE_DIAG;
	}

	DSPGMMCodebook getCodebook(int num);

	int SigmaL() const {
		if (cbType == CB_TYPE_DIAG)
			return MixNum * FDim;

		if (cbType == CB_TYPE_FULL_RANK)
			return MixNum * FDim * (FDim + 1 )/2;

		if (cbType == CB_TYPE_FULL_RANK_SHARE)
			return FDim * FDim;

		printf("error: unrecognized cbType [%d]\n", cbType);
		//exit(-1);

		return -1;
	}
};
extern"C" float Call_c_GetCodeSetBuff(DSPGMMCodebookSet* cb,char* inBuf);

#endif
