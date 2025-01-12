#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 12345

void executaMutare(int socket_fd) {
    int linie, coloana;
    char mutare[10];

    printf("Introduceti linia si coloana (ex: 1 2): ");
    scanf("%d %d", &linie, &coloana);

    sprintf(mutare, "%d %d", linie, coloana);
    if (write(socket_fd, mutare, strlen(mutare)) < 0) {
        perror("Eroare la trimiterea mutarii");
        exit(1);
    }
}

int main() {
    int socket_fd;
    struct sockaddr_in adresaServer;
    char buffer[1024];
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Eroare la crearea socket-ului");
        return 1;
    }
    adresaServer.sin_family = AF_INET;
    adresaServer.sin_port = htons(PORT);
    adresaServer.sin_addr.s_addr = INADDR_ANY;
    if (connect(socket_fd, (struct sockaddr *)&adresaServer, sizeof(adresaServer)) < 0) {
        perror("Conectarea a esuat");
        return 1;
    }
    printf("Conectat la server. Asteptati...\n");
    printf("1. Creeaza joc nou\n");
    printf("2. Intra in joc dupa ID\n");
    printf("3. Intra in joc aleator\n");
    printf("Alegeti optiunea: ");
    int optiune;
    scanf("%d", &optiune);
    switch(optiune) {
        case 1:
            if (write(socket_fd, "NEW", 3) < 0) {
                perror("Eroare la trimiterea comenzii NEW");
                return 1;
            }
            if (read(socket_fd, buffer, sizeof(buffer)) < 0) {
                perror("Eroare la citirea ID-ului jocului");
                return 1;
            }
            printf("Joc creat! ID: %s\n", buffer);
            break;
        case 2:
            printf("Introduceti ID-ul jocului: ");
            scanf("%s", buffer);
            if (write(socket_fd, buffer, strlen(buffer)) < 0) {
                perror("Eroare la trimiterea ID-ului");
                return 1;
            }
            break;
        case 3:
            if (write(socket_fd, "RANDOM", 6) < 0) {
                perror("Eroare la trimiterea comenzii RANDOM");
                return 1;
            }
            break;
    }

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int octeti_primiti = read(socket_fd, buffer, sizeof(buffer) - 1);

        if (octeti_primiti < 0) {
            perror("Eroare la citirea datelor de la server");
            break;
        }
        if (octeti_primiti == 0) {
            printf("Conexiunea cu serverul s-a inchis.\n");
            break;
        }
        buffer[octeti_primiti] = '\0';
        printf("%s\n", buffer);
        if (strcmp(buffer, "TURN") == 0) {
            executaMutare(socket_fd);
        }
    }
    close(socket_fd);
    return 0;
}