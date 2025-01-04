// client.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 12345

void joacaMutare(int sockfd) {
    int row, col;
    char mutare[10];

    printf("Introduceti linia si coloana (ex: 1 2): ");
    scanf("%d %d", &row, &col);

    sprintf(mutare, "%d %d", row, col);
    send(sockfd, mutare, strlen(mutare), 0);
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
    printf("Conectat la server. Asteptati...");

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
        }
    }

    close(sockfd);
    return 0;
}
