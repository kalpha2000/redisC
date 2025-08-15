#pragma comment (lib, "Ws2_32.lib")

#include<iostream>
#include<WinSock2.h>
#include<assert.h>
#include<string>

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

static int32_t process(SOCKET connfd) {

    // 4 bytes header
    char rbuf[4 + k_max_msg];
    errno = 0;
    // reading length of message
    int32_t err = read_full(connfd, rbuf, 4);
    if (err) {
        msg(errno == 0 ? "EOF" : "read() error");
        return err;
    }
    uint32_t len = 0;
    memcpy(&len, rbuf, 4);  // assume little endian
    std::cout << len<<" ";
    if (len > k_max_msg) {
        msg("too long");
        return -1;
    }
    // request body - reading message of length what we just read, rbug[4] as 1st 3 position if for space
    err = read_full(connfd, &rbuf[4], len);
    if (err) {
        msg("read() error");
        return err;
    }
    // do something
    std::cout << "client says: " << std::string(&rbuf[4], len) << std::endl;
    // reply using the same protocol
    char reply[] = "world";
    char wbuf[4 + sizeof(reply)];
    len = (uint32_t)strlen(reply);
    memcpy(wbuf, &len, 4);
    memcpy(&wbuf[4], reply, len);
    return write_all(connfd, wbuf, 4 + len);
}
int main() {
    // init winsock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        die("Wsa startup");
        return 1;
    }
    // just a constant to start a socket taken in (type of address(ip4/ip6),tcp/udp,protocol)
    SOCKET fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd == INVALID_SOCKET) {
        die("socket creation");
        return 0;
    }

    // set the reuse port on this socket REUSEADDR, so that on startup
    // server quickly binds to the port
    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&val, sizeof(val));
    
    // lets bind this socket to a port
    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8088); // since this socket is going to be visible to network, the host port(8088) is to be converted to network order
    addr.sin_addr.s_addr = htonl(0); // same with address (0.0.0.0) localhost
    int rv = bind(fd, (const sockaddr*)&addr, sizeof(addr));
    if (rv) {
        die("socket bind");
        return 0;
    }

    rv = listen(fd, SOMAXCONN);
    if(rv) {
        die("socket listen");
        return 0;
    }

    while(true) {
        // accept a connection
        sockaddr_in client_addr = {};
        int size = sizeof(client_addr);
        SOCKET connfd = accept(fd, (sockaddr*)&client_addr, &size);
        if(connfd < 0) {
            continue; // error
        }
        // will serve only 1 connection at a time
        while(true) {
            int32_t err = process(connfd);
            if(err < 0) {
                break;
            }
        }
        closesocket(connfd);
    }
    closesocket(fd);
    WSACleanup();
    return 0;
}