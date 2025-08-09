#pragma comment (lib, "Ws2_32.lib")

#include<iostream>
#include<WinSock2.h>




static void die(const char *msg) {
    int err = errno;
    std::cout << err << " " << msg << std::endl;
    abort();
}


static void msg(const char *msg) {
    fprintf(stderr, "%s\n", msg);
}


static void process(SOCKET connfd) {
    char rbf[64] = "";
    SSIZE_T n = recv(connfd, rbf, 64,0);
    if(n<0) {
        msg("read() error");
        return;
    }
    std::cout << stderr <<"client says: "<<rbf;
    char wbf[] = "world";
    send(connfd, wbf, strlen(wbf),0);
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
        process(connfd);
        closesocket(connfd);
    }
    closesocket(fd);
    WSACleanup();
    return 0;
}