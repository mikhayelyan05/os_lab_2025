#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

// Глобальные переменные для результата и мьютекса
long long result = 1;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Структура для передачи параметров в поток
typedef struct {
    long long start;
    long long end;
    long long mod;
} thread_data_t;

// Функция для вычисления частичного произведения
void* partial_factorial(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    long long partial = 1;
    
    for (long long i = data->start; i <= data->end; ++i) {
        partial = (partial * i) % data->mod;
    }
    
    // Защищаем доступ к общему результату с помощью мьютекса
    pthread_mutex_lock(&mutex);
    result = (result * partial) % data->mod;
    pthread_mutex_unlock(&mutex);
    
    return NULL;
}

int main(int argc, char* argv[]) {
    long long k = 0, pnum = 0, mod = 0;

    //разбор аргументов
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-k") == 0 && i + 1 < argc) {
            k = atoll(argv[++i]);
        } else if (strstr(argv[i], "--pnum=") == argv[i]) {
            pnum = atoll(argv[i] + 7);
        } else if (strstr(argv[i], "--mod=") == argv[i]) {
            mod = atoll(argv[i] + 6);
        }
    }

    // Проверка, что все параметры установлены
    if (k == 0 || pnum == 0 || mod == 0) {
        printf("Использование: %s -k <число> --pnum=<потоки> --mod=<модуль>\n", argv[0]);
        printf("Пример: %s -k 10 --pnum=4 --mod=10\n", argv[0]);
        return 1;
    }

    // Остальной код остается без изменений...
    // [остальная часть кода такая же как выше]
    
    // Проверка входных данных
    if (k < 0) {
        printf("Ошибка: k должно быть неотрицательным числом\n");
        return 1;
    }
    if (pnum <= 0) {
        printf("Ошибка: pnum должно быть положительным числом\n");
        return 1;
    }
    if (mod <= 0) {
        printf("Ошибка: mod должно быть положительным числом\n");
        return 1;
    }

    // Если k = 0, факториал = 1
    if (k == 0) {
        printf("0! mod %lld = 1\n", mod);
        return 0;
    }

    // Определение размера блока для каждого потока
    long long base = k / pnum;
    long long remainder = k % pnum;
    pthread_t* threads = malloc(pnum * sizeof(pthread_t));
    thread_data_t* thread_data = malloc(pnum * sizeof(thread_data_t));
    
    if (threads == NULL || thread_data == NULL) {
        printf("Ошибка выделения памяти\n");
        return 1;
    }

    long long start = 1;
    int thread_count = 0;

    // Создание и запуск потоков
    for (int i = 0; i < pnum; ++i) {
        long long end = start + base - 1;
        if (i < remainder) {
            end += 1;
        }
        
        if (start > k) {
            break;
        }

        thread_data[i].start = start;
        thread_data[i].end = end;
        thread_data[i].mod = mod;

        if (pthread_create(&threads[i], NULL, partial_factorial, &thread_data[i]) != 0) {
            printf("Ошибка создания потока %d\n", i);
            free(threads);
            free(thread_data);
            return 1;
        }
        thread_count++;

        start = end + 1;
    }

    // Ожидание завершения всех потоков
    for (int i = 0; i < thread_count; ++i) {
        pthread_join(threads[i], NULL);
    }

    printf("%lld! mod %lld = %lld\n", k, mod, result);

    // Освобождение ресурсов
    free(threads);
    free(thread_data);
    pthread_mutex_destroy(&mutex);

    return 0;
}