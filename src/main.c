#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>

#include <pthread.h>
#include <unistd.h>
#include <time.h>

#include "server.h"
#include "client.h"
#include "utils.h"

#define SA struct sockaddr
#define MAX_CLIENTS 64

// counting clients connected (kinda)
int client_c = 0;

void pr_usage() {
    printf("Usage: buglog -H/-C IP Port\n");
}

void pr_help() {
    printf("Connecting             -C IP PORT\n");
    printf("Hosting:               -H IP PORT (Need port forward)\n");
    printf("Search:                -s\n");
    printf("Or visit https://github.com/chouilles/buglog for more details\n");
}

int main(int argc, char** argv) {
    if (argc == 1) {
        pr_usage();
        return 1;
    }

    // parsing util (other than connect and host) arguments in command line
    if (strcmp(argv[1], "-h") == 0) {
       pr_help(); 
       return 0; 
    }
    else if (strcmp(argv[1], "-s") == 0) {
        int r = request_hosts();
        if (r == 1) {
            printf("Error: requesting hosts\n");
        }
        return 0;
    }

    srand(time(0));
    const int PORT = atoi(argv[3]); // parsing to int
    int sockfd;
    struct sockaddr_in servaddr; // declaring struct for handling servaddr 
    pthread_t thid[MAX_CLIENTS]; // declaring threads
    // pthread_t check_connection_th; // declaring threads for checking connections

    sockfd = socket(AF_INET, SOCK_STREAM, 0); // declaring socket
    if (sockfd == -1) {
        perror("Failed to create socket");
        return 1;
    }

    // configuring socket
    bzero(&servaddr, sizeof(servaddr)); 
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(argv[2]);
    servaddr.sin_port = htons(PORT);

    if (strcmp(argv[1], "-H") == 0) {
        // binding socket
        if (bind(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) {
            perror("Failed to bind socket");
            return 1;
        }

        // listening for a connection
        if (listen(sockfd, 5) != 0) {
            perror("Failed to listen");
            return 1;
        }

        // opening thread for checking connection
        // pthread_create(&check_connection_th, NULL, &check_connection, NULL);

        int client_n = 0;
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            avail[i] = 0;
        }

        while (1) {
            if (client_c == MAX_CLIENTS) {
                client_c = 0;
            }

            socklen_t len;
            
            for (int i = 0; i < MAX_CLIENTS; ++i) {
                if (avail[i] != 1) {
                    avail[i] = 1;
                    break;
                }
                ++client_c;
            }

            // accepting incoming connection
            clients[client_c].connfd = accept(sockfd, (SA*)&servaddr, &len);

            if (clients[client_c].connfd < 0) {
                perror("Could not accept.");
                continue;
            }
            
            // configuring client settings
            clients[client_c].client_n = client_c;
            clients[client_c].id = rand() % 32767; // setting random id for client
            if (client_c < MAX_CLIENTS) {
                // creating new thread for client and sending 'clients[client_c]'
                pthread_create(&thid[client_c], NULL, (void*)start_server, &clients[client_c]);
                ++client_c;
            } else {
                printf("Max clients reached\n");
                break; 
            }
        }

        // waiting for threads to finish executing
        for (int i = 0; i < client_c; ++i) {
            pthread_join(thid[i], NULL);
        }
        ++client_n;

    }
    else if (strcmp(argv[1], "-C") == 0) {
        // connecting to server
        if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) {
            perror("Connection failed");
            close(sockfd);
            return 1;
        }

        start_client(sockfd);
    }
    else {
        pr_usage();
        return 1;
    }

    close(sockfd);
    return 0;
}

