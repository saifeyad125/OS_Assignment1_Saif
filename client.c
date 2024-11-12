#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>    // for close
#include <sys/types.h> // for socket types
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> // for gethostbyname

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr,"Usage: %s <serverHost> <serverPort> <command>\n", argv[0]);
        exit(1);
    }
    char *serverHost = argv[1];
    int serverPort = atoi(argv[2]);

    // Build the command from argv[3] onwards
    char command[256];
    strcpy(command, argv[3]);
    for (int i = 4; i < argc; i++) {
        strcat(command, " ");
        strcat(command, argv[i]);
    }

    int sockfd;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("ERROR opening socket");
        exit(1);
    }

    // Get server by name
    server = gethostbyname(serverHost);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }

    // Build server address
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr_list[0],
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(serverPort);

    // Connect to server
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
        perror("ERROR connecting");
        exit(1);
    }

    // Send command
    int n = write(sockfd, command, strlen(command));
    if (n < 0) {
         perror("ERROR writing to socket");
         exit(1);
    }

    // Read response
    char buffer[1024];
    bzero(buffer,1024);
    n = read(sockfd,buffer,1023);
    if (n < 0) {
         perror("ERROR reading from socket");
         exit(1);
    }
    printf("%s\n",buffer);

    close(sockfd);
    return 0;
}
