
#include "get_location.h"

/////////SHIT I DONT UNDERSTAND//////////
/////////////////////////////////////////
/////////////////////////////////////////
bool terminateProgram=false;
void CtrlHandler(int signum)
{
    terminateProgram=true;
}
// Linux
static sem_t *sem;
struct timespec ts;
void semCallback()
{
    sem_post(sem);
}
/////////SHIT I DONT UNDERSTAND//////////
/////////////////////////////////////////
/////////////////////////////////////////

struct MarvelmindHedge * setupHedge(int argc, char * argv[]) {
    // get port name from command line arguments (if specified)
    printf("\nSETTING UP HEDGE\n\n");
    const char * ttyFileName;
    if (argc == 3) {
        ttyFileName = "/dev/ttyACM0";
    }
    else {
        printf("PROVIDE MISSIONX, MISSIONY\n");
    }

    // Init
    struct MarvelmindHedge * hedge = createMarvelmindHedge();
    if (hedge==NULL)
    {
        puts ("Error: Unable to create MarvelmindHedge");
        exit(EXIT_FAILURE);
    }
    hedge->ttyFileName=ttyFileName;
    hedge->verbose=true; // show errors and warnings
    hedge->anyInputPacketCallback= semCallback;
    startMarvelmindHedge(hedge);
  
    sem = sem_open(DATA_INPUT_SEMAPHORE, O_CREAT, 0777, 0);
    
    printf("\nSETUP HEDGE FINISHED\n\n");

    /*//sleep (3);
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
    {
        printf("clock_gettime error");
        return -1;
    }
    ts.tv_sec += 2;
    sem_timedwait(sem,&ts);
    */
    return hedge;
}

void * get_location (void* arg) {
    struct PARAMS * params = (struct PARAMS*)arg;
    struct PositionValue * mobPos = (struct PositionValue*)malloc(sizeof(struct PositionValue)); //position du mobile
    mobPos->x = 0;
    mobPos->y = 0;
    //clock_t start;
  
    while(1) {
        //start = clock();
        
        //debug mode without marvelminds
        if (DEBUG_MM != 1) {
            getPositionFromMarvelmindHedge(params->hedge, mobPos);
            printf("\npos MM x: %d, pos y: %d\n",mobPos->x,mobPos->y);
            //free(params->pos);
            /*
            if (mobPos->x == 0 && mobPos->y == 0) {
                printf("pos == 0\n");
                usleep(500 * 1000); //waiting to ask again
                continue;
            
            }*/
            params->pos = mobPos;
            params->currentPoint.x = mobPos->x;
            params->currentPoint.y = mobPos->y;
        } else {
            mobPos->x += 1;
            mobPos->y += 1;
            params->pos = mobPos;
            params->currentPoint.x = mobPos->x;
            params->currentPoint.y = mobPos->y;
        }

        //50 milliseconds
        usleep(50 * 1000);
    }
}
