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
    write(sockfd, mutare, strlen(mutare));
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

    printf("1. Create new game\n");
    printf("2. Join game by ID\n");
    printf("3. Join random game\n");
    printf("Choice: ");
    
    int choice;
    scanf("%d", &choice);

    switch(choice) {
        case 1:
            write(sockfd, "NEW", 3);
            read(sockfd, buffer, sizeof(buffer));
            printf("Game created! ID: %s\n", buffer);
            break;
        case 2:
            printf("Enter game ID: ");
            scanf("%s", buffer);
            write(sockfd, buffer, strlen(buffer));
            break;
        case 3:
            write(sockfd, "RANDOM", 6);
            break;
    }

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = read(sockfd, buffer, sizeof(buffer) - 1);

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
