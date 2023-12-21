#ifndef _MAIN_H_
#define _MAIN_H_

//#define END_OF_LINE_X 1840
//#define END_OF_LINE_Y 3323

//#define END_OF_LINE_X 0
//#define END_OF_LINE_Y 0


#define START_OF_LINE_X 4000
#define START_OF_LINE_Y 3323

#define DEBUG_STRAIGHT 1

#define MAX_OCTETS 1024
#define DEBUG_COMM 0 //if 1 we are in debug mode = no server communication
#define DEBUG_MM 0
#define NB_RESSOURCES 1

#define PI 3.1415962

#define _GNU_SOURCE
#include <sched.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
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
#include "marvelmind.h"
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/poll.h>
#include <netdb.h>
#include <math.h>
//////////////////////////////////////////7
//////////UNCOMMENT ON RASPBERRY///////////
///////////////////////////////////////////
/*#include <openssl/rand.h>
#include <openssl/des.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
*/

#define CHECK_ERROR(val1, val2, msg) \
    if (val1 == val2) {               \
        perror(msg);                  \
        exit(EXIT_FAILURE);           \
    }

typedef struct Point {
  int32_t x;
  int32_t y;
  int ind;
  int ressource;
  int approcheRessource;
} Point;

//structure pour contenir toutes les données partagées
struct PARAMS{
    int portArduino;
    struct PositionValue * pos;
    struct Point currentPoint;
    int sd;
    struct sockaddr_in * server_adr;
    struct sockaddr_in * client_adr;
    struct Point * carte; //la carte
    struct Point * carte_index; //carte pour les parkings
    int nb_points; //nombre points de la carte
    struct MarvelmindHedge * hedge;
    struct Point * chemin;
    int nb_points_chemin;
    int nb_points_traversed;
    //TRAJECTOIRE
    struct Point last_goal;
    struct Point next_goal;
    int indice_next_goal;
    int reservedRessources[NB_RESSOURCES];
    int END_OF_LINE_X;
    int END_OF_LINE_Y;
};

void attendre(clock_t start, float time_in_ms);

void calculate_next_point(struct PARAMS * params);

int isNextPointAllowed(struct PARAMS * params, Point nextPoint);

void *advance(void* arg);

int setupUDP(int argc, char * argv[], struct sockaddr_in * server_adr, struct sockaddr_in * client_adr);

#endif
    
