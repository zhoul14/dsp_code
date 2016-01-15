#include "CalCGaussLh.h"
#include "string.h"
CCalcGaussLh::CCalcGaussLh(int cbNum, int fDim, float* invSigma, float* mu, float* cst, float* alpha, int mixNum, char* memPool, int& ptIdx)
{
	this->cbNum = cbNum;
	this->fDim = fDim;
	this->mixNum = mixNum;
	this->mempool = memPool;
	this->m_pIdx = &ptIdx;
	alphaHost = alpha;

	int invSigmaLen = fDim * (fDim + 1) / 2;
	invSigmaHost = invSigma;
		
	/* = (float*)(memPool + ptIdx);
	ptIdx += invSigmaLen * cbNum * sizeof(float);
	for (int i = 0; i < cbNum; i++) {
		float* invSigmaPtr = invSigma + fDim * fDim * i;
		float* invSigmaCompressPtr = invSigmaHost + invSigmaLen * i;
		for (int j = 0; j < fDim; j++) {
			for (int k = 0; k < j; k++) {
				invSigmaCompressPtr[j * (j + 1) / 2 + k] = invSigmaPtr[j * fDim + k];
			}
			invSigmaCompressPtr[j * (j + 1) / 2 + j] = invSigmaPtr[j * fDim + j] / 2;
		}
	}*/

	muHost = mu;

	invSigmaMuHost = (float*)(memPool + ptIdx);
	ptIdx += fDim *cbNum * sizeof(float);

	for (int k = 0; k < cbNum; k++) {
		float* r = invSigmaMuHost + k * fDim;
		float* v = muHost + k * fDim;
		float* A = invSigmaHost + k * invSigmaLen;
		for (int i = 0; i < fDim; i++) {
			float t = 0;
			for (int j = 0; j < i; j++)
				t += A[i * (i + 1) / 2 + j] * v[j];

			for (int j = i + 1; j < fDim; j++) 
				t += A[j * (j + 1) / 2 + i] * v[j];

			t += A[i * (i + 1) / 2 + i] * 2 * v[i];
			r[i] = t;
		}
	}

	cstHost = cst;

	for (int k = 0; k < cbNum; k++) {
		float t = 0;
		float* v1 = invSigmaMuHost + k * fDim;
		float* v2 = muHost + k * fDim;
		for (int i = 0; i < fDim; i++) {
			t += v1[i] * v2[i];
		}
		cstHost[k] += t / 2;
	}
}

void CCalcGaussLh::initFirst(int cbNum, int fDim, float* invSigma, float* invSigmaMu, float* mu, float* cst, float* alpha, int mixNum, char* memPool, int& ptIdx)
{
	this->cbNum = cbNum;
	this->fDim = fDim;
	this->mixNum = mixNum;
	this->mempool = memPool;
	this->m_pIdx = &ptIdx;
	alphaHost = alpha;

	int invSigmaLen = fDim * (fDim + 1) / 2;
	invSigmaHost = invSigma;
	muHost = mu;

	invSigmaMuHost = invSigmaMu;

	for (int k = 0; k < cbNum; k++) {
		float* r = invSigmaMuHost + k * fDim;
		float* v = muHost + k * fDim;
		float* A = invSigmaHost + k * invSigmaLen;
		for (int i = 0; i < fDim; i++) {
			float t = 0;
			for (int j = 0; j < i; j++)
				t += A[i * (i + 1) / 2 + j] * v[j];

			for (int j = i + 1; j < fDim; j++) 
				t += A[j * (j + 1) / 2 + i] * v[j];

			t += A[i * (i + 1) / 2 + i] * 2 * v[i];
			r[i] = t;
		}
	}

	cstHost = cst;

	for (int k = 0; k < cbNum; k++) {
		float t = 0;
		float* v1 = invSigmaMuHost + k * fDim;
		float* v2 = muHost + k * fDim;
		for (int i = 0; i < fDim; i++) {
			t += v1[i] * v2[i];
		}
		cstHost[k] += t / 2;
	}
}

