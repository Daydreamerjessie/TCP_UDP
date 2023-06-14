
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
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <list>
#include <iomanip>
#define MAXBUFLEN 1024

#define SERVERPORT "32859"    // the port users will be connecting to
#define APORT "30859" 

int main(int argc, char *argv[])
{
    int sockfd,sockfd_s;
    struct addrinfo hints, *servinfo, *servinfo_s, *pr, *ps;
    int rv;
    int numbytes;
    struct sockaddr_storage their_addr;
    socklen_t addr_len= 1024;
    std::string address="127.0.0.1";
    char buf[MAXBUFLEN];
    char buf_try[MAXBUFLEN];

    // if (argc != 3) {
    //     fprintf(stderr,"usage: talker hostname message\n");
    //     exit(1);
    // }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // set to AF_INET to use IPv4
    hints.ai_socktype = SOCK_DGRAM;

    //p_receive

    if ((rv = getaddrinfo(address.c_str(), APORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and make a socket
    for(pr = servinfo; pr != NULL; pr = pr->ai_next) {
        if ((sockfd = socket(pr->ai_family, pr->ai_socktype,
                pr->ai_protocol)) == -1) {
            perror("talker: socket");
            continue;
        }

        if (bind(sockfd, pr->ai_addr, pr->ai_addrlen) == -1) {
            close(sockfd);
            perror("listener: bind");
            continue;
        }

        break;
    }

    if (pr == NULL) {
        fprintf(stderr, "talker: failed to create socket\n");
        return 2;
    }
    printf("Server A is up and running using UDP on port 30859\n");

    //p_send

    if ((rv = getaddrinfo(address.c_str(), SERVERPORT, &hints, &servinfo_s)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    for(ps = servinfo_s; ps != NULL; ps = ps->ai_next) {
        if ((sockfd_s = socket(ps->ai_family, ps->ai_socktype,
                ps->ai_protocol)) == -1) {
            perror("talker: socket");
            continue;
        }

        break;
    }

    if (ps == NULL) {
        fprintf(stderr, "talker: failed to create socket\n");
        return 2;
    }


    //Read dataA.txt


    std::unordered_map<int, std::string> stu_dpt;
    std::unordered_map<int, double> stu_gpa;
    std::ifstream file("data/dataA.csv");
    std::string data;
    std::string dpt;
    int flag=0;
    int stu;
    int gpa=0;
    int c=0;
    int n=0;
    std::string score;
    std::string depart_list;

    while(file>>data){
        if(flag==0){
            flag=1;
        }
        else{
            std::stringstream is(data);
            while(getline(is,score,',')){

                if(n==0){
                    dpt=score;
                    n+=1;
                }
                else if(n==1){
                    stu=std::stoi(score);
                    n+=1;
                }
                else{
                    if(score.size()){
                        gpa+=std::stoi(score);
                        // std::cout<<score<<std::endl;
                        n+=1;
                    }
                }
            }
            stu_gpa.insert(std::make_pair(stu,double(gpa)/(n-2)));
            stu_dpt.insert(std::make_pair(stu,dpt));
            // std::cout<<double(gpa)/(n-2)<<','<<stu<<dpt<<std::endl;
            gpa=0;
            n=0;
        }
    }

    std::unordered_map<std::string, double> dpt_mean;
    std::unordered_map<std::string, double> dpt_variance;
    std::unordered_map<std::string, double> dpt_max;
    std::unordered_map<std::string, double> dpt_min;
    std::unordered_map<std::string, int> dpt_num;
    std::unordered_map<int, double> stu_rank;

    for(auto& i : stu_dpt){
        // std::cout<< i.first<<','<<i.second<<std::endl;
        if(dpt_mean.find(i.second)==dpt_mean.end()){
            dpt_mean.insert(std::make_pair(i.second,stu_gpa[i.first]));
            dpt_min.insert(std::make_pair(i.second,stu_gpa[i.first]));
            dpt_max.insert(std::make_pair(i.second,stu_gpa[i.first]));
            dpt_num.insert(std::make_pair(i.second,1));
        }
        else{
            dpt_mean[i.second]+=stu_gpa[i.first];
            dpt_num[i.second]+=1;
            if(stu_gpa[i.first]<dpt_min[i.second]){
                dpt_min[i.second]=stu_gpa[i.first];
            }
            if(dpt_max[i.second]<stu_gpa[i.first]){
                dpt_max[i.second]=stu_gpa[i.first];
            }
        }
    }
    for(auto& i : dpt_mean){
        // std::cout<<i.second<<i.first<<std::endl;
        i.second=i.second/dpt_num[i.first];
        // std::cout<<i.second<<i.first<<std::endl;
        depart_list+=i.first;
        depart_list+=',';
    }
    //variance
    for(auto& i: stu_dpt){
        if(dpt_variance.find(i.second)==dpt_variance.end()){
            dpt_variance.insert(std::make_pair(i.second,(double(stu_gpa[i.first])-dpt_mean[i.second])*(double(stu_gpa[i.first])-dpt_mean[i.second])));
        }
        else{
            dpt_variance[i.second]+=(double(stu_gpa[i.first])-dpt_mean[i.second])*(double(stu_gpa[i.first])-dpt_mean[i.second]);
        }
    }
    for(auto& i : dpt_variance){
        i.second=i.second/dpt_num[i.first];
        // std::cout<<i.second<<i.first<<std::endl;
    }
    //ranking



    //waiting for the servermain
    if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0, (struct sockaddr *)&their_addr, &addr_len)) == -1) {
        perror("recvfrom");
        exit(1);
    }
    //send the department list
    if ((numbytes = sendto(sockfd_s, depart_list.c_str(), strlen(depart_list.c_str()), 0, ps->ai_addr, ps->ai_addrlen)) == -1) {
        perror("serverA: sendto");
        exit(1);
    }



    printf("Server A has sent a department list to Main Server\n");

    std::string cur_stu;
    std::string cur_dpt;

    

    while(1){
        if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0, (struct sockaddr *)&their_addr, &addr_len)) == -1) {
            perror("recvfrom");
            exit(1);
        }

        buf[numbytes] = '\0';
        cur_stu=buf;
        // std::cout<<cur_stu<<buf<<std::endl;

        if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0, (struct sockaddr *)&their_addr, &addr_len)) == -1) {
            perror("recvfrom");
            exit(1);
        }
        buf[numbytes] = '\0';
        cur_dpt=buf;
        // std::cout<<cur_dpt<<buf<<std::endl;

        printf("Server A has received a request for Student %s in %s\n", cur_stu.c_str(),buf);
        //if not find this student

        if(stu_dpt.find(stoi(cur_stu))==stu_dpt.end() or stu_dpt[stoi(cur_stu)]!=cur_dpt){
            printf("Student %s does not show up in %s\n", cur_stu.c_str(),buf);
            printf("Server A has sent “Student %s not found” to Main Server\n",cur_stu.c_str());
            std::string sig="1";
            if ((numbytes = sendto(sockfd_s, sig.c_str(), strlen(sig.c_str()), 0, ps->ai_addr, ps->ai_addrlen)) == -1) {
                perror("serverA: sendto");
                exit(1);
            }

        }
        else{
            printf("Server A calculated following academic statistics for Student %s in %s:\n",cur_stu.c_str(),cur_dpt.c_str());
            std::stringstream stream;
            stream << std::fixed << std::setprecision(1) << stu_gpa[stoi(cur_stu)];
            std::string x = stream.str();
            printf("Student GPA:%s\n",x.c_str());
            if ((numbytes = sendto(sockfd_s, x.c_str(), strlen(x.c_str()), 0, ps->ai_addr, ps->ai_addrlen)) == -1) {
                perror("serverA: sendto");
                exit(1);
            }
            //rank
            int rank=0;
            std::string rank_str;
            for(auto& i : stu_dpt){
                // std::cout<< i.second<<','<<cur_dpt<<stu_gpa[stoi(cur_stu)]<<std::endl;
                if(i.second==cur_dpt and stu_gpa[i.first]<stu_gpa[stoi(cur_stu)]){
                    rank+=1;
                }
            }
            // rank_str=std::to_string(100*double(rank)/dpt_num[cur_dpt]);
            std::stringstream r;
            r << std::fixed << std::setprecision(1) << 100*double(rank)/dpt_num[cur_dpt];
            x = r.str();
            printf("Percentage Rank: %s%%\n",x.c_str());
            if ((numbytes = sendto(sockfd_s, x.c_str(), strlen(x.c_str()), 0, ps->ai_addr, ps->ai_addrlen)) == -1) {
                perror("serverA: sendto");
                exit(1);
            }
            std::stringstream m;
            m << std::fixed << std::setprecision(1) << dpt_mean[cur_dpt];
            x = m.str();
            printf("Department GPA Mean: %s\n",x.c_str());
            if ((numbytes = sendto(sockfd_s, x.c_str(), strlen(x.c_str()), 0, ps->ai_addr, ps->ai_addrlen)) == -1) {
                perror("serverA: sendto");
                exit(1);
            }
            std::stringstream v;
            v << std::fixed << std::setprecision(1) << dpt_variance[cur_dpt];
            x = v.str();
            printf("Department GPA Variance: %s\n",x.c_str());
            if ((numbytes = sendto(sockfd_s, x.c_str(), strlen(x.c_str()), 0, ps->ai_addr, ps->ai_addrlen)) == -1) {
                perror("serverA: sendto");
                exit(1);
            }
            std::stringstream a;
            a << std::fixed << std::setprecision(1) << dpt_max[cur_dpt];
            x = a.str();
            printf("Department Max GPA: %s\n",x.c_str());
            if ((numbytes = sendto(sockfd_s, x.c_str(), strlen(x.c_str()), 0, ps->ai_addr, ps->ai_addrlen)) == -1) {
                perror("serverA: sendto");
                exit(1);
            }
            std::stringstream i;
            i << std::fixed << std::setprecision(1) << dpt_min[cur_dpt];
            x = i.str();
            printf("Department Min GPA: %s\n",x.c_str());
            if ((numbytes = sendto(sockfd_s, x.c_str(), strlen(x.c_str()), 0, ps->ai_addr, ps->ai_addrlen)) == -1) {
                perror("serverA: sendto");
                exit(1);
            }
            printf("Server A has sent the results to Main Server\n");
        }
    }



    freeaddrinfo(servinfo);
    freeaddrinfo(servinfo_s);

    close(sockfd);
    close(sockfd_s);
    

    return 0;
}