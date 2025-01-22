#include "mbed.h"
#include "L3_FSMevent.h"
#include "L3_msg.h"
#include "protocol_parameters.h"
#include "time.h"

static uint8_t rcvdMsg[L3_MAXDATASIZE];
static uint8_t rcvdSize;
static int16_t rcvdRssi;
static int8_t rcvdSnr;
static uint8_t rcvdSrcId;

//Downward primitives
//TX function
void (*L3_LLI_dataReqFunc)(uint8_t* msg, uint8_t size, uint8_t destId);
void (*L3_LLI_reconfigSrcIdReqFunc)(uint8_t myId);

//interface event : DATA_IND, RX data has arrived
void L3_LLI_dataInd(uint8_t* dataPtr, uint8_t srcId, uint8_t size, int8_t snr, int16_t rssi)
{
    debug_if(DBGMSG_L3, "\n[L3] --> DATA IND : size:%i, type : %i\n", size, dataPtr[0]);
    memcpy(rcvdMsg, dataPtr, size*sizeof(uint8_t));
    rcvdSize = size;
    rcvdSnr = snr;
    rcvdRssi = rssi;
    rcvdSrcId = srcId;

    if (L3_checkIfquizStartPdu(dataPtr))
    {
        L3_event_setEventFlag(L3_event_quizStartPduRcvd);
    }
    else if(L3_checkIfpartiConfirmedPdu(dataPtr))
    {
        L3_event_setEventFlag(L3_event_partiConfirmedPduRcvd);
    }
    else if(L3_checkIfcollectCompletePdu(dataPtr))
    {
        L3_event_setEventFlag(L3_event_collectCompletePduRcvd);
    }
    else if(L3_checkIfquestionPdu(dataPtr))
    {
        L3_event_setEventFlag(L3_event_questionPduRcvd);
    }
    else if(L3_checkIftotalPdu(dataPtr))
    {
        L3_event_setEventFlag(L3_event_totalPduRcvd);
    }
    else if(L3_checkIfnoAnswerPdu(dataPtr))
    {
        L3_event_setEventFlag(L3_event_noAnswerPduRcvd);
    }
}

void L3_LLI_dataCnf(uint8_t res)
{
    debug_if(DBGMSG_L3, "\n --> DATA CNF : res : %i\n", res);
}
void L3_LLI_reconfigSrcIdCnf(uint8_t res)
{
    debug_if(DBGMSG_L3, "\n --> RECONFIG SRCID CNF : res : %i\n", res);
}


uint8_t* L3_LLI_getMsgPtr() // 받은 메시지
{
    return rcvdMsg;
}
uint8_t L3_LLI_getSize() // 받은 사이즈
{
    return rcvdSize;
}

uint8_t L3_LLI_getSrcId() // 소스 아이디
{
    return rcvdSrcId;
}

void L3_LLI_setDataReqFunc(void (*funcPtr)(uint8_t*, uint8_t, uint8_t))
{
    L3_LLI_dataReqFunc = funcPtr;
}


void L3_LLI_setReconfigSrcIdReqFunc(void (*funcPtr)(uint8_t))
{
    L3_LLI_reconfigSrcIdReqFunc = funcPtr;
}