#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 12345

void joacaMutare(int sockfd) {
    int row, col;
    char mutare[10];

    while (1) {
        printf("Introduceti linia si coloana (ex: 1 2): ");
        scanf("%d %d", &row, &col);

        if (row >= 0 && row < 3 && col >= 0 && col < 3) {
            sprintf(mutare, "%d %d", row, col);
            send(sockfd, mutare, strlen(mutare), 0);
            break;
        } else {
            printf("Mutare invalida. Incercati din nou.\n");
        }
    }
}

int main() {
    int sockfd;
    struct sockaddr_in serv_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connect failed");
        return 1;
    }

    char buffer[1024];
    printf("Conectat la server. Asteptati...\n");

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);

        if (bytes_received <= 0) {
            printf("Conexiunea s-a inchis.\n");
            break;
        }

        buffer[bytes_received] = '\0';
        printf("%s\n", buffer);

        if (strcmp(buffer, "TURN") == 0) {
            joacaMutare(sockfd);
        } else if (strstr(buffer, "A castigat") || strstr(buffer, "Egalitate")) {
            printf("%s\n", buffer);

            printf("Doriti sa jucati din nou? (da/nu): ");
            char raspuns[10];
            scanf("%s", raspuns);
            send(sockfd, raspuns, strlen(raspuns), 0);

            if (strcmp(raspuns, "nu") == 0) {
                printf("Iesire din joc.\n");
                break;
            }
        }
    }

    close(sockfd);
    return 0;
}