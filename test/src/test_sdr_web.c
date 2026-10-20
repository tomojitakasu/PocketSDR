//
//  Unit tests for sdr_web.c.
//
//  The tests drive the server over a real loopback socket with a minimal
//  HTTP / WebSocket client, so no test-only hook is needed in the source.
//
#include "test_sdr.h"

#include <sys/stat.h>
#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <direct.h>
typedef SOCKET sock_t;
#define sock_close closesocket
#define make_dir(dir) _mkdir(dir)
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
typedef int sock_t;
#define INVALID_SOCKET (-1)
#define sock_close close
#define make_dir(dir) mkdir(dir, 0755)
#endif

// constants -------------------------------------------------------------------
#define TEST_PORT   18127   // loopback port for the tests
#define TEST_HTML   "test_sdr_web_html" // temporary document root
#define TEST_INI    "test_sdr_web.ini"  // temporary settings file
#define INDEX_BODY  "<html><body>pocket sdr</body></html>\n"

// RFC 6455 5.1 example key and its expected Sec-WebSocket-Accept
#define WS_KEY      "dGhlIHNhbXBsZSBub25jZQ=="
#define WS_ACCEPT   "s3pPLMBiTxaQ9kYGzzhZRbK+xOo="

// write a text file -----------------------------------------------------------
static void write_file(const char *path, const char *text)
{
    FILE *fp = fopen(path, "wb");

    TEST_ASSERT_TRUE(fp != NULL);
    fwrite(text, 1, strlen(text), fp);
    fclose(fp);
}

// connect to the test server --------------------------------------------------
static sock_t connect_server(void)
{
    struct sockaddr_in addr = {0};
    sock_t sock;

    for (int i = 0; i < 100; i++) { // the server thread may not be listening yet
        sock = socket(AF_INET, SOCK_STREAM, 0);
        TEST_ASSERT_TRUE(sock != INVALID_SOCKET);
        addr.sin_family = AF_INET;
        addr.sin_port = htons(TEST_PORT);
        addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        if (!connect(sock, (struct sockaddr *)&addr, sizeof(addr))) {
            int on = 1;
            setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char *)&on,
                sizeof(on));
            return sock;
        }
        sock_close(sock);
        sdr_sleep_msec(20);
    }
    TEST_ASSERT_TRUE(0); // server not reachable
    return INVALID_SOCKET;
}

// receive until the peer closes or the buffer is full -------------------------
static int recv_all(sock_t sock, char *buff, int size)
{
    int n = 0;

    while (n < size - 1) {
        int m = recv(sock, buff + n, size - 1 - n, 0);
        if (m <= 0) break;
        n += m;
    }
    buff[n] = '\0';
    return n;
}

// receive at least n bytes ----------------------------------------------------
static int recv_n(sock_t sock, uint8_t *buff, int n)
{
    int m = 0;

    while (m < n) {
        int k = recv(sock, (char *)buff + m, n - m, 0);
        if (k <= 0) return m;
        m += k;
    }
    return m;
}

// send an HTTP request and return the response --------------------------------
static void http_req(const char *req, char *resp, int size)
{
    sock_t sock = connect_server();

    TEST_ASSERT_TRUE(send(sock, req, (int)strlen(req), 0) > 0);
    recv_all(sock, resp, size);
    sock_close(sock);
}

// open a WebSocket connection -------------------------------------------------
static sock_t ws_open(void)
{
    char req[512], resp[1024];
    sock_t sock = connect_server();
    int n;

    snprintf(req, sizeof(req), "GET /ws HTTP/1.1\r\nHost: 127.0.0.1\r\n"
        "Upgrade: websocket\r\nConnection: Upgrade\r\n"
        "Sec-WebSocket-Key: %s\r\nSec-WebSocket-Version: 13\r\n\r\n", WS_KEY);
    TEST_ASSERT_TRUE(send(sock, req, (int)strlen(req), 0) > 0);

    for (n = 0; n < (int)sizeof(resp) - 1; ) { // read the handshake response
        int m = recv(sock, resp + n, 1, 0);
        if (m <= 0) break;
        n += m;
        resp[n] = '\0';
        if (n >= 4 && !strcmp(resp + n - 4, "\r\n\r\n")) break;
    }
    TEST_ASSERT_TRUE(strstr(resp, "101") != NULL);
    TEST_ASSERT_TRUE(strstr(resp, WS_ACCEPT) != NULL);
    return sock;
}

