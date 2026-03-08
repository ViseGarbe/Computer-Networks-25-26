// ./webserver &
// jobs
// strace

#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>

int tmp; 

int main() {
    struct sockaddr_in addr;  // Structure for addressing internet services
    int i; // Loop counter
    int s; // Socket file descriptor
    int t; // Temporary variable for bytes count
    int total; // Total bytes read from the server
    char request[5000]; // Request buffer
    char response[1000000]; // Response buffer
    //unsigned char targetip[4] = {142, 250, 178, 4}; // Server IP address
    unsigned char targetip[4] = {127, 0, 0, 1}; // Localhost IP address for testing

    // Create a socket for the client
    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == -1) {
        perror("Socket creation failed");
        printf("Error code: %d\n", errno);
        return 1;
    }

    // Set address structure for the server
    addr.sin_family = AF_INET; // IPv4 internet address family
    //addr.sin_port = htons(80); // Port number (HTTP default)
    addr.sin_port = htons(9090); // Port number for testing with local server
    addr.sin_addr.s_addr = * (unsigned int *) targetip; // Server IP address

    // Connect to the server
    if (-1 == connect(s, (struct sockaddr *) &addr, sizeof(struct sockaddr_in))) {
        perror("Connection failed"); // Print error if connection fails
        printf("Error code: %d\n", errno);
        return 1;
    }
    printf("Connected to the server successfully.\n");

    // Prepare the HTTP GET request
    printf("Socket descriptor: %d\n", s);
    sprintf(request, "GET / \r\n");

    // Send the HTTP GET request to the server
    if (-1 == write(s, request, strlen(request))) {
        perror("Send failed"); // Print error if sending fails
        printf("Error code: %d\n", errno);
        return 1;
    }
    printf("Request sent to the server successfully.\n");

    // Read the response from the server
    //for (total = 0; (t = read(s, response + total, sizeof(response) - total)) > 0; total += t); 
    while ((t=read(s, response + total, sizeof(response) - total)) > 0) {
        total += t; // Accumulate the total bytes read
    }

    // Check for read errors
    if (t == -1) {
        perror("Read failed"); // Print error if reading fails
        printf("Error code: %d\n", errno);
        return 1;
    }

    // Print the server's response
    for (i = 0; i < total; i++) {
        printf("%c", response[i]);
    }
    
    return 0; 
}