#ifndef _TRAJ_H_
#define _TRAJ_H_

#include <math.h>
#include "main.h"

// Structure pour représenter un point en 3D

// Calcule la distance entre deux points en 3D

double distance(Point p1, Point p2);

// Fonction pour trouver le point le plus proche entre deux points

int trouverPointPlusProche(Point points[], int n, int pointInit,
                           int currentPoint, int visited[]);

void Trajectory(struct PARAMS * params, Point point_final); //Fonction pour avoir la liste des// points à parcourir

void advanced_Trajectory(struct PARAMS *params, Point point_final);

#endif
