#include <stdio.h>
#include <stdlib.h>

#include "gopher.h"

#define CHUNK_SIZE (2048)

int base_test();
int process_buffer(char* buf, u32 blen, item_entry* entries, u32 *elen, u32 max_len);
int add_entry(char* buf, u32 blen, item_entry* entries, u32 index, u32 max_len);

/* Helper functions */
void draw_items(item_entry *items, u32 length);
void draw_hex(char *buffer, int size);

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
    u32 x, y;
    for (x = y = 0; x < size; ++x) {
        printf(" 0x%02X", buffer[x]);
        if ((++y) >= limit) {
            printf("\n");
            y = 0;
        }
    }
    printf("\n\n");
}

/* Helper function to write item entries to terminal */
void draw_items(item_entry *items, u32 length) {
    u32 x;
    for (x = 0; x < length; ++x) {
        printf("{\n\t\"id\": %d\n\t\"type\": %d,\n\t\"display\": \"%s\",\n\t\"selector\": \"%s\",\n\t\"hostname\": \"%s\",\n\t\"port\": %hu\n}\n",
            x, 
            items[x].item_type, 
            items[x].item_display, 
            items[x].item_selector, 
            items[x].item_hostname, 
            items[x].item_port);
    }
}

int base_test() {
    const char* test_host = "gopher.floodgap.com"; /* Test Host (active) */
    unsigned short test_port = GOPHER_PORT;
    int socket_fd, read_size;
    char read_buffer[CHUNK_SIZE] = {0};

    /* Create Connection */
    if ((socket_fd = create_connection(test_host, test_port)) < 0) {
        printf("\tCLIENT couldn't make a connection\n");
        return 1;
    }

    /* Send Magic String (Selector) */
    send(socket_fd, "/gopher" GOPHER_CRLF, sizeof(char) * (7 + 2), 0);

    /* Read response */
    char *buf = (char*) malloc(CHUNK_SIZE);
    memset(buf, '\0', CHUNK_SIZE);

    int buf_size = CHUNK_SIZE, cur_end = 0;
    while(read_size = read(socket_fd, read_buffer, CHUNK_SIZE)) {
        /* Double check to see if the buffer exists */
        if (!buf) {
            printf("Buffer is NULL?\n");
            close(socket_fd);
            return 1;
        }

        /* Set the new buffer content length */
        cur_end += read_size;

        /* Check to see if the buffer is too small */
        if (cur_end > buf_size) {
            buf_size *= 2;
            if (!(buf = (char*) realloc(buf, buf_size))) {
                printf("Failed to reallocate buffer\n");
                close(socket_fd);
                return 1;
            }
        }

        /* Append response chunk to buffer */
        strcpy(buf + (cur_end - read_size), read_buffer);
    }
    
    /* Process item entries from buffer */
    item_entry entries[CHUNK_SIZE];
    u32 elen;
    memset(entries, 0, CHUNK_SIZE * sizeof(item_entry));
    process_buffer(buf, cur_end, entries, &elen, CHUNK_SIZE);

    draw_items(entries, elen);

    /* Close Connection */
    close(socket_fd);
}

int process_buffer(char* buf, u32 blen, item_entry* entries, u32 *elen, u32 max_len) {
    char sub_buf[GOPHER_MAX_LINE];
    int x = 0, line_start = 0;
    (*elen) = 0;

    /* Go through buffer finding entries */
    while (x < blen && (*elen) < max_len) {

        /* Execute add entry for every row */
        if (buf[x] == '\r' && buf[x + 1] == '\n') {
            
            /* Check if end request */
            if (buf[line_start] == GOPHER_END) break;

            /* Add entry (if valid) */
            (*elen) += add_entry(buf + line_start, x - line_start, 
                                 entries, (*elen), max_len);
            line_start = x + 2;
        }
        x++;
    }
}

int add_entry(char* buf, u32 blen, item_entry* entries, u32 index, u32 max_len) {
    char *display, *selector, *hostname, *port;

    /* Check valid */
    if (get_type(buf[0]) == G_ERR) return 0;

    /* Set easy variables */
    entries[index].item_type = get_type(buf[0]);
    memcpy(entries[index].item_buf, buf, blen);

    /* Pluck out segmented data from buffer */
    /* TODO: Better than this method */
    display     = strtok(buf+1, "\t");
    selector    = strtok(NULL,  "\t");
    hostname    = strtok(NULL,  "\t");
    port        = strtok(NULL,  "\r\n");

    /* Set challenging variables */
    strcpy(entries[index].item_display, display);
    strcpy(entries[index].item_selector, selector);
    strcpy(entries[index].item_hostname, hostname);
    entries[index].item_port = (port) ? atoi(port) : GOPHER_PORT;

    return 1;
}
