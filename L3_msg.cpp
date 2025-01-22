#include "mbed.h"
#include "L3_msg.h"

//encode Demand SDU
uint8_t L3_MSG_encode_DEMAND(uint8_t* msg_data, uint8_t* data, int len)
{
    msg_data[L3_MSG_OFFSET_TYPE] = L3_MSG_TYPE_DEMAND;
    msg_data[L3_MSG_OFFSET_LEN] = len;
    memcpy(&msg_data[L3_MSG_OFFSET_DATA], data, len*sizeof(uint8_t));
    return len+L3_MSG_OFFSET_DATA;
}

//encode Answer SDU
uint8_t L3_MSG_encode_ANSWER(uint8_t* msg_data, uint8_t* data, int len)
{
    msg_data[L3_MSG_OFFSET_TYPE] = L3_MSG_TYPE_ANSWER;
    msg_data[L3_MSG_OFFSET_LEN] = len;
    memcpy(&msg_data[L3_MSG_OFFSET_DATA], data, len*sizeof(uint8_t));
    
    return len+L3_MSG_OFFSET_DATA;
}

uint8_t* L3_MSG_getWord(uint8_t* msg_data)
{
    return &msg_data[L3_MSG_OFFSET_DATA];
}


// Check Pdu 
//1) quizStart
int L3_checkIfquizStartPdu(uint8_t* msg)
{
    return (msg[L3_MSG_OFFSET_TYPE] == L3_MSG_TYPE_QUIZSTART);
} 
//2) particonfirmed
int L3_checkIfpartiConfirmedPdu(uint8_t* msg)
{
    return (msg[L3_MSG_OFFSET_TYPE] == L3_MSG_TYPE_PARTICONFIRMED);
} 
//3) collectComplete 
int L3_checkIfcollectCompletePdu(uint8_t* msg)
{
    return (msg[L3_MSG_OFFSET_TYPE] == L3_MSG_TYPE_COLLECTCOMPLETE);
}
//4) question 
int L3_checkIfquestionPdu(uint8_t* msg)
{
    return (msg[L3_MSG_OFFSET_TYPE] == L3_MSG_TYPE_QUESTION);
}
//5) total
int L3_checkIftotalPdu(uint8_t* msg)
{
    return (msg[L3_MSG_OFFSET_TYPE] == L3_MSG_TYPE_TOTAL);
}
//5) noAnswer
int L3_checkIfnoAnswerPdu(uint8_t* msg)
{
    return (msg[L3_MSG_OFFSET_TYPE] == L3_MSG_TYPE_NOANSWER);
}