#include "main.h"
#include "get_location.h"
#include "comm.h"
#include "traj.h"
#include "extraction_point.h"
#include "send_to_arduino.h"

void attendre(clock_t start, float time_in_ms) {
  while ((float)(clock() - start)/(float)CLOCKS_PER_SEC * 1000 < time_in_ms);
}

void* send_pos_to_server(void* arg) {
    int core_id = 1;
    clock_t start;
  
    //créer un masque d'affinité 
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);

    //l'affinité du thread
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
  
    printf("Thread \"send_pos_to_server\" lancé\n\n");
    struct PARAMS * params = (struct PARAMS*)arg;
    struct PositionValue * pos = params->pos;
    char message[MAX_OCTETS]; // Character array to store the formatted string
  
    while(1) {
      start = clock();
      message[0] = '\0';
      
      printf("\nEnvoi d'un rapport de position : X:%d Y:%d Z:%d\n", pos->x, pos->y, pos->z);
      sprintf(message, "102:%d:%d:%d", pos->x, pos->y, pos->z);

      send_data(message, *params);
      
      attendre(start, 300.0); // pauser pour 300 millisecondes
	}
    pthread_exit(NULL);
}

/*
//OLD ADVANCE FUNCTION
void* send_next_point_to_arduino(void* arg) {
    struct PARAMS * params = (struct PARAMS*)arg;
    if(params->next_goal.x == -1) {
        printf("ENTER A MISSION PLEASE!\n");
        sleep(1);
        return NULL;
    }
    struct PositionValue * pos = params->pos;
    //temporary Point struct to be able to use distance()
    struct Point actuel;
    actuel.x = pos->x;
    actuel.y = pos->y;
    //value to be communicated to the Arduino
    struct Point next;
    //EXTRAIRE LE PROCHAIN POINT DE TRAJECTOIRE
    if (distance(actuel, params->last_goal) > distance(actuel, params->next_goal)){
        params->last_goal = params->next_goal;
        params->indice_next_goal ++;
        params->next_goal = params->chemin[params->indice_next_goal];
    }
    
    //ENVOYER À L'ARDUINO
    int serialPort;
    
    if ((serialPort = serialOpen("/dev/ttyS0", 9600)) < 0) {
        fprintf(stderr, "Unable to open serial device: %s\n", strerror(errno));
        return 1;
    }

    while (1) {
        // Send the numbers to the Arduino
        serialPrintf(serialPort, "%d %d %d %d\n", pos->x, pos->y, params->next_goal.x, params->next_goal.y);

        // Wait for the Arduino to process and respond
        usleep(100000);

        // Read and print the result from the Arduino
        char resp[100];
        int i = 0;
        while (serialDataAvail(serialPort) > 0) {
            printf("Result from Arduino: %c", serialGetchar(serialPort));
            resp[i] = serialGetchar(serialPort);
        }
        if (strcmp(resp, "ACK") == 0) {
                //
        } else {
            printf("ERROR IN TRANSMISSION TO ARDUINO\n");
        }
    }
}
*/

void calculate_next_point(struct PARAMS * params) {
    Point actuel = params->currentPoint;
    Point last = params->last_goal;
    Point next = params->next_goal;
    printf("current x: %d, y: %d\n", actuel.x, actuel.y);
    
    //EXTRAIRE LE PROCHAIN POINT DE TRAJECTOIRE
    if (distance(actuel, last) > distance(actuel, next)){
        params->last_goal = next;
        printf("next goal x: %d, y: %d\n",next.x,next.y);
        
        params->indice_next_goal ++;
        params->next_goal = params->chemin[params->indice_next_goal];
        printf("found next goal in params\n");
    }
    else {
        //point rests the same
        //printf("next goal hasn't changed\n");
        printf("next x: %d, y: %d\n", next.x, next.y);

        //start = clock();
        //attendre(start, 1000);
    }
        
}

void *advance(void* arg) {
    struct PARAMS * params = (struct PARAMS*)arg;

    if (params->currentPoint.x == 0 && params->currentPoint.y == 0) {
		return;
	}

    while(params->next_goal.x == -1) {
        //envoyer code 106 au serveur == j'ai pas de mission
        printf("ENTER A MISSION PLEASE!\n");
        sleep(1);
    }
    while (1) {
        printf("\n---NEXT ITERATION---\n");

        //printf("current x: %d, y: %d\n",params->pos->x,params->pos->y);
        //put next point in params->next_goal
        calculate_next_point(params);
        
        send_next_point_to_arduino(params->portArduino, params->next_goal, params->currentPoint);
        //wait for arduino to do a loop
        //sleep(5);
    }
    
}


