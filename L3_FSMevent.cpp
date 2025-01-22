#include "L3_FSMevent.h"
#include "L3_msg.h"
#include "L3_timer.h"
#include "L3_LLinterface.h"
#include "protocol_parameters.h"
#include "mbed.h"


//FSM state -------------------------------------------------
#define L3STATE_Collect_Participants      0
#define L3STATE_Question_Transmission     1
#define L3STATE_Answer_Transmission       2


//state variables
static uint8_t main_state = L3STATE_Collect_Participants; //protocol state
static uint8_t prev_state = main_state;

//parameter
uint8_t partiId[2];
uint8_t partiNum;
uint8_t srcId;

//PDU context/size
uint8_t quizStart_Pdu[200];
uint8_t collectComplete_Pdu[200];
uint8_t partiConfirmed_Pdu[200];
uint8_t noAnswer_Pdu[200];
uint8_t total_Pdu[200];
uint8_t pduSize;


//SDU (input)
static uint8_t originalWord[1030];
static uint8_t wordLen=0;

static uint8_t sdu[1030];

uint8_t* totalPtr;
uint8_t total_rcvdId;

//serial port interface
static Serial pc(USBTX, USBRX);
static uint8_t myDestId;
//uint8_t answer_id;
uint8_t* rcvdAnswer;
static uint8_t flag_quizStart = 0;
static uint8_t flag_aid = 0;

uint8_t* word;
uint8_t* question;
uint8_t* answer_id;
uint8_t answer;
uint8_t AID;

//application event handler : generating SDU from keyboard input
static void L3service_processInputWord(void)
{
    char c = pc.getc();
    pc.printf("%c", c); // 타자를 칠 때마다 화면에 표시되게 함.
    
    if (main_state == L3STATE_Collect_Participants || main_state == L3STATE_Answer_Transmission)
    {
        if (c == '\n' || c == '\r')
        {
            originalWord[wordLen++] = '\0';
            L3_event_setEventFlag(L3_event_quizStartPduSend);
            word = originalWord;
            debug_if(DBGMSG_L3, "word is ready! ::: %s\n", originalWord);
        }
        else
        {
            originalWord[wordLen++] = c;
            #if 0
            if (wordLen >= L3_MAXDATASIZE-1)
            {
                originalWord[wordLen++] = '\0';
                L3_event_setEventFlag(L3_event_dataToSend);
                pc.printf("\n max reached! word forced to be ready :::: %s\n", originalWord);
            }
            #endif
        }
    }
    else if (main_state == L3STATE_Question_Transmission)
    {
        if (c == '\n' || c == '\r')
        {
            originalWord[wordLen++] = '\0';

            if (flag_aid == 0)
            {
                answer_id = originalWord;
                L3_event_setEventFlag(L3_event_answerIDinput);
            }
            else
            {
                question = originalWord;
                L3_event_setEventFlag(L3_event_questSDUinput);
            }
            debug_if(DBGMSG_L3, "word is ready! ::: %s\n", originalWord);
        }
        else
        {
            originalWord[wordLen++] = c;
            #if 0
            if (wordLen >= L3_MAXDATASIZE-1)
            {
                originalWord[wordLen++] = '\0';
                L3_event_setEventFlag(L3_event_dataToSend);
                pc.printf("\n max reached! word forced to be ready :::: %s\n", originalWord);
            }
            #endif
        }
        
    }
}



void L3_initFSM(uint8_t destId)
{

    myDestId = destId;
    //initialize service layer
    pc.attach(&L3service_processInputWord, Serial::RxIrq);
    pc.printf("protocol starts when you press any type : "); 
}

