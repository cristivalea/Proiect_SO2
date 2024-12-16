#include <stdlib.h>
#include <time.h>
#include "functii.h"

void generare_id(char id[], int lungime) {
    const char caractere[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    int nr_caractere = sizeof(caractere) - 1;
    srand(time(NULL));
    for (int i = 0; i < lungime; i++) {
        id[i] = caractere[rand() % nr_caractere];
    }
    id[lungime] = '\0';
}


void initializare_tabla(char tabla[LINII][COLOANE]) {
    for (int i = 0; i < LINII; i++) {
        for (int j = 0; j < COLOANE; j++) {
            tabla[i][j] = '_';
        }
    }
}


