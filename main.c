#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h> // For perror

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int server_fd, new_socket;
    long valread;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // --- Step 1: Create socket file descriptor ---
    // AF_INET: IPv4 Internet protocols
    // SOCK_STREAM: Sequenced, reliable, two-way, connection-based byte streams (TCP)
    // 0: Default protocol (TCP for SOCK_STREAM)
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // --- Optional: Set socket options (e.g., reuse address) ---
    // This helps in reuse of address and port immediately after server restart
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
         perror("setsockopt SO_REUSEADDR failed");
         // Not exiting here, as it might still work without it
    }
     if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt))) {
         perror("setsockopt SO_REUSEPORT failed");
         // Not exiting here, as it might still work without it
    }


    // --- Step 2: Bind the socket to the network address and port ---
    address.sin_family = AF_INET;         // IPv4
    address.sin_addr.s_addr = INADDR_ANY; // Listen on all available interfaces
    address.sin_port = htons(PORT);       // Convert port to network byte order

    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // --- Step 3: Start listening for incoming connections ---
    // 3 is the maximum length to which the queue of pending connections may grow.
    if (listen(server_fd, 3) < 0) {
        perror("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    // --- Step 4: Accept incoming connections in a loop ---
    while(1) {
        printf("\n+++++++ Waiting for new connection ++++++++\n\n");

        // Accept a new connection
        // It extracts the first connection request on the queue of pending connections
        // for the listening socket, creates a new connected socket, and returns a
        // new file descriptor referring to that socket.
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept failed");
            // Continue to the next iteration instead of exiting, to handle other connections
            continue;
        }

        printf("Connection accepted.\n");

        // --- Step 5: Read the HTTP request from the client ---
        char buffer[BUFFER_SIZE] = {0};
        valread = read(new_socket, buffer, BUFFER_SIZE - 1); // Read data into buffer
        if (valread < 0) {
            perror("read failed");
        } else if (valread == 0) {
             printf("Client closed connection.\n");
        } else {
             // Null-terminate the received data
             buffer[valread] = '\0';
             printf("Received Request:\n%s\n", buffer);

             // --- Step 6: Prepare and send the HTTP response ---
             // Basic HTTP 1.1 response
             char *hello =
                 "HTTP/1.1 200 OK\r\n"           // Status line
                 "Content-Type: text/html\r\n"   // Header: Content type
                 "Content-Length: 76\r\n"        // Header: Length of the body
                 "\r\n"                          // Blank line separating headers and body
                 "<html><body>"                  // Body starts
                 "<h1>Hello, World!</h1>"
                 "<p>This is a simple C HTTP server.</p>"
                 "</body></html>";

             // Send the response
             ssize_t bytes_sent = write(new_socket, hello, strlen(hello));
             if (bytes_sent < 0) {
                 perror("write failed");
             } else {
                  printf("------------------Response sent-------------------\n");
                  printf("Sent %zd bytes.\n", bytes_sent);
             }
        }


        // --- Step 7: Close the client socket ---
        // The listening socket (server_fd) remains open to accept new connections
        close(new_socket);
        printf("Client socket closed.\n");
    }

    // --- Step 8: Close the listening server socket (optional, as the loop runs forever) ---
    // This part is usually reached only if the loop condition changes or on signal handling
    printf("Shutting down server.\n");
    close(server_fd);

    return 0;
}
