

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <list>

#define MYPORT "32859"    // the port users will be connecting to
#define APORT "30859" 
#define BPORT "31859" 
#define PORT "33859"  // the port clients will be connecting to


#define MAXBUFLEN 1024
#define BUF_SIZE 1024
#define BACKLOG 10   // how many pending connections queue will hold
#define MAXDATASIZE 100 // max number of bytes we can get at once 

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}



int main(void)
{
    int sockfd;
    int sockfd1;
    int sockfd2;
    struct addrinfo hints, *servinfo,*servinfo1,*servinfo2, *p,*p1,*p2;
    int rv;
    int numbytes;
    std::string address="127.0.0.1";
    struct sockaddr_storage their_addr;
    char buf1[MAXBUFLEN];
    char buf2[MAXBUFLEN];
    socklen_t addr_len= 1024;
    char s[INET6_ADDRSTRLEN];

    //tcp_client

    int sockfd_client, new_fd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints_c, *servinfo_c, *p_c;
    struct sockaddr_storage their_addr_c; // connector's address information
    socklen_t sin_size_c;
    struct sigaction sa_c;
    int yes=1;
    char s_c[INET6_ADDRSTRLEN];
    int rv_c;

    memset(&hints_c, 0, sizeof hints_c);
    hints_c.ai_family = AF_UNSPEC;
    hints_c.ai_socktype = SOCK_STREAM;
    hints_c.ai_flags = AI_PASSIVE; // use my IP

    // std::cout<<1<<std::endl;

    //  TCP: 1. getaddrinfo(): set up structures
    if ((rv_c = getaddrinfo(NULL, PORT, &hints_c, &servinfo_c)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv_c));
        return 1;
    }

    // std::cout<<2<<std::endl;
    // loop through all the results and bind to the first we can
    for(p_c = servinfo_c; p_c != NULL; p_c = p_c->ai_next) {
        //2. socket(): return socket descriptor
        if ((sockfd_client = socket(p_c->ai_family, p_c->ai_socktype,
                p_c->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd_client, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }
        //3. bind the port

        if (bind(sockfd_client, p_c->ai_addr, p_c->ai_addrlen) == -1) {
            close(sockfd_client);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo_c); // all done with this structure
    // std::cout<<1<<std::endl;

    if (p_c == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }
    //4. connect()
    //5. listen()

    if (listen(sockfd_client, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa_c.sa_handler = sigchld_handler; // reap all dead processes
    
    sigemptyset(&sa_c.sa_mask);
    sa_c.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa_c, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // set to AF_INET to use IPv4
    hints.ai_socktype = SOCK_DGRAM;
    // hints.ai_flags = AI_PASSIVE; // use my IP
    // std::cout<<2<<std::endl;

    //sendto serverA

    if ((rv = getaddrinfo(address.c_str(), APORT, &hints, &servinfo1)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    for(p1 = servinfo1; p1 != NULL; p1 = p1->ai_next) {
        if ((sockfd1 = socket(p1->ai_family, p1->ai_socktype, p1->ai_protocol)) == -1) {
            perror("listener: socket");
            continue;
        }

        break;
    }

    if (p1 == NULL) {
        fprintf(stderr, "listener: failed to bind socket\n");
        return 2;
    }



    // sendto serverB

    if ((rv = getaddrinfo(address.c_str(), BPORT, &hints, &servinfo2)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    for(p2 = servinfo2; p2 != NULL; p2 = p2->ai_next) {
        if ((sockfd2 = socket(p2->ai_family, p2->ai_socktype,
                p2->ai_protocol)) == -1) {
            perror("listener: socket");
            continue;
        }

        break;
    }

    //receive from A and B

    if (p2 == NULL) {
        fprintf(stderr, "listener: failed to bind socket\n");
        return 2;
    }



    if ((rv = getaddrinfo(address.c_str(), MYPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("listener: socket");
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("listener: bind");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "listener: failed to bind socket\n");
        return 2;
    }

    printf("Main server is up and running.\n");

    //send signal to A and B: the server is running!

    std::string sig="1";
    std::string N="1";
    std::string Y="0";

    if ((numbytes = sendto(sockfd1, sig.c_str(), strlen(sig.c_str()), 0, p1->ai_addr, p1->ai_addrlen)) == -1) {
        perror("serverA: sendto");
        exit(1);
    }

    if ((numbytes = recvfrom(sockfd, buf1, MAXBUFLEN-1 , 0, (struct sockaddr *)&their_addr, &addr_len)) == -1) {
        perror("recvfrom");
        exit(1);
    }

    printf("Main server has received the department list from server A using UDP over port 32859\n");
    printf("Main server has received the department list from server B using UDP over port 32859\n");

    buf1[numbytes] = '\0';

    // store A's info
    std::unordered_map<std::string, int> department_backend_mapping;
    std::stringstream is(buf1);
    std::string depart_str;

    printf("Server A\n");

    while(getline(is,depart_str,',')){
        department_backend_mapping.insert(std::make_pair(depart_str,1));
        std::cout<<depart_str<<std::endl; 
    }

    if ((numbytes = sendto(sockfd2, sig.c_str(), strlen(sig.c_str()), 0, p2->ai_addr, p2->ai_addrlen)) == -1) {
        perror("serverB: sendto");
        exit(1);
    }
    

    if ((numbytes = recvfrom(sockfd, buf2, MAXBUFLEN-1 , 0, (struct sockaddr *)&their_addr, &addr_len)) == -1) {
        perror("recvfrom");
        exit(1);
    }
    printf("\n");

    buf2[numbytes] = '\0';

    printf("Server B\n");

    //store B's info

    std::stringstream is2(buf2);
    while(getline(is2,depart_str,',')){
        department_backend_mapping.insert(std::make_pair(depart_str,2));
        std::cout<<depart_str<<std::endl; 
    }
    printf("\n");

    int client=0;

    while(1){
        client+=1;
        sin_size_c = sizeof their_addr;
        // 6. accept()
        new_fd = accept(sockfd_client, (struct sockaddr *)&their_addr_c, &sin_size_c);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr_c.ss_family,
            get_in_addr((struct sockaddr *)&their_addr_c),
            s_c, sizeof s_c);
        if (!fork()) { // this is the child process
            close(sockfd_client); // child doesn't need the listener

            //7. send() and recv()
            // if (send(new_fd, "Hello, world!", 13, 0) == -1)
            //     perror("send");
            
            int numbytes;
            char buf[MAXDATASIZE];
            std::string department;
            std::string ID;

            while(1)
            {
                //department+ID
                if ((numbytes = recv(new_fd, buf, MAXDATASIZE-1, 0)) == -1) {
                perror("recv");
                exit(1);
                break;
                }      
                if(numbytes==0){
                    break;
                }   
                buf[numbytes] = '\0';
                std::stringstream dpt_ID(buf);
                getline(dpt_ID,department,',');
                getline(dpt_ID,ID,',');

                printf("Main server has received the request on Student %s in %s from client %d using TCP over port 33859\n",ID.c_str(),department.c_str(),client);

                std::string s;

                if(department_backend_mapping.find(department)==department_backend_mapping.end()){
                    s=department+": Not found";
                    printf("%s does not show up in server A&B\n",department.c_str());

                    if (send(new_fd, (char*)s.data(), s.size(), 0) == -1){
                        perror("send");
                        break;
                    }

                    printf("Main Server has sent “%s: Not found” to client %d using TCP over port 33859\n", department.c_str(),client);
                }
                else{

                    // found in server A
                    if(department_backend_mapping[department]==1){
                        printf("%s shows up in server A\n",department.c_str());
                        printf("The Main Server has sent request of Student %s to server A using UDP over port 32859\n",ID.c_str());
                        
                        if ((numbytes = sendto(sockfd1, ID.c_str(), strlen(ID.c_str()), 0, p1->ai_addr, p1->ai_addrlen)) == -1) {
                            perror("serverA: sendto");
                            exit(1);
                        }


                        if ((numbytes = sendto(sockfd1, department.c_str(), strlen(department.c_str()), 0, p1->ai_addr, p1->ai_addrlen)) == -1) {
                            perror("serverA: sendto");
                            exit(1);
                        }

                        if ((numbytes = recvfrom(sockfd, buf1, MAXBUFLEN-1 , 0, (struct sockaddr *)&their_addr, &addr_len)) == -1) {
                            perror("recvfrom");
                            exit(1);
                        }
                        buf1[numbytes] = '\0';

                        if(buf1 ==sig){
                            printf("Main server has received “Student %s: Not found” from server A\n",ID.c_str());
                            //N2
                            s="Student "+ID+": Not found";
                            if (send(new_fd, (char*)s.data(), s.size(), 0) == -1){
                                perror("send");
                                break;
                            }
                            
                            printf("Main Server has sent message to client %d using TCP over 33859\n",client);
                        }
                        else
                        {
                            printf("Main server has received searching result of Student %s from server A\n",ID.c_str());
                            printf("Main Server has sent searching result(s) to client %d using TCP over port 33859\n",client);
                            // printf("Student GPA:%s\n",buf1);
                            //GPA
                            std::string data=buf1;

                            //rank
                            if ((numbytes = recvfrom(sockfd, buf1, MAXBUFLEN-1 , 0, (struct sockaddr *)&their_addr, &addr_len)) == -1) {
                                perror("recvfrom");
                                exit(1);
                            }
                            buf1[numbytes] = '\0';
                            data+=',';
                            data+=buf1;

                            // printf("Percentage Rank: %s\n",buf1);
                            //mean

                            if ((numbytes = recvfrom(sockfd, buf1, MAXBUFLEN-1 , 0, (struct sockaddr *)&their_addr, &addr_len)) == -1) {
                                perror("recvfrom");
                                exit(1);
                            }
                             buf1[numbytes] = '\0';
                            data+=',';                    
                            data+=buf1;

                            //variance
                            if ((numbytes = recvfrom(sockfd, buf1, MAXBUFLEN-1 , 0, (struct sockaddr *)&their_addr, &addr_len)) == -1) {
                                perror("recvfrom");
                                exit(1);
                            }
                             buf1[numbytes] = '\0';
                             data+=',';                    
                            data+=buf1;

                            //max
                            if ((numbytes = recvfrom(sockfd, buf1, MAXBUFLEN-1 , 0, (struct sockaddr *)&their_addr, &addr_len)) == -1) {
                                perror("recvfrom");
                                exit(1);
                            }
                             buf1[numbytes] = '\0';
                            data+=',';                    
                            data+=buf1;
                            //min
                            if ((numbytes = recvfrom(sockfd, buf1, MAXBUFLEN-1 , 0, (struct sockaddr *)&their_addr, &addr_len)) == -1) {
                                perror("recvfrom");
                                exit(1);
                            }
                            buf1[numbytes] = '\0';
                            data+=',';                    
                            data+=buf1;

                            if (send(new_fd, data.c_str(), strlen(data.c_str()), 0) == -1){
                                perror("send");
                                break;
                            }
                        }
                    }
                    //found in server B
                    else{
                        printf("%s shows up in server B\n",department.c_str());
                        printf("The Main Server has sent request of Student %s to server B using UDP over port 32859\n",ID.c_str());
                        
                        if ((numbytes = sendto(sockfd2, ID.c_str(), strlen(ID.c_str()), 0, p2->ai_addr, p2->ai_addrlen)) == -1) {
                            perror("serverB: sendto");
                            exit(1);
                        }

                        if ((numbytes = sendto(sockfd2, department.c_str(), strlen(department.c_str()), 0, p2->ai_addr, p2->ai_addrlen)) == -1) {
                            perror("serverB: sendto");
                            exit(1);
                        }
                        if ((numbytes = recvfrom(sockfd, buf1, MAXBUFLEN-1 , 0, (struct sockaddr *)&their_addr, &addr_len)) == -1) {
                            perror("recvfrom");
                            exit(1);
                        }
                        buf1[numbytes] = '\0';

                        if(buf1 ==sig){
                            //N3
                            s="Student "+ID+": Not found";
                            if (send(new_fd, (char*)s.data(), s.size(), 0) == -1){
                                perror("send");
                                break;
                            }
                            printf("Main server has received “Student %s: Not found” from server B\n",ID.c_str());
                            printf("Main Server has sent message to client %d using TCP over 33859\n",client);
                        }
                        else
                        {

                            printf("Main server has received searching result of Student %s from server B\n",ID.c_str());
                            printf("Main Server has sent searching result(s) to client %d using TCP over port 33859\n",client);
                            // printf("Student GPA:%s\n",buf1);
                            //GPA
                            std::string data=buf1;

                            //rank
                            if ((numbytes = recvfrom(sockfd, buf1, MAXBUFLEN-1 , 0, (struct sockaddr *)&their_addr, &addr_len)) == -1) {
                                perror("recvfrom");
                                exit(1);
                            }
                            buf1[numbytes] = '\0';
                            data+=',';
                            data+=buf1;

                            // printf("Percentage Rank: %s\n",buf1);
                            //mean

                            if ((numbytes = recvfrom(sockfd, buf1, MAXBUFLEN-1 , 0, (struct sockaddr *)&their_addr, &addr_len)) == -1) {
                                perror("recvfrom");
                                exit(1);
                            }
                             buf1[numbytes] = '\0';
                            data+=',';                    
                            data+=buf1;

                            //variance
                            if ((numbytes = recvfrom(sockfd, buf1, MAXBUFLEN-1 , 0, (struct sockaddr *)&their_addr, &addr_len)) == -1) {
                                perror("recvfrom");
                                exit(1);
                            }
                             buf1[numbytes] = '\0';
                             data+=',';                    
                            data+=buf1;

                            //max
                            if ((numbytes = recvfrom(sockfd, buf1, MAXBUFLEN-1 , 0, (struct sockaddr *)&their_addr, &addr_len)) == -1) {
                                perror("recvfrom");
                                exit(1);
                            }
                             buf1[numbytes] = '\0';
                            data+=',';                    
                            data+=buf1;
                            //min
                            if ((numbytes = recvfrom(sockfd, buf1, MAXBUFLEN-1 , 0, (struct sockaddr *)&their_addr, &addr_len)) == -1) {
                                perror("recvfrom");
                                exit(1);
                            }
                            buf1[numbytes] = '\0';
                            data+=',';                    
                            data+=buf1;

                            if (send(new_fd, data.c_str(), strlen(data.c_str()), 0) == -1){
                                perror("send");
                                break;
                            }
                        }
                    }
                }

                // printf("-----Start a new query-----\n");

            }
                

            close(new_fd);
            exit(0);
        }
    }

    freeaddrinfo(servinfo);
    freeaddrinfo(servinfo1);
    freeaddrinfo(servinfo2);

    close(sockfd);
    close(sockfd1);
    close(sockfd2);


    return 0;
}