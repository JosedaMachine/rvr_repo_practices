#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define BUF_SIZE 500


bool initConnection(const char* host, char* serv, addrinfo hints, addrinfo *result, int* socketDesc){
    //Transalate name to socket addresses
    int rc = getaddrinfo(host, serv, &hints, &result);

    if (rc != 0) {
        std::cerr << "Error: getaddrinfo -> " << gai_strerror(rc) << "\n";
        return -1;
    }

    //Open socket with result content. Always 0 to TCP and UDP
    (*socketDesc) = socket(result->ai_family, result->ai_socktype, 0);

    //Associate address. Where is going to listen
    rc = bind((*socketDesc), (struct sockaddr *) result->ai_addr, result->ai_addrlen);
    if (rc != 0) {
        std::cerr << "Error: bind -> " << gai_strerror(rc) << "\n";
        return -1;
    }

    return true;
}

int main(int argc, char *argv[]){
    
    int error_code, socketDesc;
    
    time_t rawtime;
    struct tm * timeinfo;

    addrinfo hints;
    addrinfo* result;

    memset(&hints, 0, sizeof(addrinfo));
    memset(&result, 0, sizeof(addrinfo));
    hints.ai_flags    = AI_PASSIVE; //Devolver 0.0.0.0
    hints.ai_family   = AF_INET;    // IPv4
    hints.ai_socktype = SOCK_DGRAM;

    time(&rawtime);

    //Initialize connection and listening
    if(!initConnection(argv[1],argv[2], hints, result, &socketDesc)){
        std::cerr << "Error: Initiliazation\n";
        return -1;
    }

    struct sockaddr cliente;
    socklen_t cliente_len = sizeof(cliente);
    char buf[BUF_SIZE];
    char host[NI_MAXHOST], serv[NI_MAXSERV];
    bool exit = false;


    while (!exit) {
        //Recive message
        int bytes = recvfrom(socketDesc, buf, BUF_SIZE, 0, &cliente, &cliente_len);
        
        if(bytes == -1) std::cerr << "Error: recvfrom.\n"; 
    
        buf[bytes]='\0'; 
        
        //Who's is sending message
        error_code = getnameinfo(&cliente, cliente_len, host, NI_MAXHOST, serv, NI_MAXSERV, NI_NUMERICHOST| NI_NUMERICSERV);
        if (error_code != 0) {
            std::cerr << "Error: getnameinfo -> " << gai_strerror(error_code) << "\n";
            return -1;
        }

        //No Mesage
        if(!bytes) continue;      

        printf("%d bytes de %s:%s\n",bytes, host, serv);
        
        //Process message
        switch (buf[0]) {
            case 't':
                timeinfo = localtime(&rawtime);
                bytes = strftime (buf,BUF_SIZE,"%I:%M:%S %p.",timeinfo);
                sendto(socketDesc, buf, bytes, 0, (struct sockaddr *) &cliente, cliente_len);
                break;
            case 'd':
                timeinfo = localtime(&rawtime);
                bytes = strftime (buf,BUF_SIZE,"%F",timeinfo);
                sendto(socketDesc, buf, bytes, 0, (struct sockaddr *) &cliente, cliente_len);
                break;
            case 'q':
                exit = true;
                std::cout << "Saliendo...\n"; 
                break;
            default:
                std::cout << "Comando no soportado " << buf[0] << "\n";
        }
    }
    close(socketDesc);
    freeaddrinfo(result);
    return 0;
}