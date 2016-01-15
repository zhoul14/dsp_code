#include "stdio.h"
//#include <iostream>
#include "math.h"

float logSumExp(float* lh, float* alpha, int n);
class CCalcGaussLh
{
private:
	int cbNum;
	int mixNum;
	int fDim;
	int* m_pIdx;
	char* mempool;
	float *invSigmaHost, *muHost, *cstHost, *alphaHost;

public:
	float *invSigmaMuHost;
	CCalcGaussLh(int cbNum, int fDim, float* invSigma, float* mu, float* cst, float* alpha, int mixNum, char* mempool, int& ptIdx);
	CCalcGaussLh();
	void init(int cbNum, int fDim, float* invSigma, float* invSigmaMu, float* mu, float* cst, float* alpha, int mixNum, char* mempool, int& ptIdx);
	void initFirst(int cbNum, int fDim, float* invSigma, float* invSigmaMu, float* mu, float* cst, float* alpha, int mixNum, char* mempool, int& ptIdx);
	void runWeightedCalc(float* features, int& fnum, float* Res);
	void runWeightedCalcDMA(float* features, int& fnum, float* Res, char* mem, long CBSize);
	float logSumExp(float* lh, float* alpha, int n);
};


class CCalcDiagGaussLh
{
private:
	int cbNum;
	int mixNum;
	float *invSigmaHost, *muHost, *cstHost, *alphaHost;
	int* m_pIdx;
	char* mempool;
	int fDim;
public:
	CCalcDiagGaussLh(int cbNum, int fDim, float* invSigma, float* mu, float* cst, float* alpha, int mixNum, char* memPool, int& ptIdx);
	CCalcDiagGaussLh();
	void init(int cbNum, int fDim, float* invSigma, float* mu, float* cst, float* alpha, int mixNum, char* memPool, int& ptIdx);
	float runWeightedCalc(float* features, int& fnum, float* Res);
	float logSumExp(float* lh, float* alpha, int n);
	float runWeightedCalcDMA(float* features, int& fnum, float* Res , char* CBMem, long CBSize);
};
