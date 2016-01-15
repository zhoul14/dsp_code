/* 
 * Copyright (c) 2012, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */
/*
 *  ======== videnc_copy.c ========
 *  Video encoder "copy" algorithm.
 *
 *  This file contains an implementation of the deprecated IVIDENC interface
 *  defined by XDM.
 */

/* This define must precede inclusion of any xdc header files */
#define Registry_CURDESC ti_sdo_ce_examples_codecs_videnc_copy_desc

#include <xdc/std.h>
#include <xdc/runtime/Assert.h>
#include <xdc/runtime/Diags.h>
#include <xdc/runtime/Log.h>
#include <xdc/runtime/Registry.h>

#include <string.h>

#include <ti/xdais/dm/ividenc.h>

#include <ti/sdo/ce/global/CESettings.h>

#include "videnc_copy_ti.h"
#include "videnc_copy_ti_priv.h"
#include <ti/sdo/fc/acpy3/acpy3.h>

/* buffer definitions */
#define MININBUFS       1
#define MINOUTBUFS      1
#define MININBUFSIZE    1
#define MINOUTBUFSIZE   1


/* ---------------zhoulu define----------------*/
struct DSPWordDict* g_pWordict;
struct DSPGMMCodebookSet* g_pCodeBookSet;
struct DSPGMMCodebookSet* g_pPreCodeBookSet;
struct DSPCmdWordRecg* g_pCmdWordRec;
struct CSPEECH_DETECTION* g_pSpeechDetector;
struct DSPGMMCalc* g_pGMMCalc;
#define SIZE_OF_WORD_DICT 60000
#define SIZE_OF_CODEBOOK 2048
#define SIZE_OF_CMDRECG 2048
#define SIZE_OF_DETECTOR 716536
IDMA3_Handle g_handle;

int g_nStatus = 0;//status
int SizeOfLargeCB;
int SizeOfPreCB;
char* g_pLargeCbMem = NULL;//Large codebookset memory pointer
char* g_pPreCbMem = NULL;//Large codebookset memory pointer
char* g_pMemHead = NULL;	//Program Class memory pointer
char* g_pDARAM_MEM = NULL; //DARAM MEM

int MemPosition;	//Program Class memory offset 
int LargeCb_Position;//large codebookset memory pointer offset
int PreCb_Position;
long (*pfDoRecg)(void *pInfo,short *SpeechBuf,long DataNum);

extern float Call_C_doTest(struct DSPGMMCalc* gbc, float* features, int fnumm);
extern float Call_C_doTestDMA(struct DSPGMMCalc* gbc, float* features, int fnum,char* mem);

extern int Call_c_GetCmdBuff(struct DSPWordDict* wd,char* inBuf,int dataSize);
extern int Call_c_GetDictBuff(struct DSPWordDict* wd,char* inBuf,int dataSize);
extern float Call_c_GetCodeSetBuff(struct DSPGMMCodebookSet* cb,char* inBuf);
extern void Call_C_InitVAD(struct CSPEECH_DETECTION* detector);
extern long Call_C_doVAD(struct CSPEECH_DETECTION* detector,char* inBuf,int len,long (*pfDoRecg)(void *pInfo,short *SpeechBuf,int DataNum),void* recClass);
extern float Call_C_doCmdRecg(struct DSPCmdWordRecg* rec,short* sample, int sampleNum);
extern long Call_C_doCmdRecg2(struct DSPCmdWordRecg* rec,short* sample, int sampleNum);

extern float Call_C_InitCmdRecg(struct DSPCmdWordRecg* rec, struct DSPWordDict* Dict, struct DSPGMMCodebookSet* Set,struct DSPGMMCodebookSet* pSet, char* memPool, int* MPidx, char* daram, void* DMA_handle);


int DspModule(char* inBuf, char* outBuf);
void runOver();

void DMA_start(void* handle);
void DMA_wait(void* handle);
int DMA_complete(void* handle);
int DMA_setParams(void* handle,void* Params, void* src, void* dst, long bufferSize);


