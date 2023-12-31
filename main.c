#include "main.h"
#include "get_location.h"
#include "comm.h"
#include "traj.h"
#include "extraction_point.h"
#include "send_to_arduino.h"

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

void calculate_next_point(struct PARAMS * params) {
    Point actuel = params->currentPoint;
    //Point last = params->last_goal;
    Point next = params->next_goal;
    int dist_min = 500;

    if (params->nb_points_traversed >= params->nb_points_chemin) {
        printf("\nEND REACHED\n\n");
        exit(0);
    }

    printf("current x: %d, y: %d\n", actuel.x, actuel.y);
    //printf("last x: %d, y: %d\n", last.x, last.y);
    
    //EXTRAIRE LE PROCHAIN POINT DE TRAJECTOIRE
    //printf("Distance actuel,last: %lf\nDistance actuel,next: %lf\n",distance(actuel,last),distance(actuel,next));
    //if (distance(actuel, last) > distance(actuel, next)){
    if (distance(actuel,next) < dist_min) {
        //params->last_goal = next;
        
        params->indice_next_goal ++;
        params->nb_points_traversed += 1;
        params->next_goal = params->chemin[params->indice_next_goal];
        printf("next goal CHANGED x: %d, y: %d\n",next.x,next.y);
    }
    else {
        //point rests the same
        //printf("next goal hasn't changed\n");
        printf("next (same) x: %d, y: %d\n", next.x, next.y);

        //start = clock();
        //attendre(start, 1000);
        //test
    }
        
}

//provided one center point and two other points we calculate the angle between
//the rays going from the center point to the other points
float calculate_angle_between_rays(struct Point center, struct Point a, struct Point b) {
    int32_t vectorAX = a.x - center.x;
    int32_t vectorAY = a.y - center.y;

    int32_t vectorBX = b.x - center.x;
    int32_t vectorBY = b.y - center.y;

    //calculate dot product
    int32_t dot = vectorAX * vectorBX + vectorAY * vectorBY;

    //calculate magnitude
    float magA = sqrt(vectorAX * vectorAX + vectorAY * vectorAY);
    float magB = sqrt(vectorBX * vectorBX + vectorBY * vectorBY);

    float cosTheta = dot / (magA * magB);

    float angleDegrees = acos(cosTheta) * (180.0 / PI);

    return angleDegrees;
}

int isNextPointAllowed(struct PARAMS * params, Point nextPoint) {
    char buffer[MAX_OCTETS];
    if (nextPoint.approcheRessource != -1 && nextPoint.ressource == -1) {
        if (!params->reservedRessources[nextPoint.approcheRessource]) {
            buffer[0] = '\0';
            sprintf(buffer, "103:%d", nextPoint.approcheRessource);
            send_data(buffer, *params);
        }
        return 1;
    } else if (nextPoint.ressource != -1) {
        if (params->reservedRessources[nextPoint.ressource]) return 1;
        else {
            buffer[0] = '\0';
            sprintf(buffer, "103:%d", nextPoint.ressource);
            send_data(buffer, *params);
            return 0;
        }
    } else if (nextPoint.ressource == -1 && params->currentPoint.ressource != -1) {
        buffer[0] = '\0';
        sprintf(buffer, "104:%d", params->currentPoint.ressource);
        send_data(buffer, *params);
        return 1;
    } else return 1;
}

void *advance(void* arg) {
    struct PARAMS * params = (struct PARAMS*)arg;
    params->nb_points_traversed = 0;


    while(params->next_goal.x == -1) {
        //envoyer code 106 au serveur == j'ai pas de mission
        printf("ENTER A MISSION PLEASE!\n");
        sleep(1);
    }
    while (1) {
        
        
        if (params->currentPoint.x == 0 && params->currentPoint.y == 0) {
		    //send_stop_command(params->portArduino);
            //sleep(1);
            continue;
	    }

        printf("\n---NEXT ADVANCE ITERATION---\n");


        //printf("current x: %d, y: %d\n",params->pos->x,params->pos->y);
        //put next point in params->next_goal
        calculate_next_point(params);
        
        send_next_point_to_arduino(params, params->portArduino, params->next_goal, params->currentPoint);
        //wait for arduino to do a loop
        //sleep(5);
    }
    
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
    int sd = 0;
    struct MarvelmindHedge * hedge;
    
    //bloquer threads de communication et de marvelmind en mode debug
    if (DEBUG_COMM != 1) {
        sd = setupUDP(argc, argv, &server_adr, &client_adr);
    }
    if (DEBUG_MM != 1) {
        hedge = setupHedge(argc, argv);
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
    
    params->currentPoint.x = 0;
    params->currentPoint.y = 0;

    //this gives a seg fault
    //for (int i = 0; i < NB_RESSOURCES; i++) params->reservedRessources[i] = 0;
    
    //charger carte en params
    if (extract_points(params) == -1) {
      printf("Impossible de récupérer les données de la carte\n");
      exit(EXIT_FAILURE);
    }
    
    //recevoir la localisation des marvelminds continuellement
    if (pthread_create(&thread_id_get_location, NULL, get_location, (void*)params) != 0) {
        perror("pthread_create");
        return 1;
    }

    //in debug mode we give the final mission directly
    if (DEBUG_COMM == 1) {
        if (argc < 2) {
            printf("DEBUG MODE: ENTER X AND Y OF MISSION PLEASE!\n");
        }
        struct Point * mission = parse_point(argv[1],argv[2]);
        params->END_OF_LINE_X = atoi(argv[1]);
        params->END_OF_LINE_Y = atoi(argv[2]);

        printf("\n---WAITING FOR MARVELMINDS---\n\n");  

        while (params->currentPoint.x == 0 && params->currentPoint.y == 0) {
            //waiting that marvelmind doesn't send 0
        }
        printf("\n---CALCULATING TRAJECTORY---\n\n");
        printf("Sending starting point to traj function:\nx: %d\ny: %d\n",params->currentPoint.x,params->currentPoint.y);
        Trajectory(params, *mission);

    }
  
    //bloquer threads de communication en mode debug
    if (DEBUG_COMM != 1) {
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
