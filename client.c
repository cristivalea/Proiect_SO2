#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "functii.h"

void afiseaza_tabla(char* tabla_str) {
    printf("\nTabla curenta:\n");
    printf("%s\n", tabla_str);
}

void citireClient(CLIENT *c, char simbol_opus, char* password) {
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
            printf("Optiune este invalida. Se va atribui un ID.\n");
            generare_id(c->nume, 20);
            break;
    }

    printf("Introduceti parola dorita: ");
    scanf("%49s", password);

    printf("Introduceti simbolul dorit (X sau 0):\n");
    while (1) {
        scanf(" %c", &c->simbol);
        if (c->simbol == simbol_opus) {
            printf("A fost ales deja acest simbol, alegeti din nou\n");
        } else if (c->simbol != 'X' && c->simbol != '0') {
            printf("Simbol invalid, alegeti din nou.\n");
        } else {
            break;
        }
    }
}

void meniu_joc(GAME *game, int sockfd) {
    CLIENT *jucator = malloc(sizeof(CLIENT)); 
    if (!jucator) {
        perror("Failed to allocate memory for jucator");
        exit(1);
    }
    
    printf("            Bine ati venit!         \n");
    printf("Optiuni:\n");
    printf("1. Incepeti meci.\n");
    printf("2. Alaturati-va unui meci.\n");
    int optiune = 0;
    char s[100];
    scanf("%d", &optiune);
    switch(optiune) {
        case 1: {
            citireClient(jucator, '_', game->password);
            game->client1 = *jucator;
            initializare_tabla(game->tabla); 
            send(sockfd, jucator, sizeof(CLIENT), 0);
            send(sockfd, game->password, 50, 0);
            break;
        }
        case 2: {
            printf("Introduceti parola: ");
            scanf("%99s", s);
            send(sockfd, s, 50, 0);
            recv(sockfd, s, sizeof(s), 0);
            if (strcmp(s, "OK") != 0) {
                printf("Parola incorecta! Iesire...\n");
                exit(1);
            }
            citireClient(jucator, game->client1.simbol, NULL); 
            game->client2 = *jucator; 
            send(sockfd, jucator, sizeof(CLIENT), 0); 
            break;
        }
        default: {
            printf("Optiune invalida. Iesire...\n");
            exit(1);
        }
    }

    free(jucator); 
}

int main() {
    int fd_client;
    struct sockaddr_in server_addr;
    char buffer[1024];
    GAME game;

    fd_client = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_client == -1) {
        perror("Eroare la crearea socketului");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (connect(fd_client, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Eroare la conectare la server");
        exit(2);
    }

    printf("Te-ai conectat la server!\n");
    memset(&game, 0, sizeof(GAME)); 
    meniu_joc(&game, fd_client);

    while (1) {
        int n = recv(fd_client, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) {
            perror("Eroare la primirea datelor de la server");
            break;
        }
        buffer[n] = '\0';
        afiseaza_tabla(buffer);

        int x, y;
        printf("Introduceti linia (0-2): ");
        scanf("%d", &x);
        printf("Introduceti coloana (0-2): ");
        scanf("%d", &y);

        sprintf(buffer, "%d %d", x, y);
        send(fd_client, buffer, strlen(buffer), 0);
    }

    close(fd_client);
    return 0;
}
