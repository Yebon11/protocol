#include "L3_FSMevent.h"
#include "L3_msg.h"
#include "L3_timer.h"
#include "L3_LLinterface.h"
#include "protocol_parameters.h"
#include "mbed.h"


//FSM state -------------------------------------------------
#define L3STATE_ConnectArbitrator   0
#define L3STATE_QuestionReception   1
#define L3STATE_AnswerTransmission  2
#define L3STATE_AnswerReception     3


//state variables
static uint8_t main_state = L3STATE_ConnectArbitrator; //protocol state
static uint8_t prev_state = main_state;

//PDU context/size
uint8_t demand_Pdu[200];
uint8_t answer_Pdu[200];
uint8_t pduSize;
uint8_t srcId = L3_LLI_getSrcId();

static uint8_t pdu[1030];

//SDU (input)
static uint8_t originalWord[1030];
static uint8_t wordLen=0;

static uint8_t sdu[1030];

int partiConfirmed = 0;

//serial port interface
static Serial pc(USBTX, USBRX);

//parameter
static uint8_t myDestId;
//static uint8_t arbiId = 1;
static uint8_t flag = 0;
uint8_t* word;
uint8_t answerId;

uint8_t* dataPtr;
uint8_t* rcvdMsg;
uint8_t rcvdSrcId;


