//
// Created by andrebrandao on 10/19/23.
//
#include <iostream>
#include <cstring>
#include <thread>
#include <mutex>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unordered_map>
#include <cstdarg>

#define SERVER_PORT 6969
#define BUF_SIZE 1024
#define MAX_CLIENTS 256

int client_count = 0;
std::mutex mtx;

std::unordered_map<std::string, int>client_sockets;



int output(const char *arg, ...){
    int res;
    va_list ap;
    va_start(ap, arg);
    res = vfprintf(stdout, arg, ap);
    va_end(ap);
    return res;
}

void send_message(const std::string &msg){
    mtx.lock();

    std::string pre = "@";
    int first_space = msg.find_first_of(" ");
    if (msg.compare(first_space+1, 1, pre) == 0){

        int space = msg.find_first_of(" ", first_space+1);
        std::string receive_name = msg.substr(first_space+2, space-first_space-2);
        std::string send_name = msg.substr(1, first_space-2);
        if(client_sockets.find(receive_name) == client_sockets.end()) {

            std::string error_msg = "[error] client nao existe " + receive_name;
            send(client_sockets[send_name], error_msg.c_str(), error_msg.length() + 1, 0);
        }
        else {
            send(client_sockets[receive_name], msg.c_str(), msg.length() + 1, 0);
            send(client_sockets[send_name], msg.c_str(), msg.length() + 1, 0);
        }
    }
    else {

        for (auto it = client_sockets.begin(); it != client_sockets.end(); it++) {
            send(it->second, msg.c_str(), msg.length()+1, 0);
        }
    }
    mtx.unlock();
}


void handle_client(int clnt_sock){
    char msg[BUF_SIZE];
    int flag = 0;

    char tell_name[13] = "#new client:";
    while(recv(clnt_sock, msg, sizeof(msg),0) != 0){

        if (std::strlen(msg) > std::strlen(tell_name)) {

            char pre_name[13];
            std::strncpy(pre_name, msg, 12);
            pre_name[12] = '\0';
            if (std::strcmp(pre_name, tell_name) == 0) {

                char name[20];
                std::strcpy(name, msg+12);
                if(client_sockets.find(name) == client_sockets.end()){
                    output("-> %s se conectou no socket n%d\n", name, clnt_sock);
                    client_sockets[name] = clnt_sock;
                }
                else {

                    std::string error_msg = std::string(name) + " ja existe. use outro nome!";
                    send(clnt_sock, error_msg.c_str(), error_msg.length()+1, 0);
                    mtx.lock();
                    client_count--;
                    mtx.unlock();
                    flag = 1;
                }
            }
        }

        if(flag == 0)
            send_message(std::string(msg));
    }
    if(flag == 0){

        std::string leave_msg;
        std::string name;
        mtx.lock();
        for (auto it = client_sockets.begin(); it != client_sockets.end(); ++it ){
            if(it->second == clnt_sock){
                name = it->first;
                client_sockets.erase(it->first);
            }
        }
        client_count--;
        mtx.unlock();
        leave_msg = "-> " + name + " quitou";
        send_message(leave_msg);
        output("-> %s quitou\n", name.c_str());
        close(clnt_sock);
    }
    else {
        close(clnt_sock);
    }
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

int main(int argc,const char **argv,const char **envp){

    //socket setup
    int serv_sock,clnt_sock;
    struct sockaddr_in serv_addr, clnt_addr;
    socklen_t clnt_addr_size;

    serv_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serv_sock == -1){
        error_handling("socket() failed!");
    }

    //endereco do servidor
    memset(&serv_addr,0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    // serv_addr.sin_port=htons(atoi(argv[1]));
    serv_addr.sin_port = htons(SERVER_PORT);

    //bind socket com endereco do servidor
    if (bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1){
        error_handling("bind() failed!");
    }

    printf("server port: %d\n", SERVER_PORT);

    // listen novas conexoes
    if (listen(serv_sock, MAX_CLIENTS) == -1){
        error_handling("listen() error!");
    }

    // accept and handle clients
    while(true){
        clnt_addr_size = sizeof(clnt_addr);

        clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
        if (clnt_sock == -1){
            error_handling("accept() failed!");
        }

        // nova tread para cada client
        mtx.lock();
        client_count++;
        mtx.unlock();

        std::thread th(handle_client, clnt_sock);
        th.detach();

        //info client
        output("connected client IP: %s \n", inet_ntoa(clnt_addr.sin_addr));
    }
    close(serv_sock);
    return 0;
}

