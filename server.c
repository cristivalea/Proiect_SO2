#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "functii.h"

GAME game;

void generare_id(char id[], int lungime){
    const char caractere[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    int nr_caractere = sizeof(caractere) - 1;

    for (int i = 0; i < lungime; i++) {
        id[i] = caractere[rand() % nr_caractere];
    }
    id[lungime] = '\0';
}

void gestioneaza_cerere_generare_id(CLIENT *client) {
    char id[16];
    generare_id(id, 15); 
    
    char mesaj[64];
    snprintf(mesaj, sizeof(mesaj), "ID generat: %s\n", id);
    
    send(client->socket, mesaj, strlen(mesaj), 0); 
    printf("ID generat si trimis clientului: %s\n", id);
}

void proceseaza_comunicare_client(CLIENT *client) {
    char buffer[1024];
    int bytes_received = recv(client->socket, buffer, sizeof(buffer) - 1, 0);
    
    if (bytes_received <= 0) {
        perror("Eroare la primirea datelor de la client");
        close(client->socket);
        return;
    }
    
    buffer[bytes_received] = '\0'; 
    printf("Mesaj primit de la client: %s\n", buffer);
    
    if (strncmp(buffer, "GEN_ID", 6) == 0) {
        gestioneaza_cerere_generare_id(client);
    } else {
        printf("Mesaj necunoscut de la client: %s\n", buffer);
        char mesaj[] = "Cerere necunoscuta\n";
        send(client->socket, mesaj, strlen(mesaj), 0);
    }
}


void initializare_tabla(char tabla[LINII][COLOANE]) {
    for (int i = 0; i < LINII; i++) {
        for (int j = 0; j < COLOANE; j++) {
            tabla[i][j] = '_';
        }
    }
}

void trimite_tabla_client(CLIENT *client, char tabla[LINII][COLOANE]) {
    char tabla_str[LINII * COLOANE + LINII];
    int k = 0;
    for (int i = 0; i < LINII; i++) {
        for (int j = 0; j < COLOANE; j++) {
            tabla_str[k++] = tabla[i][j];
            if (j < COLOANE - 1) tabla_str[k++] = '|';
        }
        if (i < LINII - 1) tabla_str[k++] = '\n';
    }
    tabla_str[k] = '\0';
    send(client->socket, tabla_str, strlen(tabla_str), 0);
}

void citire_mutare_client(CLIENT *client, char tabla[LINII][COLOANE], char simbol) {
    char buffer[16];
    recv(client->socket, buffer, sizeof(buffer), 0);
    int linie, coloana;
    sscanf(buffer, "%d %d", &linie, &coloana);
    tabla[linie][coloana] = simbol;
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

void citire_informatii_client(CLIENT *client) {
    char buffer[1024];
    recv(client->socket, buffer, sizeof(buffer), 0);
    sscanf(buffer, "%s %c", client->nume, &client->simbol);
    printf("Client conectat: %s (Simbol: %c)\n", client->nume, client->simbol);
}

void *joc(void *arg) {
    GAME *game = (GAME *)arg;
    initializare_tabla(game->tabla);

    while (1) {
        int mutari = 0;
        while (1) {
            CLIENT *client_curent = (mutari % 2 == 0) ? &game->client1 : &game->client2;
            char simbol = client_curent->simbol;

            trimite_tabla_client(&game->client1, game->tabla);
            trimite_tabla_client(&game->client2, game->tabla);

            citire_mutare_client(client_curent, game->tabla, simbol);

            if (verificare_castigator(game->tabla, simbol)) {
                char mesaj[256];
                sprintf(mesaj, "Jucatorul %s a castigat!\n", client_curent->nume);
                send(game->client1.socket, mesaj, strlen(mesaj), 0);
                send(game->client2.socket, mesaj, strlen(mesaj), 0);
                break;
            }

            if (++mutari == LINII * COLOANE) {
                char mesaj[] = "Remiza! Tabla este plina.\n";
                send(game->client1.socket, mesaj, strlen(mesaj), 0);
                send(game->client2.socket, mesaj, strlen(mesaj), 0);
                break;
            }
        }

        char optiune[16];
        recv(game->client1.socket, optiune, sizeof(optiune), 0);
        int opt1 = atoi(optiune);

        recv(game->client2.socket, optiune, sizeof(optiune), 0);
        int opt2 = atoi(optiune);

        if (opt1 == 0 || opt2 == 0) {
            close(game->client1.socket);
            close(game->client2.socket);
            free(game);
            return NULL;
        }

        initializare_tabla(game->tabla);
    }
}

int main() {
    int fd_server;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    fd_server = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_server == 0) {
        perror("Eroare la socket\n");
        exit(1);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(fd_server, (struct sockaddr *)&address, addrlen) < 0) {
        perror("Eroare la bind");
        exit(2);
    }

    if (listen(fd_server, MAX_CLIENTI) < 0) {
        perror("Eroare la listen\n");
        exit(3);
    }

    printf("Serverul asteapta conexiuni pe portul: %d\n", PORT);

    while (1) {
        GAME *game = (GAME *)malloc(sizeof(GAME));
        if (game == NULL) {
            perror("Eroare la alocarea jocului\n");
            exit(4);
        }

        if ((game->client1.socket = accept(fd_server, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("Eroare la accept client1\n");
            free(game);
            continue;
        }
        printf("Primul jucator conectat\n");
        proceseaza_comunicare_client(&game->client1);

        if ((game->client2.socket = accept(fd_server, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("Eroare la accept client2\n");
            close(game->client1.socket);
            free(game);
            continue;
        }
        printf("Al doilea jucator conectat\n");
        proceseaza_comunicare_client(&game->client2);

        pthread_t th;
        pthread_create(&th, NULL, joc, (void *)game);
        pthread_detach(th);
    }

    return 0;
}
