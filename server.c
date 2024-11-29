#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "functii.h"


void generare_id(char id[], int lungime) {
    const char caractere[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    int nr_caractere = sizeof(caractere) - 1;

    for (int i = 0; i < lungime; i++) {
        id[i] = caractere[rand() % nr_caractere];
    }
    id[lungime] = '\0';
}

void citireClient(CLIENT *c, char simbol_opus) {
    printf("Doriti sa introduceti numele sau vi se va atribui un id?\n");
    int optiune = 0;
    printf("1. Adaugare nume\n");
    printf("2. Atribuire id\n");
    printf("Introduceti optiunea dorita: ");
    scanf("%d", &optiune);

    switch(optiune) {
        case 1: {
            printf("Introduceti numele: ");
            scanf("%99s", c->nume);
            break;
        }
        case 2: {
            generare_id(c->nume, 20);
            printf("ID generat: %s\n", c->nume);
            break;
        }
        default:
            printf("Optiune este invalida.Se va atribui un ID.\n");
            generare_id(c->nume, 20);
            break;
    }

    printf("Introduceti parola dorita: ");
    scanf("%99s", c->parola);

    printf("Introduceti simbolul dorit:\n");
    printf("X\n");
    printf("0\n");
    while (1) {
        scanf(" %c", &c->simbol);
        if (c->simbol == simbol_opus) {
            printf("A fost ales deja acest simbol.");
        } else {
            break;
        }
    }
}

void initializare_tabla(char tabla[LINII][COLOANE]) {
    for (int i = 0; i < LINII; i++) {
        for (int j = 0; j < COLOANE; j++) {
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

int verificare_tabla_plina(char tabla[LINII][COLOANE]) {
    for (int i = 0; i < LINII; i++) {
        for (int j = 0; j < COLOANE; j++) {
            if (tabla[i][j] == '_') {
                return FALSE;
            }
        }
    }
    return TRUE;
}

int verificare_castigator(char tabla[LINII][COLOANE], char simbol) {
    for (int i = 0; i < LINII; i++) {
        if (tabla[i][0] == simbol && tabla[i][1] == simbol && tabla[i][2] == simbol) {
            return TRUE;
        }
    }
    for (int j = 0; j < COLOANE; j++) {
        if (tabla[0][j] == simbol && tabla[1][j] == simbol && tabla[2][j] == simbol) {
            return TRUE;
        }
    }
    if (tabla[0][0] == simbol && tabla[1][1] == simbol && tabla[2][2] == simbol) {
        return TRUE;
    }
    if (tabla[0][2] == simbol && tabla[1][1] == simbol && tabla[2][0] == simbol) {
        return TRUE;
    }
    return FALSE;
}

void citire_mutare_clienti(GAME *g, char simbol) {
    MUTARE m1;
    printf("Introduceti mutarea (%c):\n", simbol);
    printf("Introduceti linia: ");
    scanf("%d", &m1.x);
    printf("Introduceti coloana: ");
    scanf("%d", &m1.y);

    if (m1.x < 0 || m1.x >= LINII || m1.y < 0 || m1.y >= COLOANE) {
        printf("Mutare este invalida. Introduceti alta mutare.\n");
        citire_mutare_clienti(g, simbol);
    } else if (g->tabla[m1.x][m1.y] != '_') {
        printf("Pozitia aleasa este ocupata. Alegeti alta pozitie.\n");
        citire_mutare_clienti(g, simbol);
    } else {
        g->tabla[m1.x][m1.y] = simbol;
    }
}

void joc(GAME *game) {
    initializare_tabla(game->tabla);
    desenare_tabla(game->tabla);

    int mutari = 0;
    while (verificare_tabla_plina(game->tabla) == FALSE) {
        if (mutari % 2 == 0) {
            printf("Tura clientului %s (%c):\n", game->client1.nume, game->client1.simbol);
            citire_mutare_clienti(game, game->client1.simbol);
            desenare_tabla(game->tabla);
            if (verificare_castigator(game->tabla, game->client1.simbol)) {
                printf("Clientul %s a castigat!\n", game->client1.nume);
                return;
            }
        } else {
            printf("Tura clientului %s (%c):\n", game->client2.nume, game->client2.simbol);
            citire_mutare_clienti(game, game->client2.simbol);
            desenare_tabla(game->tabla);
            if (verificare_castigator(game->tabla, game->client2.simbol)) {
                printf("Clientul %s a castigat!\n", game->client2.nume);
                return;
            }
        }
        mutari++;
    }

    printf("Tabla este plină. Egalitate intre jucatori.\n");
}

int main(void) {
    srand((unsigned int)time(NULL)); 

    GAME game;

    printf("Introduceti detalii pentru clientul 1:\n");
    citireClient(&game.client1, '\0');
    printf("Introduceti detalii pentru clientul 2:\n");
    citireClient(&game.client2, game.client1.simbol);

    joc(&game);

    return 0;
}