/*****************************************************************************/
int DMA_setParams(void* handle,void* Params, void* src, void* dst, long bufferSize)
{
	int nRes = -6;
	Uint32 temp_src = (Uint32)src; Uint32 temp_dst = (Uint32)dst;
	
	if(((unsigned int)src >= 0x10800000u)&&((unsigned int)src <= 0x10840000u))
	{
		temp_src += 0x30000000u;

	}

	if(((unsigned int)dst >= 0x10800000u)&&((unsigned int)dst <= 0x10840000u))
	{
		temp_dst = temp_dst + 0x30000000u;
		/*temp_src =  (char*)src + 0x30000000u;*/
		nRes += 6;
	}
	ACPY3_Params *params = (ACPY3_Params*)Params;
	params->transferType = ACPY3_1D1D;
    params->srcAddr      = (void*)temp_src;
    params->dstAddr      = (void*)temp_dst;
    params->elementSize  = bufferSize;
    params->numElements  = 1;
    params->waitId       = 0;
    params->numFrames    = 1;
    ACPY3_configure((IDMA3_Handle)handle, params, 0);
	return nRes;
}

void DMA_start(void* handle)
{

	 ACPY3_start((IDMA3_Handle)handle);

            /* wait for transfer to finish  */
           // ACPY3_wait(videncObj->dmaHandle1D1D8B);
}


void DMA_wait(void* handle)
{
	ACPY3_wait((IDMA3_Handle)handle);
}

int DMA_complete(void* handle)
{
	return ACPY3_complete((IDMA3_Handle)handle);
}
void runOver()
{
	//MemPosition = 0;
	//LargeCb_Position = 0;
}
void runOnce(char* ProgramMemHead, char* DataMemHead, char* DaramMem, IDMA3_Handle handle);
void runOnce(char* ProgramMemHead, char* DataMemHead, char* DaramMem, IDMA3_Handle handle)
{
	if(g_pMemHead)return;
	g_pMemHead = ProgramMemHead;
	g_pPreCbMem = DataMemHead;
	g_pLargeCbMem = DataMemHead + 1024 *2048 *5;
	MemPosition = 0;
	
	g_pSpeechDetector = (struct CSPEECH_DETECTION* ) (g_pMemHead + MemPosition);
	MemPosition += SIZE_OF_DETECTOR;	

	g_pWordict =(struct DSPWordDict* ) (g_pMemHead + MemPosition);
	MemPosition += SIZE_OF_WORD_DICT;

	g_pCodeBookSet = (struct DSPGMMCodebookSet* ) (g_pMemHead + MemPosition);
	MemPosition += SIZE_OF_CODEBOOK;

    g_pPreCodeBookSet = (struct DSPGMMCodebookSet* ) (g_pMemHead + MemPosition);
	MemPosition += SIZE_OF_CODEBOOK;

	g_pCmdWordRec = (struct DSPCmdWordRecg* ) (g_pMemHead + MemPosition);
	MemPosition += SIZE_OF_CMDRECG;	

	LargeCb_Position = 0;
	PreCb_Position = 0;

	g_pDARAM_MEM = DaramMem;

	Call_C_InitVAD(g_pSpeechDetector);

	g_handle = handle;
	g_nStatus = 49;
}
float mergeLargeCb(char* memBegin, int *ptr, char* inBuf, int len);

float mergePreCb(char* memBegin, int *ptr, char* inBuf, int len);
/*int doVadRecg(char * inBuf,int len);
int doVadRecg(char * inBuf,int len)
{
	long FrameLen = g_pSpeechDetector->m_FrameLen;
	if(len != FrameLen)
		return -4;
	if(Call_C_doVAD(g_pSpeechDetector,inBuf) == 3)
	{
		short *pVADSpeechBuf	= g_pSpeechDetector->m_SpeechBuffer;
		long VADDataNum			= g_pSpeechDetector->m_SpeechFrameNum*g_pSpeechDetector->m_FrameLen;
		
		return Call_C_doCmdRecg(g_pCmdWordRec, pVADSpeechBuf, VADDataNum);
	}
	else
	{
		return -3;
	}
}*/
int matchLargeCb();
int matchLargecb()
{
	int i =0,temp =0,y=0;
	for(i =0 ;i < SizeOfLargeCB/4;i++)
	{
		memcpy(&y,&g_pLargeCbMem[i*4],4);
		temp = temp ^ y;
	}
	return temp;
}

