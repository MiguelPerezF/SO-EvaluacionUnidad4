#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <string.h>

#define PORT 6666
#define BUF_SIZE 1000

struct client_t
{
    int socket;
    int rxState;
};
struct event_t
{
    char *event_name;
    int subs[20];
};

void *readThread(void *arg);
void *serverCommandsThread(void *arg);

//static struct client_t *clients[10];

int main(int argc, char *argv[])
{
    //char buf[BUF_SIZE];
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

    while ((client_sd = accept(server_sd, (struct sockaddr *)&addr, (socklen_t *)&size)))
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
        printf("from client %d: %s\n", client.socket, message_from_client);

        char *command = strtok(message_from_client, " ");

        if (strcmp(command, "sub") == 0)
        {
            char *event_name = strtok(NULL, " ");
            printf("Subscribing client %d to the event: %s.\n", client.socket, event_name);

        }
        if (strcmp(command, "unsub") == 0)
        {
            char *event_name = strtok(NULL, " ");
            printf("Unsubscribing client %d from the event: %s.\n", client.socket, event_name);
        }
        if (strcmp(command, "list") == 0)
        {
            printf("Listing all events client is subscribed to.\n");
        }
        if (strcmp(command, "ask") == 0)
        {
            printf("Listing all available events.\n");
        }
        if (strcmp(command, "save") == 0)
        {
            char *file_name = strtok(NULL, " ");
            printf("Saving file: %s\n", file_name);
        }
        if (strcmp(command, "load") == 0)
        {
            char *file_name = strtok(NULL, " ");
            printf("Loading file: %s\n", file_name);
        }
    }

    if (0 == numOfBytes)
    {
        printf("client closed the socket end\n");
        //removeEventsFromClient();
    }
    else if (-1 == numOfBytes)
    {
        perror("ReadThread read() fails: ");
        //removeEventsFromClient();
    }

    printf("Terminate Pthread for reading\n");
    client.rxState = 0;
    close(client.socket);
    return 0;
}

