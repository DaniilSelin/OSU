#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <errno.h>

#define PORT 8000
#define BUF_SIZE 1024

volatile sig_atomic_t wasSigHup = 0;
int client_socket = -1;

// Обработчик сигнала SIGHUP
void sigHupHandler(int r){
  if (r == SIGHUP) {
    wasSigHup = 1;
  }
}

// Настраивает обработчик сигнала
void setupSignalHandling() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigHupHandler;
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGHUP, &sa, NULL) == -1) {
        perror("Ошибка установки обработчика SIGHUP");
        exit(EXIT_FAILURE);
    }
}

int createServerSocket() {
    int serv_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (serv_socket == -1) {
        perror("Ошибка создания серверного сокета");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(serv_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Ошибка привязки сокета");
        close(serv_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(serv_socket, 1) == -1) {
        perror("Ошибка установки режима прослушивания");
        close(serv_socket);
        exit(EXIT_FAILURE);
    }

    printf("Сервер запущен и слушает порт %d\n", PORT);
    return serv_socket;
}

// Обработчик сигнала SIGHUP
void handleSighup() {
    if (wasSigHup) {
        printf("Получен сигнал SIGHUP.\n");
        if (client_socket != -1) {
            close(client_socket);
            client_socket = -1;
            printf("Сокет клиента закрыт.\n");
        }
        wasSigHup = 0;
    }
}

// Принимает новое соединение
void handleNewConnection(int serv_socket) {
    int new_client_socket = accept(serv_socket, NULL, NULL);
    if (new_client_socket == -1) {
        perror("Ошибка при принятии соединения");
        return;
    }
    printf("Принято новое соединение.\n");

    if (client_socket == -1) {
        client_socket = new_client_socket;
    } else {
        close(new_client_socket);
        printf("Закрыто лишнее соединение.\n");
    }
}

// Читает данные от клиента и обрабатывает их 
void handleClientData() {
    char buf[BUF_SIZE];
    ssize_t size_data = read(client_socket, buf, BUF_SIZE);

    if (size_data > 0) {
        printf("Получено %ld байтов данных.\n", size_data);
    } else if (size_data == 0) {
        printf("Клиент закрыл соединение.\n");
        close(client_socket);
        client_socket = -1;
    } else {
        perror("Ошибка чтения данных");
    }
}

// Основной цикл обработки событий
void eventLoop(int serv_socket, sigset_t origMask) {
    fd_set fds;

    while (1) {
        handleSighup();

        // Настраиваем дескрипторы для pselect
        FD_ZERO(&fds);
        FD_SET(serv_socket, &fds);
        if (client_socket != -1) {
            FD_SET(client_socket, &fds);
        }

        int max_fd = (client_socket > serv_socket) ? client_socket : serv_socket;

        int res = pselect(max_fd + 1, &fds, NULL, NULL, NULL, &origMask);
        if (res == -1) {
            if (errno == EINTR) {
                continue;
            }
            perror("Ошибка pselect");
            break;
        }

        if (FD_ISSET(serv_socket, &fds)) {
            handleNewConnection(serv_socket);
        }

        if (client_socket != -1 && FD_ISSET(client_socket, &fds)) {
            handleClientData();
        }
    }
}

int main() {
    setupSignalHandling();

    int serv_socket = createServerSocket();

    // Блокировка сигнала SIGHUP во время pselect
    sigset_t blockedMask, origMask;
    sigemptyset(&blockedMask);
    sigaddset(&blockedMask, SIGHUP);
    if (sigprocmask(SIG_BLOCK, &blockedMask, &origMask) == -1) {
        perror("Ошибка блокировки сигналов");
        close(serv_socket);
        exit(EXIT_FAILURE);
    }

    eventLoop(serv_socket, origMask);

    close(serv_socket);
    if (client_socket != -1) {
        close(client_socket);
    }
    printf("Сервер завершил работу.\n");

    return 0;
}