float mergeLargeCb(char* memBegin, int *ptr, char* inBuf, int len)
{
	
	float test,test1;int i =0;
	unsigned int testptr;
	if (LargeCb_Position + len <= SizeOfLargeCB)
	{
		memcpy(&test,&inBuf[8+64*4*4],4);
		memcpy((char*)(g_pLargeCbMem+ LargeCb_Position),inBuf,len);
		memcpy(&test1,&g_pLargeCbMem[8+64*4*4+LargeCb_Position],sizeof(float));	
		LargeCb_Position += len;
		
		
		if (*ptr == SizeOfLargeCB)
		{
			
			Call_c_GetCodeSetBuff(g_pCodeBookSet,g_pLargeCbMem);
		    return Call_C_InitCmdRecg(g_pCmdWordRec, g_pWordict, g_pCodeBookSet, g_pPreCodeBookSet, g_pMemHead, &MemPosition, g_pDARAM_MEM, g_handle);
			return (int)((long)g_pGMMCalc - 0x80000000);
			//  matchLargecb();
		}

		return test1;
		
	}
	else
	{
		return 999;
	}
}

float mergePreCb(char* memBegin, int *ptr, char* inBuf, int len)
{
	
	float test,test1;int i =0;
	unsigned int testptr;
	if (PreCb_Position + len <= SizeOfPreCB)
	{
		memcpy(&test,&inBuf[8+64*4*4],4);
		memcpy((char*)(g_pPreCbMem+ PreCb_Position),inBuf,len);
		memcpy(&test1,&g_pPreCbMem[8+64*4*4+PreCb_Position],sizeof(float));	
		PreCb_Position += len;
		
		
		if (*ptr == SizeOfPreCB)
		{
			
			Call_c_GetCodeSetBuff(g_pPreCodeBookSet,g_pPreCbMem);
			return PreCb_Position;
			
		}

		return test1;
		
	}
	else
	{
		return PreCb_Position + len;
	}
}


