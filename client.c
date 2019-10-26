#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "state.h"
#include <arpa/inet.h>

#define BUF_SIZE 1024*1024

void print_closed_socket(int, char*);

int comparator(const char*, const char*);

int sort(char*, int);

int main(int argc, char **argv){
    char *path, *buffer;
    int socket_fd;
    long input_len;
    struct sockaddr_in addr;
    ssize_t data;
    if (argc < 3) {
        perror("Exception: Invalid arguments count\n"
               "Usage: ./client 127.0.0.1 /home/nevdev/Desktop/client-server-tcp/tmp");
        return -1;
    }
    path = malloc(sizeof(char) * 260);
    buffer = malloc(sizeof(char) * BUF_SIZE);
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, SOCKET_PROTOCOL)) == -1){
        perror("Exception: Failed create an endpoint for communication");
        return -1;
    }
    if (strcmp(*argv, "localhost")){
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    }
    else {
        if ((addr.sin_addr.s_addr = inet_addr(*argv)) == (unsigned int)-1){
            perror("Exception: Uncorrected IP (arg[0])");
            return -1;
        }
    }
    addr.sin_family = AF_INET;
    addr.sin_port = htons(TCP_PORT);
    if (connect(socket_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1){
        perror("Exception: Bad connection to socket");
        return -1;
    }
    while (*(++argv) != NULL){
        strcpy(path, *argv);
        input_len = strlen(path);
        path[input_len++] = '\r';
        path[input_len++] = '\n';
        path[input_len++] = '\0';
        if (send(socket_fd, path, input_len, FLAG) == -1){
            print_closed_socket(socket_fd, "Exception: Failed send the message");
        }
        while ((data = recv(socket_fd, buffer, BUF_SIZE, FLAG)) < 0) {
            print_closed_socket(socket_fd, "Exception: Failed receive the data");
        }
        if (data == 0){
            break;
        }
        printf("%s\n", *argv);
        sort(buffer, data);
    }
    close(socket_fd);
    return 0;
}

void print_closed_socket(int socket_fd, char* msg){
    close(socket_fd);
    perror(msg);
}

int comparator(const char* first, const char* second){
    return strcasecmp(*(char**)first, *(char**)second);
}

int sort(char *buff, int data){
    char **lines;
    int allocated_size = STD_ALLOCATE_COUNT;
    int start_position = 0, i, lines_count = 0;
    lines = malloc(allocated_size * sizeof(char*));
    for (i = 0; i < data; i++){
        if (buff[i] == '\0'){
            if(allocated_size <= lines_count){
                allocated_size = allocated_size * 3 / 2;
                lines = (char**)realloc(lines, allocated_size * sizeof(char *));
                if (lines == NULL){
                    perror("Exception: Failed realloc memory");
                    return -1;
                }
            }
            lines[lines_count] = malloc(sizeof(char) * (++i - start_position));
            memcpy(lines[lines_count++], buff + start_position, i - start_position);
            start_position = i--;
        }
    }
    qsort(lines, lines_count, sizeof(char*), (__compar_fn_t) comparator);
    for(i = 0; i < lines_count; i++){
        if (write(STDOUT_FILENO, lines[i], strlen(lines[i])) < 0){
            perror("Exception: Failed receive data");
            return -1;
        }
        free(lines[i]);
    }
    free(lines);
    return 0;
}