//application event handler : generating SDU from keyboard input
static void L3service_processInputWord(void)
{
    
    if (main_state == L3STATE_ConnectArbitrator)
    {
        char c = pc.getc();
        pc.printf("%c", c);
        if (c == '\n' || c == '\r')
            {
                originalWord[wordLen++] = '\0';
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
    else if(main_state == L3STATE_AnswerTransmission)
    {
        char c = pc.getc();
        pc.printf("%c", c);
        if (c == '\n' || c == '\r')
            {
                originalWord[wordLen++] = '\0';
                L3_event_setEventFlag(L3_event_answerInput);
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

    //pc.printf("Give a word to send : ");
}

void L3_FSMrun(void)
{   
    if (prev_state != main_state)
    {
        debug_if(DBGMSG_L3, "[L3] State transition from %i to %i\n", prev_state, main_state);
        prev_state = main_state;
    }

    //FSM should be implemented here! ---->>>>
    switch (main_state)
    {
        case L3STATE_ConnectArbitrator:

            if (L3_event_checkEventFlag(L3_event_quizStartPduRcvd) && flag == 0)
            {
                void L3_LLI_dataInd(uint8_t* dataPtr, uint8_t srcId, uint8_t size, int8_t snr, int16_t rssi);
                uint8_t* dataPtr = L3_LLI_getMsgPtr();
                uint8_t* rcvdMsg = &dataPtr[L3_MSG_OFFSET_DATA];
                uint8_t rcvdSrcId = L3_LLI_getSrcId();
                debug("\n -------------------------------------------------\nRCVD MSG : %s (from %i)\n -------------------------------------------------\n", 
                            rcvdMsg, rcvdSrcId);

                debug("Give a word to send : ");
                flag = 1;

                main_state = L3STATE_ConnectArbitrator;
                L3_event_clearEventFlag(L3_event_quizStartPduRcvd);
            }
            else if(flag == 1)
            {
                if (*word == 'Y')             
                {
                    char demand_sdu[] = "I want to enjoy QnA protocol!!";

                    uint8_t demand_len = strlen(demand_sdu);

                    strcpy((char*)sdu, (char*)demand_sdu);
                    uint8_t demand_pduSize = L3_MSG_encode_DEMAND(sdu, (uint8_t*)demand_sdu, demand_len);
                    debug("[Connect Arbitrator state] demand Pdu transmitted (len: %i)\n", demand_len);
                    L3_LLI_dataReqFunc(sdu, demand_pduSize, myDestId);

                    wordLen = 0;

                    main_state = L3STATE_ConnectArbitrator;
                    L3_event_clearEventFlag(L3_event_quizStartPduRcvd);
                    flag =2;
                }
                break;
            }
            else if(flag == 2)
            {
                if(L3_event_checkEventFlag(L3_event_partiConfirmedPduRcvd))
                {
                    dataPtr = L3_LLI_getMsgPtr();
                    rcvdMsg = &dataPtr[L3_MSG_OFFSET_DATA];
                    rcvdSrcId = L3_LLI_getSrcId();
                    debug("\n -------------------------------------------------\nRCVD MSG : %s (from %i)\n -------------------------------------------------\n", 
                            rcvdMsg, rcvdSrcId);
                    debug("\n -------------------------------------------------\nWaiting until rcvd collectCompleted Pdu from arbi\n -------------------------------------------------\n"); //수정하기

                    main_state = L3STATE_ConnectArbitrator;
                    L3_event_clearEventFlag(L3_event_partiConfirmedPduRcvd);
                    flag = 3;

                }
                break;
            }
            else if(flag == 3)
            {
                if(L3_event_checkEventFlag(L3_event_collectCompletePduRcvd))
                {
                    dataPtr = L3_LLI_getMsgPtr();
                    rcvdMsg = &dataPtr[L3_MSG_OFFSET_DATA];

                    //rcvdMsg L3_MSG_OFFSET_LEN만큼 자르기
                    uint8_t strlen = dataPtr[L3_MSG_OFFSET_LEN];
                    uint8_t collectComplete_Msg[strlen];
                    for (int i=0; i < strlen;)
                    {
                        collectComplete_Msg[i] = rcvdMsg[i];
                        i++;
                    }


                    rcvdSrcId = L3_LLI_getSrcId(); 
                    pc.printf("\n -------------------------------------------------\nRCVD MSG : %s (from %i)\n -------------------------------------------------\n", 
                                collectComplete_Msg, rcvdSrcId);
                    flag = 0;
                    main_state = L3STATE_QuestionReception;
                    L3_event_clearEventFlag(L3_event_collectCompletePduRcvd);
                }
                break;
            }
            else
                wordLen = 0;
            break;

      
        case L3STATE_QuestionReception:
            if (L3_event_checkEventFlag(L3_event_questionPduRcvd))
            {

                uint8_t* dataPtr = L3_LLI_getMsgPtr();
                uint8_t* rcvdMsg = &dataPtr[L3_MSG_OFFSET_DATA];
                uint8_t rcvdSrcId = L3_LLI_getSrcId();
                uint8_t answerId = 0;
                answerId = dataPtr[L3_MSG_OFFSET_AID];
                pc.printf("\n -------------------------------------------------\nRCVD MSG : Named Respondent ID %i \n\t\t%s (from %i)\n -------------------------------------------------\n", 
                        answerId, rcvdMsg, rcvdSrcId);
                pc.printf("Input your answer: ");
                                
                main_state = L3STATE_AnswerTransmission;
                L3_event_checkEventFlag(L3_event_questionPduRcvd);
            }
        break;
        
        case L3STATE_AnswerTransmission:
            if (L3_event_checkEventFlag(L3_event_totalPduRcvd))
            {
                uint8_t* dataPtr = L3_LLI_getMsgPtr();

                rcvdMsg =  &dataPtr[L3_MSG_OFFSET_DATA];
                uint8_t AID = dataPtr[L3_MSG_OFFSET_AID];

                uint8_t strlen = dataPtr[L3_MSG_OFFSET_LEN];
                uint8_t total_Msg[strlen];

                for (int i = 0; i< strlen;)
                {
                    total_Msg[i]=rcvdMsg[i];
                    i++;
                }


                pc.printf("\n -------------------------------------------------\nRCVD MSG(final MSG) : %s (AID: %i)\n -------------------------------------------------\n", 
                     total_Msg, AID);
                pc.printf("------------------ protocol stack end! ----------------------------\n");
        
                main_state = L3STATE_ConnectArbitrator;
                L3_event_clearEventFlag(L3_event_totalPduRcvd);
                    
            }
            else if(L3_event_checkEventFlag(L3_event_noAnswerPduRcvd))
            {
                uint8_t* dataPtr = L3_LLI_getMsgPtr();
                uint8_t* rcvdMsg = 0;
                rcvdMsg =  &dataPtr[L3_MSG_OFFSET_DATA];
                uint8_t AID = dataPtr[L3_MSG_OFFSET_AID];

                uint8_t strlen = dataPtr[L3_MSG_OFFSET_LEN];
                uint8_t noAnswer_Msg[strlen];
                for (int i=0; i<strlen;)
                {
                    noAnswer_Msg[i] = rcvdMsg[i];
                    i++;
                }


                pc.printf("\n -------------------------------------------------\nRCVD MSG(final MSG) : %s (AID: %i)\n -------------------------------------------------\n", 
                     noAnswer_Msg, AID);
                pc.printf("------------------ protocol stack end! ----------------------------\n");
                main_state = L3STATE_ConnectArbitrator;
                L3_event_clearEventFlag(L3_event_noAnswerPduRcvd);
            }
            else if (L3_event_checkEventFlag(L3_event_answerInput))
            {
                uint8_t answerSize = L3_MSG_encode_ANSWER(pdu, originalWord, wordLen);
                debug("ATX state : A transmitted - (len:%i) %s\n", wordLen, originalWord);
                //uint8_t answerSize = sizeof(pdu);
                L3_LLI_dataReqFunc(pdu, answerSize, myDestId);

                main_state = L3STATE_AnswerReception;
                L3_event_clearEventFlag(L3_event_answerInput);
            }
            break;
        break;



        case L3STATE_AnswerReception:
            if (L3_event_checkEventFlag(L3_event_totalPduRcvd))
            {
                uint8_t* dataPtr = L3_LLI_getMsgPtr();

                uint8_t AID = dataPtr[L3_MSG_OFFSET_AID];
                uint8_t* rcvdMsg = L3_MSG_getWord(dataPtr);

                pc.printf("\n -------------------------------------------------\nRCVD MSG(final MSG) : %s (AID: %i)\n -------------------------------------------------\n", 
                     rcvdMsg, AID);
                pc.printf("------------------ protocol stack end! ----------------------------\n");

                main_state = L3STATE_ConnectArbitrator;
                L3_event_clearEventFlag(L3_event_totalPduRcvd);
            }
            else if(L3_event_checkEventFlag(L3_event_noAnswerPduRcvd))
            {
                uint8_t* dataPtr = L3_LLI_getMsgPtr();
                uint8_t* rcvdMsg = 0;
                rcvdMsg =  &dataPtr[L3_MSG_OFFSET_DATA];
                uint8_t AID = dataPtr[L3_MSG_OFFSET_AID];

                pc.printf("\n -------------------------------------------------\nRCVD MSG(final MSG) : %s (AID: %i)\n -------------------------------------------------\n", 
                     rcvdMsg, AID);
                pc.printf("------------------ protocol stack end! ----------------------------\n");
                main_state = L3STATE_ConnectArbitrator;
                L3_event_clearEventFlag(L3_event_noAnswerPduRcvd);
            }
        break;
        default :
            break;
    }
}