int DspModule(char* inBuf, char* outBuf)
{
	static int flag = 0;

	if (flag == 0)
	{
		outBuf[0] = 49;
		memcpy(outBuf + 4, &g_pMemHead,sizeof(char*)); 
		memcpy(outBuf + 8, &g_pDARAM_MEM,sizeof(char*)); 
		flag = 1;
	}
	int res = -5;
	float fRes =0.0;

	if((g_nStatus + 1)%100 != inBuf[0])
		return res;

	g_nStatus = inBuf[0] + 1;
	outBuf[0] = g_nStatus;
	
	int len;
	memcpy(&len,inBuf + 2,4);


	switch (inBuf[1])
	{
	case 'C'://ARM cmd word
		res = Call_c_GetCmdBuff(g_pWordict, inBuf + 8, len);
		outBuf[1] = 'C';
		outBuf[2] = '1';
		memcpy(outBuf + 4, &res,sizeof(int));
		return 0;
		break;
	case 'D'://
		res = Call_c_GetDictBuff(g_pWordict, inBuf + 8, len);
		outBuf[1] = 'D';
		outBuf[2] = '1';
		memcpy(outBuf + 4, &res,sizeof(int));
		return 0;
		break;
	case 'E':
		if (!g_pLargeCbMem)
		{
			return -1;
		}
		break;
	case 'F':
		memcpy(&SizeOfLargeCB,inBuf + 8,sizeof(int));
		LargeCb_Position = 0;
		outBuf[1] = 'F';
		outBuf[2] = '1';
		memcpy(outBuf + 4, &MemPosition,sizeof(char*));
		//memcpy(outBuf + 8, &SizeOfLargeCB,sizeof(int));
		return 0;
		break;
	case 'Q':
		memcpy(&SizeOfPreCB,inBuf + 8,sizeof(int));
		PreCb_Position = 0;
		outBuf[1] = 'Q';
		outBuf[2] = '1';
		memcpy(outBuf + 4, &MemPosition,sizeof(char*));
		//memcpy(outBuf + 8, &SizeOfLargeCB,sizeof(int));
		return 0;
		break;
	case 'P':
		fRes = mergePreCb(g_pPreCbMem,&PreCb_Position,inBuf + 8,len);


		outBuf[1] = 'P';
		outBuf[2] = '1';
		memcpy(outBuf + 4, &fRes,sizeof(float));
		return 0;
		break;
	case 'G':
		fRes = mergeLargeCb(g_pLargeCbMem,&LargeCb_Position,inBuf + 8,len);
		outBuf[1] = 'G';
		outBuf[2] = '1';
		memcpy(outBuf + 4, &fRes,sizeof(char*));
		memcpy(outBuf + 8, &fRes,sizeof(float));
		return 0;
		break;
	case 'L':
		res = Call_C_doCmdRecg2(g_pCmdWordRec, (short*)(inBuf + 8), len);
		memcpy(&outBuf[2],&res,4);
		return 0;
		break;
	case 'H':
		break;
	case 'Z':
		 res = Call_C_doVAD(g_pSpeechDetector, inBuf+8, len/2, Call_C_doCmdRecg2, g_pCmdWordRec);
		break;
	case 'T'://test
			fRes = Call_C_doTest(g_pGMMCalc,  (float*)(inBuf+8), len);
			outBuf[1] = 'S';
			memcpy(&outBuf[2],&fRes,4);
			return 0;
	case 't'://test dma
			fRes = Call_C_doTestDMA(g_pGMMCalc, (float*)(inBuf+8), len, g_pDARAM_MEM);
			outBuf[1] = 's';
			memcpy(&outBuf[2],&fRes,4);
			return 0;
	default:
		break;
	} 

	if(res>=0)
	{
		outBuf[1] = 'R';
		memcpy(&outBuf[2],&res,4);
	}
	else
	{	outBuf[1] = 'N';
		memcpy(&outBuf[2],&res,4);
	}
	return 0;
}
/***************************zhoulu func end****************************/


#define IALGFXNS  \
    &VIDENCCOPY_TI_IALG,        /* module ID */                         \
    VIDENCCOPY_TI_activate,     /* activate */                          \
    VIDENCCOPY_TI_alloc,        /* alloc */                             \
    NULL,                       /* control (NULL => no control ops) */  \
    VIDENCCOPY_TI_deactivate,   /* deactivate */                        \
    VIDENCCOPY_TI_free,         /* free */                              \
    VIDENCCOPY_TI_initObj,      /* init */                              \
    NULL,                       /* moved */                             \
    NULL                        /* numAlloc (NULL => IALG_MAXMEMRECS) */

/*
 *  ======== VIDENCCOPY_TI_IVIDENC ========
 *  This structure defines TI's implementation of the IVIDENC interface
 *  for the VIDENCCOPY_TI module.
 */
IVIDENC_Fxns VIDENCCOPY_TI_VIDENCCOPY = {    /* module_vendor_interface */
    {IALGFXNS},
    VIDENCCOPY_TI_process,
    VIDENCCOPY_TI_control,
};

/*
 *  ======== VIDENCCOPY_TI_IALG ========
 *  This structure defines TI's implementation of the IALG interface
 *  for the VIDENCCOPY_TI module.
 */
#ifdef __TI_COMPILER_VERSION__
/* satisfy XDAIS symbol requirement without any overhead */
#if defined(__TI_ELFABI__) || defined(__TI_EABI_SUPPORT__)

/* Symbol doesn't have any leading underscores */
asm("VIDENCCOPY_TI_IALG .set VIDENCCOPY_TI_VIDENCCOPY");

#else

/* Symbol has a single leading underscore */
asm("_VIDENCCOPY_TI_IALG .set _VIDENCCOPY_TI_VIDENCCOPY");

#endif
#else

/*
 *  We duplicate the structure here to allow this code to be compiled and
 *  run non-DSP platforms at the expense of unnecessary data space
 *  consumed by the definition below.
 */
IALG_Fxns VIDENCCOPY_TI_IALG = {      /* module_vendor_interface */
    IALGFXNS
};

#endif

