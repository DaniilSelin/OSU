#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>

// Монитор
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

// Флаг готовности события
bool ready = false;

// Функция поставщика
void *provide() {
    while (1) {
        sleep(1); // Задержка 1 секунда
        
        pthread_mutex_lock(&lock);
        
        while (ready) {
            // Если потребитель ещё не обработал событие, ждём
            pthread_cond_wait(&cond, &lock);
        }

        ready = true;
        printf("ПИНГ\n");
        
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&lock);
    }
    return NULL;
}

// Функция потребителя
void *consume() {
    while (1) {
        pthread_mutex_lock(&lock);
        
        while (!ready) {
            // ждём-с событий
            pthread_cond_wait(&cond, &lock);
        }

        ready = false;
        printf("ПОНГ\n");
        
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&lock);
    }
    return NULL;
}

int main() {
    pthread_t producer, consumer;

    // Создаём потоки и проверяем на ошибки
    if (pthread_create(&producer, NULL, provide, NULL) != 0) {
        perror("Не удалось создать поток-поставщик");
        exit(1);
    }

    if (pthread_create(&consumer, NULL, consume, NULL) != 0) {
        perror("Не удалось создать поток-потребитель");
        exit(1);
    }

    // Ожидаем завершения потоков, хотя они бесконечны))))
    pthread_join(producer, NULL);
    pthread_join(consumer, NULL);

    return 0;
}
