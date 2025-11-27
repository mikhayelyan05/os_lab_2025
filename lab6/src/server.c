#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <getopt.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "pthread.h"
#include "common.h"

// Функция вычисления факториала для диапазона
uint64_t Factorial(const struct FactorialArgs *args) {
    uint64_t ans = 1;

    if (args->begin > args->end) {
        return 1;
    }

    for (uint64_t i = args->begin; i <= args->end; i++) {
        ans = MultModulo(ans, i, args->mod);
    }

    return ans;
}

void *ThreadFactorial(void *args) {
    struct FactorialArgs *fargs = (struct FactorialArgs *)args;
    uint64_t* result = malloc(sizeof(uint64_t));
    *result = Factorial(fargs);
    return (void*)result;
}

int main(int argc, char **argv) {
    int tnum = -1;
    int port = -1;

    while (true) {
        static struct option options[] = {{"port", required_argument, 0, 0},
                                        {"tnum", required_argument, 0, 0},
                                        {0, 0, 0, 0}};

        int option_index = 0;
        int c = getopt_long(argc, argv, "", options, &option_index);

        if (c == -1)
            break;

        switch (c) {
        case 0: {
            switch (option_index) {
            case 0:
                port = atoi(optarg);
                if (port <= 0) {
                    fprintf(stderr, "Port must be positive number\n");
                    return 1;
                }
                break;
            case 1:
                tnum = atoi(optarg);
                if (tnum <= 0) {
                    fprintf(stderr, "Thread number must be positive\n");
                    return 1;
                }
                break;
            default:
                printf("Index %d is out of options\n", option_index);
            }
        } break;

        case '?':
            printf("Unknown argument\n");
            break;
        default:
            fprintf(stderr, "getopt returned character code 0%o?\n", c);
        }
    }

    if (port == -1 || tnum == -1) {
        fprintf(stderr, "Using: %s --port 20001 --tnum 4\n", argv[0]);
        return 1;
    }

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        fprintf(stderr, "Can not create server socket!");
        return 1;
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons((uint16_t)port);
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    int opt_val = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val));

    int err = bind(server_fd, (struct sockaddr *)&server, sizeof(server));
    if (err < 0) {
        fprintf(stderr, "Can not bind to socket!");
        close(server_fd);
        return 1;
    }

    err = listen(server_fd, 128);
    if (err < 0) {
        fprintf(stderr, "Could not listen on socket\n");
        close(server_fd);
        return 1;
    }

    printf("Server listening at %d with %d threads\n", port, tnum);

    while (true) {
        struct sockaddr_in client;
        socklen_t client_len = sizeof(client);
        int client_fd = accept(server_fd, (struct sockaddr *)&client, &client_len);

        if (client_fd < 0) {
            fprintf(stderr, "Could not establish new connection\n");
            continue;
        }

        while (true) {
            unsigned int buffer_size = sizeof(uint64_t) * 3;
            char from_client[buffer_size];
            int read_bytes = recv(client_fd, from_client, buffer_size, 0);

            if (read_bytes == 0) {
                break;
            }
            
            if (read_bytes < 0) {
                fprintf(stderr, "Client read failed\n");
                break;
            }
            
            if (read_bytes < (int)buffer_size) {
                fprintf(stderr, "Client send wrong data format\n");
                break;
            }

            uint64_t begin = 0;
            uint64_t end = 0;
            uint64_t mod = 0;
            memcpy(&begin, from_client, sizeof(uint64_t));
            memcpy(&end, from_client + sizeof(uint64_t), sizeof(uint64_t));
            memcpy(&mod, from_client + 2 * sizeof(uint64_t), sizeof(uint64_t));

            printf("Receive from client: begin=%" PRIu64 ", end=%" PRIu64 ", mod=%" PRIu64 "\n", begin, end, mod);

            // Проверка валидности данных
            if (begin > end || mod == 0) {
                fprintf(stderr, "Invalid data from client\n");
                break;
            }

            // Распределение работы между потоками
            pthread_t threads[tnum];
            struct FactorialArgs args[tnum];
            
            uint64_t range = end - begin + 1;
            uint64_t base = range / tnum;
            uint64_t remainder = range % tnum;
            uint64_t current_start = begin;

            // Создание потоков для параллельного вычисления
            for (int i = 0; i < tnum; i++) {
                args[i].begin = current_start;
                args[i].end = current_start + base - 1;
                if (i < (int)remainder) {
                    args[i].end += 1;
                }
                args[i].mod = mod;

                // Проверка границ
                if (args[i].begin > end) {
                    args[i].begin = 1;
                    args[i].end = 1;
                } else if (args[i].end > end) {
                    args[i].end = end;
                }

                if (pthread_create(&threads[i], NULL, ThreadFactorial, (void *)&args[i])) {
                    fprintf(stderr, "Error: pthread_create failed!\n");
                    break;
                }

                current_start = args[i].end + 1;
            }

            // Сбор результатов от всех потоков
            uint64_t total = 1;
            for (int i = 0; i < tnum; i++) {
                uint64_t *result_ptr = NULL;
                pthread_join(threads[i], (void **)&result_ptr);
                if (result_ptr) {
                    total = MultModulo(total, *result_ptr, mod);
                    free(result_ptr);
                }
            }

            printf("Computed result: %" PRIu64 "\n", total);

            // Отправка результата клиенту
            char buffer[sizeof(total)];
            memcpy(buffer, &total, sizeof(total));
            err = send(client_fd, buffer, sizeof(total), 0);
            if (err < 0) {
                fprintf(stderr, "Can't send data to client\n");
                break;
            }
            
            printf("Result sent to client\n");
            break;
        }

        shutdown(client_fd, SHUT_RDWR);
        close(client_fd);
        printf("Connection closed\n");
    }

    close(server_fd);
    return 0;
}