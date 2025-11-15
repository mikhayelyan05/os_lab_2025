#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

// Два мьютекса для демонстрации deadlock
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;

// Общие ресурсы
int resource1 = 100;
int resource2 = 200;

// Функция для первого потока
void* thread1_function(void* arg) {
    printf("Поток 1: пытается захватить мьютекс 1...\n");
    pthread_mutex_lock(&mutex1);
    printf("Поток 1: захватил мьютекс 1\n");
    
    // Имитация работы с ресурсом
    sleep(1);
    
    printf("Поток 1: пытается захватить мьютекс 2...\n");
    pthread_mutex_lock(&mutex2);
    printf("Поток 1: захватил мьютекс 2\n");
    
    // Критическая секция (никогда не выполнится из-за deadlock)
    resource1 += 50;
    resource2 += 50;
    printf("Поток 1: выполнил работу: resource1=%d, resource2=%d\n", resource1, resource2);
    
    pthread_mutex_unlock(&mutex2);
    pthread_mutex_unlock(&mutex1);
    
    return NULL;
}

// Функция для второго потока
void* thread2_function(void* arg) {
    printf("Поток 2: пытается захватить мьютекс 2...\n");
    pthread_mutex_lock(&mutex2);
    printf("Поток 2: захватил мьютекс 2\n");
    
    // Имитация работы с ресурсом
    sleep(1);
    
    printf("Поток 2: пытается захватить мьютекс 1...\n");
    pthread_mutex_lock(&mutex1);
    printf("Поток 2: захватил мьютекс 1\n");
    
    // Критическая секция (никогда не выполнится из-за deadlock)
    resource1 -= 30;
    resource2 -= 30;
    printf("Поток 2: выполнил работу: resource1=%d, resource2=%d\n", resource1, resource2);
    
    pthread_mutex_unlock(&mutex1);
    pthread_mutex_unlock(&mutex2);
    
    return NULL;
}

int main() {
    pthread_t thread1, thread2;
    
    printf("=== Демонстрация Deadlock ===\n");
    printf("Начальные значения: resource1=%d, resource2=%d\n\n", resource1, resource2);
    
    // Создаем потоки
    if (pthread_create(&thread1, NULL, thread1_function, NULL) != 0) {
        perror("Ошибка создания потока 1");
        return 1;
    }
    
    if (pthread_create(&thread2, NULL, thread2_function, NULL) != 0) {
        perror("Ошибка создания потока 2");
        return 1;
    }
    
    // Даем потокам время на выполнение
    sleep(3);
    
    // Проверяем, живут ли еще потоки
    printf("\n=== Проверка состояния через 3 секунды ===\n");
    
    // Пытаемся присоединиться к потокам с таймаутом
    printf("Программа заблокирована в состоянии deadlock!\n");
    printf("Поток 1 заблокировал мьютекс 1 и ждет мьютекс 2\n");
    printf("Поток 2 заблокировал мьютекс 2 и ждет мьютекс 1\n");
    printf("Программа никогда не завершится самостоятельно...\n");
    
    // Ожидаем завершения потоков (которого никогда не произойдет)
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    
    printf("Эта строка никогда не будет напечатана\n");
    
    return 0;
}