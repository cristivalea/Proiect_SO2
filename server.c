#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "functii.h"

void generare_id(char id[], int lungime){
    const char caractere[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    int nr_caractere = sizeof(caractere) - 1;

    srand((unsigned int)time(NULL));

    for (int i = 0; i < lungime; i++) {
        id[i] = caractere[rand() % nr_caractere];
    }
    id[lungime] = '\0';
}

void citireClient(CLIENT *c){
    printf("Doriti sa introduceti numele sau vi se va atribui un id?\n");
    int optiune = 0;
    printf("1. Adaugare nume\n");
    printf("2. Atribuire id\n");
    printf("Introduceti optiunea dorita: ");
    scanf("%d\n", &optiune);
    switch(optiune){
        case 1:
        {
            printf("Introduceti numele: ");
            scanf("%s\n", c->nume);
            break;
        }
        case 2:
        {
            generare_id(c->nume, 20);
            break;
        }
    }
    printf("%s\n", c->parola);
}


void citire_mutare_clienti(GAME *g){
    MUTARE m1;
    printf("Clientul %s introduce mutarea:\n", g->client1.nume);
    printf("Prima muatare la pozitia:\n");
    printf("Introduceti linia: ");
    scanf("%d\n", &m1.x);
    printf("Introduceti coloana: ");
    scanf("%d\n", m1.y);
}

void initializare_tabla(char tabla[LINII][COLOANE]){
    for(int i = 0; i < LINII; i++){
        for(int j = 0; j < COLOANE; j++){
            tabla[i][j] = '_';
        }
    }
}

void desenare_tabla(char tabla[LINII][COLOANE]) {
    for (int i = 0; i < LINII; i++) {
        for (int j = 0; j < COLOANE; j++) {
            printf(" %c ", tabla[i][j]); 
            if (j < COLOANE - 1) {
                printf("|"); 
            }
        }
        printf("\n");

        if (i < LINII - 1) {
            for (int j = 0; j < COLOANE; j++) {
                printf("---");
                if (j < COLOANE - 1) {
                    printf("+"); 
                }
            }
            printf("\n");
        }
    }
}

int verigficare_tabla_plina(char tabla[LINII][COLOANE]){
    BOOLEAN verificare = FALSE;
    int contor = 0;
    for(int i = 0; i < LINII; i++){
        for(int j = 0; j < COLOANE; j++){
            if(tabla[i][j] == '_'){
                contor++;
            }
        }
    }
    if(contor == 9){
        verificare = TRUE;
    }
    return verificare;
}


void joc(GAME game){
    initializare_tabla(game.tabla);
    //desenare_tabla(game.tabla);
    while(verigficare_tabla_plina(game.tabla) == FALSE){

    }
}

int main(void){
    GAME game;
    joc(game);
    return 0;
}

