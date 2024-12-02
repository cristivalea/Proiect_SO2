#ifndef __FUNCTII_H
#define __FUNCTII_H

#define SIZE_PAROLA 20
#define SIZE_NUME 200
#define LINII 3
#define COLOANE 3

typedef enum {
    TRUE = 1,
    FALSE = 0
} BOOLEAN;

typedef struct {
    int x;
    int y;
} MUTARE;

typedef struct {
    char nume[SIZE_NUME];
    char parola[SIZE_PAROLA];
    char simbol;
} CLIENT;

typedef struct {
    CLIENT client1;
    CLIENT client2;
    char tabla[LINII][COLOANE];
} GAME;

void generare_id(char id[], int lungime);
void citireClient(CLIENT *c, char simbol_opus);
void citire_mutare_clienti(GAME *g, char simbol);
void initializare_tabla(char tabla[LINII][COLOANE]);
void desenare_tabla(char tabla[LINII][COLOANE]);
int verificare_tabla_plina(char tabla[LINII][COLOANE]);
int verificare_castigator(char tabla[LINII][COLOANE], char simbol);
int reincepe_joc(GAME *game);
void joc(GAME *game);

#endif
