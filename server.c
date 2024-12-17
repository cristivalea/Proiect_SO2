#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "functii.h"

GAME game;

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

void *thread_func(void *arg) {
    CLIENT *client = (CLIENT *)arg;
    
    CLIENT client_info;
    char password[50];
    
    recv(client->socket, &client_info, sizeof(CLIENT), 0); // Receive client info
    recv(client->socket, password, 50, 0); // Receive password

    if (game.client1.socket == 0) {
        game.client1 = client_info;
        strcpy(game.password, password);
        printf("Client 1 conectat: %s\n", game.client1.nume);
        send(client->socket, "Asteptare client 2...\n", 22, 0);
        
        while (game.client2.socket == 0) {
            sleep(1);
        }
        
        send(client->socket, "Client 2 conectat. Incepe jocul!\n", 33, 0);
        trimite_tabla_client(client, game.tabla);
    } else {
        char received_password[50];
        recv(client->socket, received_password, 50, 0);
        
        if (strcmp(received_password, game.password) != 0) {
            send(client->socket, "Parola incorecta\n", 17, 0);
            close(client->socket);
            pthread_exit(NULL);
        }
        
        game.client2 = client_info;
        printf("Client 2 conectat: %s\n", game.client2.nume);
        send(game.client1.socket, "Client 2 conectat. Incepe jocul!\n", 33, 0);
        trimite_tabla_client(client, game.tabla);
    }
    
    while (1) {
        trimite_tabla_client(client, game.tabla);
        citire_mutare_client(client, game.tabla, client_info.simbol);
        if (verificare_castigator(game.tabla, client_info.simbol)) {
            send(client->socket, "Ai castigat!\n", 13, 0);
            send(game.client1.socket == client->socket ? game.client2.socket : game.client1.socket, "Ai pierdut!\n", 12, 0);
            break;
        }
    }
    
    close(client->socket);
    pthread_exit(NULL);
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("Eroare la crearea socketului");
        exit(1);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Eroare la bind");
        close(server_fd);
        exit(1);
    }

    if (listen(server_fd, 3) < 0) {
        perror("Eroare la listen");
        close(server_fd);
        exit(1);
    }

    printf("Serverul asteapta conexiuni...\n");

    while (1) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (new_socket < 0) {
            perror("Eroare la accept");
            continue;
        }

        CLIENT *client = malloc(sizeof(CLIENT));
        if (!client) {
            perror("Eroare la alocarea memoriei pentru client");
            close(new_socket);
            continue;
        }

        client->socket = new_socket;
        pthread_t thread_id;
        pthread_create(&thread_id, NULL, thread_func, (void *)client);
        pthread_detach(thread_id);
    }

    close(server_fd);
    return 0;
}
