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
int moves_count = 0;
int game_running = 1;

void initialize_board() {
    pthread_mutex_lock(&game_mutex);
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            board[i][j] = ' ';
        }
    }
    moves_count = 0;
    current_turn = 0;
    pthread_mutex_unlock(&game_mutex);
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

int check_draw() {
    return moves_count >= 9;
}

void *client_handler(void *arg) {
    int client_socket = *(int *)arg;
    free(arg);
    char buffer[1024];
    int row, col;

    while (game_running) {
        pthread_mutex_lock(&game_mutex);
        if (client_socket == clients[current_turn]) {
            send(client_socket, "TURN", 4, 0);
            pthread_mutex_unlock(&game_mutex);

            memset(buffer, 0, sizeof(buffer));
            int received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
            if (received <= 0) {
                pthread_mutex_unlock(&game_mutex);
                break;
            }
            sscanf(buffer, "%d %d", &row, &col);

            pthread_mutex_lock(&game_mutex);
            if (row >= 0 && row < 3 && col >= 0 && col < 3 && board[row][col] == ' ') {
                board[row][col] = (current_turn == 0) ? 'X' : 'O';
                moves_count++;
                if (!check_winner() && !check_draw()) {
                    current_turn = 1 - current_turn;
                }
            } else {
                send(client_socket, "TURN", 4, 0);
                pthread_mutex_unlock(&game_mutex);
                continue;
            }

            print_board(buffer);
            send(clients[0], buffer, strlen(buffer), 0);
            send(clients[1], buffer, strlen(buffer), 0);

            if (check_winner()) {
                sprintf(buffer, "A castigat jucatorul %d!\n", current_turn + 1);
                send(clients[0], buffer, strlen(buffer), 0);
                send(clients[1], buffer, strlen(buffer), 0);
            }
            if (check_draw()) {
                strcpy(buffer, "Egalitate!\n");
                send(clients[0], buffer, strlen(buffer), 0);
                send(clients[1], buffer, strlen(buffer), 0);
            }

            if (check_winner() || check_draw()) {
                pthread_mutex_unlock(&game_mutex);
                memset(buffer, 0, sizeof(buffer));
                recv(client_socket, buffer, sizeof(buffer) - 1, 0);

                if (strcmp(buffer, "nu") == 0) {
                    game_running = 0;
                    send(clients[0], "Server inchis.\n", 15, 0);
                    send(clients[1], "Server inchis.\n", 15, 0);
                    close(clients[0]);
                    close(clients[1]);
                    break;
                } else {
                    initialize_board();
                    char reset_msg[1024];
                    print_board(reset_msg);
                    strcat(reset_msg, "\nJoc resetat. Este randul primului jucator.\n");
                    send(clients[0], reset_msg, strlen(reset_msg), 0);
                    send(clients[1], reset_msg, strlen(reset_msg), 0);
                }
                continue;
            }
            pthread_mutex_unlock(&game_mutex);
        } else {
            pthread_mutex_unlock(&game_mutex);
        }
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

    while (game_running) {
        sleep(1);
    }

    close(server_fd);
    return 0;
}