#include "DSPStateTraceBin.h"



void DSPStateTraceBin::addStateTrace(DSPStateTrace* s) {
	if (s == NULL) {
	}
	content[contentSize++] = s;
}

void DSPStateTraceBin::addPreviousBin(DSPStateTraceBin* b) {
	if (b == NULL) {
	}
	previous[previousSize++]= b;
}

DSPStateTraceBin::DSPStateTraceBin(int cbidx,char* memp, int & mpidx) {
	this->cbidx = cbidx;
	contentSize = 0;
	m_pMpIdx = &mpidx;
	m_pMemPool = memp;
	previousSize = 0;
}

DSPStateTrace* DSPStateTraceBin::getBestTrace() {
	return contentSize == 0 ? NULL : content[0];
}



int DSPStateTraceBin::stateTraceNum() {
	return contentSize;
}

void DSPStateTraceBin::init(int cbidx,char* memp, int & mpidx) 
{
	this->cbidx = cbidx;
	contentSize = 0;
	m_pMpIdx = &mpidx;
	m_pMemPool = memp;
	previousSize = 0;
}