// send a masked WebSocket text frame -------------------------------------------
static void ws_send(sock_t sock, const char *text)
{
    uint8_t buff[4096], mask[4] = {0x12, 0x34, 0x56, 0x78};
    int len = (int)strlen(text), n = 0;

    TEST_ASSERT_TRUE(len < (int)sizeof(buff) - 8);
    buff[n++] = 0x81; // FIN + text
    if (len < 126) {
        buff[n++] = (uint8_t)(0x80 | len);
    }
    else {
        buff[n++] = 0x80 | 126;
        buff[n++] = (uint8_t)(len >> 8);
        buff[n++] = (uint8_t)len;
    }
    memcpy(buff + n, mask, 4);
    n += 4;
    for (int i = 0; i < len; i++) {
        buff[n+i] = (uint8_t)(text[i] ^ mask[i % 4]);
    }
    TEST_ASSERT_TRUE(send(sock, (const char *)buff, n + len, 0) > 0);
}

// receive a WebSocket frame (opcode returned, payload in buff) ------------------
static int ws_recv(sock_t sock, uint8_t *buff, int size, int *len)
{
    uint8_t hdr[8];

    if (recv_n(sock, hdr, 2) < 2) return -1;
    int op = hdr[0] & 0x0F, n = hdr[1] & 0x7F;

    if (n == 126) {
        if (recv_n(sock, hdr, 2) < 2) return -1;
        n = (hdr[0] << 8) + hdr[1];
    }
    else if (n == 127) {
        if (recv_n(sock, hdr, 8) < 8) return -1;
        n = (hdr[6] << 8) + hdr[7]; // the tests stay well below 64 KB
    }
    if (n > size - 1) return -1;
    if (recv_n(sock, buff, n) < n) return -1;
    buff[n] = '\0';
    *len = n;
    return op;
}

// receive the next text frame of the given type --------------------------------
static void ws_recv_type(sock_t sock, const char *type, char *buff, int size)
{
    char key[64];

    snprintf(key, sizeof(key), "\"type\":\"%s\"", type);

    for (int i = 0; i < 200; i++) {
        int len, op = ws_recv(sock, (uint8_t *)buff, size, &len);
        TEST_ASSERT_TRUE(op >= 0);
        if (op == 0x01 && strstr(buff, key)) return;
    }
    TEST_ASSERT_TRUE(0); // message not received
}

// start the test server --------------------------------------------------------
static sdr_web_t *start_server(void)
{
    sdr_web_t *web = sdr_web_start(NULL, "127.0.0.1", TEST_PORT, TEST_HTML);

    TEST_ASSERT_TRUE(web != NULL);
    return web;
}

// sdr_web_start(), sdr_web_stop() ---------------------------------------------
static void test_sdr_web_start_stop(void)
{
    sdr_web_t *web = start_server();

    // no receiver was given, so none is returned
    TEST_ASSERT_TRUE(sdr_web_stop(web) == NULL);

    TEST_ASSERT_TRUE(sdr_web_stop(NULL) == NULL); // NULL is a no-operation

    // an address that cannot be bound is an error, not a crash
    web = sdr_web_start(NULL, "203.0.113.1", TEST_PORT, TEST_HTML);
    TEST_ASSERT_TRUE(web == NULL);

    // the port is released, so the server can be started again
    web = start_server();
    sdr_web_stop(web);
}

