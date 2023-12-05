#include <stdio.h>
#include <stdlib.h>

#include "main.h"

int extract_points(struct PARAMS * params) {

    FILE *file = fopen("map.txt", "r");
    
    if (file == NULL) {
        printf("Impossible d'ouvrir le fichier.\n");
        return -1;
    }
  
    int x, y;
    char * line;
    char * ptr;
  
    char ch;
    int lines = 0;
    int i = 0;
  
    while(!feof(file)) {
      ch = fgetc(file);
      if(ch == '\n') lines++;
    }
    lines++;
  
    rewind(file);
  
    struct Point * points = (Point *)malloc(lines * sizeof(Point));
    size_t len = 0;
  
    while (getline(&line,&len, file) != -1) {
      ptr = strtok(line, ":");
      x = atoi(ptr);
      ptr = strtok(NULL, ":");
      y = atoi(ptr);

      struct Point point;
      point.x = x;
      point.y = y;
      points[i] = point;
      
      i++;
    }
  
    fclose(file);

    params->carte = points;
    params->nb_points = lines;
    return 0;
}


int extract_points_index(struct PARAMS *params) {

  FILE *file = fopen("map.txt", "r");

  if (file == NULL) {
    printf("Impossible d'ouvrir le fichier.\n");
    return -1;
  }

  int x, y;
  char *line;
  char *ptr;
  int index;

  char ch;
  int lines = 0;
  int i = 0;

  while (!feof(file)) {
    ch = fgetc(file);
    if (ch == '\n')
      lines++;
  }
  lines++;
  rewind(file);

  struct Point *points = (Point *)malloc(lines * sizeof(Point));
  size_t len = 0;

  while (getline(&line, &len, file) != -1) {
    ptr = strtok(line, ":");
    if (!strcmp(ptr,
                "Index")) { // J'ai assumé que l'index 0 : cercle initale, index
                            // 1 : le chemin de premier parking etc ...
      ptr = strtok(NULL, ":");
      index = atoi(ptr);
    } else {
      x = atoi(ptr);
      ptr = strtok(NULL, ":");
      y = atoi(ptr);
      struct Point point;
      point.x = x;
      point.y = y;
      point.ind = index;
      points[i] = point;
      i++;
    }
  }

  fclose(file);

  params->carte_index = points;
  return 0;
}
