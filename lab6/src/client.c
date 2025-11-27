#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>

#include "common.h"

#define MAX_SERVERS 100

// Структура для передачи данных в поток
typedef struct {
    struct Server server;
    uint64_t begin;
    uint64_t end;
    uint64_t mod;
    uint64_t result;
    int completed;
} thread_data_t;

// Глобальные переменные для многопоточности
uint64_t final_result = 1;
pthread_mutex_t result_mutex = PTHREAD_MUTEX_INITIALIZER;

// Функция потока для обработки одного сервера
void* handle_server_thread(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    
    struct hostent *hostname = gethostbyname(data->server.ip);
    if (hostname == NULL) {
        fprintf(stderr, "gethostbyname failed with %s\n", data->server.ip);
        data->completed = -1;
        return NULL;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(data->server.port);
    
    if (hostname->h_addr_list[0] != NULL) {
        server_addr.sin_addr.s_addr = *((unsigned long *)hostname->h_addr_list[0]);
    } else {
        fprintf(stderr, "No address found for %s\n", data->server.ip);
        data->completed = -1;
        return NULL;
    }

    int sck = socket(AF_INET, SOCK_STREAM, 0);
    if (sck < 0) {
        fprintf(stderr, "Socket creation failed!\n");
        data->completed = -1;
        return NULL;
    }

    if (connect(sck, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        fprintf(stderr, "Connection to %s:%d failed\n", data->server.ip, data->server.port);
        close(sck);
        data->completed = -1;
        return NULL;
    }

    // Подготовка и отправка задачи
    char task[sizeof(uint64_t) * 3];
    memcpy(task, &data->begin, sizeof(uint64_t));
    memcpy(task + sizeof(uint64_t), &data->end, sizeof(uint64_t));
    memcpy(task + 2 * sizeof(uint64_t), &data->mod, sizeof(uint64_t));

    if (send(sck, task, sizeof(task), 0) < 0) {
        fprintf(stderr, "Send to %s:%d failed\n", data->server.ip, data->server.port);
        close(sck);
        data->completed = -1;
        return NULL;
    }

    printf("Отправлено серверу %s:%d: диапазон [%" PRIu64 ", %" PRIu64 "] mod %" PRIu64 "\n",
           data->server.ip, data->server.port, data->begin, data->end, data->mod);

    // Получение ответа
    char response[sizeof(uint64_t)];
    ssize_t bytes_received = recv(sck, response, sizeof(response), 0);
    if (bytes_received < 0) {
        fprintf(stderr, "Receive from %s:%d failed\n", data->server.ip, data->server.port);
        close(sck);
        data->completed = -1;
        return NULL;
    }

    if (bytes_received != sizeof(uint64_t)) {
        fprintf(stderr, "Invalid response size from %s:%d\n", data->server.ip, data->server.port);
        close(sck);
        data->completed = -1;
        return NULL;
    }

    memcpy(&data->result, response, sizeof(uint64_t));
    data->completed = 1;
    
    printf("Получен результат от %s:%d: %" PRIu64 "\n", 
           data->server.ip, data->server.port, data->result);

    // Обновление общего результата
    pthread_mutex_lock(&result_mutex);
    final_result = MultModulo(final_result, data->result, data->mod);
    pthread_mutex_unlock(&result_mutex);

    close(sck);
    return NULL;
}

int main(int argc, char **argv) {
    uint64_t k = 0;
    uint64_t mod = 0;
    char servers_file[255] = {'\0'};

    while (true) {
        static struct option options[] = {{"k", required_argument, 0, 0},
                                        {"mod", required_argument, 0, 0},
                                        {"servers", required_argument, 0, 0},
                                        {0, 0, 0, 0}};

        int option_index = 0;
        int c = getopt_long(argc, argv, "", options, &option_index);

        if (c == -1)
            break;

        switch (c) {
        case 0: {
            switch (option_index) {
            case 0:
                ConvertStringToUI64(optarg, &k);
                break;
            case 1:
                ConvertStringToUI64(optarg, &mod);
                break;
            case 2:
                strncpy(servers_file, optarg, sizeof(servers_file) - 1);
                servers_file[sizeof(servers_file) - 1] = '\0';
                break;
            default:
                printf("Index %d is out of options\n", option_index);
            }
        } break;

        case '?':
            printf("Arguments error\n");
            break;
        default:
            fprintf(stderr, "getopt returned character code 0%o?\n", c);
        }
    }

    if (k == 0 || mod == 0 || !strlen(servers_file)) {
        fprintf(stderr, "Using: %s --k 1000 --mod 5 --servers /path/to/file\n", argv[0]);
        return 1;
    }

    // Чтение серверов из файла
    struct Server servers[MAX_SERVERS];
    int servers_num = read_servers(servers_file, servers, MAX_SERVERS);
    if (servers_num <= 0) {
        fprintf(stderr, "Не удалось прочитать серверы из файла %s\n", servers_file);
        return 1;
    }

    printf("Найдено %d серверов\n", servers_num);

    // Распределение работы между серверами
    uint64_t base = k / servers_num;
    uint64_t remainder = k % servers_num;
    uint64_t start = 1;

    pthread_t threads[servers_num];
    thread_data_t thread_data[servers_num];
    int active_threads = 0;

    // Создание потоков для каждого сервера
    for (int i = 0; i < servers_num; i++) {
        uint64_t end = start + base - 1;
        if (i < (int)remainder) {
            end += 1;
        }

        if (start > k) {
            break;
        }

        // Инициализация данных для потока
        thread_data[i].server = servers[i];
        thread_data[i].begin = start;
        thread_data[i].end = end;
        thread_data[i].mod = mod;
        thread_data[i].result = 1;
        thread_data[i].completed = 0;

        if (pthread_create(&threads[i], NULL, handle_server_thread, &thread_data[i]) == 0) {
            active_threads++;
            printf("Создан поток для сервера %s:%d (диапазон %" PRIu64 "-%" PRIu64 ")\n",
                servers[i].ip, servers[i].port, start, end);
        } else {
            fprintf(stderr, "Ошибка создания потока для сервера %s:%d\n", servers[i].ip, servers[i].port);
        }

        start = end + 1;
    }

    // Ожидание завершения всех потоков
    for (int i = 0; i < active_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    // Проверка результатов
    int all_success = 1;
    for (int i = 0; i < active_threads; i++) {
        if (thread_data[i].completed != 1) {
            fprintf(stderr, "Ошибка: сервер %s:%d не завершил вычисления\n",
                thread_data[i].server.ip, thread_data[i].server.port);
            all_success = 0;
        }
    }

    if (all_success) {
        printf("\nФинальный результат: %" PRIu64 "! mod %" PRIu64 " = %" PRIu64 "\n", k, mod, final_result);
    } else {
        fprintf(stderr, "\nНе все серверы успешно завершили вычисления\n");
        return 1;
    }

    pthread_mutex_destroy(&result_mutex);
    return 0;
}