// HTTP static file server ------------------------------------------------------
static void test_sdr_web_http(void)
{
    char resp[4096];
    sdr_web_t *web = start_server();

    http_req("GET / HTTP/1.1\r\nHost: x\r\n\r\n", resp, sizeof(resp));
    TEST_ASSERT_TRUE(strstr(resp, "200") != NULL);
    TEST_ASSERT_TRUE(strstr(resp, "text/html") != NULL);
    TEST_ASSERT_TRUE(strstr(resp, INDEX_BODY) != NULL);

    http_req("GET /index.html HTTP/1.1\r\nHost: x\r\n\r\n", resp, sizeof(resp));
    TEST_ASSERT_TRUE(strstr(resp, "200") != NULL);

    http_req("GET /no_such_file HTTP/1.1\r\nHost: x\r\n\r\n", resp,
        sizeof(resp));
    TEST_ASSERT_TRUE(strstr(resp, "404") != NULL);

    // path traversal must not escape the document root
    http_req("GET /../src/sdr_web.c HTTP/1.1\r\nHost: x\r\n\r\n", resp,
        sizeof(resp));
    TEST_ASSERT_TRUE(strstr(resp, "200") == NULL);

    sdr_web_stop(web);
}

// WebSocket handshake and hello ------------------------------------------------
static void test_sdr_web_handshake(void)
{
    char buff[8192];
    sdr_web_t *web = start_server();
    sock_t sock = ws_open();

    // hello is pushed to a new client
    ws_recv_type(sock, "hello", buff, sizeof(buff));
    TEST_ASSERT_TRUE(strstr(buff, sdr_get_ver()) != NULL);
    TEST_ASSERT_TRUE(strstr(buff, "\"run\":0") != NULL);
    TEST_ASSERT_TRUE(strstr(buff, "\"cfg_ena\":0") != NULL); // no configuration

    sock_close(sock);
    sdr_web_stop(web);
}

// commands and monitor topics --------------------------------------------------
static void test_sdr_web_command(void)
{
    char buff[262144]; // ch_stat is the largest topic
    sdr_web_t *web = start_server();
    sock_t sock = ws_open();

    ws_recv_type(sock, "hello", buff, sizeof(buff));

    // get of a monitor topic answers with the topic
    ws_send(sock, "{\"cmd\":\"get\",\"topic\":\"rcv_stat\"}");
    ws_recv_type(sock, "rcv_stat", buff, sizeof(buff));

    // the receiver status carries the process CPU load
    const char *p = strstr(buff, "\"cpu\":");
    TEST_ASSERT_TRUE(p != NULL);
    double cpu = atof(p + 6);
    TEST_ASSERT_TRUE(cpu >= 0.0 && cpu <= 100.0);

    ws_send(sock, "{\"cmd\":\"get\",\"topic\":\"pvt_sol\"}");
    ws_recv_type(sock, "pvt_sol", buff, sizeof(buff));

    ws_send(sock, "{\"cmd\":\"get\",\"topic\":\"opts\"}");
    ws_recv_type(sock, "opts", buff, sizeof(buff));
    TEST_ASSERT_TRUE(strstr(buff, "\"el_mask\"") != NULL);

    // a command is acknowledged
    ws_send(sock, "{\"cmd\":\"setopt\",\"name\":\"el_mask\",\"value\":20}");
    ws_recv_type(sock, "ack", buff, sizeof(buff));
    TEST_ASSERT_TRUE(strstr(buff, "\"cmd\":\"setopt\"") != NULL);
    TEST_ASSERT_TRUE(strstr(buff, "\"ok\":true") != NULL);

    ws_send(sock, "{\"cmd\":\"get\",\"topic\":\"opts\"}");
    ws_recv_type(sock, "opts", buff, sizeof(buff));
    TEST_ASSERT_TRUE(strstr(buff, "\"el_mask\":20") != NULL);

    // the lifecycle commands are rejected without a configuration
    ws_send(sock, "{\"cmd\":\"start\"}");
    ws_recv_type(sock, "ack", buff, sizeof(buff));
    TEST_ASSERT_TRUE(strstr(buff, "\"ok\":false") != NULL);
    TEST_ASSERT_TRUE(strstr(buff, "not supported") != NULL);

    // an unknown command is answered, not ignored
    ws_send(sock, "{\"cmd\":\"no_such_command\"}");
    ws_recv_type(sock, "error", buff, sizeof(buff));
    TEST_ASSERT_TRUE(strstr(buff, "unknown cmd") != NULL);

    // subscription pushes the topic periodically
    ws_send(sock, "{\"cmd\":\"sub\",\"topic\":\"rcv_stat\",\"cyc\":50}");
    ws_recv_type(sock, "rcv_stat", buff, sizeof(buff));
    ws_recv_type(sock, "rcv_stat", buff, sizeof(buff));
    ws_send(sock, "{\"cmd\":\"unsub\",\"topic\":\"rcv_stat\"}");

    sdr_rcv_setopt("el_mask", 15.0); // restore the default
    sock_close(sock);
    sdr_web_stop(web);
}

