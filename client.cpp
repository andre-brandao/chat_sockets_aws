//
// Created by andrebrandao on 10/19/23.
//
#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstdarg>

#define BUF_SIZE 1024
#define SERVER_PORT 6969
#define IP "54.160.185.74"

std::string name = "DEFAULT";
std::string msg;

void send_msg(int sock){
    while(true){


        getline(std::cin, msg);
        if (msg == "Quit"|| msg == "quit"){
            close(sock);
            exit(0);
        }

        std::string name_msg = name + " " + msg;
        send(sock, name_msg.c_str(), name_msg.length() + 1, 0);
    }
}

void receive_msg(int sock){
    char name_msg[BUF_SIZE + name.length() + 1];
    while (true){
        int str_len = recv(sock, name_msg, BUF_SIZE+name.length() + 1, 0);
        if (str_len == -1){
            exit(-1);
        }
        std::cout<<std::string(name_msg)<<std::endl;
    }
}

int output(const char *arg, ...){
    int res;
    va_list ap;
    va_start(ap, arg);
    res = vfprintf(stdout, arg, ap);
    va_end(ap);
    return res;
}

int error_output(const char *arg, ...){
    int res;
    va_list ap;
    va_start(ap, arg);
    res = vfprintf(stderr, arg, ap);
    va_end(ap);
    return res;
}

void error_handling(const std::string &message){
    std::cerr<<message<<std::endl;
    exit(1);
}

// argv[1] = name
int main(int argc,const char **argv,const char **envp){
    int sock;
    // sockaddr_in serv_addr{};
    struct sockaddr_in serv_addr;

    if (argc != 2){
        error_output("Usage : %s <Name> \n",argv[0]);
        exit(1);
    }

    name = "["+std::string(argv[1])+"]";

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == -1){
        error_handling("socket() failed!");
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(IP);
    serv_addr.sin_port = htons(SERVER_PORT);

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1){
        error_handling("connect() failed!");
    }

    std::string my_name = "#new client:" + std::string(argv[1]);
    send(sock, my_name.c_str(), my_name.length() + 1, 0);

    std::thread snd(send_msg, sock);
    std::thread rcv(receive_msg, sock);

    snd.join();
    rcv.join();

    close(sock);

    return 0;

}

