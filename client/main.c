#include <stdio.h>
#include <stdlib.h>

#include "gopher.h"

int base_test();

int main (int argc, char* argv[]) {
    int err = 0;

    printf("RUNNING base_test CLIENT\n");
    if ((err = base_test())) {
        printf("CLIENT FAILED (exit %d)\n", err);
        return 1;
    }
    printf("CLIENT SUCCEEDED\n");

    return 0;
}

int create_connection(const char *hostname, unsigned short port) {
    int fd;
    struct sockaddr_in addr;
    struct hostent *server_addr;

    // Open Socket
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\tCANT MAKE SOCKET\n");
        return -1;
    }

    // Resolve host and set server address
    server_addr = gethostbyname(hostname);
    addr.sin_family = AF_INET;
    addr.sin_addr = *((struct in_addr *)server_addr->h_addr);
    addr.sin_port = htons(port);

    // Make connection
    if (connect(fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        printf("\tCONNECTION ERROR\n");
        return -1;
    }

    return fd;
}

/* Helper function to get raw hexdump of buffer */
void draw_hex(char *buffer, int size) {
    const unsigned char limit = 8;
    int x, y;

    for (x = y = 0; x < size; ++x) {
        printf(" 0x%02X", buffer[x]);
        if ((++y) >= limit) {
            printf("\n");
            y = 0;
        }
    }
    printf("\n\n");
}

int base_test() {
    const char* test_host = "gopher.floodgap.com"; /* Test Host (active) */
    unsigned short test_port = GOPHER_PORT;
    int socket_fd, read_size;
    char read_buffer[1024] = {0};

    // Create Connection
    if ((socket_fd = create_connection(test_host, test_port)) < 0) {
        printf("\tCLIENT couldn't make a connection\n");
        return 1;
    }

    // Send Magic String (Selector)
    send(socket_fd, "/gopher" CRLF, sizeof(char) * (7 + 2), 0);

    // Read response
    char *buf = (char*) malloc(1024 * sizeof(char));
    int buf_size = 0;
    while(read_size = read(socket_fd, read_buffer, 1024)) {
        buf_size += read_size;
        if (buf) {
            buf = (char*) realloc(buf, buf_size * sizeof(char));
        }
        strcpy(buf + (buf_size - read_size), read_buffer);
    }
    
    // The following are for debugging purposes
    printf("%s\n", buf); // Print full response to screen
    draw_hex(buf, buf_size); // Print full raw hexdump of response
    
    // Close Connection
    close(socket_fd);
}