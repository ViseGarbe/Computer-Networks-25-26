#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>

int main() {
    struct sockaddr_in addr, remote_addr; // Server and client address structures
    int s1; // Server Socket file descriptor
    int s2; // Client Socket file descriptor
    int len; // Length of the client address 
    int t; // Temporary variable for return values
    int i; // Loop counter
    int yes = 1; // Flag for setsockopt
    char *method, *path, *version; // Pointers for HTTP request parsing
    char c; // Variable for reading file content
    char request[5000]; // Buffer for incoming HTTP request
    char response[1000000]; // Buffer for HTTP response

    FILE *fin; // File pointer for the requested file

    // Create a TCP socket for the server
    s1 = socket(AF_INET, SOCK_STREAM, 0);
    if (s1 == -1) {
        perror("Socket creation failed");
        printf("Error code: %d\n", errno);
        return 1;
    }
    
    // Initialize the server address structure
    addr.sin_family = AF_INET; // IPv4
    addr.sin_port = htons(80); // Port number (HTTP default)
    addr.sin_addr.s_addr = INADDR_ANY; // Listen on all interfaces

    // Allow socket to reuse the address, helpful for quick restarts
    t = setsockopt(s1, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    if (t == -1) {
        perror("Setsockopt failed");
        printf("Error code: %d\n", errno);
        return 1;
    }

    // Blind the socket to the specified address and port
    if (-1 == bind(s1, (struct sockaddr *) &addr, sizeof(struct sockaddr_in))) {
        perror("Bind failed");
        printf("Error code: %d\n", errno);
        return 1;
    }

    // Listen for incoming connections
    if (-1 == listen(s1, 5)) { // 5 is the maximum number of pending connections that can be queued up before the server starts rejecting new connections.
        perror("Listen failed");
        printf("Error code: %d\n", errno);
        return 1;
    }

    // Server loop to accept and handle incoming connections
    while (1) {
        //Accept a new client connection
        len = sizeof(remote_addr);
        s2 = accept(s1, (struct sockaddr *) &remote_addr, &len);
        if (s2 == -1) {
            perror("Accept failed");
            printf("Error code: %d\n", errno);
            continue; // Continue to accept the next connection
        }

        // Read the HTTP request from the client
        t = read(s2, request, sizeof(request) - 1);
        if (t == -1) {
            perror("Read failed");
            printf("Error code: %d\n", errno);
            close(s2); // Close the client socket on error
            continue; // Continue to accept the next connection
        }

        // Properly terminate the request string
        request[t] = '\0';
        printf("Received request:\n%s\n", request);

        // Parse the HTTP request to extract method, path, and version
        method = request; 
        for(i = 0; request[i] != ' '; i++) {} 
        request[i] = '\0'; 

        path = request + i + 1;
        for(i = 0; path[i] != ' '; i++) {} 
        path[i] = '\0'; 

        version = path + i + 1;
        for(i = 0; version[i] != '\r'; i++) {} 
        version[i] = '\0'; 

        printf("Parsed request - Method: %s, Path: %s, Version: %s\n", method, path, version);

        // Try to open the requested file
        fin = fopen(path + 1, "rt"); // Skip the leading '/' in the path
        if (fin == NULL) {
            // If the file is not found, send a 404 Not Found response
            sprintf(response, "HTTP/1.1 404 Not Found\r\n\r\n");
            write(s2, response, strlen(response)); // Send the 404 response to the client
        } else {
            // If the file is found, read its contents and prepare a 200 OK response
            sprintf(response, "HTTP/1.1 200 OK\r\n\r\n");
            write(s2, response, strlen(response)); // Send the 200 response to the client
            while ((c = fgetc(fin)) != EOF) { // Read the file character by character
                write(s2, &c, 1); // Send each character to the client
            }
            fclose(fin); // Close the file after reading
        }
        close(s2); // Close the client socket after handling the request
    }

    return 0; 
}