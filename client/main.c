#include <stdio.h>
#include <stdlib.h>

#include "../gopher.h"

#define CHUNK_SIZE (1024)

u16 send_gopher_request(item_entry*, const gopher_config);
int create_connection(const char *, u16);
u16 select_request(int, const char*, item_entry*);
int process_buffer(char*, u16, item_entry*, u16*);
int add_entry(char*, u16, item_entry*, u16);


/* Helper functions */
int string_to_delm(const char*, char, u16);

void usage(void) {
    fprintf(stderr,"usage: gopher-client [-vf] [-p port] [-s selector] host\n");
    exit(1);
}

void parseopt(int argc, char **argv, gopher_config* config) {
    config->gopher_mode = GOPHER_MODE_DEFAULT;

    int opt;
    while ((opt = getopt(argc, argv, "vfp:s:")) != -1) {
        switch (opt)
        {
        case 'v':
            config->gopher_mode |= GOPHER_MODE_VERBOSE;
            break;
        case 'f':
            config->gopher_mode |= GOPHER_MODE_FILE;
            break;
        case 'p':
            config->gopher_mode |= GOPHER_MODE_PORT_SET;
            config->gopher_port = atoi(optarg);
            break;
        case 's':
            config->gopher_mode |= GOPHER_MODE_SELECT_SET;
            strcpy(config->gopher_selector, optarg);
            break;
        default:
            usage();
            break;
        }
    }

    argc -= optind;
    argv += optind;

    if (argc <= 0 || argc > 1) usage();

    /* Required */
    strcpy(config->gopher_host, argv[0]);

    /* Setting defaults if not set already */
    if (!(config->gopher_mode & GOPHER_MODE_PORT_SET)) config->gopher_port = 70;
    if (!(config->gopher_mode & GOPHER_MODE_SELECT_SET)) 
        memset(config->gopher_selector, '\0', ENTRY_STR_LEN);    
} 

/* Active Server to Test */
const char* test_host   = "gopher.floodgap.com";
const char* test_select = "" GOPHER_CRLF;
const u16   test_port   = GOPHER_PORT;

int main (int argc, char* argv[]) {

    gopher_config config;
    parseopt(argc, argv, &config);

    if (config.gopher_mode & GOPHER_MODE_FILE) {
        printf("FILE MODE\n");
    } else {
        u16 item_num = 0;
        item_entry entries[GOPHER_MAX_ENTRIES];
        if ((item_num = send_gopher_request(entries, config)) < 0) {
            printf("Client Failed (exit %d)\n", item_num);
            return 1;
        }

        /* Write all entries to display */
        draw_items_render(entries, item_num);
    }

    return 0;
}

u16 send_gopher_request(item_entry* entries, const gopher_config config) {
    int socket_fd;
    u16 item_numbers = 0;

    /* Create Connection */
    if ((socket_fd = create_connection(config.gopher_host, config.gopher_port)) < 0) {
        printf("Client couldn't make a connection\n");
        return -1;
    }

    /* Send Select Request */
    if ((item_numbers = select_request(socket_fd, config.gopher_selector, entries)) < 0) {
        close(socket_fd);
        return -1;
    }

    /* Close Connection and Clean Buffer */
    close(socket_fd);
    return item_numbers;
}

int create_connection(const char *hostname, u16 port) {
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

u16 select_request(int socket_fd, const char* select, item_entry* entries) {
    int buffer_size = CHUNK_SIZE;
    u16 entry_len = 0;

    char end_str[2] = GOPHER_CRLF;

    /* Send Magic String (Selector) */
    send(socket_fd, select, strlen(select), 0);
    send(socket_fd, end_str, 2, 0);

    /* Read response */
    char *buffer = (char*) malloc(buffer_size);
    
    if (sock_recv_all(socket_fd, buffer, &buffer_size)) {
        printf("Client failed to receive all data\n");
        return -1;
    }
    
    /* Process item entries from buffer */
    memset(entries, 0, GOPHER_MAX_ENTRIES * sizeof(item_entry));
    process_buffer(buffer, buffer_size, entries, &entry_len);

    free(buffer);
    return entry_len;
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