/* Logging information */
#define MODNAME "ti.sdo.ce.examples.codecs.videnc_copy"

Registry_Desc ti_sdo_ce_examples_codecs_videnc_copy_desc;

static Int regInit = 0;     /* Registry_addModule() called */


#ifdef USE_ACPY3

/* Implementation of IDMA3 interface functions & IDMA3_Fxns vtable */
#include <ti/xdais/idma3.h>
#include <ti/sdo/fc/acpy3/acpy3.h>

#define NUM_LOGICAL_CH 1

/*
 *  ======== VIDENCCOPY_TI_dmaChangeChannels ========
 *  Update instance object with new logical channel.
 */
Void VIDENCCOPY_TI_dmaChangeChannels(IALG_Handle handle,
    IDMA3_ChannelRec dmaTab[])
{
    VIDENCCOPY_TI_Obj *videncObj = (Void *)handle;

    Log_print2(Diags_ENTRY, "[+E] VIDENCCOPY_TI_dmaChangeChannels(0x%x, 0x%x)",
            (IArg)handle, (IArg)dmaTab);

    videncObj->dmaHandle1D1D8B = dmaTab[0].handle;
}


/*
 *  ======== VIDENCCOPY_TI_dmaGetChannelCnt ========
 *  Return max number of logical channels requested.
 */
Uns VIDENCCOPY_TI_dmaGetChannelCnt(Void)
{
    Log_print0(Diags_ENTRY, "[+E] VIDENCCOPY_TI_dmaGetChannelCnt()");

    return (NUM_LOGICAL_CH);
}


/*
 *  ======== VIDENCCOPY_TI_dmaGetChannels ========
 *  Declare DMA resource requirement/holdings.
 */
Uns VIDENCCOPY_TI_dmaGetChannels(IALG_Handle handle, IDMA3_ChannelRec dmaTab[])
{
    VIDENCCOPY_TI_Obj *videncObj = (Void *)handle;
    int i;

    Log_print2(Diags_ENTRY, "[+E] VIDENCCOPY_TI_dmaGetChannels(0x%x, 0x%x)",
            (IArg)handle, (IArg)dmaTab);

    /* Initial values on logical channels */
    dmaTab[0].handle = videncObj->dmaHandle1D1D8B;

    dmaTab[0].numTransfers = 1;
    dmaTab[0].numWaits = 1;

    /*
     * Request logical DMA channels for use with ACPY3
     * AND with environment size obtained from ACPY3 implementation
     * AND with low priority.
     */
    for (i = 0; i < NUM_LOGICAL_CH; i++) {
        dmaTab[i].priority = IDMA3_PRIORITY_LOW;
        dmaTab[i].persistent = FALSE;
        dmaTab[i].protocol = &ACPY3_PROTOCOL;
    }

    return (NUM_LOGICAL_CH);
}


/*
 *  ======== VIDENCCOPY_TI_dmaInit ========
 *  Initialize instance object with granted logical channel.
 */
Int VIDENCCOPY_TI_dmaInit(IALG_Handle handle, IDMA3_ChannelRec dmaTab[])
{
    VIDENCCOPY_TI_Obj *videncObj = (Void *)handle;

    Log_print2(Diags_ENTRY, "[+E] VIDENCCOPY_TI_dmaInit(0x%x, 0x%x)",
            (IArg)handle, (IArg)dmaTab);

    videncObj->dmaHandle1D1D8B = dmaTab[0].handle;

    return (IALG_EOK);
}


/*
 *  ======== VIDENCCOPY_TI_IDMA3 ========
 *  This structure defines TI's implementation of the IDMA3 interface
 *  for the VIDENCCOPY_TI module.
 */
IDMA3_Fxns VIDENCCOPY_TI_IDMA3 = {      /* module_vendor_interface */
    &VIDENCCOPY_TI_IALG,              /* IALG functions */
    VIDENCCOPY_TI_dmaChangeChannels,  /* ChangeChannels */
    VIDENCCOPY_TI_dmaGetChannelCnt,   /* GetChannelCnt */
    VIDENCCOPY_TI_dmaGetChannels,     /* GetChannels */
    VIDENCCOPY_TI_dmaInit             /* initialize logical channels */
};

