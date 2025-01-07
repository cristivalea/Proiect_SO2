#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <sys/select.h>

#define PORT 12345
#define MAX_CLIENTS 10
#define ID_LENGTH 6
#define MAX_NAME_LENGTH 20

volatile sig_atomic_t server_running = 1;

pthread_mutex_t game_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    char game_id[ID_LENGTH + 1];
    int clients[2];
    char player_names[2][MAX_NAME_LENGTH];
    int client_count;
    char board[3][3];
    int current_turn;
    int moves_count;
    int game_running;
} Game;

Game games[MAX_CLIENTS];
int game_count = 0;

void handle_signal(int sig) {
    server_running = 0;
}

void cleanup_server(int server_fd) {
    for (int i = 0; i < game_count; i++) {
        for (int j = 0; j < games[i].client_count; j++) {
            if (games[i].clients[j] > 0) {
                close(games[i].clients[j]);
            }
        }
    }
    if (server_fd > 0) {
        close(server_fd);
    }
    pthread_mutex_destroy(&game_mutex);
}

void initialize_board(char board[3][3]) {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            board[i][j] = ' ';
        }
    }
}

void generate_id(char id[], int length) {
    const char characters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    int num_characters = sizeof(characters) - 1;
    srand((unsigned int)time(NULL));
    for (int i = 0; i < length; i++) {
        id[i] = characters[rand() % num_characters];
    }
    id[length] = '\0';
}

void print_board(char *buffer, char board[3][3]) {
    sprintf(buffer, "\n %c | %c | %c \n---+---+---\n %c | %c | %c \n---+---+---\n %c | %c | %c \n",
            board[0][0], board[0][1], board[0][2],
            board[1][0], board[1][1], board[1][2],
            board[2][0], board[2][1], board[2][2]);
}

int check_win(char board[3][3], char symbol) {
    for (int i = 0; i < 3; i++) {
        if (board[i][0] == symbol && board[i][1] == symbol && board[i][2] == symbol)
            return 1;
    }
    for (int i = 0; i < 3; i++) {
        if (board[0][i] == symbol && board[1][i] == symbol && board[2][i] == symbol)
            return 1;
    }
    if (board[0][0] == symbol && board[1][1] == symbol && board[2][2] == symbol)
        return 1;
    if (board[0][2] == symbol && board[1][1] == symbol && board[2][0] == symbol)
        return 1;
    
    return 0;
}

Game* find_game_by_id(const char *game_id) {
    for (int i = 0; i < game_count; i++) {
        if (strcmp(games[i].game_id, game_id) == 0) {
            return &games[i];
        }
    }
    return NULL;
}

void reset_game(Game *game) {
    initialize_board(game->board);
    game->current_turn = 0;
    game->moves_count = 0;
    game->game_running = 1;
    
    char message[1024];
    sprintf(message, "Joc nou inceput! Este randul lui %s (X)\n", game->player_names[0]);
    write(game->clients[0], message, strlen(message));
    write(game->clients[1], message, strlen(message));
    write(game->clients[0], "TURN", 4);
}

void notify_game_start(Game *game) {
    char message[1024];
    sprintf(message, "Jocul a inceput! %s (X) vs %s (O)\n", 
            game->player_names[0], game->player_names[1]);
    write(game->clients[0], message, strlen(message));
    write(game->clients[1], message, strlen(message));
    write(game->clients[0], "TURN", 4);
}

void handle_disconnection(Game *game, int client_socket) {
    char message[1024] = "Un jucator s-a deconectat. Jocul se inchide.\n";
    for (int i = 0; i < game->client_count; i++) {
        if (game->clients[i] != client_socket) {
            write(game->clients[i], message, strlen(message));
            close(game->clients[i]);
        }
    }
    game->game_running = 0;
}

void process_turn(Game *game, int client_socket) {
    char buffer[1024];
    int current_player = (game->clients[0] == client_socket) ? 0 : 1;
    char symbol = (current_player == 0) ? 'X' : 'O';
    
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        
        if (received <= 0) {
            handle_disconnection(game, client_socket);
            return;
        }
        
        if (strcmp(buffer, "restart") == 0) {
            reset_game(game);
            return;
        }
        
        int row, col;
        if (sscanf(buffer, "%d %d", &row, &col) == 2) {
            if (row >= 0 && row < 3 && col >= 0 && col < 3 && game->board[row][col] == ' ') {
                game->board[row][col] = symbol;
                game->moves_count++;
                
                char board_display[1024];
                print_board(board_display, game->board);
                write(game->clients[0], board_display, strlen(board_display));
                write(game->clients[1], board_display, strlen(board_display));
                
                if (check_win(game->board, symbol)) {
                    char win_message[1024];
                    sprintf(win_message, "Jucatorul %s a castigat!\n", game->player_names[current_player]);
                    write(game->clients[0], win_message, strlen(win_message));
                    write(game->clients[1], win_message, strlen(win_message));
                    write(game->clients[0], "REPLAY", 6);
                    write(game->clients[1], "REPLAY", 6);
                    return;
                } else if (game->moves_count >= 9) {
                    write(game->clients[0], "Egalitate!\n", 11);
                    write(game->clients[1], "Egalitate!\n", 11);
                    
                    write(game->clients[0], "REPLAY", 6);
                    write(game->clients[1], "REPLAY", 6);
                    return;
                }
                
                game->current_turn = 1 - game->current_turn;
                write(game->clients[game->current_turn], "TURN", 4);
                return;
            } else {
                write(client_socket, "Mutare invalida. Incercati din nou.\n", 400);
            }
        } else {
            write(client_socket, "Format invalid. Introduceti linia si coloana (ex: 1 2).\n", 600);
        }
    }
}

