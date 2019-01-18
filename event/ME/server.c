#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>
#include<vector.h>

#include <sys/epoll.h>  //epoll ways file
#define IPADDRESS   "127.0.0.1"
#define PORT        8787
#define MAXSIZE     1024
#define LISTENQ     5
#define FDSIZE      1000
#define MAX_EVENTS 100

int setblock(int sock)
{
    int ret = fcntl(sock,F_SETEL, 0);
    if(ret<0){
        hand_erro("setblock");
    }
    return 0;
}

int setnoblock(int sock){
    int ret = fcntl(sock, F_SETFL, O_NOBLOCK);
    if(ret<0){
        hand_erro("setnoblokc");
    }
    return 0;
}




int main(){
    // 获取一个信号的东西
    signal(SIGPIPE, SIG_IGN);
    int listenfd;
    listenfd = socket(AF_INET, SOCK_STREAM,0); // 创建一个套接字流， 套接字就是一个文件描述符
    if(listenfd<0){
        hand_erro("socket_create");
    }
    int on =1;

    if(setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&on, sizeof(on)<0))
        hand_erro("setsockopt");

    
    struct sockaddr_in my_addr;
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(18000);   //here is host  sequeue
    my_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
     

//

    // 将套接字和端口号绑定  BIND
    if(bind(listenfd,(struct sockaddr *)& my_addr,sizeof(my_addr)<0))
        hand_erro("bind");

    int lisId = listen(listenfd, SOMAXCONN);
    if( lisId < 0)   //LISTEN
        hand_error("listen");

    struct sockaddr_in peer_addr;   //用来 save client addr
    socklen_t peerlen;
    vector<int>clients;
    int count = 0;
    int cli_sock =0;

    int epfd = 0;  //epoll 的文件描述符
    int ret_events;  //epoll_wait()的返回值

    struct epoll_event ev_remov, ev, events[MAX_EVENTS];  //events 用来存放从内核读取的的事件

    // 注意区别 ev, 和events[max_events], 一个代表注册的， 另一个代表的是注册的情况

    //设置第一个 要监听的事件和方式，  设置为EPOLLET 边缘触发， 并且监听的时间为可读事件 epollin
    ev.events = EPOLLET | EPOLLIN;

    efpd = epoll_create(MAX_EVENTS); //创建一个epoll 句柄

    if(efpd<0){
        hand_erro("epoll_create failure");
    }
    // 内核为每一个epoll 对象(也就是epdf) 保持一个红黑树和链表
    // int opt EPOLL_CTL_DEL
    // int opt EPOLL_CTL_ADD  // 将一个文件描述符上面的监听事件event 添加到内核注册表当中，  
    // int opt EPOLL_CTL_MOD 对内核中注册表正在监听的文件描述符listenFD 进行一定的修改
    int ret = epoll_ctl(efpd, EPOLL_CTL_ADD, listenfd, &ev);

    if(ret < 0)
        hand_error("epoll_ctl");


    while(1){
        // events 代表返回结果 ，是就绪的读写事件集合， ret_events 返回值代表了其状态 
        ret_events =  epoll_wait(efpd, events, MAX_EVENTS, -1);
        // 错误
        if(ret_events==-1){
            printf("ret_events", ret_events);
            hand_error("epoll_wait");
        }
        //没有准备就绪的可读可写事件， events 返回结果当中为0，
        if(ret_events==0){
            printf("ret_events",0);
            continue;
        }
     // ret_events >0  代表 eventS 有准备就绪的事件， 对准备就绪的事件进行遍历， (遍历的都是准备好了的文件描述符， 不用对这个注册表当中的文件描述符进行扫描)

     // struct epoll_event events[最大值的基本情况]或者  * events
     
     // 一个events 当中很多个struct epoll_event， 每个结构体包括一个文
         for(int nums=0;  nums<ret_events;nums++){
             printf("nums",nums);
             printf("events[num].data.fd =d", events[i].data.fd);
             
             
             //  遍历返回集合，找到之前标记的套接字结合
             
             
             if(events[num].data.fd = listenfd){
                 printf("listen sucess and listenfd： ", listenfd);
                 
                 // 处理新的客户端，建立一个新的连接, 这是代码中的基本编程习惯， 处理一个新的链接

                //  返回值是一个新的套接字描述符，它代表的是和客户端的新的连接，
                //  可以把它理解成是一个客户端的socket,这个socket包含的是客户端的ip和port信息 。

                 cli_sock = accept(listenfd, (struct sockaddr*)&peer_addr,&peerlen)
                 if(cli_sock<0)
                    hand_error("accept");
                printf("count=", count++);
                printf("ip=%s,port = %d\n", inet_ntoa(peer_addr.sin_addr),peer_addr.sin_port);
                clients.push_back(cli_sock);
                setnoblock(cli_sock);   //设置为非阻塞模式
                ev.data.fd = cli_sock;// 将新连接也加入EPOLL的监听队列

         }
         // 如果不是之前的文件描述符， 套接字， 但是监听的时间相同的话， 也可以进行处理
         else if(events[nums].events & EPOLLIN){
             cli_sock = events[nums].data.fd;
             if(cli_sock<0)
                hand_error("cli_sock");
            char recvbuf[1024];
             memset(recvbuf, 0 , sizeof(recvbuf));
                int num = read( cli_sock, recvbuf, sizeof(recvbuf));
                if(num == -1)
                    hand_error("read have some problem:");
                if( num == 0 )  //stand of client have exit
                {
                    cout<<"client have exit"<<endl;
                    close(cli_sock);
                    ev_remov = events[num];
                    epoll_ctl(epfd, EPOLL_CTL_DEL, cli_sock, &ev_remov);
                    clients.erase(remove(clients.begin(), clients.end(), cli_sock),clients.end());
                }
                fputs(recvbuf,stdout);
                write(cli_sock, recvbuf, strlen(recvbuf));

         }
         
         }

    }
    return 0;
}

// 有关这个代码情况的话也可以查看其它的基本内容
// 基本内容[http://www.voidcn.com/article/p-uvhjqbmi-uo.html]