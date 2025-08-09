#pragma comment (lib, "Ws2_32.lib")

#include<iostream>
#include<WinSock2.h>


static void die(const char *msg) {
    int err = errno;
    std::cout << err << " " << msg << std::endl;
    abort();
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

    char msg[] = "hello";
    send(fd, msg, strlen(msg),0);

    char rbuf[64] = {};
    int n = recv(fd, rbuf, 64,0);
    if (n < 0) {
        die("read");
    }
    printf("server says: %s\n", rbuf);
    closesocket(fd);
    WSACleanup();
    return 0;
}
