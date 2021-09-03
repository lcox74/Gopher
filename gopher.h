#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#define GOPHER_PORT (70)
#define GOPHER_MAX_LINE (2048)
#define GOPHER_MAX_ENTRIES (256)
#define GOPHER_CRLF "\r\n"         /* 0x0D 0x0A */
#define GOPHER_TAB "\t"            /*      0x09 */
#define GOPHER_END '.'             /*      0x2E */

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

typedef enum {
    G_FILE,           /* 0 - Item is a file */
    G_DIR,            /* 1 - Item is a directory */
    G_CCSO,           /* 2 - Item is a CSO phone-book server */
    G_ERR,            /* 3 - Error */
    G_BINHEX,         /* 4 - Item is a BinHexed Macintosh file */
    G_DOS,            /* 5 - Item is a DOS binary archive (*) */
    G_UUENCODED,      /* 6 - Item is a UNIX uuencoded file */
    G_SEARCH,         /* 7 - Item is an Index-Search server */
    G_TELNET,         /* 8 - Item points to a text-based telnet session */
    G_BIN,            /* 9 - Item is a binary file (*) */
    G_REDUNDANT,      /* + - Item is a redundant server */
    G_TELNET3270,     /* T - Item points to a text-based tn3270 session */
    G_GIF,            /* g - Item is a GIP format graphics file */
    G_IMG,            /* I - Item is some kind of image file */
    G_END
    /* 
     * Anything labelled with a (*) means:
     *      Client must read until the TCP connection closes
     */
} Itype;

static Itype get_type(char t) {
    switch(t) {
        case '0': return G_FILE;
        case '1': return G_DIR;
        case '2': return G_CCSO;
        case '4': return G_BINHEX;
        case '5': return G_DOS;
        case '6': return G_UUENCODED;
        case '7': return G_SEARCH;
        case '8': return G_TELNET;
        case '9': return G_BIN;
        case '+': return G_REDUNDANT;
        case 'T': return G_TELNET3270;
        case 'g': return G_GIF;
        case 'I': return G_IMG;
        case '.': return G_END;
    }
    return G_ERR;
}

#define ENTRY_STR_LEN  (256)
#define ENTRY_DISPLAY  (0)
#define ENTRY_SELECTOR (1)
#define ENTRY_HOSTNAME (2)
#define ENTRY_STR_SIZE (3)
typedef struct _item_entry {
    Itype   item_type;
    char    item_strings[ENTRY_STR_SIZE][ENTRY_STR_LEN];
    u16     item_port;

    char    item_buf[GOPHER_MAX_LINE]; /* Temporary for debugging */
} item_entry;


/* Helper function to get raw hexdump of buffer */
void draw_hex(char *buffer, int size) {
    const unsigned char limit = 8;
    u16 x, y;
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
void draw_items(item_entry *items, u16 length) {
    printf("[");
    for (u16 x = 0; x < length; ++x) {
        if (x) printf(",\n");
        printf("{\n\t"
                    "\"id\": %d\n\t"
                    "\"type\": %d,\n\t"
                    "\"display\": \"%s\",\n\t"
                    "\"selector\": \"%s\",\n\t"
                    "\"hostname\": \"%s\",\n\t"
                    "\"port\": %hu\n"
                "}",
            x, 
            items[x].item_type, 
            items[x].item_strings[ENTRY_DISPLAY], 
            items[x].item_strings[ENTRY_SELECTOR], 
            items[x].item_strings[ENTRY_HOSTNAME], 
            items[x].item_port);
    }
    printf("]\n");
}