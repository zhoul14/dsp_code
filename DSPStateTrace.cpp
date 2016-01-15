#include "DSPStateTrace.h"


int DSPStateTrace::getEnterTime() const {
	return this->enterTime;
}

int DSPStateTrace::getCodebookIndex() const {
	return this->cbidx;
}

DSPStateTrace* DSPStateTrace::getPrevious() const {
	return this->prev;
}

void DSPStateTrace::set(int enterTime, int cbidx, float lh, int refcnt, DSPStateTrace* prev, int wordId) {
	this->enterTime = enterTime;
	this->cbidx = cbidx;
	this->lh = lh;
	this->refcnt = refcnt;
	this->prev = prev;
	this->wordId = wordId;
}

int DSPStateTrace::getWordId() const {
	return wordId;
}

float destoryST(DSPStateTrace** st){
	if ((*st)->refcnt == 0) {
		DSPStateTrace* prev = (*st)->getPrevious();
		//free st;
		*st = NULL;
		if (prev != NULL) {
			prev->refcnt--;
			destoryST(&prev);
		}
	}
}
