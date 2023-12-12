#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>

#define DOMAIN "mta.test.org"
#define BUFFER_SIZE 409

struct {
    char *domain;
    int socket_fd;
} state;

struct {
    char *sender_domain;
    char *recipient;
    char *sender;
    char *data;
} session;


void init_socket();
void smtp_session(int client_socket);
void imap_session(int sockfd);
void sendImapCommand(int sockfd, const char* command);
void receiveIMAPResponse(int sockfd);

int main(int argc, char *argv[])
{
    state.domain = DOMAIN;

    int client_sock, client_size;
    init_socket();    

    while(1) {
       
        struct sockaddr_in client_addr;
        client_size = sizeof(client_addr);
        client_sock = accept(state.socket_fd, (struct sockaddr*)&client_addr, &client_size);
        
        if (client_sock < 0){
            fprintf(stderr, "Can't accept\n");
            return (EXIT_FAILURE);
        }
        fprintf(stdout, "Client connected at IP: %s and port: %i\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        
        smtp_session(client_sock);
    }

    return 0;
}

void init_socket()
{
    struct sockaddr_in server_addr;
    int rc, socket_desc;

    socket_desc = socket(AF_INET, SOCK_STREAM, 0);

    memset(&server_addr, 0, sizeof(struct sockaddr_in));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(2000);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    rc = bind(socket_desc, (struct sockaddr*)&server_addr, sizeof(server_addr));

    if (rc < 0){
        close(socket_desc);
        printf("Couldn't bind to the port\n");
        exit (EXIT_FAILURE);
    }

    rc = listen(socket_desc, 10);

    if (rc < 0) {
        fprintf(stderr, "Failed to listen to the socket!\n");
        exit (EXIT_FAILURE);
    }

    state.socket_fd = socket_desc;

    return;

}

void smtp_session(int client_socket)
{
    char send_buffer[BUFFER_SIZE];
    char recv_buffer[BUFFER_SIZE];
    char *token;
    int rc;
    int socketfd = client_socket;
    int data_message = 0;

    sprintf(send_buffer, "220 \r\n");
    rc = send(socketfd, send_buffer, strlen(send_buffer), 0);
    if (rc == -1) {
        fprintf(stderr, "ERROR at trying to send the GREETING\n");
        exit (EXIT_FAILURE);
    }


    while(1) {
        rc = recv(socketfd,  recv_buffer, BUFFER_SIZE, 0);
        
        if (rc == 0) {
            fprintf(stderr, "Host closed the socket!\n");
            break;
        }
        if (rc == -1) {
            fprintf(stderr, "Error on sockett!\n");
            exit (EXIT_FAILURE);
        }

    if (data_message == 0) {
        if (strcmp(recv_buffer, "HELO") == 0) {
            strcpy(send_buffer, "250 OK\r\n");
            send(socketfd, send_buffer, strlen(send_buffer), 0);
        }
        if (strcmp(recv_buffer, "MAIL") == 0) {
            strcpy(send_buffer, "250 OK\r\n");
            send(socketfd, send_buffer, strlen(send_buffer), 0);
        }
        if (strcmp(recv_buffer, "RCPT") == 0) {
            strcpy(send_buffer, "250 OK\r\n");
            send(socketfd, send_buffer, strlen(send_buffer), 0);
        }
        if (strcmp(recv_buffer, "DATA") == 0) {
            strcpy(send_buffer, "354\r\n");
            send(socketfd, send_buffer, strlen(send_buffer), 0);
            data_message = 1;
        }
        if (strcmp(recv_buffer, "QUIT") == 0) {
            strcpy(send_buffer, "221\r\n");
            send(socketfd, send_buffer, strlen(send_buffer), 0);
        }

        if (strcmp(recv_buffer, "QUIT") == 0) {
            strcpy(send_buffer, "221\r\n");
            send(socketfd, send_buffer, strlen(send_buffer), 0);
            break;
        }

        //Sesiunea IMAP
        if (strcmp(recv_buffer, "IMAP") == 0) {
            imap_session(socketfd);
        }
    } else if (data_message == 1) {
        if (strcmp(recv_buffer, ".") == 0) {
            strcpy(send_buffer, "250\r\n");
            send(socketfd, send_buffer, strlen(send_buffer), 0);
            data_message = 0;
        }
    }
    }
    
}

void sendImapCommand(int sockfd, const char* command){
    char buffer[BUFFER_SIZE];
    sprintf(buffer,"%s\r\n", command);
    send(sockfd, buffer, strlen(buffer), 0);
    printf("IMAP Client: %s", buffer);
}

 void receiveIMAPResponse(int sockfd) {
    char buffer[BUFFER_SIZE];
    recv(sockfd, buffer, BUFFER_SIZE, 0);
    printf("IMAP Server: %s", buffer);
}

void imap_session(int sockfd) {

    sendImapCommand(sockfd, "a001 LOGIN utilizator parola");
    receiveIMAPResponse(sockfd);


    sendImapCommand(sockfd, "a002 SELECT INBOX");
    receiveIMAPResponse(sockfd);


    sendImapCommand(sockfd, "a003 FETCH 1:* (FLAGS BODY[HEADER.FIELDS (SUBJECT FROM DATE)])");
    receiveIMAPResponse(sockfd);


    sendImapCommand(sockfd, "a004 LOGOUT");
    receiveIMAPResponse(sockfd);
}
