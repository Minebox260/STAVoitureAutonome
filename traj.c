#include "traj.h"

// Structure pour représenter un point en 3D
// Calcule la distance entre deux points en 3D

double distance(Point p1, Point p2) {
  /*if(p1.x == NULL) {
    printf("p1 is null\n");
  }
  printf("calculating distance\n");
  printf("p1 - x: %d, y: %d\n", p1.x, p1.y);
  printf("p2 - x: %d, y: %d\n", p2.x, p2.y);*/
  int dx = p1.x - p2.x;
  int dy = p1.y - p2.y;
  return sqrt(dx * dx + dy * dy);
}

// Fonction pour trouver le point le  plus proche entre deux points

int trouverPointPlusProche(Point carte[], int n, int pointInit,
                           int currentPoint, int visited[]) {
  int plusProche;
  double distanceMin = 1000000000;
  double dist;
  int i;

  for (i = 0; i < n; i++) {
    if (i != pointInit && visited[i] == 0) {
      dist = distance(carte[i], carte[currentPoint]);
      if (dist < distanceMin) {
        printf("new dist : %d", i);
        distanceMin = dist;
        plusProche = i;
      }
    }
  }
  visited[i] = 1;
  return plusProche;
}

void Trajectory(struct PARAMS *params, Point point_final) {
  int n = params->nb_points; // Nombre de points
  params->chemin = (struct Point *)malloc(sizeof(struct Point) * n);
    for(int i=0; i<n; i++) {
      params->chemin[i].x = 0;
      params->chemin[i].y = 0;
    }
    
  Point depart;
  depart.x = params->pos->x;
  depart.y = params->pos->y;
  int ind;
  // Point points[n];
  Point ordre;
  Point final;
  int indice = 1;
  int ix_pos_final;
  int indice_pos_init;
  double dist_min = 1000000.0;
  ordre.x = point_final.x;
  ordre.y = point_final.y;
  // Déterminer le point le plus proche de l'ordre
  for (int i = 0; i < n; i++) {
    if (distance(params->carte[i], ordre) < dist_min) {
      dist_min = distance(params->carte[i], ordre);
      final.x = params->carte[i].x;
      final.y = params->carte[i].y;
      ix_pos_final = i;
    }
  }

  printf(" Pos final :  %d %d \n", final.x, final.y);
  double minimum = distance(params->carte[0], depart);
  for (int j = 0; j < n; j++) {
    // Déterminer le point le plus proche de la position initiale
    if (distance(params->carte[j], depart) < minimum) {
      indice_pos_init = j;
      minimum = distance(params->carte[j], depart);
    }
  }
  printf("---Calcul de trajectoire---\n");
  printf("Point Initial : %d, Point Final : %d\n", indice_pos_init,
         ix_pos_final);
  int ix_pointActuel =
      indice_pos_init; // indice du point de la carte correspondant à la
                       // position actuelle de la voiture
  params->chemin[0].x =
      params->carte[ix_pointActuel]
          .x; // premier point du chemin = point de la carte correspondant à la
              // position actuelle de la voiture
  params->chemin[0].y = params->carte[ix_pointActuel].y;
  // génerer le chemin!
  ind = indice_pos_init;
  while ((ind % n) != ix_pos_final) {
    printf("Point (%d) : %d,%d\n", ind, params->carte[ind].x,
           params->carte[ind].y);
    params->chemin[indice].x = params->carte[ind].x;
    params->chemin[indice].y = params->carte[ind].y;
    indice++;
    ind++;
    ind = ind % n;
  }
  params->next_goal = params->chemin[1];
  params->last_goal = params->chemin[0];
  params->indice_next_goal = 1;
}


void advanced_Trajectory(struct PARAMS *params, Point point_final) {
  int n = params->nb_points; // Nombre de points
  params->chemin = malloc(params->nb_points * sizeof(struct Point));
  Point depart;
  depart.x = params->pos->x;
  depart.y = params->pos->y;
  int ind;
  // Point points[n];
  Point ordre;
  Point final;
  int indice = 1;
  int ix_pos_final;
  int indice_pos_init;
  ordre.x = point_final.x;
  ordre.y = point_final.y;

  // Déterminer le point le plus proche de l'ordre
  double dist_min = distance(params->carte_index[0], ordre);
  for (int i = 0; i < n; i++) {
    if (distance(params->carte_index[i], ordre) < dist_min) {
      dist_min = distance(params->carte_index[i], ordre);
      final.x = params->carte_index[i].x;
      final.y = params->carte_index[i].y;
      final.ind = params->carte_index[i].ind;
      ix_pos_final = i;
    }
  }
  ////////////////////////////////////////////////////////

  // Déterminer le point le plus proche de la position initiale

  printf(" Pos final :  %d %d \n", final.x, final.y);
  double minimum = distance(params->carte_index[0], depart);
  for (int j = 0; j < n; j++) {
    // Déterminer le point le plus proche de la position initiale
    if (distance(params->carte_index[j], depart) < minimum) {
      indice_pos_init = j;
      minimum = distance(params->carte_index[j], depart);
      depart.ind = params->carte_index[j].ind;
    }
  }
  /////////////////////////////////////////////////////////////

  printf("---Calcul de trajectoire---\n");
  printf("Point Initial : %d, Point Final : %d\n", indice_pos_init,
         ix_pos_final);

  params->chemin[0].x =
      params->carte_index[indice_pos_init]
          .x; // premier point du chemin = point de la carte correspondant à la
              // position actuelle de la voiture
  params->chemin[0].y = params->carte_index[indice_pos_init].y;
  // génerer le chemin!
  ind = indice_pos_init;

  while (ind != ix_pos_final) {
    // Sens de parcours qui dépend de la carte
    if (params->carte_index[ind].ind ==
            0 || // 0 correspond à l'index pour le cercle
        params->carte_index[ind].ind == final.ind ||
        params->carte_index[ind].ind == depart.ind) {
      printf("Point (%d) : %d,%d\n", ind, params->carte[ind].x,
             params->carte[ind].y);
      params->chemin[indice].x = params->carte_index[ind].x;
      params->chemin[indice].y = params->carte_index[ind].y;
      indice++;
    }
    ind++;
    ind = ind % n;
  }
  params->next_goal = params->chemin[1];
  params->last_goal = params->chemin[0];
  params->indice_next_goal = 1;
}
