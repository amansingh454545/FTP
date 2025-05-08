#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <dirent.h>

#define PORT 21
#define BUFFER_SIZE 1024

void *handle_client(void *client_sock);

int main() {
    int server_fd, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    pthread_t thread_id;

    // Create server socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind the socket
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("FTP Server listening on port %d\n", PORT);

    while (1) {
        // Accept a client connection
        if ((client_sock = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len)) < 0) {
            perror("Accept failed");
            continue;
        }

        // Create a new thread to handle the client
        if (pthread_create(&thread_id, NULL, handle_client, (void *)&client_sock) != 0) {
            perror("Thread creation failed");
            continue;
        }
        pthread_detach(thread_id);
    }

    close(server_fd);
    return 0;
}

#define FTP_DIRECTORY "/path/to/ftp_files" // Define a directory for storing and retrieving files

void *handle_client(void *client_sock) {
    int sock = *(int *)client_sock;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    // Send a greeting message
    send(sock, "220 Welcome to FTP server\r\n", 26, 0);

    // Process client commands
    while ((bytes_read = recv(sock, buffer, sizeof(buffer), 0)) > 0) {
        buffer[bytes_read] = '\0'; // Null-terminate the buffer
        printf("Received: %s", buffer);

        if (strncmp(buffer, "STOR", 4) == 0) {
            char filename[256];
            sscanf(buffer + 5, "%s", filename);

            // Construct full path for storing the file in the FTP directory
            char full_path[512];
            snprintf(full_path, sizeof(full_path), "%s/%s", FTP_DIRECTORY, filename);

            FILE *file = fopen(full_path, "wb");
            if (!file) {
                send(sock, "550 File not created.\r\n", 22, 0);
            } else {
                send(sock, "150 File status okay; about to open data connection.\r\n", 54, 0);
                recv(sock, buffer, sizeof(buffer), 0); // Wait for file content
                fwrite(buffer, 1, bytes_read, file);
                fclose(file);
                send(sock, "226 File transfer successful.\r\n", 31, 0);
            }
        } else if (strncmp(buffer, "RETR", 4) == 0) {
            char filename[256];
            sscanf(buffer + 5, "%s", filename);

            // Construct full path to retrieve the file from the FTP directory
            char full_path[512];
            snprintf(full_path, sizeof(full_path), "%s/%s", FTP_DIRECTORY, filename);

            FILE *file = fopen(full_path, "rb");
            if (!file) {
                send(sock, "550 File not found.\r\n", 20, 0);
            } else {
                send(sock, "150 Opening data connection.\r\n", 30, 0);
                while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
                    send(sock, buffer, bytes_read, 0);
                }
                fclose(file);
                send(sock, "226 Transfer complete.\r\n", 24, 0);
            }
        } else {
            send(sock, "500 Command not recognized.\r\n", 28, 0);
        }
    }

    close(sock);
    return NULL;
}
