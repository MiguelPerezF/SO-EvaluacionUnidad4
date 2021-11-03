#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
//#include <netinet/in.h>
#include<arpa/inet.h>
#include <signal.h>
#include<string.h>

#define PORT 6666
#define BUF_SIZE 1000

struct client_t
{
    int socket;
    int rxState;
};

void *readThread(void *arg);
void *serverCommandsThread(void *arg);
void removeEventsFromClient();

static char *events[10];

//ADÁPTAME PARA RECIBIR MÁS CLIENTES
int main(int argc, char *argv[])
{

    char buf[BUF_SIZE];
    int status;
    int enable = 1;
    int server_sd;
    int client_sd, *new_client;
    int size;
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

    status = listen(server_sd, 3);

    if (-1 == status)
    {
        perror("Listen fails: ");
        close(server_sd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening\n");

    // 6. Accept:
    printf("Waiting for a client\n");
    size = sizeof(struct sockaddr_in);
    status = pthread_create(&rxThreadId, NULL, &serverCommandsThread, &server_sd);

    while ((client_sd = accept(server_sd, (struct sockaddr *)&addr, (socklen_t*)&size)))
    {

        printf("Client connected\n");
        if (-1 == client_sd)
        {
            perror("Accept fails: ");
            close(server_sd);
            exit(EXIT_FAILURE);
        }
        // 7. Create a thread for receiving messages from client
        new_client = malloc(1);
        *new_client = client_sd;
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

    }

    exit(EXIT_SUCCESS);
}

void *readThread(void *arg)
{
    struct client_t client = *((struct client_t *)arg);
    ssize_t numOfBytes;
    char message_from_client[2000];

    while ((numOfBytes = recv(client.socket, message_from_client, BUF_SIZE, 0)) > 0)
    {
        printf("from client: %s\n", message_from_client);

        char * command = strtok(message_from_client, " ");

        if(strcmp(command, "sub") == 0)
        {
            char * event_name = strtok(NULL, " ");
            printf("Subscribe to an event: %s.\n", event_name);
        }
        if(strcmp(command, "unsub") == 0)
        {
            char * event_name = strtok(NULL, " ");
            printf("Unsubscribe from an event: %s.\n", event_name);
        }
        if(strcmp(command, "list") == 0)
        {
            printf("Listing all events client is subscribed to.\n");
        }
        if(strcmp(command, "ask") == 0)
        {
            printf("Listing all available events.\n");
        }
        if(strcmp(command, "save") == 0)
        {
            char * file_name = strtok(NULL, " ");
            printf("Saving file: %s\n", file_name);
        }
        if(strcmp(command, "load") == 0)
        {
            char * file_name = strtok(NULL, " ");
            printf("Loading file: %s\n", file_name);
        }
    }

    if (0 == numOfBytes)
    {
        printf("client closed the socket end\n");
        removeEventsFromClient();
    }
    else if (-1 == numOfBytes)
    {
        perror("ReadThread read() fails: ");
        removeEventsFromClient();
    }

    printf("Terminate Pthread for reading\n");
    client.rxState = 0;
    close(client.socket);
    return 0;
}

// Método que recibe los comandos del servidor
void *serverCommandsThread(void *arg)
{
    int server_sd = ((int)arg);
    char input[BUF_SIZE];


    while (1)
    {

        if (fgets(input, BUF_SIZE, stdin) == NULL)
        {
            printf("Fgets NULL\n");
        }

        if (input[strlen(input) - 1] == '\n')
            input[strlen(input) - 1] = 0;
     
        char * command = strtok(input, " ");

        if((strcmp(command, "exit")) == 0)
        {
            printf("Terminating server.\n");
        }
        if(strcmp(command, "add") == 0)
        {
            char * event_name = strtok(NULL, " ");
            for (int i = 0; i < 10; i++) {
                if(events[i] == NULL)
                {
                    printf("Adding event: %s.\n", event_name);
                    events[i] = event_name;
                    printf("Events: %s.\n", events[i]);
                    break;
                }
            }
        }
        if(strcmp(command, "remove") == 0)
        {
            char * event_name = strtok(NULL, " ");
            printf("Removing event: %s.\n", event_name);
        }
        if(strcmp(command, "trigger") == 0)
        {
            char * event_name = strtok(NULL, " ");
            printf("Triggering event: %s.\n", event_name);
        }
        if(strcmp(command, "list") == 0)
        {
            char * event_name = strtok(NULL, " ");
            printf("Listing clients in event: %s.\n", event_name);
        }
        if(strcmp(command, "all") == 0)
        {
            printf("All events and clients.\n");
        }
        if(strcmp(command, "save") == 0)
        {
            char * file_name = strtok(NULL, " ");
            printf("Saving file: %s\n", file_name);
        }
        if(strcmp(command, "load") == 0)
        {
            char * file_name = strtok(NULL, " ");
            printf("Loading file: %s\n", file_name);
        }

    }
    close(server_sd);
    return 0;
}

//Método que retira el cliente que se desconectó de todos los eventos subscritos
void removeEventsFromClient()
{
    printf("Removing events from client");
}