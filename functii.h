#ifndef __FUNCTII_H
#define __FUNCTII_H

#define SIZE_PAROLA 20
#define SIZE_NUME 200
#define LINII 3
#define COLOANE 3

typedef enum{
    TRUE = 1,
    FALSE = 0
}BOOLEAN;

typedef struct MUTARE{
    int x;
    int y;
}MUTARE;

typedef struct{
    char nume[SIZE_NUME];
    char parola[SIZE_PAROLA];
}CLIENT;

typedef struct GAME{
    CLIENT client1;
    CLIENT client2;
    char tabla[3][3];
}GAME;

void generare_id(char id[], int lungime);
void citireClient(CLIENT *c);
void citire_mutare_clienti(GAME *g);
void initializare_tabla(char tabla[LINII][COLOANE]);
void desenare_tabla(char tabla[LINII][COLOANE]);
int verigficare_tabla_plina(char tabla[LINII][COLOANE]);
void joc(GAME game);


#endif