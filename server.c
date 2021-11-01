#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>

#define PORT 6666
#define BUF_SIZE 128

struct client_t
{
    int socket;
    int rxState;
};

void *readThread(void *arg);

//ADAPTAME PARA RECIBIR MÁS CLIENTES
int main(int argc, char *argv[])
{

    char buf[BUF_SIZE];
    int status;
    int enable = 1;
    int server_sd;
    int client_sd;
    pthread_t rxThreadId;
    struct client_t client;

    // 1. Ignore SIGPIPE
    signal(SIGPIPE, SIG_IGN);

    // 2. Create socket
    server_sd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sd == -1)
    {
        perror("Socket creation fails\n");
        exit(EXIT_FAILURE);
    }
    printf("Socket created\n");

    // 3. turn off bind address checking
    status = setsockopt(server_sd, SOL_SOCKET, SO_REUSEADDR, (int *)&enable, sizeof(enable));
    if (-1 == status)
    {
        perror("setsockopt error: ");
    }

    //4. BIND the socket to an address
    // Prepare the address
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    status = bind(server_sd, (struct sockaddr *)&addr, sizeof(addr));
    if (-1 == status)
    {
        perror("Bind fails: ");
        close(server_sd);
        exit(EXIT_FAILURE);
    }
    printf("Socket binded\n");

    // 5. Set backlog

    status = listen(server_sd, 1);

    if (-1 == status)
    {
        perror("Listen fails: ");
        close(server_sd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening\n");

    while (1)
    {
        // 6. Accept:
        printf("Waiting for a client\n");
        client_sd = accept(server_sd, NULL, NULL);

        printf("Client connected\n");
        if (-1 == client_sd)
        {
            perror("Accept fails: ");
            close(server_sd);
            exit(EXIT_FAILURE);
        }
        // 7. Create a thread for receiving messages from client
        client.socket = client_sd;
        client.rxState = 1;

        printf("Create Pthread for reading\n");
        status = pthread_create(&rxThreadId, NULL, &readThread, &client);
        if (-1 == status)
        {
            perror("Pthread read fails: ");
            close(server_sd);
            exit(EXIT_FAILURE);
        }

        while (1)
        {
            if (0 == client.rxState)
            {
                printf("Client closed the socket\n");
                break;
            }

            if (fgets(buf, BUF_SIZE, stdin) == NULL)
            {
                printf("Fgets NULL\n");
            }

            if (buf[strlen(buf) - 1] == '\n')
                buf[strlen(buf) - 1] = 0;

            status = write(client.socket, buf, strlen(buf) + 1);
            if (-1 == status)
            {
                perror("Server write to client fails: ");
                break;
            }
        }
        close(client.socket);
    }

    exit(EXIT_SUCCESS);
}

void *readThread(void *arg)
{
    struct client_t *client = ((struct client_t *)arg);
    ssize_t numOfBytes;
    char buf[BUF_SIZE];

    while (1)
    {
        numOfBytes = read(client->socket, buf, BUF_SIZE);
        if (0 == numOfBytes)
        {
            printf("client closed the socket end\n");
            break;
        }
        else if (-1 == numOfBytes)
        {
            perror("ReadThread read() fails: ");
            break;
        }
        else
        {
            printf("from client: %s\n", buf);
        }
    }
    printf("Terminate Pthread for reading\n");
    client->rxState = 0;
    return NULL;
}