int setupUDP(int argc, char * argv[], struct sockaddr_in * server_adr, struct sockaddr_in * client_adr) {
    int sd;                    // Descripteur de la socket
    int erreur;                // Gestion des erreurs
    socklen_t len_addr = sizeof(*client_adr);
    char recv_buff[MAX_OCTETS+1];
    char * ptr;
    char * client_adr_str;
    int code;
    int nb_tries = 0;
    char hostbuffer[MAX_OCTETS];
    struct hostent *host_entry;
    if (argc < 3){
      printf("Utilisation : ./client.exe <IP Serveur> <Port Serveur>");
      exit(EXIT_FAILURE);
    }

    // Récupération de l'adresse IP de la machine

    // To retrieve hostname
    gethostname(hostbuffer, sizeof(hostbuffer));
    strcat(hostbuffer, ".local");
    // To retrieve host information
    host_entry = gethostbyname(hostbuffer);
    client_adr_str = inet_ntoa(*((struct in_addr*) host_entry->h_addr_list[0]));
    printf("STARTING CLIENT ON %s\n", client_adr_str);
    
    sd = socket(AF_INET, SOCK_DGRAM, 0); // Créer une socket UDP
    CHECK_ERROR(sd, -1, "Erreur lors de la création de la socket\n");
    printf("Numéro de la socket : %d\n", sd);

    server_adr->sin_family = AF_INET;
    server_adr->sin_port = htons(atoi(argv[2]));
    server_adr->sin_addr.s_addr = inet_addr(argv[1]);

    client_adr->sin_family = AF_INET;
    client_adr->sin_port = htons(0);
    client_adr->sin_addr.s_addr = inet_addr(client_adr_str);

    erreur = bind(sd,(const struct sockaddr *)client_adr, len_addr);
    CHECK_ERROR(erreur, -1, "Erreur lors du bind de socket\n");

    //Enregistrement auprès du serveur
    printf("Connexion au serveur : %s:%d\n", inet_ntoa(server_adr->sin_addr), atoi(argv[2]));
    while(1) {
        printf("Enregistrement auprès du serveur...\n");
        // Envoi code 101
        sendto(sd, "101", 4, 0, (const struct sockaddr *)server_adr, sizeof(*server_adr));
        recvfrom(sd, recv_buff, MAX_OCTETS+1, 0, NULL, NULL);
        printf("Réponse reçue: %s\n", recv_buff);
        ptr = strtok(recv_buff, ":");
        code = atoi(ptr);
        if (code==201 || code==401) break;
      
        nb_tries++;
        if (nb_tries >= 3) {
          printf("Impossible de se connecter au serveur\n");
          exit(EXIT_FAILURE);
        }
        sleep(1);
    }
    return sd;
}

int main(int argc, char *argv[]) {
    
    
    pthread_t thread_id_send_pos_to_server;
    pthread_t thread_id_get_location;
    pthread_t thread_id_receive;
    pthread_t thread_id_advance;
    
    //initialiser params
    struct PARAMS *params = (struct PARAMS *)malloc(sizeof(struct PARAMS));
    
    struct sockaddr_in server_adr, client_adr; // Structure d'adresses serveur et client
    struct PositionValue * pos = (struct PositionValue *)malloc(sizeof(struct PositionValue)); // dummy starting pos
    /*DOIT ETRE DECOMMENTER SUR RASPERRY POUR UTILISER OPENSSL
    *SSL_load_error_strings();
    *SSL_library_init();
    */
    int port = open_comm_arduino();
    //int port = serial_ouvert(); version de yann
    int sd;
    struct MarvelmindHedge * hedge;
    
    //bloquer threads de communication et de marvelmind en mode debug
    if (DEBUG != 1) {
        sd = setupUDP(argc, argv, &server_adr, &client_adr);
    } else if (DEBUG_MM != 1) {
        hedge = setupHedge(argc, argv);
    } else {
        sd = 0; //just to suppress warnings
    }
       
    
    params->portArduino = port;
    
    params->next_goal.x = -1;
    params->nb_points = -1;
    params->hedge = hedge;
    
    params->sd = sd;
    params->client_adr = &client_adr;
    params->server_adr = &server_adr;
    
    pos->x = 0;
    pos->y = 0;
    pos->z = 0;
    params->pos = pos;
    
    Point current;
    current.x = 0;
    current.y = 0;
    params->currentPoint = current;

    //charger carte en params
    if (extract_points(params) == -1) {
      printf("Impossible de récupérer les données de la carte\n");
      exit(EXIT_FAILURE);
    }
    
    //in debug mode we give the final mission directly
    if (DEBUG == 1) {
        if (argc < 2) {
            printf("DEBUG MODE: ENTER X AND Y OF MISSION PLEASE!\n");
        }
        struct Point * mission = parse_point(argv[1],argv[2]);
        Trajectory(params, *mission);

    }
  
    //bloquer threads de communication en mode debug
    if (DEBUG != 1) {
        //recevoir continuellement des messages depuis le serveur
        if (pthread_create(&thread_id_receive, NULL, receive_data, (void*)params) != 0) {
            perror("pthread_create");
            return 1;
        }
        //envoyer position au serveur
        if (pthread_create(&thread_id_send_pos_to_server, NULL, send_pos_to_server, (void*)params) != 0) {
            perror("pthread_create");
            return 1;
        }
    }
    
    //recevoir la localisation des marvelminds continuellement
    if (pthread_create(&thread_id_get_location, NULL, get_location, (void*)params) != 0) {
        perror("pthread_create");
        return 1;
    }

    //avancer à l'aide de l'Arduino
    if (pthread_create(&thread_id_advance, NULL, advance, (void*)params) != 0) {
        perror("pthread_create");
        return 1;
    }

	//close threads
    pthread_join(thread_id_send_pos_to_server, NULL);
    pthread_join(thread_id_get_location, NULL);
    pthread_join(thread_id_receive, NULL);
    pthread_join(thread_id_advance, NULL);
    
    

    close_comm_arduino(port);
    
    //ending position
    printf("La DERNIÈRE position du mobile est X: %d mm, Y: %d mm, Z: %d mm\n",
        params->pos->x, params->pos->y, params->pos->z);
    return EXIT_SUCCESS;
}