void CCalcGaussLh::init(int cbNum, int fDim, float* invSigma, float* invSigmaMu, float* mu, float* cst, float* alpha, int mixNum, char* memPool, int& ptIdx)
{
	this->cbNum = cbNum;
	this->fDim = fDim;
	this->mixNum = mixNum;
	this->mempool = memPool;
	this->m_pIdx = &ptIdx;
	alphaHost = alpha;

	int invSigmaLen = fDim * (fDim + 1) / 2;
	invSigmaHost = invSigma;
	muHost = mu;

	this->invSigmaMuHost = invSigmaMu;/*= (float*)(memPool + ptIdx);
	ptIdx += fDim *cbNum * sizeof(float);

	for (int k = 0; k < cbNum; k++) {
		float* r = invSigmaMuHost + k * fDim;
		float* v = muHost + k * fDim;
		float* A = invSigmaHost + k * invSigmaLen;
		for (int i = 0; i < fDim; i++) {
			float t = 0;
			for (int j = 0; j < i; j++)
				t += A[i * (i + 1) / 2 + j] * v[j];

			for (int j = i + 1; j < fDim; j++) 
				t += A[j * (j + 1) / 2 + i] * v[j];

			t += A[i * (i + 1) / 2 + i] * 2 * v[i];
			r[i] = t;
		}
	}
*/
	cstHost = cst;
/*
	for (int k = 0; k < cbNum; k++) {
		float t = 0;
		float* v1 = invSigmaMuHost + k * fDim;
		float* v2 = muHost + k * fDim;
		for (int i = 0; i < fDim; i++) {
			t += v1[i] * v2[i];
		}
		cstHost[k] += t / 2;
	}*/
}


void CCalcGaussLh::runWeightedCalc(float* features, int& fnum, float* Res)
{
	if((int)((mempool + *m_pIdx))%4)
		*m_pIdx += 4 - (int)((mempool + *m_pIdx))%4;
	if (alphaHost == NULL) {
		return;
	}
	int invSigmaLen = fDim * (fDim + 1) / 2;
	float* sepLh = (float*)(mempool + *m_pIdx);
	for (int i = 0; i < fnum; i++) {
		for (int j = 0; j < cbNum / mixNum; j++) {
			for (int k = 0; k < mixNum; k++) {
				int c = j * mixNum + k;
				float res = cstHost[c];
				int idx = 0;
				for (int l = 0; l < fDim; l++) {
					float rest = 0;
					for (int m = 0; m <= l; m++) {
						rest += invSigmaHost[c * invSigmaLen + idx++] * features[i * fDim + m];
					}
					rest -= invSigmaMuHost[c * fDim + l];
						res += rest * features[i * fDim + l];
				}
				sepLh[k] = -res;
			}
			Res[i * cbNum / mixNum + j] = logSumExp(sepLh, alphaHost + j * mixNum, mixNum);
		}
	}
}


void CCalcGaussLh::runWeightedCalcDMA(float* features, int& fnum, float* Res, char* CBMem, long CBSize)
{
	if((int)((mempool + *m_pIdx))%4)
		*m_pIdx += 4 - (int)((mempool + *m_pIdx))%4;

	int invSigmaLen = fDim * (fDim + 1) / 2;
	float* sepLh = (float*)(mempool + *m_pIdx);
	*m_pIdx += sizeof(float) * mixNum;
	float featuresBuf[45];
	float *alpha, *mu, *InvSigma, *cst,*invSigmaMu;

	for (int j = 0; j < cbNum / mixNum; j++) {

		alpha = (float*)(CBMem + CBSize * j);
		cst = alpha + mixNum;
		mu = cst + mixNum;
		InvSigma = mu + mixNum * 45;
		invSigmaMu = InvSigma + mixNum * invSigmaLen;

		for (int i = 0; i < fnum; i++) {
			memcpy(featuresBuf,features + i * fDim, sizeof(float) * fDim); 
			for (int k = 0; k < mixNum; k++) {
				int c = k;
				float res = cst[k];
				int idx = 0;
				for (int l = 0; l < fDim; l++) {
					float rest = 0;
					for (int m = 0; m <= l; m++) {
						rest += InvSigma[k * invSigmaLen + idx++] * featuresBuf[m];
					}
					rest -= invSigmaMu[k * fDim + l];
					res += rest * featuresBuf[l];
				}
				sepLh[k] = -res;
			}
			Res[i + j * fnum] = logSumExp(sepLh, alpha, mixNum);
		}
	}
}


