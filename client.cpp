#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <fstream>
#include <unordered_map>
#include <sstream>
#include <iostream>
#include <string>

#include <arpa/inet.h>

#define PORT "33859" // the port client will be connecting to 

#define MAXDATASIZE 100 // max number of bytes we can get at once 

std::string getSelfSockPort(int sockfd) {
    struct sockaddr_in saddr;
    socklen_t slent = sizeof(saddr);
    getsockname(sockfd, (struct sockaddr *)&saddr, &slent);
    return std::to_string(ntohs(saddr.sin_port));
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
    int sockfd, numbytes;  
    char buf[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];


    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo("127.0.0.1", PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
            s, sizeof s);
    printf("Client is up and running.\n");

    freeaddrinfo(servinfo); // all done with this structure


    std::string department;
    std::string ID;
    while(1)
    {
        printf("Enter department name:");
        std::cin>>department;

        printf("Enter student ID:");
        std::cin>>ID;

        std::string data=department+','+ID;


        if (send(sockfd, (char*)data.data(),data.size(), 0) < 0) {
            perror("send");
            std::cout << "send" << std::endl;
        }

        printf("Client has sent %s and Student%s to Main Server using TCP over port %s \n",(char*)department.data(),ID.c_str(),getSelfSockPort(sockfd).c_str());


        if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) < 0) {
            perror("recv");
            std::cout << "recv" << std::endl;
            exit(1);
        }

        buf[numbytes] = '\0';
        std::string label=buf;
        if(label.find("Not found")!=std::string::npos){
            printf("%s\n",buf);
        }
        else{
            printf("The performance statistics for Student %s in %s is:\n",ID.c_str(),department.c_str());
            //GPA

            // if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) < 0) {
            //     perror("recv");
            //     std::cout << "recv" << std::endl;
            //     exit(1);
            // }
            // buf[numbytes] = '\0';
            // std::cout<<buf<<std::endl;
            std::stringstream recording(buf);
            std::string x;
            getline(recording,x,',');
            printf("Student GPA:%s\n",x.c_str());
            //rank
            getline(recording,x,',');
            printf("Percentage Rank:%s%%\n",x.c_str());
            //mean
            getline(recording,x,',');
            printf("Department GPA Mean:%s\n",x.c_str());
            //Variance
            getline(recording,x,',');
            printf("Department GPA Variance:%s\n",x.c_str());
            //max
            getline(recording,x,',');
            printf("Department Max GPA:%s\n",x.c_str());
            //min
            getline(recording,x,',');
            printf("Department Min GPA:%s\n",x.c_str());
                

        }
        printf("-----Start a new request-----\n");
    }

    close(sockfd);

    return 0;
}