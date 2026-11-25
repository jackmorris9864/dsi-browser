/*
 * DSi Mini Browser
 * Lightweight HTTP/1.0 text browser for Nintendo DS/DSi homebrew.
 *
 * Controls:
 *   A      load the URL currently shown
 *   B      clear URL / go back to the prompt
 *   START  quit
 *   Touch  type a URL with the on-screen keyboard
 *
 * Limitations:
 *   - HTTP only (http://), intentionally lightweight
 *   - text-oriented rendering
 *   - no JavaScript/CSS/HTTPS/images
 */

#include <nds.h>
#include <dswifi9.h>

#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define URL_MAX 160
#define PAGE_MAX 12288
#define LINE_MAX 33

static char url[URL_MAX] = "http://example.com/";
static char page[PAGE_MAX];
static int page_len = 0;

static void wait_frames(int n) {
    while (n-- > 0) swiWaitForVBlank();
}

static void draw_header(const char *title) {
    consoleClear();
    iprintf("DSi Mini Browser\n");
    iprintf("----------------\n");
    iprintf("%s\n\n", title);
}

static void strip_html_to_text(const char *src, char *dst, int dstsz) {
    int out = 0;
    int in_tag = 0;
    int last_space = 0;

    for (int i = 0; src[i] && out < dstsz - 1; ++i) {
        unsigned char c = (unsigned char)src[i];

        if (c == '<') {
            in_tag = 1;
            continue;
        }
        if (c == '>') {
            in_tag = 0;
            if (out && !last_space && out < dstsz - 1) {
                dst[out++] = ' ';
                last_space = 1;
            }
            continue;
        }
        if (in_tag) continue;

        if (c == '&') {
            if (!strncmp(&src[i], "&amp;", 5)) { c = '&'; i += 4; }
            else if (!strncmp(&src[i], "&lt;", 4)) { c = '<'; i += 3; }
            else if (!strncmp(&src[i], "&gt;", 4)) { c = '>'; i += 3; }
            else if (!strncmp(&src[i], "&quot;", 6)) { c = '"'; i += 5; }
            else if (!strncmp(&src[i], "&nbsp;", 6)) { c = ' '; i += 5; }
        }

        if (c == '\r') continue;

        if (c == '\n' || c == '\t' || c == ' ') {
            if (!last_space && out < dstsz - 1) {
                dst[out++] = (c == '\n') ? '\n' : ' ';
                last_space = (c != '\n');
            }
        } else {
            dst[out++] = (char)c;
            last_space = 0;
        }
    }

    dst[out] = '\0';
}

static void show_page(const char *text) {
    draw_header("Page");

    int col = 0;
    int lines = 0;

    for (int i = 0; text[i] && lines < 20; ++i) {
        char c = text[i];

        if (c == '\n') {
            iprintf("\n");
            col = 0;
            ++lines;
            continue;
        }

        if (col >= LINE_MAX - 1) {
            iprintf("\n");
            col = 0;
            ++lines;
            if (lines >= 20) break;
        }

        iprintf("%c", c);
        ++col;
    }

    iprintf("\n\nA: reload  B: URL  START: quit");
}

static int parse_url(const char *input, char *host, int hostsz,
                     char *path, int pathsz, int *port) {
    const char *p = input;

    if (!strncmp(p, "http://", 7)) p += 7;
    else return 0;

    const char *slash = strchr(p, '/');
    const char *end = slash ? slash : p + strlen(p);
    const char *colon = NULL;

    for (const char *q = p; q < end; ++q) {
        if (*q == ':') { colon = q; break; }
    }

    const char *host_end = colon ? colon : end;
    int host_len = (int)(host_end - p);
    if (host_len <= 0 || host_len >= hostsz) return 0;

    memcpy(host, p, host_len);
    host[host_len] = '\0';

    *port = 80;
    if (colon) {
        *port = atoi(colon + 1);
        if (*port <= 0 || *port > 65535) return 0;
    }

    if (slash) {
        strncpy(path, slash, pathsz - 1);
        path[pathsz - 1] = '\0';
    } else {
        strncpy(path, "/", pathsz - 1);
        path[pathsz - 1] = '\0';
    }

    return 1;
}