float CCalcGaussLh::logSumExp(float* lh, float* alpha, int n) {
	int maxIdx = -1;
	float maxVal = 0;
	for (int i = 0; i < n; i++) {
		if (maxIdx == -1 || lh[i] > maxVal) {
			maxIdx = i;
			maxVal = lh[i];
		}
	}

	float sum = 0;
	for (int i = 0; i < n; i++) {
		sum += alpha[i] * exp(lh[i] - maxVal);
	}
	float res = maxVal + log(sum);
	return res;
}
float CCalcDiagGaussLh::logSumExp(float* lh, float* alpha, int n) {
		int maxIdx = -1;
	float maxVal = 0;
	for (int i = 0; i < n; i++) {
		if (maxIdx == -1 || lh[i] > maxVal) {
			maxIdx = i;
			maxVal = lh[i];
		}
	}

	float sum = 0;
	for (int i = 0; i < n; i++) {
		sum += alpha[i] * exp(lh[i] - maxVal);
	}
	float res = maxVal + log(sum);
	return res;
}

CCalcDiagGaussLh::CCalcDiagGaussLh(int cbNum, int fDim, float* invSigma, float* mu, float* cst, float* alpha, int mixNum, char* memPool, int& ptIdx)
{
	this->cbNum = cbNum;
	this->fDim = fDim;
	this->mixNum = mixNum;
	this->mempool = memPool;
	this->m_pIdx = &ptIdx;

	alphaHost = alpha;
	muHost = mu;
	cstHost = cst;
	invSigmaHost = invSigma;

}

void CCalcDiagGaussLh::init(int cbNum, int fDim, float* invSigma, float* mu, float* cst, float* alpha, int mixNum, char* memPool, int& ptIdx)
{
	this->cbNum = cbNum;
	this->fDim = fDim;
	this->mixNum = mixNum;
	this->mempool = memPool;
	this->m_pIdx = &ptIdx;

	alphaHost = alpha;
	muHost = mu;
	cstHost = cst;
	invSigmaHost = invSigma;

}

 


float CCalcDiagGaussLh::runWeightedCalc(float* features, int& fnum, float* Res)
{
	if((int)((mempool + *m_pIdx))%4)
		*m_pIdx += 4 - (int)((mempool + *m_pIdx))%4;
	
	float* sepLh = (float*)(mempool + *m_pIdx);
	*m_pIdx += sizeof(float) * mixNum;

	/*float* featuresBuf = (float*)(mempool + *m_pIdx);
	*m_pIdx += sizeof(float) * fDim;*/
	float featuresBuf[45];
	//float reslh,xt; int c;
	for (int i = 0; i < fnum; i++) {

		memcpy(featuresBuf,features + i * fDim, sizeof(float) * fDim);

		for (int j = 0; j < cbNum / mixNum; j++) {
			for (int k = 0; k < mixNum; k++) {
				float reslh = 0;
				int c = (j * mixNum + k);
				for (int l = 0; l < fDim; l++) {
					float xt = featuresBuf[l] - muHost[c * fDim + l];
					//float xt = features[i* fDim +l] - muHost[c * fDim + l];
					reslh += xt * xt * invSigmaHost[c* fDim + l];
				}
				reslh = -(cstHost[c] + reslh/2);
				sepLh[k] = reslh;
			}				
			Res[i * cbNum / mixNum + j] = logSumExp(sepLh, alphaHost + j * mixNum, mixNum);
		}
	}
	return 0;
}


float CCalcDiagGaussLh::runWeightedCalcDMA(float* features, int& fnum, float* Res , char* CBMem, long CBSize)
{
	if((int)((mempool + *m_pIdx))%4)
		*m_pIdx += 4 - (int)((mempool + *m_pIdx))%4;	

	float* sepLh = (float*)(mempool + *m_pIdx);
	*m_pIdx += sizeof(float) * mixNum;
	float featuresBuf[45];
	float *alpha, *mu, *InvSigma, *cst;
	//float reslh,xt; int c;
	
		//memcpy(featuresBuf,features + i * fDim, sizeof(float) * fDim); 
	for (int j = 0; j < cbNum / mixNum; j++) {
			alpha = (float*)(CBMem + CBSize * j);
			cst = alpha + mixNum;
			mu = cst + mixNum;
			InvSigma = mu + mixNum * 45;
		for (int i = 0; i < fnum; i++) {
			memcpy(featuresBuf,features + i * fDim, sizeof(float) * fDim); 
			for (int k = 0; k < mixNum; k++) {
				float reslh = 0;
				for (int l = 0; l < 45; l++) {
					float xt = featuresBuf[l] - mu[k * 45 + l];
					//float xt = features[i* fDim +l] - mu[k * 45 + l];
					reslh += xt * xt * InvSigma[k * 45 + l];
				}
				reslh = -(cst[k] + reslh/2);
				sepLh[k] = reslh;
			}				
			Res[i + j * fnum] = logSumExp(sepLh, alpha, mixNum);
		}
	}
	return 0.0;
}