// Método que recibe los comandos del servidor
void *serverCommandsThread(void *arg)
{
    struct event_t *arrevents = malloc(sizeof(struct event_t) * (20));
    int server_sd = atoi(arg);
    char input[BUF_SIZE];

    while (1)
    {

        if (fgets(input, BUF_SIZE, stdin) == NULL)
        {
            printf("Fgets NULL\n");
        }

        if (input[strlen(input) - 1] == '\n')
            input[strlen(input) - 1] = 0;

        char *command = strtok(input, " ");

        //comando exit
        if ((strcmp(command, "exit")) == 0)
        {
            printf("Terminating server.\n");
            exit(1);
        }

        //comando add
        if (strcmp(command, "add") == 0)
        {
            char *event_name = strdup(strtok(NULL, " "));
            struct event_t e =
                {
                    event_name,
                    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};
            //printf("Adding sub: %d.\n", e.subs[10]);
            for (int i = 0; i < 20; i++)
            {
                if (arrevents[i].event_name == NULL)
                {
                    arrevents[i] = e;
                    printf("Events[%d]: %s.\n", i, arrevents[i].event_name);
                    break;
                }
            }
        }

        //comando remove
        if (strcmp(command, "remove") == 0)
        {
            int rmvexist = 0, pos = 0;
            char *event_name = strtok(NULL, " ");
            printf("Removing event: %s.\n", event_name);
            for (int i = 0; i < 20; i++)
            {
                if (arrevents[i].event_name != NULL)
                {
                    if (strcmp(arrevents[i].event_name, event_name) == 0)
                    {
                        rmvexist = 1; //Flag indicating the event exist
                        pos = i;
                        break;
                    }
                }
            }
            if (rmvexist == 1) //If the event was found
            {
                int controlprint = 0; //Controls the times tah confirmation prints
                for (int i = pos; i < 20; i++)
                {
                    if (arrevents[i + 1].event_name != NULL)
                    {
                        controlprint++;
                        strcpy(arrevents[i].event_name, arrevents[i + 1].event_name);
                        if (controlprint == 1)
                        {
                            printf("Evento %s eliminado con exito\n", event_name);
                        }
                    }
                    else //The last event added
                    {
                        arrevents[i].event_name = NULL;
                        arrevents[i].subs[i] = 0;
                        if (controlprint == 0)
                        {
                            printf("Evento %s eliminado con exito\n", event_name);
                        }
                        break;
                    }
                }
            }
            if (rmvexist == 0) //If the event was not found
            {
                printf("El evento %s no existe.\n", event_name);
            }
        }

        //comando trigger
        if (strcmp(command, "trigger") == 0)
        {
            char *event_name = strtok(NULL, " ");
            printf("Triggering event: %s.\n", event_name);
        }

        //comando list: lista los suscriptores del evento enviado por consola
        if (strcmp(command, "list") == 0)
        {
            char *event_name = strtok(NULL, " ");
            for (int i = 0; i < 20; i++)
            {
                if (arrevents[i].event_name != NULL)
                {
                    if (strcmp(arrevents[i].event_name, event_name) == 0)
                    {
                        printf("Listando clientes en el event %s:\n", arrevents[i].event_name);
                        if (arrevents[i].subs[0] != 0)
                        {
                            for (int j = 0; j < 20; j++) //iterates number of subs
                            {
                                printf("Sub[%d]: %d", j,arrevents[i].subs[j]); //
                                printf("\n");
                                /*if (arrevents[i].subs[j] != 0)
                                {
                                    printf("Cliente %d subscrito.\n", arrevents[i].subs[j]);
                                }*/
                            }
                        }
                        else
                        {
                            printf("El evento %s no tiene subs aún\n", event_name);
                            break;
                        }
                    }
                }
            }
        }

        if (strcmp(command, "show") == 0)
        {
            for (int i = 0; i < 20; i++)
            {
                printf("Event %d: %s\n", i, arrevents[i].event_name);
            }
        }
        if (strcmp(command, "all") == 0)
        {
            printf("Todos los eventos y clientes.\n");
            for (int i = 0; i < 20; i++)
            {
                if (arrevents[i].event_name != NULL)
                {
                    printf("Evento %s\n", arrevents[i].event_name);
                    if (arrevents[i].subs[0] != 0)
                    {
                        printf("justo antes de iterar en subs");
                        for (int j = 1; j <= 20; i++)
                        {
                            printf("Sub %d: cliente con ID %d", j, arrevents[i].subs[j]);
                        }
                    }
                    else
                    {
                        printf("Este evento no tiene suscriptores \n");
                    }
                }
                else
                {
                    printf("No hay más eventos\n");
                    break;
                }
            }
        }
        if (strcmp(command, "save") == 0)
        {
            char *file_name = strtok(NULL, " ");
            FILE *fout = fopen(file_name, "w+");
            if (fout == NULL)
            {
                perror("Falla en la apertura del archivo de salida: ");
                EXIT_FAILURE;
            }
            printf("Archivo abierto, guardando...\n");
            for (int i = 0; i < 20; i++)
            {
                if (arrevents[i].event_name != NULL)
                {
                    printf("Escribiré %s\n", arrevents[i].event_name);
                    fputs(arrevents[i].event_name, fout);
                    fputs("\n", fout);
                }
                else
                {
                    break;
                }
            }
            //printf("Eventos guardados en el archivo con éxito\n");
            fclose(fout);
        }
        if (strcmp(command, "load") == 0)
        {
            char *file_name = strtok(NULL, " ");
            FILE *fin = fopen(file_name, "r");
            if (fin == NULL)
            {
                perror("Falla en la apertura del archivo de entrada: ");
                EXIT_FAILURE;
            }
            char buff[64];
            char *status = NULL;
            int cont = 0;
            status = fgets(buff, sizeof(buff), fin);

            while (status != NULL && cont<20)
            {
                printf("Evento %d leido: %s", cont, buff);
                char *event = strdup(buff);
                struct event_t e =
                {
                    event,
                    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
                };
                status = fgets(buff, sizeof(buff), fin);
                arrevents[cont] = e;
                cont++;
            }
            printf("Archivo %s leido\n", file_name);
        }
    }
    close(server_sd);
    return 0;
}


