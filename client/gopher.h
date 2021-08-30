#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>


#define GOPHER_PORT 70
#define CRLF "\r\n" /* 0x0D 0x0A*/
#define TAB "\t" /* 0x09 */
typedef unsigned short u32;

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

#define DISPLAY_LEN 70 /* 3.9: string should be kept under 70 chars in length */
#define SELECT_LEN 255 /* Appendix: Selector string no longer than 255 chars */
typedef struct {
    Itype   item_type;
    char    item_display[DISPLAY_LEN];
    char    item_selector[SELECT_LEN];
    char    item_hostname[SELECT_LEN];
    u32     item_port;
} item_entry;