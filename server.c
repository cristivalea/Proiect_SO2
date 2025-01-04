// server.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 12345
#define MAX_CLIENTS 2

pthread_mutex_t game_mutex = PTHREAD_MUTEX_INITIALIZER;
char board[3][3];
int clients[MAX_CLIENTS];
int current_turn = 0;

void initialize_board() {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            board[i][j] = ' ';
        }
    }
}

void print_board(char *buffer) {
    sprintf(buffer, "\n %c | %c | %c \n---+---+---\n %c | %c | %c \n---+---+---\n %c | %c | %c \n",
            board[0][0], board[0][1], board[0][2],
            board[1][0], board[1][1], board[1][2],
            board[2][0], board[2][1], board[2][2]);
}

int check_winner() {
    for (int i = 0; i < 3; i++) {
        if (board[i][0] == board[i][1] && board[i][1] == board[i][2] && board[i][0] != ' ') return 1;
        if (board[0][i] == board[1][i] && board[1][i] == board[2][i] && board[0][i] != ' ') return 1;
    }
    if (board[0][0] == board[1][1] && board[1][1] == board[2][2] && board[0][0] != ' ') return 1;
    if (board[0][2] == board[1][1] && board[1][1] == board[2][0] && board[0][2] != ' ') return 1;
    return 0;
}

void *client_handler(void *arg) {
    int client_socket = *(int *)arg;
    free(arg);
    char buffer[1024];
    int row, col;

    while (1) {
        pthread_mutex_lock(&game_mutex);
        if (client_socket == clients[current_turn]) {
            send(client_socket, "TURN", 4, 0);
            pthread_mutex_unlock(&game_mutex);

            memset(buffer, 0, sizeof(buffer));
            recv(client_socket, buffer, sizeof(buffer) - 1, 0);
            sscanf(buffer, "%d %d", &row, &col);

            pthread_mutex_lock(&game_mutex);
            if (board[row][col] == ' ') {
                board[row][col] = (current_turn == 0) ? 'X' : 'O';
                current_turn = 1 - current_turn;
            }
            print_board(buffer);
            send(clients[0], buffer, strlen(buffer), 0);
            send(clients[1], buffer, strlen(buffer), 0);

            if (check_winner()) {
                strcat(buffer, "\nA castigat unul dintre jucatori!\n");
                send(clients[0], buffer, strlen(buffer), 0);
                send(clients[1], buffer, strlen(buffer), 0);
                break;
            }
        }
        pthread_mutex_unlock(&game_mutex);
    }

    close(client_socket);
    return NULL;
}

int main() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    socklen_t addr_len = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, MAX_CLIENTS);

    printf("Server pornit pe portul %d\n", PORT);
    initialize_board();

    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_socket = accept(server_fd, (struct sockaddr *)&address, &addr_len);
        printf("Client conectat.\n");
        clients[i] = client_socket;

        pthread_t thread_id;
        int *new_sock = malloc(sizeof(int));
        *new_sock = client_socket;
        pthread_create(&thread_id, NULL, client_handler, (void *)new_sock);
    }

    while (1) {
        sleep(1);
    }

    close(server_fd);
    return 0;
}