// sdr_web_init_cfg() -----------------------------------------------------------
static void test_sdr_web_init_cfg(void)
{
    sdr_web_cfg_t cfg;

    memset(&cfg, 0xFF, sizeof(cfg));
    sdr_web_init_cfg(&cfg);

    TEST_ASSERT_EQ_INT(0, cfg.inp);
    TEST_ASSERT_EQ_INT(SDR_FMT_INT8X2, cfg.fmt);
    TEST_ASSERT_NEAR(12e6, cfg.fs, 1.0);
    TEST_ASSERT_NEAR(1.0, cfg.tscale, 1e-9);
    TEST_ASSERT_EQ_INT(-1, cfg.bus);
    TEST_ASSERT_EQ_INT(-1, cfg.port);
    TEST_ASSERT_EQ_INT(0, cfg.nsig);
    TEST_ASSERT_EQ_INT(0, cfg.conf_ena);
    TEST_ASSERT_EQ_INT(0, (int)strlen(cfg.dev_opt));
    for (int i = 0; i < SDR_MAX_RFCH; i++) {
        TEST_ASSERT_EQ_INT(2, cfg.IQ[i]);
        TEST_ASSERT_EQ_INT(2, cfg.bits[i]);
        TEST_ASSERT_NEAR(0.0, cfg.lpf_bw[i], 1e-9);
    }
    for (int i = 0; i < SDR_WEB_N_LOG; i++) { // $EPH and $ALM off by default
        TEST_ASSERT_EQ_INT(i == 7 || i == 8 ? 0 : 1, cfg.log_mask[i]);
    }
    sdr_web_init_cfg(NULL); // NULL is a no-operation
}

