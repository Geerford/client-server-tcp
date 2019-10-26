#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <stddef.h>
#include "state.h"

#define MAX_CURRENT_THREADS_COUNT 10
#define THREADS 5
#define MAX_WAITED_THREADS_COUNT 7
#define MAX_FILENAME 260
#define SEND_ERROR_MSG "Failed while read directory\n"

pthread_mutex_t mutex_start, mutex_socket;
pthread_cond_t condition;
size_t waiting_threads, created_threads;
int input_socket_fd, fd = -1;
pthread_t threads[MAX_CURRENT_THREADS_COUNT];
char isWorked[MAX_CURRENT_THREADS_COUNT];

void close_socket_fd();
void close_fd(int func);
void *process_request(void *arg);
int get_directory_name(char *path, char **name_b);
void free_names(char *names);
void print_closed_socket(int);
void start_receive(int);

int main(void){
    size_t i;
    struct sockaddr_in addr;
    close_socket_fd();
    if (pthread_mutex_init(&mutex_start, NULL) != 0 || pthread_mutex_init(&mutex_socket, NULL) != 0){
        perror("Exception: Failed initialize the mutex");
        return -1;
    }
    while (created_threads < THREADS){
        if (pthread_create(&threads[created_threads], NULL, process_request,
                isWorked + created_threads) != 0){
            perror("Exception: Failed create thread");
            return -1;
        }
		++created_threads;
    }
    if ((fd = socket(AF_INET, SOCK_STREAM, SOCKET_PROTOCOL)) == -1){
        perror("Exception: Failed create an endpoint for communication");
        return -1;
    }
    addr.sin_family = AF_INET;
    addr.sin_port = htons(TCP_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1){
        perror("Exception: Failed bind a socket");
        return -1;
    }
    if (listen(fd, MAX_CURRENT_THREADS_COUNT) == -1){
        perror("Exception: Failed listen for connections a socket");
        return -1;
    }
    while (1){
        if (waiting_threads <= 0) {
            if (created_threads < MAX_CURRENT_THREADS_COUNT){
                if (pthread_create(&threads[created_threads], NULL, process_request,
                        isWorked + created_threads) != 0){
                    perror("Exception: Failed create thread");
                    return -1;
                }
			    ++created_threads;
            }
        }
        pthread_mutex_lock(&mutex_start);
        while (1) {
            if (waiting_threads > MAX_WAITED_THREADS_COUNT) {
                for (i = 0; i < MAX_CURRENT_THREADS_COUNT; i++) {
                    if (isWorked[i] == 1) {
                        pthread_cancel(threads[i]);
                    }
                }
            }
            else{
                break;
            }
        }
        pthread_mutex_unlock(&mutex_start);
        pthread_mutex_lock(&mutex_socket);
        if ((input_socket_fd = accept(fd, NULL, NULL)) == -1){
            print_closed_socket(fd);
        }
        if (waiting_threads <= 0){
            close(input_socket_fd);
        }
        else{
            pthread_cond_signal(&condition);
        }
    }
}

void close_fd(int func){
    if(func == -1){

    }
    if (fd != -1) {
        close(fd);
    }
}

void close_socket_fd(void){
    signal(SIGINT, &close_fd);
    signal(SIGTERM, &close_fd);
}

void *process_request(void *arg){
    int socket_fd;
    char *val = arg;
    while (1){
        pthread_mutex_lock(&mutex_start);
        ++waiting_threads;
        *val = 1;
        if(pthread_cond_wait(&condition, &mutex_start) != 0){
            perror("Exception: Failed block mutex by condition");
            return (void *) -1;
        }
        --waiting_threads;
        socket_fd = input_socket_fd;
        *val = 0;
        pthread_mutex_unlock(&mutex_start);
        pthread_mutex_unlock(&mutex_socket);
        start_receive(socket_fd);
    }
}

int get_directory_name(char *path, char **name_b){
    DIR *directory;
    struct dirent *directory_entry;
    size_t count = 0, size;
    size_t allocated_memory = STD_ALLOCATE_COUNT;
    char *names = NULL;
    names = (char *) malloc(sizeof(char) * STD_ALLOCATE_COUNT);
    if ((directory = opendir(path)) == NULL) {
        return -1;
    }
    while ((directory_entry = readdir(directory)) != NULL){
        size = strlen(directory_entry -> d_name);
        if (allocated_memory <= count + size + 2) {
            allocated_memory = allocated_memory * 3 / 2;
            names = (char *) realloc(names, allocated_memory * sizeof(char));
            if (names == NULL){
                perror("Exception: Failed realloc memory");
                return -1;
            }
        }
        memcpy(names + count, directory_entry -> d_name, size);
        count += size;
        names[count] = '\n';
        names[count + 1] = '\0';
        count += 2;
    }
    closedir(directory);
    *name_b = names;
    return count;
}

void free_names(char *names){
    free(names);
}

void print_closed_socket(int socket_fd){
    close(socket_fd);
    while (--created_threads > 0){
        pthread_cancel(threads[created_threads]);
    }
    pthread_mutex_destroy(&mutex_socket);
    pthread_mutex_destroy(&mutex_start);
    perror("Exception: Failed send the message");
}

void start_receive(int socket_fd){
    char buff[MAX_FILENAME];
    char *names;
    ssize_t i;
    while (1) {
        if ((i = recv(socket_fd, buff, MAX_FILENAME, FLAG)) <= 0) {
            close(socket_fd);
            return;
        }
        buff[i] = '\0';
        for (i = 0; i < MAX_FILENAME; i++) {
            if (buff[i] == '\0') {
                if (buff[--i] == '\n'){
                    buff[i] = '\0';
                }
                if (buff[--i] == '\r'){
                    buff[i] = '\0';
                }
                i = -1;
                break;
            }
        }
        if (!strcmp(buff, "exit")){
            close(socket_fd);
            return;
        }
        if (i != -1) {
            close(socket_fd);
            return;
        }
        i = get_directory_name(buff, &names);
        if (i < 0){
            if(send(socket_fd, SEND_ERROR_MSG, strlen(SEND_ERROR_MSG) + 1, FLAG) == -1){
                print_closed_socket(socket_fd);
            }
            continue;
        }
        if (send(socket_fd, names, i, FLAG) == -1){
            print_closed_socket(socket_fd);
        }
        free_names(names);
    }
}
