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

typedef struct {
    char id[7];
    int clients[2];
    char board[3][3];
    int current_turn;
    int active;
} Game;

#define MAX_GAMES 10
Game games[MAX_GAMES];
int game_count = 0;

void initialize_board(char board[3][3]) {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            board[i][j] = ' ';
        }
    }
}

void generate_game_id(char *id) {
    const char chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for(int i = 0; i < 6; i++) {
        id[i] = chars[rand() % strlen(chars)];
    }
    id[6] = '\0';
}

Game* find_game(const char* id) {
    for(int i = 0; i < game_count; i++) {
        if(strcmp(games[i].id, id) == 0 && games[i].active) {
            return &games[i];
        }
    }
    return NULL;
}

Game* create_game() {
    if(game_count >= MAX_GAMES) return NULL;
    Game* game = &games[game_count++];
    generate_game_id(game->id);
    game->clients[0] = -1;
    game->clients[1] = -1;
    game->current_turn = 0;
    game->active = 1;
    initialize_board(game->board);
    return game;
}

Game* find_random_game() {
    for(int i = 0; i < game_count; i++) {
        if(games[i].active && games[i].clients[1] == -1) {
            return &games[i];
        }
    }
    return NULL;
}

void print_game_board(Game* game, char* buffer) {
    sprintf(buffer, "\n %c | %c | %c \n---+---+---\n %c | %c | %c \n---+---+---\n %c | %c | %c \n",
            game->board[0][0], game->board[0][1], game->board[0][2],
            game->board[1][0], game->board[1][1], game->board[1][2],
            game->board[2][0], game->board[2][1], game->board[2][2]);
}

int check_game_winner(Game* game) {
    char (*board)[3] = game->board;
    for (int i = 0; i < 3; i++) {
        if (board[i][0] != ' ' && board[i][0] == board[i][1] && board[i][1] == board[i][2]) return 1;
        if (board[0][i] != ' ' && board[0][i] == board[1][i] && board[1][i] == board[2][i]) return 1;
    }
    if (board[0][0] != ' ' && board[0][0] == board[1][1] && board[1][1] == board[2][2]) return 1;
    if (board[0][2] != ' ' && board[0][2] == board[1][1] && board[1][1] == board[2][0]) return 1;
    return 0;
}

void *client_handler(void *arg) {
    int client_socket = *(int *)arg;
    free(arg);
    char buffer[1024];
    Game* game = NULL;
    int player_number;

    if(read(client_socket, buffer, sizeof(buffer) - 1) < 0) {
        perror("Error reading from client\n");
        close(client_socket);
        return NULL;
    }
    
    pthread_mutex_lock(&game_mutex);
    if(strcmp(buffer, "NEW") == 0) {
        game = create_game();
        if(game) {
            game->clients[0] = client_socket;
            player_number = 0;
            write(client_socket, game->id, strlen(game->id));
        }
    } else if(strcmp(buffer, "RANDOM") == 0) {
        game = find_random_game();
        if(!game) {
            game = create_game();
            player_number = 0;
        } else {
            player_number = 1;
        }
        game->clients[player_number] = client_socket;
    } else {
        game = find_game(buffer);
        if(game && game->clients[1] == -1) {
            game->clients[1] = client_socket;
            player_number = 1;
        }
    }
    pthread_mutex_unlock(&game_mutex);

    if(!game) {
        write(client_socket, "ERROR", 5);
        close(client_socket);
        return NULL;
    }

    // Wait for opponent
    while(game->clients[1] == -1) {
        usleep(100000);
    }

    // Notify both players
    if (player_number == 1) {
        write(game->clients[0], "START", 5);
        write(game->clients[1], "START", 5);
    }

    // Game loop
    while(1) {
        pthread_mutex_lock(&game_mutex);
        if(game->current_turn == player_number) {
            write(client_socket, "TURN", 4);
            pthread_mutex_unlock(&game_mutex);

            memset(buffer, 0, sizeof(buffer));
            if(read(client_socket, buffer, sizeof(buffer) - 1) <= 0) {
                break;
            }

            int row, col;
            sscanf(buffer, "%d %d", &row, &col);

            pthread_mutex_lock(&game_mutex);
            if(row >= 0 && row < 3 && col >= 0 && col < 3 && game->board[row][col] == ' ') {
                game->board[row][col] = (player_number == 0) ? 'X' : 'O';
                print_game_board(game, buffer);
                write(game->clients[0], buffer, strlen(buffer));
                write(game->clients[1], buffer, strlen(buffer));

                if(check_game_winner(game)) {
                    sprintf(buffer, "\nPlayer %d wins!\n", player_number + 1);
                    write(game->clients[0], buffer, strlen(buffer));
                    write(game->clients[1], buffer, strlen(buffer));
                    game->active = 0;
                    pthread_mutex_unlock(&game_mutex);
                    break;
                }
                game->current_turn = 1 - game->current_turn;
            }
            pthread_mutex_unlock(&game_mutex);
        } else {
            pthread_mutex_unlock(&game_mutex);
            usleep(100000);
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

    printf("Server started on port %d\n", PORT);

    while(1) {
        client_socket = accept(server_fd, (struct sockaddr *)&address, &addr_len);
        if(client_socket < 0) continue;

        printf("Client connected.\n");
        int *new_sock = malloc(sizeof(int));
        *new_sock = client_socket;
        
        pthread_t thread_id;
        pthread_create(&thread_id, NULL, client_handler, (void*)new_sock);
        pthread_detach(thread_id);
    }

    close(server_fd);
    return 0;
}
