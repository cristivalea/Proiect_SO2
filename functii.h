#ifndef FUNCTII_H
#define FUNCTII_H

#define PORT 8080
#define MAX_CLIENTI 10
#define LINII 3
#define COLOANE 3
#define TRUE 1
#define FALSE 0

typedef struct {
    char nume[50];
    char simbol;
    int socket;
} CLIENT;

typedef struct {
    CLIENT client1;
    CLIENT client2;
    char tabla[LINII][COLOANE];
} GAME;


void generare_id(char id[], int lungime);
void gestioneaza_cerere_generare_id(CLIENT *client);
void proceseaza_comunicare_client(CLIENT *client);
void initializare_tabla(char tabla[LINII][COLOANE]);
void trimite_tabla_client(CLIENT *client, char tabla[LINII][COLOANE]);
void citire_mutare_client(CLIENT *client, char tabla[LINII][COLOANE], char simbol);
int verificare_castigator(char tabla[LINII][COLOANE], char simbol);
void citire_informatii_client(CLIENT *client);
void *joc(void *arg);


#endif
