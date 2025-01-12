#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 12345
#define MAX_CLIENTI 2
#define MAX_JOCURI 10

pthread_mutex_t mutexJoc = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    char id[7];
    int conexiuniClienti[2];
    char tabla[3][3];
    int jucatorCurent;
    int activ;
} Joc;

Joc jocuri[MAX_JOCURI];
int numarJocuri = 0;

void initializeazaTabla(char tabla[3][3]) {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            tabla[i][j] = ' ';
        }
    }
}

void genereazaIdJoc(char *id) {
    const char caractere[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for(int i = 0; i < 6; i++) {
        id[i] = caractere[rand() % strlen(caractere)];
    }
    id[6] = '\0';
}

Joc* cautaJoc(const char* id) {
    for(int i = 0; i < numarJocuri; i++) {
        if(strcmp(jocuri[i].id, id) == 0 && jocuri[i].activ) {
            return &jocuri[i];
        }
    }
    return NULL;
}

Joc* creeazaJoc() {
    if(numarJocuri >= MAX_JOCURI) return NULL;
    Joc* joc = &jocuri[numarJocuri++];
    genereazaIdJoc(joc->id);
    joc->conexiuniClienti[0] = -1;
    joc->conexiuniClienti[1] = -1;
    joc->jucatorCurent = 0;
    joc->activ = 1;
    initializeazaTabla(joc->tabla);
    return joc;
}

Joc* cautaJocDisponibil() {
    for(int i = 0; i < numarJocuri; i++) {
        if(jocuri[i].activ && jocuri[i].conexiuniClienti[1] == -1) {
            return &jocuri[i];
        }
    }
    return NULL;
}

void deseneazaTabla(Joc* joc, char* buffer) {
    sprintf(buffer, "\n %c | %c | %c \n---+---+---\n %c | %c | %c \n---+---+---\n %c | %c | %c \n",joc->tabla[0][0], joc->tabla[0][1], joc->tabla[0][2],joc->tabla[1][0], joc->tabla[1][1], joc->tabla[1][2],
            joc->tabla[2][0], joc->tabla[2][1], joc->tabla[2][2]);
}

int verificaCastigator(Joc* joc) {
    for (int i = 0; i < 3; i++) {
        if (joc->tabla[i][0] != ' ' && joc->tabla[i][0] == joc->tabla[i][1] && joc->tabla[i][1] == joc->tabla[i][2]) 
            return 1;
        if (joc->tabla[0][i] != ' ' && joc->tabla[0][i] == joc->tabla[1][i] && joc->tabla[1][i] == joc->tabla[2][i]) 
            return 1;
    }
    if (joc->tabla[0][0] != ' ' && joc->tabla[0][0] == joc->tabla[1][1] && joc->tabla[1][1] == joc->tabla[2][2]) 
        return 1;
    if (joc->tabla[0][2] != ' ' && joc->tabla[0][2] == joc->tabla[1][1] && joc->tabla[1][1] == joc->tabla[2][0]) 
        return 1;
    return 0;
}

void trimiteDate(int socket_fd, const char* data, size_t len) {
    if (write(socket_fd, data, len) < 0) {
        perror("Eroare trimitere date");
        exit(1);
    }
}

void *proceseazaClient(void *arg) {
    int socket_fd = *(int *)arg;
    free(arg);
    char buffer[1024];
    Joc* joc = NULL;
    int numarJucator;
    ssize_t citit = read(socket_fd, buffer, sizeof(buffer) - 1);
    if(citit < 0) {
        perror("Eroare la citirea comenzii inițiale");
        close(socket_fd);
        return NULL;
    }
    buffer[citit] = '\0';
    pthread_mutex_lock(&mutexJoc);
    if(strcmp(buffer, "NEW") == 0) {
        joc = creeazaJoc();
        if(joc) {
            joc->conexiuniClienti[0] = socket_fd;
            numarJucator = 0;
            trimiteDate(socket_fd, joc->id, strlen(joc->id));
        }
    } else if(strcmp(buffer, "RANDOM") == 0) {
        joc = cautaJocDisponibil();
        if(!joc) {
            joc = creeazaJoc();
            numarJucator = 0;
        } else {
            numarJucator = 1;
        }
        if(joc) {
            joc->conexiuniClienti[numarJucator] = socket_fd;
        }
    } else {
        joc = cautaJoc(buffer);
        if(joc && joc->conexiuniClienti[1] == -1) {
            joc->conexiuniClienti[1] = socket_fd;
            numarJucator = 1;
        }
    }
    pthread_mutex_unlock(&mutexJoc);
    if(!joc) {
        trimiteDate(socket_fd, "ERROR", 5);
        close(socket_fd);
        return NULL;
    }
    while(joc->conexiuniClienti[1] == -1) {
        usleep(100000);
    }
    if (numarJucator == 1) {
        trimiteDate(joc->conexiuniClienti[0], "START", 5);
        trimiteDate(joc->conexiuniClienti[1], "START", 5);
    }
    while(1) {
        pthread_mutex_lock(&mutexJoc);
        if(joc->jucatorCurent == numarJucator) {
            trimiteDate(socket_fd, "TURN", 4);
            pthread_mutex_unlock(&mutexJoc);
            memset(buffer, 0, sizeof(buffer));
            ssize_t citit = read(socket_fd, buffer, sizeof(buffer) - 1);
            if(citit <= 0) {
                if(citit < 0) {
                    perror("Eroare la citirea mutării");
                }
                break;
            }
            int linie, coloana;
            sscanf(buffer, "%d %d", &linie, &coloana);
            pthread_mutex_lock(&mutexJoc);
            if(linie >= 0 && linie < 3 && coloana >= 0 && coloana < 3 && joc->tabla[linie][coloana] == ' ') {
                joc->tabla[linie][coloana] = (numarJucator == 0) ? 'X' : 'O';
                deseneazaTabla(joc, buffer);
                trimiteDate(joc->conexiuniClienti[0], buffer, strlen(buffer));
                trimiteDate(joc->conexiuniClienti[1], buffer, strlen(buffer));
                if(verificaCastigator(joc)) {
                    sprintf(buffer, "\nJucătorul %d a câștigat!\n", numarJucator + 1);
                    trimiteDate(joc->conexiuniClienti[0], buffer, strlen(buffer));
                    trimiteDate(joc->conexiuniClienti[1], buffer, strlen(buffer));
                    joc->activ = 0;
                    pthread_mutex_unlock(&mutexJoc);
                    break;
                }
                joc->jucatorCurent = 1 - joc->jucatorCurent;
            }
            pthread_mutex_unlock(&mutexJoc);
        } else {
            pthread_mutex_unlock(&mutexJoc);
            usleep(100000);
        }
    }
    close(socket_fd);
    return NULL;
}

int main() {
    int socket_server;
    struct sockaddr_in adresaServer;
    int optiune = 1;
    if ((socket_server = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Eroare la crearea socket-ului server");
        return 1;
    }
    if (setsockopt(socket_server, SOL_SOCKET, SO_REUSEADDR, &optiune, sizeof(optiune)) < 0) {
        perror("Eroare la setarea opțiunilor socket");
        return 1;
    }
    adresaServer.sin_family = AF_INET;
    adresaServer.sin_addr.s_addr = INADDR_ANY;
    adresaServer.sin_port = htons(PORT);
    if (bind(socket_server, (struct sockaddr *)&adresaServer, sizeof(adresaServer)) < 0) {
        perror("Eroare la bind");
        close(socket_server);
        return 1;
    }
    if (listen(socket_server, MAX_CLIENTI) < 0) {
        perror("Eroare la listen");
        close(socket_server);
        return 1;
    }
    printf("Serverul a pornit pe portul %d\n", PORT);
    pthread_t idThread[1000];
    int numarClienti = 0;
    while(1) {
        struct sockaddr_in adresaClient;
        socklen_t lungimeAdresa = sizeof(adresaClient);
        int socket_client = accept(socket_server, (struct sockaddr *)&adresaClient, &lungimeAdresa);
        if(socket_client < 0) {
            perror("Eroare la accept");
            continue;
        }
        printf("Client nou conectat.\n");
        int *socket_nou = malloc(sizeof(int));
        if (!socket_nou) {
            perror("Eroare la alocarea memoriei");
            close(socket_client);
            continue;
        }
        *socket_nou = socket_client;
        if (pthread_create(&idThread[numarClienti], NULL, proceseazaClient, (void*)socket_nou) != 0) {
            perror("Eroare la crearea thread-ului");
            free(socket_nou);
            close(socket_client);
            continue;
        }
        pthread_detach(idThread[numarClienti]);
        if(numarClienti == 999) {
            printf("Număr maxim de clienți atins\n");
            break;
        }
        numarClienti++;
    }
    close(socket_server);
    return 0;
}