void L3_FSMrun(void)
{  
    //initialization
    if (prev_state != main_state)
    {
        debug_if(DBGMSG_L3, "[L3] State transition from %i to %i\n", prev_state, main_state);
        prev_state = main_state;
    }

    //FSM should be implemented here! ---->>>>
    switch (main_state)
    {   
        case L3STATE_Collect_Participants:
            if (flag_quizStart == 0)
            {
               if (L3_event_checkEventFlag(L3_event_quizStartPduSend))
                {  
                    char quizStart_sdu[] = "Enter 'Y' to enjoy!!";
                    uint8_t quizStart_len = strlen(quizStart_sdu);
                    uint8_t quizStart_pduSize = L3_MSG_encode_QUIZSTART(sdu, (uint8_t*)quizStart_sdu, quizStart_len);
                    debug("[Collect Parti state] quizStartPdu transmitted (len: %i)\n", quizStart_len);
                    L3_LLI_dataReqFunc(sdu, quizStart_pduSize, 255);
                    flag_quizStart = 1;
                    wordLen = 0;
                    main_state = L3STATE_Collect_Participants;
                    L3_event_clearEventFlag(L3_event_quizStartPduSend);
                }
                break;
            }
            else if (flag_quizStart == 1)
            {   
                if (L3_event_checkEventFlag(L3_event_demandPduRcvd))
                {
                    void L3_LLI_dataInd(uint8_t* dataPtr, uint8_t srcId, uint8_t size, int8_t snr, int16_t rssi);
                    uint8_t* dataPtr = L3_LLI_getMsgPtr();
                    uint8_t* rcvdMsg = &dataPtr[L3_MSG_OFFSET_DATA];
                    uint8_t rcvdSrcId = L3_LLI_getSrcId();
                    char partiConfirmed_sdu[] = "You've become a participant!";
                    uint8_t partiConfirmed_len;
                    uint8_t partiConfirmed_pduSize;
                    debug("\n -------------------------------------------------\nRCVD MSG : %s (from %i)\n -------------------------------------------------\n", 
                                rcvdMsg, rcvdSrcId);
                    partiId[partiNum++] = rcvdSrcId;
                    partiConfirmed_len = strlen(partiConfirmed_sdu);
                    partiConfirmed_pduSize = L3_MSG_encode_PARTICONFIRMED(sdu, (uint8_t*) partiConfirmed_sdu, partiConfirmed_len);

                    debug("[L3] Sending partiConfirmedPdu.... (msg length : %i,id: %i)\n", partiConfirmed_len, rcvdSrcId);
                    L3_LLI_dataReqFunc(sdu,partiConfirmed_pduSize, rcvdSrcId);
                    flag_quizStart = 2;
                    main_state = L3STATE_Collect_Participants;

                    L3_event_clearEventFlag(L3_event_demandPduRcvd);
                }
                break;
            }
            else if(flag_quizStart == 2)
            {
                if (L3_event_checkEventFlag(L3_event_demandPduRcvd))
                {
                    void L3_LLI_dataInd(uint8_t* dataPtr, uint8_t srcId, uint8_t size, int8_t snr, int16_t rssi);
                    uint8_t* dataPtr = L3_LLI_getMsgPtr();
                    uint8_t* rcvdMsg = &dataPtr[L3_MSG_OFFSET_DATA];
                    uint8_t rcvdSrcId = L3_LLI_getSrcId();
                    debug("\n -------------------------------------------------\nRCVD MSG : %s (from %i)\n -------------------------------------------------\n", 
                                rcvdMsg, rcvdSrcId);
                        
                    partiId[partiNum++] = rcvdSrcId;

                    //partiConfirmed Pdu
                    char partiConfirmed_sdu[] = "You've become a participant!!";
                    uint8_t partiConfirmed_len = strlen(partiConfirmed_sdu);
                    uint8_t partiConfirmed_pduSize = L3_MSG_encode_PARTICONFIRMED(sdu, (uint8_t*) partiConfirmed_sdu, partiConfirmed_len);
                    pc.printf("\n",(char*)sdu,"\n");
                    debug("[L3] Sending partiConfirmedPdu.... (msg length : %i,id: %i)\n", partiConfirmed_len, rcvdSrcId);
                    debug("Input 'F' to send collect Complete: ");
                    L3_LLI_dataReqFunc(sdu,partiConfirmed_pduSize, rcvdSrcId);
                    flag_quizStart = 3;
                    main_state = L3STATE_Collect_Participants;
                    L3_event_clearEventFlag(L3_event_demandPduRcvd);
                    
                }
                break;
            }
            else if (flag_quizStart == 3)
            {
                if(*word == 'F')
                {
                    pc.printf("\n");
                    //collectComplete Pdu
                    char collectComplete_sdu[] = "Let's start!";
                    uint8_t collectComplete_len = strlen(collectComplete_sdu);
                    pc.printf("%i", collectComplete_len);
                    uint8_t collectComplete_pduSize = L3_MSG_encode_COLLECTCOMPLETE(sdu, (uint8_t*) collectComplete_sdu, collectComplete_len);
                    pc.printf("\n",(char*)sdu,"\n");
                    
                    debug("[L3] Sending collectCompletePdu.... (msg length : %i)\n", collectComplete_len);
                    L3_LLI_dataReqFunc(sdu,collectComplete_pduSize, 255);
                    flag_quizStart = 0;
                    pc.printf("INPUT Answer Id : ");

                    main_state = L3STATE_Question_Transmission;
                    wordLen = 0;
                }
                break;

            }
            break;
        case L3STATE_Question_Transmission:
            if (L3_event_checkEventFlag(L3_event_answerIDinput))
            {
                AID = (uint8_t)atoi((const char*)answer_id); // atoi 문자열을 정수로 변환

                flag_aid= 1;
                wordLen = 0;

                pc.printf("INPUT Question : ");
                main_state = L3STATE_Question_Transmission;
                L3_event_clearEventFlag(L3_event_answerIDinput);
            }
            else if (L3_event_checkEventFlag(L3_event_questSDUinput))
            {
                uint8_t question_pdu[1030];
                L3_MSG_encode_QUESTION(question_pdu, AID, wordLen, question);
                debug("[QTX state] Q transmitted - (AID:%i, len:%i) %s\n", AID, wordLen, question);
                uint8_t questSize = wordLen+4;
                L3_LLI_dataReqFunc(question_pdu, questSize, 255);
                L3_timer_startTimer(); 

                wordLen = 0;
                flag_aid = 0;
                main_state = L3STATE_Answer_Transmission;
                L3_event_clearEventFlag(L3_event_questSDUinput);
            }
            break;
        case L3STATE_Answer_Transmission:
            if(flag_aid == 0)
            {  
                if (L3_event_checkEventFlag(L3_event_answerPDURcvd))
                {
                    totalPtr = L3_LLI_getMsgPtr();
                    rcvdAnswer = L3_MSG_getWord(totalPtr);
                    total_rcvdId = L3_LLI_getSrcId();

                    if (AID == total_rcvdId)
                    {
                        L3_timer_stopTimer();
                        pc.printf("\n -------------------------------------------------\nRCVD from %i : %s\n -------------------------------------------------\n", 
                                    total_rcvdId, rcvdAnswer);

                        debug("Input 'F' to send total Pdu :");    
                        main_state = L3STATE_Answer_Transmission;
                        L3_event_clearEventFlag(L3_event_answerPDURcvd);
                        flag_aid = 1;
                    }
                    break;
                }
                else if(L3_event_checkEventFlag(L3_event_timeout))
                {

                    char noAnswer_sdu[] = "no answer!!";
                    uint8_t noAnswer_len;
                    uint8_t noAnswer_pduSize;
                    debug("\n -------------------------------------------------\nRCVD MSG : %s (AID %i)\n -------------------------------------------------\n", 
                                noAnswer_sdu, AID);
                    noAnswer_len = strlen(noAnswer_sdu);
                    noAnswer_pduSize = L3_MSG_encode_NOANSWER(sdu, (uint8_t*)noAnswer_sdu,AID, noAnswer_len);
                    L3_LLI_dataReqFunc(sdu,noAnswer_pduSize, 255);
    
                    main_state = L3STATE_Collect_Participants;
                    L3_event_clearEventFlag(L3_event_timeout);
                }
                break;

            }
            else if (flag_aid == 1)
            {   
                if(*word == 'F')//답변을 받으면 flag_aid가 1로 변함.
                {
                    // total pdu 생성 
                    uint8_t Len_total = sizeof(rcvdAnswer);
                    L3_MSG_encode_TOTAL(sdu, rcvdAnswer, total_rcvdId, Len_total);
                    pc.printf("\nAID after encode: %i\n", sdu[L3_MSG_OFFSET_AID]);
                    uint8_t totalSize = Len_total+3;
                    debug("\n[ATX state] total PDU transmitted - (AID:%i) %s\n",total_rcvdId , rcvdAnswer);
                    flag_aid = 0;
                    pc.printf("------------------ protocol stack end! ----------------------------\n");
                    L3_LLI_dataReqFunc(sdu, totalSize, 255);
                    main_state = L3STATE_Collect_Participants;
                }
                break;
                

            }
            break;
        default :
            break;
    }
}