// configuration commands and settings file -------------------------------------
static void test_sdr_web_config(void)
{
    char buff[8192];
    sdr_web_cfg_t cfg;
    sdr_web_t *web = start_server();

    sdr_web_init_cfg(&cfg);
    sdr_web_set_cfg(web, &cfg, TEST_INI);
    sdr_web_set_cfg(NULL, &cfg, TEST_INI); // NULL is a no-operation

    sock_t sock = ws_open();
    ws_recv_type(sock, "hello", buff, sizeof(buff));
    TEST_ASSERT_TRUE(strstr(buff, "\"cfg_ena\":1") != NULL);

    // set the input, signal, output and system configuration
    ws_send(sock, "{\"cmd\":\"set_inp\",\"inp\":1,\"file\":\"if_data.bin\","
        "\"fmt\":1,\"fs\":24.0,\"fo\":\"1575.42\",\"IQ\":\"1\",\"bits\":\"3\","
        "\"lpf\":\"8.0\",\"toff\":0.5,\"tscale\":2.0,\"conf\":\"dev.conf\","
        "\"conf_ena\":1,\"dev_opt\":\"-GAIN=30.0\"}");
    ws_recv_type(sock, "ack", buff, sizeof(buff));
    ws_send(sock, "{\"cmd\":\"set_sig\",\"sigs\":\"L1CA:1-8\",\"rfch\":\"\","
        "\"satno\":\"1-8|1-5/1-10|1-36|1-9|1-63|1-14|120-158\"}");
    ws_recv_type(sock, "ack", buff, sizeof(buff));
    ws_send(sock, "{\"cmd\":\"set_out\",\"types\":\"1,0,0,0,0,0,0,0\","
        "\"paths\":\"sol.nmea|||||||\",\"log_mask\":\"1,1,1,1,1,1,1,1,1,1\","
        "\"array_sep\":1}");
    ws_recv_type(sock, "ack", buff, sizeof(buff));
    ws_send(sock, "{\"cmd\":\"set_sys\",\"opt\":\"-ARCH=2\",\"fast_acq\":1,"
        "\"fftw\":\"wisdom.txt\"}");
    ws_recv_type(sock, "ack", buff, sizeof(buff));

    // the configuration reads back as it was set
    ws_send(sock, "{\"cmd\":\"get\",\"topic\":\"cfg\"}");
    ws_recv_type(sock, "cfg", buff, sizeof(buff));
    TEST_ASSERT_TRUE(strstr(buff, "\"inp\":1") != NULL);
    TEST_ASSERT_TRUE(strstr(buff, "\"file\":\"if_data.bin\"") != NULL);
    TEST_ASSERT_TRUE(strstr(buff, "\"fmt\":1") != NULL);
    TEST_ASSERT_TRUE(strstr(buff, "\"sigs\":\"L1CA:1-8\"") != NULL);
    TEST_ASSERT_TRUE(strstr(buff,
        "\"satno\":\"1-8|1-5/1-10|1-36|1-9|1-63|1-14|120-158\"") != NULL);
    TEST_ASSERT_TRUE(strstr(buff, "\"conf_ena\":1") != NULL);
    TEST_ASSERT_TRUE(strstr(buff, "\"dev_opt\":\"-GAIN=30.0\"") != NULL);
    TEST_ASSERT_TRUE(strstr(buff, "\"array_sep\":1") != NULL);
    TEST_ASSERT_TRUE(strstr(buff, "\"fast_acq\":1") != NULL);
    TEST_ASSERT_TRUE(strstr(buff, "\"opt\":\"-ARCH=2\"") != NULL);

    // the receiver options text keeps only what was typed there
    TEST_ASSERT_TRUE(strstr(buff, "-FAST_SRCH") == NULL);
    TEST_ASSERT_TRUE(strstr(buff, "-ARRAY") == NULL);
    TEST_ASSERT_TRUE(strstr(buff, "-LPF=") == NULL);

    sock_close(sock);
    sdr_web_stop(web); // saves the settings file

    // a new server restores the settings
    web = start_server();
    sdr_web_init_cfg(&cfg);
    sdr_web_set_cfg(web, &cfg, TEST_INI);
    TEST_ASSERT_EQ_INT(1, sdr_web_load_cfg(web));
    TEST_ASSERT_EQ_INT(0, sdr_web_load_cfg(NULL)); // NULL is a no-operation

    sock = ws_open();
    ws_recv_type(sock, "hello", buff, sizeof(buff));
    ws_send(sock, "{\"cmd\":\"get\",\"topic\":\"cfg\"}");
    ws_recv_type(sock, "cfg", buff, sizeof(buff));
    TEST_ASSERT_TRUE(strstr(buff, "\"inp\":1") != NULL);
    TEST_ASSERT_TRUE(strstr(buff, "\"file\":\"if_data.bin\"") != NULL);
    TEST_ASSERT_TRUE(strstr(buff, "\"sigs\":\"L1CA:1-8\"") != NULL);
    TEST_ASSERT_TRUE(strstr(buff,
        "\"satno\":\"1-8|1-5/1-10|1-36|1-9|1-63|1-14|120-158\"") != NULL);
    TEST_ASSERT_TRUE(strstr(buff, "\"conf_ena\":1") != NULL);
    TEST_ASSERT_TRUE(strstr(buff, "\"dev_opt\":\"-GAIN=30.0\"") != NULL);
    TEST_ASSERT_TRUE(strstr(buff, "\"fast_acq\":1") != NULL);

    sock_close(sock);
    sdr_web_stop(web);
    remove(TEST_INI);
}