#endif

/*
 *  ======== VIDENCCOPY_TI_activate ========
 */
Void VIDENCCOPY_TI_activate(IALG_Handle handle)
{
    Log_print1(Diags_ENTRY, "[+E] VIDENCCOPY_TI_activate(0x%x)", (IArg)handle);
}


/*
 *  ======== VIDENCCOPY_TI_deactivate ========
 */
Void VIDENCCOPY_TI_deactivate(IALG_Handle handle)
{
    Log_print1(Diags_ENTRY, "[+E] VIDENCCOPY_TI_deactivate(0x%x)",
            (IArg)handle);
}


/*
 *  ======== VIDENCCOPY_TI_alloc ========
 */
Int VIDENCCOPY_TI_alloc(const IALG_Params *algParams,
    IALG_Fxns **pf, IALG_MemRec memTab[])
{
    Registry_Result   result;


    /*
     *  No need to reference count for Registry_addModule(), since there
     *  is no way to remove the module.
     */
    if (regInit == 0) {
        /* Register this module for logging */
        result = Registry_addModule(&ti_sdo_ce_examples_codecs_videnc_copy_desc,
                MODNAME);
        Assert_isTrue(result == Registry_SUCCESS, (Assert_Id)NULL);

        if (result == Registry_SUCCESS) {
            /* Set the diags mask to the CE default */
            CESettings_init();
            CESettings_setDiags(MODNAME);
        }
        regInit = 1;
    }

    Log_print3(Diags_ENTRY, "[+E] VIDENCCOPY_TI_alloc(0x%x, 0x%x, 0x%x)",
            (IArg)algParams, (IArg)pf, (IArg)memTab);

    /* Request memory for my object */
    memTab[0].size = sizeof(VIDENCCOPY_TI_Obj);
    memTab[0].alignment = 0;
    memTab[0].space = IALG_EXTERNAL;
    memTab[0].attrs = IALG_PERSIST;

	// ADD BY ZHENGHANG
   	memTab[1].size = sizeof(char)*25000 * 1024;
    memTab[1].alignment = 0;
    memTab[1].space = IALG_EXTERNAL;//IALG_DARAM0;
    memTab[1].attrs = IALG_PERSIST;

	// ADD BY ZHOULU	
	memTab[2].size = sizeof(char) * 150000 * 1024;
	memTab[2].alignment = 0;
	memTab[2].space = IALG_EXTERNAL;
	memTab[2].attrs = IALG_PERSIST;

	memTab[3].size = sizeof(char) * 127*1024;
	memTab[3].alignment = 0;
	memTab[3].space = IALG_DARAM0;
	memTab[3].attrs = IALG_PERSIST;

//IALG_DARAM0;
    return (4);
}


/*
 *  ======== VIDENCCOPY_TI_free ========
 */
Int VIDENCCOPY_TI_free(IALG_Handle handle, IALG_MemRec memTab[])
{
    Log_print2(Diags_ENTRY, "[+E] VIDENCCOPY_TI_free(0x%lx, 0x%lx)",
            (IArg)handle, (IArg)memTab);

	VIDENCCOPY_TI_Obj *VIDENC_COPY = (Void*)handle;
	//VIDENCCOPY_TI_alloc(NULL,NULL,memTab);
    VIDENCCOPY_TI_alloc(NULL, NULL, memTab);

	memTab[1].base = VIDENC_COPY->p_data;
	memTab[1].size = sizeof(char) * 25000 * 1024;

	memTab[2].base = VIDENC_COPY->p_ddr_data;
	memTab[2].size = sizeof(char) * 150000* 1024;

	memTab[3].base = VIDENC_COPY->p_DARAM0;
	memTab[3].size = sizeof(char) * 127*1024;

    return (4);
}


/*
 *  ======== VIDENCCOPY_TI_initObj ========
 */
