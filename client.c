#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <sys/select.h>

#define PORT 12345
#define MAX_NAME_LENGTH 20
#define BUFFER_SIZE 1024
#define DEFAULT_SERVER "127.0.0.1"

void clearInputBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void handle_error(const char* message, int sockfd) {
    perror(message);
    if (sockfd >= 0) {
        close(sockfd);
    }
    exit(1);
}

void safeGetString(char *buffer, int maxLength) {
    if (fgets(buffer, maxLength, stdin) != NULL) {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len-1] == '\n') {
            buffer[len-1] = '\0';
        } else {
            clearInputBuffer();
        }
    }
}

void makeMove(int sockfd) {
    int row, col;
    char move[10];
    char input[32];

    while (1) {
        printf("\nIntroduceti mutarea (linia si coloana 0-2, ex: '1 2'): ");
        safeGetString(input, sizeof(input));

        if (sscanf(input, "%d %d", &row, &col) == 2) {
            if (row >= 0 && row < 3 && col >= 0 && col < 3) {
                sprintf(move, "%d %d", row, col);
                write(sockfd, move, strlen(move));
                break;
            }
        }
        printf("Mutare invalida! Va rugam introduceti doua numere intre 0 si 2.\n");
    }
}

void handleGameMessages(int sockfd) {
    char buffer[BUFFER_SIZE];
    int game_active = 1;
    fd_set readfds;
    struct timeval tv;
    
    while (game_active) {
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        FD_SET(STDIN_FILENO, &readfds);
        
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        
        int activity = select(sockfd + 1, &readfds, NULL, NULL, &tv);
        if (activity < 0) {
            perror("Select error");
            break;
        }
        
        if (activity > 0) {
            if (FD_ISSET(sockfd, &readfds)) {
                memset(buffer, 0, sizeof(buffer));
                int bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
                
                if (bytes_received <= 0) {
                    printf("\nConexiunea cu serverul a fost intrerupta.\n");
                    break;
                }
                
                buffer[bytes_received] = '\0';
                
                if (strcmp(buffer, "TURN") == 0) {
                    printf("\n>>> Este randul tau! <<<\n");
                    makeMove(sockfd);
                }
                else if (strcmp(buffer, "REPLAY") == 0) {
                    printf("\nDoriti sa jucati din nou? (da/nu): ");
                    char response[10];
                    safeGetString(response, sizeof(response));
                    write(sockfd, response, strlen(response));
                    
                    if (strcasecmp(response, "nu") == 0) {
                        printf("\nMultumim pentru joc! La revedere!\n");
                        game_active = 0;
                    }
                }
                else {
                    printf("%s", buffer);
                }
            }
        }
    }
}

int connectToServer(const char* address) {
    int sockfd;
    struct sockaddr_in serv_addr;
    struct timeval timeout;
    
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        handle_error("Socket creation failed", -1);
    }
    
    timeout.tv_sec = 30;  
    timeout.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        handle_error("Setsockopt failed", sockfd);
    }
    
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    
    if (inet_pton(AF_INET, address, &serv_addr.sin_addr) <= 0) {
        handle_error("Invalid address", sockfd);
    }
    
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        handle_error("Connection failed", sockfd);
    }
    
    return sockfd;
}

int main(int argc, char *argv[]) {
    const char* server_address = DEFAULT_SERVER;
    
    if (argc == 2) {
        server_address = argv[1];
    }

    system("clear");
    printf("\n=== Bine ati venit la X si 0 ===\n\n");

    int sockfd = connectToServer(server_address);
    if (sockfd < 0) {
        return 1;
    }

    char buffer[BUFFER_SIZE];
    
    printf("Introduceti numele dvs. (max %d caractere): ", MAX_NAME_LENGTH - 1);
    safeGetString(buffer, MAX_NAME_LENGTH);
    if (strlen(buffer) == 0 || strspn(buffer, " \t\n\r") == strlen(buffer)) {
        strcpy(buffer, "Player");
    }
    write(sockfd, buffer, strlen(buffer));
    printf("Nume inregistrat: %s\n", buffer);

    while (1) {
        printf("\nDoriti sa:\n");
        printf("1. Creati un joc nou\n");
        printf("2. Va alaturati unui joc existent\n");
        printf("Alegerea dvs. (1/2): ");
        
        char choice[10];
        safeGetString(choice, sizeof(choice));

        if (choice[0] == '1') {
            write(sockfd, "CREATI", 6);
            
            memset(buffer, 0, sizeof(buffer));
            int received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
            if (received > 0) {
                buffer[received] = '\0';
                printf("\nJoc creat! ID-ul jocului: %s\n", buffer);
                printf("Asteptati al doilea jucator...\n\n");
                break;
            } else {
                printf("Eroare la crearea jocului.\n");
                close(sockfd);
                return 1;
            }
        }
        else if (choice[0] == '2') {
            printf("\nIntroduceti ID-ul jocului: ");
            safeGetString(buffer, sizeof(buffer));
            write(sockfd, buffer, strlen(buffer));
            printf("\nSe incearca conectarea la joc...\n\n");
            break;
        }
        else {
            printf("\nOptiune invalida! Va rugam alegeti 1 sau 2.\n");
        }
    }

    handleGameMessages(sockfd);

    close(sockfd);
    return 0;
}