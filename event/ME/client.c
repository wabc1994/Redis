
int main(){
    signal(SIGPIPE,SIG_IGN);
    int sock;
    sock = socket( AF_INET, SOCK_STREAM,0 );   //create a socket stream
    if( sock< 0 )
        hand_error( "socket_create");

    struct sockaddr_in my_addr;
 //memset my_addr;
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(18000);   //here is host sequeue
//  my_addr.sin_addr.s_addr = htonl( INADDR_ANY );
    my_addr.sin_addr.s_addr = inet_addr("127.0.0.1");


    int conn = connect(sock, (struct sockaddr *)&my_addr, sizeof(my_addr)) ;
    if(conn != 0)
        hand_error("connect");

    char recvbuf[1024] = {0};
    char sendbuf[1024] = {0};
    fd_set rset;
    FD_ZERO(&rset);     

    int nready = 0;
    int maxfd;
    int stdinof = fileno(stdin);
    if( stdinof > sock)
        maxfd = stdinof;
    else
        maxfd = sock;
    while(1)
    {
        //select返回后把原来待检测的但是仍没就绪的描述字清0了。所以每次调用select前都要重新设置一下待检测的描述字
        FD_SET(sock, &rset);  
        FD_SET(stdinof, &rset);
        nready = select(maxfd+1, &rset, NULL, NULL, NULL); 
        cout<<"nready = "<<nready<<"  "<<"maxfd = "<<maxfd<<endl;
        if(nready == -1 )
            break;
        else if( nready == 0)
            continue;
        else
        {
            if( FD_ISSET(sock, &rset) )  //检测sock是否已经在集合rset里面。
            {
                int ret = read( sock, recvbuf, sizeof(recvbuf));  //读数据
                if( ret == -1)
                    hand_error("read");
                else if( ret == 0)
                {
                    cout<<"sever have close"<<endl;
                    close(sock);
                    break;
                }
                else
                {
                    fputs(recvbuf,stdout);    //输出数据
                    memset(recvbuf, 0, strlen(recvbuf));
                }   
            }

            if( FD_ISSET(stdinof, &rset))   //检测stdin的文件描述符是否在集合里面
            {   
                if(fgets(sendbuf, sizeof(sendbuf), stdin) != NULL)
                {
                    int num = write(sock, sendbuf, strlen(sendbuf));   //写数据
                    cout<<"sent num = "<<num<<endl;
                    memset(sendbuf, 0, sizeof(sendbuf));
                }
            }
        }
    }
    return 0;

}