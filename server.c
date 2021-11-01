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