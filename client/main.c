#include <stdio.h>
#include <stdlib.h>

#include "gopher.h"

#define CHUNK_SIZE (2048)

int base_test();
int process_buffer(char*, u16, item_entry*, u16*);
int add_entry(char*, u16, item_entry*, u16);

/* Helper functions */
int string_to_delm(const char*, char, u16);

int main (int argc, char* argv[]) {
    int err = 0;

    if ((err = base_test())) {
        printf("Client Failed (exit %d)\n", err);
        return 1;
    }

    return 0;
}

int create_connection(const char *hostname, unsigned short port) {
    int fd;
    struct sockaddr_in addr;
    struct hostent *server_addr;

    // Open Socket
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Socket wasn't able to be opened\n");
        return -1;
    }

    // Resolve host and set server address
    server_addr = gethostbyname(hostname);
    addr.sin_family = AF_INET;
    addr.sin_addr = *((struct in_addr *)server_addr->h_addr);
    addr.sin_port = htons(port);

    // Make connection
    if (connect(fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        printf("Connection error\n");
        return -1;
    }

    return fd;
}

int sock_recv_all(int socket, char *buf, u32 *buf_size) {
    /* Clear Buffer */
    memset(buf, '\0', (*buf_size));

    char read_buf[CHUNK_SIZE];
    u32 buf_end = 0, read_size = 0;
    while(read_size = read(socket, read_buf, CHUNK_SIZE)) {
        /* Set the new buffer content length */
        buf_end += read_size;

        /* Increase buffer */
        if ((*buf_size) < buf_end) {
            /* Check if server is sending too much data */
            if (buf_end >= GOPHER_MAX_LINE * GOPHER_MAX_ENTRIES) {
                printf("There are too many entries\n");
                return 1;
            }

            /* Double the size, clean up later */
            (*buf_size) *= 2;
            if (!(buf = (char*) realloc(buf, (*buf_size)))) {
                printf("Failed to reallocate buffer\n");
                return 1;
            }
        }

        /* Append response chunk to buffer */
        strcpy(buf + (buf_end - read_size), read_buf);
    }

    /* If the buffer is bigger than required, shrink it to the used size */
    if (buf_end < (*buf_size)) {
        (*buf_size) = buf_end;
        if (!(buf = (char*) realloc(buf, (*buf_size)))) {
            printf("Failed to shrink buffer\n");
            return 1;
        }
    }

    return 0;
}

/* Active Server to Test */
const char* test_host   = "gopher.floodgap.com";
const char* test_select = "/gopher" GOPHER_CRLF;
const u16   test_port   = GOPHER_PORT;

int base_test() {
    char *buffer;
    int socket_fd, buffer_size = CHUNK_SIZE;
    
    /* Create Connection */
    if ((socket_fd = create_connection(test_host, test_port)) < 0) {
        printf("Client couldn't make a connection\n");
        return 1;
    }

    /* Send Magic String (Selector) */
    send(socket_fd, test_select, strlen(test_select), 0);

    /* Read response */
    buffer = (char*) malloc(buffer_size);
    
    if (sock_recv_all(socket_fd, buffer, &buffer_size)) {
        printf("Client failed to receive all data\n");
        close(socket_fd);
        return 2;
    }
    
    /* Process item entries from buffer */
    {
        item_entry entries[GOPHER_MAX_ENTRIES];
        u16 entry_len;
        memset(entries, 0, GOPHER_MAX_ENTRIES * sizeof(item_entry));
        process_buffer(buffer, buffer_size, entries, &entry_len);

        /* Write all entries to display */
        draw_items(entries, entry_len);
    }

    /* Close Connection and Clean Buffer */
    close(socket_fd);
    free(buffer);

    return 0;
}

int process_buffer(char* buf, u16 buf_size, item_entry* entries, u16 *entry_len) {
    char sub_buf[GOPHER_MAX_LINE];
    int x = 0, line_start = 0;
    (*entry_len) = 0;

    /* Go through buffer finding entries */
    while (x < buf_size && (*entry_len) < GOPHER_MAX_ENTRIES) {

        /* Execute add entry for every row */
        if (buf[x] == '\r' && buf[x + 1] == '\n') {
            
            /* Check if end request */
            if (buf[line_start] == GOPHER_END) break;

            /* Add entry (if valid) */
            (*entry_len) += add_entry(buf + line_start, x - line_start, 
                                      entries, (*entry_len));
            line_start = x + 2;
        }
        x++;
    }
}

int string_to_delm(const char *buf, char delim, u16 max) {
    u16 len = 0;
    
    /* Get number of chars until reached the delimiter */
    while (buf[len] != delim && len < max) len++;
    return len;
}

int add_entry(char* buf, u16 buf_size, item_entry* entries, u16 index) {
    char port[6];
    u16 offset = 1, end = 0;

    /* Check valid */
    if (get_type(buf[0]) == G_ERR) return 0;

    /* Clear Entry */
    memset(&entries[index], 0, sizeof(item_entry));

    /* Set easy variables */
    entries[index].item_type = get_type(buf[0]);
    memcpy(entries[index].item_buf, buf, buf_size);

    /* Fetching main strings (Display, Selector and Hostname) */
    for (u8 i = 0; i < ENTRY_STR_SIZE; ++i) {
        if ((end = string_to_delm(buf + offset, '\t', ENTRY_STR_LEN)))
            memcpy(entries[index].item_strings[i], buf + offset, end++);
        offset += end;
    }

    /* Fetch Port buffer*/
    if ((end = string_to_delm(buf + offset, '\t', 6)))
        memcpy(port, buf + offset, end);

    /* Setting the port as an integer */
    entries[index].item_port = (port) ? atoi(port) : GOPHER_PORT;

    return 1;
}