static int http_get(const char *input) {
    char host[96];
    char path[URL_MAX];
    int port;

    if (!parse_url(input, host, sizeof(host), path, sizeof(path), &port)) {
        draw_header("URL error");
        iprintf("Only URLs beginning with\nhttp:// are supported.\n");
        return 0;
    }

    draw_header("Connecting...");
    iprintf("Host: %s\n", host);

    struct hostent *he = gethostbyname(host);
    if (!he || !he->h_addr_list[0]) {
        iprintf("DNS lookup failed.\n");
        return 0;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        iprintf("Socket creation failed.\n");
        return 0;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port);
    memcpy(&addr.sin_addr, he->h_addr_list[0], sizeof(addr.sin_addr));

    iprintf("Connecting...\n");
    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(sock);
        iprintf("Connection failed.\n");
        return 0;
    }

    char request[URL_MAX + 256];
    int req_len = snprintf(
        request, sizeof(request),
        "GET %s HTTP/1.0\r\n"
        "Host: %s\r\n"
        "User-Agent: DSi-Mini-Browser/0.1\r\n"
        "Connection: close\r\n\r\n",
        path, host
    );

    if (send(sock, request, req_len, 0) < 0) {
        close(sock);
        iprintf("Request failed.\n");
        return 0;
    }

    page_len = 0;
    while (page_len < PAGE_MAX - 1) {
        int n = recv(sock, page + page_len, PAGE_MAX - 1 - page_len, 0);
        if (n <= 0) break;
        page_len += n;
    }
    close(sock);
    page[page_len] = '\0';

    char *body = strstr(page, "\r\n\r\n");
    if (!body) body = page;
    else body += 4;

    static char text[PAGE_MAX];
    strip_html_to_text(body, text, sizeof(text));
    show_page(text);
    return 1;
}

static void url_prompt(void) {
    Keyboard *kb = keyboardDemoInit();
    (void)kb;

    draw_header("Enter URL");
    iprintf("Touch the lower screen\n");
    iprintf("to type a URL.\n\n");
    iprintf("Current:\n%s\n\n", url);
    iprintf("Press A when ready.\n");
    iprintf("B clears the URL.");

    while (pmMainLoop()) {
        swiWaitForVBlank();
        scanKeys();
        int down = keysDown();

        if (down & KEY_A) break;
        if (down & KEY_B) {
            url[0] = '\0';
            draw_header("Enter URL");
            iprintf("Type with the touch keyboard.\n\n");
            iprintf("URL: ");
        }
        if (down & KEY_START) {
            return;
        }
    }
}

int main(void) {
    consoleDemoInit();

    draw_header("Starting Wi-Fi...");
    iprintf("Using firmware settings.\n");

    if (!Wifi_InitDefault(WFC_CONNECT)) {
        iprintf("Wi-Fi init failed.\n");
        iprintf("Check your DSi Wi-Fi setup.\n");
        while (pmMainLoop()) {
            swiWaitForVBlank();
            scanKeys();
            if (keysDown() & KEY_START) break;
        }
        return 0;
    }

    iprintf("Wi-Fi connected.\n");
    wait_frames(30);

    for (;;) {
        url_prompt();

        if (!url[0]) {
            strcpy(url, "http://example.com/");
        }

        if (!http_get(url)) {
            iprintf("\nPress B for URL, START to quit.");
        }

        while (pmMainLoop()) {
            swiWaitForVBlank();
            scanKeys();
            int down = keysDown();

            if (down & KEY_A) {
                http_get(url);
            }
            if (down & KEY_B) {
                break;
            }
            if (down & KEY_START) {
                return 0;
            }
        }
    }

    return 0;
}