// oversized fftw/opt must not overflow the opts reply (issue #98) --------------
static void test_sdr_web_opts_overflow(void)
{
    char cmd[2048], buff[8192], fftw[1024], opt[1024];
    sdr_web_cfg_t cfg;
    sdr_web_t *web = start_server();

    sdr_web_init_cfg(&cfg);
    sdr_web_set_cfg(web, &cfg, ""); // enable configuration edits, no settings file

    sock_t sock = ws_open();
    ws_recv_type(sock, "hello", buff, sizeof(buff));
    TEST_ASSERT_TRUE(strstr(buff, "\"cfg_ena\":1") != NULL);

    // a long fftw alone pushes the reply past the former 1024-byte stack buffer
    memset(fftw, 'A', 900); fftw[900] = '\0';
    memset(opt,  'B', 400); opt[400]  = '\0';
    snprintf(cmd, sizeof(cmd),
        "{\"cmd\":\"set_sys\",\"fftw\":\"%s\",\"opt\":\"%s\"}", fftw, opt);
    ws_send(sock, cmd);
    ws_recv_type(sock, "ack", buff, sizeof(buff));
    TEST_ASSERT_TRUE(strstr(buff, "\"cmd\":\"set_sys\"") != NULL);
    TEST_ASSERT_TRUE(strstr(buff, "\"ok\":true") != NULL);

    // the opts reply is complete JSON and round-trips both long values intact
    ws_send(sock, "{\"cmd\":\"get\",\"topic\":\"opts\"}");
    ws_recv_type(sock, "opts", buff, sizeof(buff));
    int len = (int)strlen(buff);
    TEST_ASSERT_TRUE(len > 1024);           // exceeds the former buffer size
    TEST_ASSERT_TRUE(buff[len - 1] == '}'); // not truncated mid-object
    TEST_ASSERT_TRUE(strstr(buff, fftw) != NULL);
    TEST_ASSERT_TRUE(strstr(buff, opt) != NULL);

    // the server survived the oversized reply: a further command still answers
    ws_send(sock, "{\"cmd\":\"get\",\"topic\":\"opts\"}");
    ws_recv_type(sock, "opts", buff, sizeof(buff));

    sock_close(sock);
    sdr_web_stop(web);
}

// sdr_web_start_rcv() ----------------------------------------------------------
static void test_sdr_web_start_rcv(void)
{
    sdr_web_cfg_t cfg;
    sdr_web_t *web = start_server();

    // without a configuration the receiver cannot be started
    TEST_ASSERT_EQ_INT(0, sdr_web_start_rcv(web));
    TEST_ASSERT_EQ_INT(0, sdr_web_start_rcv(NULL));

    // an IF data file that does not exist fails, and the server stays up
    sdr_web_init_cfg(&cfg);
    cfg.inp = 1;
    snprintf(cfg.file, sizeof(cfg.file), "%s", "no_such_if_data.bin");
    snprintf(cfg.sig[0], sizeof(cfg.sig[0]), "%s", "L1CA");
    snprintf(cfg.prn[0], sizeof(cfg.prn[0]), "%s", "1");
    cfg.nsig = 1;
    sdr_web_set_cfg(web, &cfg, "");
    TEST_ASSERT_EQ_INT(0, sdr_web_start_rcv(web));

    char buff[4096];
    sock_t sock = ws_open();
    ws_recv_type(sock, "hello", buff, sizeof(buff));
    TEST_ASSERT_TRUE(strstr(buff, "\"run\":0") != NULL);

    sock_close(sock);
    TEST_ASSERT_TRUE(sdr_web_stop(web) == NULL);
}

// main ------------------------------------------------------------------------
int main(void)
{
#ifdef WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 0), &wsa);
#endif
    make_dir(TEST_HTML); // temporary document root
    write_file(TEST_HTML "/index.html", INDEX_BODY);

    TEST_RUN(test_sdr_web_start_stop);
    TEST_RUN(test_sdr_web_http);
    TEST_RUN(test_sdr_web_handshake);
    TEST_RUN(test_sdr_web_command);
    TEST_RUN(test_sdr_web_init_cfg);
    TEST_RUN(test_sdr_web_config);
    TEST_RUN(test_sdr_web_opts_overflow);
    TEST_RUN(test_sdr_web_start_rcv);

    remove(TEST_HTML "/index.html");
    return 0;
}
