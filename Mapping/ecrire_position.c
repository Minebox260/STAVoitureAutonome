#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h> //additional line
#if defined(WIN32) || defined(_WIN64)
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <semaphore.h>
#include <time.h>
#endif
#include "marvelmind.h"

void writePosToFile(struct PositionValue* mobPos, char * filename) {
		
    FILE* file = fopen(filename, "a"); 
    
    if (file == NULL) {
        perror("Error opening the file");
        return;
    }

    fprintf(file, "x: %ld\t", mobPos->x);
    fprintf(file, "y: %ld\t", mobPos->y);
    fprintf(file, "z: %ld\n", mobPos->z);
    
    fclose(file); 
}

bool terminateProgram=false;
void CtrlHandler(int signum)
{
    terminateProgram=true;
}

static sem_t *sem;
struct timespec ts;
void semCallback()
{
	sem_post(sem);
}


int main (int argc, char *argv[])
{
    time_t rawtime;
    struct tm *timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    char filename[100];
    strftime(filename, sizeof(filename), "carte_%Y%m%d_%H%M%S.txt", timeinfo);
    struct PositionValue mobPos;

    const char * ttyFileName;
    if (argc == 2) ttyFileName = argv[1];
    else ttyFileName = DEFAULT_TTY_FILENAME;

    struct MarvelmindHedge * hedge=createMarvelmindHedge ();
    if (hedge==NULL)
    {
        puts ("Error: Unable to create MarvelmindHedge");
        return -1;
    }
    hedge->ttyFileName=ttyFileName;
    hedge->verbose=true; 
    hedge->anyInputPacketCallback= semCallback;
    startMarvelmindHedge (hedge);

    signal (SIGINT, CtrlHandler);
    signal (SIGQUIT, CtrlHandler);

	sem = sem_open(DATA_INPUT_SEMAPHORE, O_CREAT, 0777, 0);

    while ((!terminateProgram) && (!hedge->terminationRequired))
    {
        struct timespec ts;
        if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
		{
			printf("clock_gettime error");
			return -1;
		}
		ts.tv_nsec += 250 * 1000000;
		if (ts.tv_nsec >= 1000000000) {
			ts.tv_nsec -= 1000000000;
			ts.tv_sec += 1;
		}
		
		sem_timedwait(sem,&ts);

		
        bool resultat = getPositionFromMarvelmindHedge(hedge, &mobPos);
        printf("La position actuelle du mobile est X: %ld mm, Y: %ld mm, Z: %ld mm\n",
                mobPos.x, mobPos.y, mobPos.z);
        writePosToFile(&mobPos, filename);
    }

    stopMarvelmindHedge (hedge);
    destroyMarvelmindHedge (hedge);
    return 0;
}