Int VIDENCCOPY_TI_initObj(IALG_Handle handle,
    const IALG_MemRec memTab[], IALG_Handle p,
    const IALG_Params *algParams)
{
    Log_print4(Diags_ENTRY,
            "[+E] VIDENCCOPY_TI_initObj(0x%x, 0x%x, 0x%x, 0x%x)",
            (IArg)handle, (IArg)memTab, (IArg)p, (IArg)algParams);

	VIDENCCOPY_TI_Obj *VIDENC_COPY = (Void*)handle;
	VIDENC_COPY->p_data = memTab[1].base;
	VIDENC_COPY->p_ddr_data = memTab[2].base;
	VIDENC_COPY->p_DARAM0 = memTab[3].base;
    return (IALG_EOK);
}


/*
 *  ======== VIDENCCOPY_TI_process ========
 */
XDAS_Int32 VIDENCCOPY_TI_process(IVIDENC_Handle h, XDM_BufDesc *inBufs,XDM_BufDesc *outBufs, IVIDENC_InArgs *inArgs, IVIDENC_OutArgs *outArgs)
{
    XDAS_Int32 curBuf;
    XDAS_UInt32 minSamples;

#ifdef USE_ACPY3
    const Uint32 maxTransferChunkSize       = 0xffff;
    Uint32       thisTransferChunkSize      = 0x0;
    Uint32       remainingTransferChunkSize;
    Uint32       thisTransferSrcAddr, thisTransferDstAddr;

    ACPY3_Params params;
    VIDENCCOPY_TI_Obj *videncObj = (VIDENCCOPY_TI_Obj *)h;
#endif

    Log_print5(Diags_ENTRY, "[+E] VIDENCCOPY_TI_process(0x%x, 0x%x, 0x%x, "
            "0x%x, 0x%x)",
            (IArg)h, (IArg)inBufs, (IArg)outBufs, (IArg)inArgs, (IArg)outArgs);

    /* validate arguments - this codec only supports "base" xDM. */
    if ((inArgs->size != sizeof(*inArgs)) ||
        (outArgs->size != sizeof(*outArgs))) {

        Log_print2(Diags_ENTRY,
                "[+E] VIDENCCOPY_TI_process, unsupported size (0x%x, 0x%x)",
                (IArg)(inArgs->size), (IArg)(outArgs->size));

        return (IVIDENC_EFAIL);
    }

#ifdef USE_ACPY3
    /*
     * Activate Channel  scratch DMA channels.
     */
    //ACPY3_activate(videncObj->dmaHandle1D1D8B);
#endif

    /* outArgs->bytesGenerated reports the total number of bytes generated */
    outArgs->bytesGenerated = 0;

    /*
     * A couple constraints for this simple "copy" codec:
     *    - Video encoding presumes a single input buffer, so only one input
     *      buffer will be encoded, regardless of inBufs->numBufs.
     *    - Given a different size of an input and output buffers, only
     *      encode (i.e., copy) the lesser of the sizes.
     */



    for (curBuf = 0; (curBuf < inBufs->numBufs) &&
        (curBuf < outBufs->numBufs); curBuf++) {

        /* there's an available in and out buffer, how many samples? */
        minSamples = inBufs->bufSizes[curBuf] < outBufs->bufSizes[curBuf] ?
            inBufs->bufSizes[curBuf] : outBufs->bufSizes[curBuf];

#ifdef USE_ACPY3

#if 0
        thisTransferSrcAddr        = (Uint32)inBufs->bufs[curBuf];
        thisTransferDstAddr        = (Uint32)outBufs->bufs[curBuf];
        remainingTransferChunkSize = minSamples;

        while (remainingTransferChunkSize > 0) {

            if (remainingTransferChunkSize > maxTransferChunkSize) {
               thisTransferChunkSize = maxTransferChunkSize;
            }
            else {
               thisTransferChunkSize = remainingTransferChunkSize;
            }

            /* Configure the logical channel */
            params.transferType = ACPY3_1D1D;
            params.srcAddr      = (void *)thisTransferSrcAddr;
            params.dstAddr      = (void *)thisTransferDstAddr;
            params.elementSize  = thisTransferChunkSize;
            params.numElements  = 1;
            params.waitId       = 0;
            params.numFrames    = 1;

            remainingTransferChunkSize -= thisTransferChunkSize;
            thisTransferSrcAddr += thisTransferChunkSize;
            thisTransferDstAddr += thisTransferChunkSize;

            /* Configure logical dma channel */
            ACPY3_configure(videncObj->dmaHandle1D1D8B, &params, 0);

            /* Use DMA to copy data */
            ACPY3_start(videncObj->dmaHandle1D1D8B);

            /* wait for transfer to finish  */
            ACPY3_wait(videncObj->dmaHandle1D1D8B);
        }
        Log_print1(Diags_USER2, "[+2] VIDENCCOPY_TI_process> "
                "ACPY3 Processed %d bytes.", (IArg)minSamples);
#endif
#else
        Log_print3(Diags_USER2, "[+2] VIDENCCOPY_TI_process> "
                "memcpy (0x%x, 0x%x, %d)",
                (IArg)(outBufs->bufs[curBuf]), (IArg)(inBufs->bufs[curBuf]),
                (IArg)minSamples);

        /* process the data: read input, produce output */
        memcpy(outBufs->bufs[curBuf], inBufs->bufs[curBuf], minSamples);
#endif

        outArgs->bytesGenerated += minSamples;
    }


#ifdef USE_ACPY3
    /*
     * Deactivate Channel  scratch DMA channels.
     */
    //ACPY3_deactivate(videncObj->dmaHandle1D1D8B);

	//videncObj->p_data //avi mem

	runOnce(videncObj->p_data, videncObj->p_ddr_data, videncObj->p_DARAM0, videncObj->dmaHandle1D1D8B);
	DspModule(inBufs->bufs[0], outBufs->bufs[0]);

	//memcpy(outBufs->bufs[0] + 8,&(videncObj->p_DARAM0),4);
	runOver();

#endif


	

    /* Fill out the rest of the outArgs struct */
    outArgs->extendedError = 0;
    outArgs->encodedFrameType = 0;    /* TODO */
    outArgs->inputFrameSkip = IVIDEO_FRAME_ENCODED;
    outArgs->reconBufs.numBufs = 0;   /* important: indicate no reconBufs */
    return (IVIDENC_EOK);
}


