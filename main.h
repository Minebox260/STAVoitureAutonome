#ifndef _MAIN_H_
#define _MAIN_H_

#define MAX_OCTETS 1024
#define DEBUG 1 //if 1 we are in debug mode = no server communication

#define _GNU_SOURCE
#include <sched.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "wiringPi.h"
#include "wiringSerial.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <netinet/in.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include "marvelmind.h"
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/poll.h>
//////////////////////////////////////////7
//////////UNCOMMENT ON RASPBERRY///////////
///////////////////////////////////////////
//#include <openssl/rand.h>
//#include <openssl/des.h>
//#include <openssl/evp.h>
//#include <openssl/err.h>
//#include <openssl/ssl.h>
//#include <openssl/x509.h>


#define CHECK_ERROR(val1, val2, msg) \
    if (val1 == val2) {               \
        perror(msg);                  \
        exit(EXIT_FAILURE);           \
    }

typedef struct Point {
  int x;
  int y;
} Point;

//arg pour fonction envoyer_donnees
struct PARAMS{
    int portArduino;
    struct PositionValue * pos;
    struct Point currentPoint;
    int sd;
    struct sockaddr_in * server_adr;
    struct sockaddr_in * client_adr;
    struct Point * carte; //la carte
    int nb_points; //nombre points de la carte
    struct MarvelmindHedge * hedge;
    struct Point * chemin;
    //TRAJECTOIRE
    struct Point last_goal;
    struct Point next_goal;
    int indice_next_goal;
};

void attendre(clock_t start, float time_in_ms);

void calculate_next_point(struct PARAMS * params);

void *advance(void* arg);

int setupUDP(int argc, char * argv[], struct sockaddr_in * server_adr, struct sockaddr_in * client_adr);

#endif
