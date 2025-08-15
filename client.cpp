#pragma comment (lib, "Ws2_32.lib")

#include<iostream>
#include<string>
#include<assert.h>
#include<WinSock2.h>

const size_t k_max_msg = 4096;

static void die(const char *msg) {
    int err = errno;
    std::cout << err << " " << msg << std::endl;
    abort();
}

static void msg(const char *msg) {
    fprintf(stderr, "%s\n", msg);
}

static int32_t read_full(int fd, char *buf, size_t n) {
    // try to read n bytes but it may be we are not able to thus a loop, same with write
    while (n > 0) {
        SSIZE_T rv = recv(fd, buf, n, 0);
        if (rv <= 0) {
            return -1;  // error, or unexpected EOF
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

static int32_t write_all(int fd, const char *buf, size_t n) {
    while (n > 0) {
        SSIZE_T rv = send(fd, buf, n, 0);
        if (rv <= 0) {
            return -1;  // error
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

static int32_t writeToServer(SOCKET fd, const char* text) {
    uint32_t len = strlen(text);
    if (len > k_max_msg) {
        return -1;
    }
    // send request
    char wbuf[4 + k_max_msg];
    memcpy(wbuf, &len, 4);  // assume little endian
    memcpy(&wbuf[4], text, len);
    if (int32_t err = write_all(fd, wbuf, 4 + len)) {
        return err;
    }
    // 4 bytes header
    char rbuf[4 + k_max_msg + 1];
    errno = 0;
    int32_t err = read_full(fd, rbuf, 4);
    if (err) {
        msg(errno == 0 ? "EOF" : "read() error");
        return err;
    }
    memcpy(&len, rbuf, 4);  // assume little endian
    if (len > k_max_msg) {
        msg("too long");
        return -1;
    }
    // reply body
    err = read_full(fd, &rbuf[4], len);
    if (err) {
        msg("read() error");
        return err;
    }
    // do something
    printf("server says: %.*s\n", len, &rbuf[4]);
    return 0;
}

int main() {
    // init winsock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        die("Wsa startup");
        return 1;
    }
    SOCKET fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == INVALID_SOCKET) {
        die("socket()");
    }

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(8088);
    addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK);  // 127.0.0.1 - local host can be accessd like so
    int rv = connect(fd, (const sockaddr *)&addr, sizeof(addr));
    if (rv) {
        die("connect");
    }
    int32_t err = writeToServer(fd, "hello1");
    if (err) {
        goto DONE;
    }
    err = writeToServer(fd, "hello2");
    if (err) {
        goto DONE;
    }
    DONE:
    closesocket(fd);
    WSACleanup();
    return 0;
}
