#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024
#define PORT 21

void send_command(int sock, const char *command) {
    send(sock, command, strlen(command), 0);
    char buffer[BUFFER_SIZE];
    recv(sock, buffer, sizeof(buffer), 0);
    printf("Server: %s", buffer);
}

int main() {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    char command[BUFFER_SIZE];
    FILE *file;
    ssize_t bytes_read;

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // FTP server IP
    server_addr.sin_port = htons(PORT);

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    // Read the server greeting
    recv(sock, buffer, sizeof(buffer), 0);
    printf("Server: %s", buffer);

    while (1) {
        printf("Enter command (STOR <filename>, LIST, RETR <filename>, QUIT): ");
        fgets(command, sizeof(command), stdin);
        command[strcspn(command, "\n")] = '\0'; // Remove newline character

        if (strncmp(command, "QUIT", 4) == 0) {
            send_command(sock, "QUIT\r\n");
            break;
        } else if (strncmp(command, "STOR", 4) == 0) {
            char filename[256];
            sscanf(command + 5, "%s", filename);
            file = fopen(filename, "rb");
            if (!file) {
                printf("File not found.\n");
            } else {
                send_command(sock, command);
                while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
                    send(sock, buffer, bytes_read, 0);
                }
                fclose(file);
            }
        } else if (strncmp(command, "LIST", 4) == 0) {
            send_command(sock, "LIST\r\n");
        } else if (strncmp(command, "RETR", 4) == 0) {
            send_command(sock, command);
            char filename[256];
            sscanf(command + 5, "%s", filename);
            file = fopen(filename, "wb");
            if (!file) {
                printf("Could not create file.\n");
            } else {
                while ((bytes_read = recv(sock, buffer, sizeof(buffer), 0)) > 0) {
                    fwrite(buffer, 1, bytes_read, file);
                }
                fclose(file);
            }
        } else {
            printf("Invalid command.\n");
        }
    }

    close(sock);
    return 0;
}