/*
 *  ======== VIDENCCOPY_TI_control ========
 */
XDAS_Int32 VIDENCCOPY_TI_control(IVIDENC_Handle handle, IVIDENC_Cmd id,
    IVIDENC_DynamicParams *params, IVIDENC_Status *status)
{
    XDAS_Int32 retVal;

    Log_print4(Diags_ENTRY, "[+E] VIDENCCOPY_TI_control(0x%x, 0x%x, 0x%x, "
            "0x%x)", (IArg)handle, (IArg)id, (IArg)params, (IArg)status);

    /* validate arguments - this codec only supports "base" xDM. */
    if ((params->size != sizeof(*params)) ||
        (status->size != sizeof(*status))) {

        Log_print2(Diags_ENTRY,
                "[+E] VIDENCCOPY_TI_control, unsupported size (0x%x, 0x%x)",
                (IArg)(params->size), (IArg)(status->size));

        return (IVIDENC_EFAIL);
    }

    switch (id) {
        case XDM_GETSTATUS:
        case XDM_GETBUFINFO:
            status->extendedError = 0;

            status->bufInfo.minNumInBufs = MININBUFS;
            status->bufInfo.minNumOutBufs = MINOUTBUFS;
            status->bufInfo.minInBufSize[0] = MININBUFSIZE;
            status->bufInfo.minOutBufSize[0] = MINOUTBUFSIZE;

            retVal = IVIDENC_EOK;

            break;

        case XDM_SETPARAMS:
        case XDM_SETDEFAULT:
        case XDM_RESET:
        case XDM_FLUSH:
            /* TODO - for now just return success. */

            retVal = IVIDENC_EOK;
            break;

        default:
            /* unsupported cmd */
            retVal = IVIDENC_EFAIL;

            break;
    }

    return (retVal);
}
/*
 *  @(#) ti.sdo.ce.examples.codecs.videnc_copy; 1, 0, 0,1; 2-24-2012 19:29:31; /db/atree/library/trees/ce/ce-t06/src/ xlibrary

 */

