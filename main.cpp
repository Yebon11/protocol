#include "mbed.h"
#include "string.h"
#include "L2_FSMmain.h"
#include "L3_FSMmain.h"

//serial port interface
Serial pc(USBTX, USBRX);

//GLOBAL variables (DO NOT TOUCH!) ------------------------------------------

//source/destination ID
uint8_t input_thisId=1;
uint8_t input_destId=60;

//FSM operation implementation ------------------------------------------------
int main(void){

    //initialization
    pc.printf("\n------------------ protocol stack starts! --------------------------\n");
        //source & destination ID setting
    pc.printf(":: ID for this node : ");
    pc.scanf("%d", &input_thisId);
    pc.printf("%i", input_thisId);
    pc.printf("\n:: ID for the destination(Arbitrator) : 1\n");
    /*pc.scanf("%d", &input_destId);
    pc.getc();*/
    //input_destId = 1;

    pc.printf("endnode : %i, dest : %i\n", input_thisId, input_destId);
    
    

    //initialize lower layer stacks
    L2_initFSM(input_thisId);
    L3_initFSM(input_destId);
    
    while(1)
    {
        L2_FSMrun();
        L3_FSMrun();
    }
}