void *client_handler(void *arg) {
    int client_socket = *(int *)arg;
    free(arg);
    
    char buffer[1024];
    Game *game = NULL;
    
    memset(buffer, 0, sizeof(buffer));
    if (read(client_socket, buffer, sizeof(buffer) - 1) <= 0) {
        close(client_socket);
        return NULL;
    }
    char player_name[MAX_NAME_LENGTH];
    strncpy(player_name, buffer, MAX_NAME_LENGTH - 1);
    player_name[MAX_NAME_LENGTH - 1] = '\0';
    printf("Player connected: %s\n", player_name);
    
    memset(buffer, 0, sizeof(buffer));
    if (read(client_socket, buffer, sizeof(buffer) - 1) <= 0) {
        close(client_socket);
        return NULL;
    }
    
    if (strcmp(buffer, "CREATI") == 0) {
        pthread_mutex_lock(&game_mutex);
        game = &games[game_count++];
        generate_id(game->game_id, ID_LENGTH);
        game->clients[0] = client_socket;
        strncpy(game->player_names[0], player_name, MAX_NAME_LENGTH - 1);
        game->client_count = 1;
        initialize_board(game->board);
        game->current_turn = 0;
        game->moves_count = 0;
        game->game_running = 1;
        printf("Game created with ID: %s\n", game->game_id);
        write(client_socket, game->game_id, strlen(game->game_id));
        pthread_mutex_unlock(&game_mutex);
        
    } else {
        pthread_mutex_lock(&game_mutex);
        game = find_game_by_id(buffer);
        if (game && game->client_count < 2) {
            game->clients[1] = client_socket;
            strncpy(game->player_names[1], player_name, MAX_NAME_LENGTH - 1);
            game->client_count++;
            notify_game_start(game);
        } else {
            write(client_socket, "Jocul este deja complet sau nu exista.\n", 39);
            pthread_mutex_unlock(&game_mutex);
            close(client_socket);
            return NULL;
        }
        pthread_mutex_unlock(&game_mutex);
    }
    
    while (game && game->game_running) {
        pthread_mutex_lock(&game_mutex);
        if (client_socket == game->clients[game->current_turn]) {
            pthread_mutex_unlock(&game_mutex);
            process_turn(game, client_socket);
        } else {
            pthread_mutex_unlock(&game_mutex);
            usleep(100000);
        }
    }
    
    close(client_socket);
    return NULL;
}

int main() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    
    int server_fd, client_socket;
    struct sockaddr_in address;
    socklen_t addr_len = sizeof(address);
    
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        return 1;
    }
    
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(server_fd);
        return 1;
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        return 1;
    }
    
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        close(server_fd);
        return 1;
    }
    
    printf("Server pornit pe portul %d\n", PORT);
    
    while (server_running) {
        fd_set readfds;
        struct timeval tv;
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        
        int activity = select(server_fd + 1, &readfds, NULL, NULL, &tv);
        if (activity < 0 && errno != EINTR) {
            perror("Select error");
            break;
        }
        
        if (activity > 0 && FD_ISSET(server_fd, &readfds)) {
            if ((client_socket = accept(server_fd, (struct sockaddr *)&address, &addr_len)) < 0) {
                perror("Accept failed");
                continue;
            }
            
            printf("Client conectat.\n");
            
            int *new_sock = malloc(sizeof(int));
            if (new_sock == NULL) {
                perror("Failed to allocate memory");
                close(client_socket);
                continue;
            }
            
            *new_sock = client_socket;
            pthread_t thread_id;
            if (pthread_create(&thread_id, NULL, client_handler, (void *)new_sock) != 0) {
                perror("Could not create thread");
                free(new_sock);
                close(client_socket);
                continue;
            }
            pthread_detach(thread_id);
        }
    }
    
    cleanup_server(server_fd);
    return 0;
}