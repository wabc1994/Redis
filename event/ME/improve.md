
# 通用封装部分情况
## 基本结构体
1. aeFileEvent 文件事件
2. 
3. aeEventLoop

我们主要关注下redis 当中是如何对原生的epoll 进行封装的

关于aeEventLoop结构体主要是定义在ac.h文件当中, 这个是比较重要的

首先定义了这一结构体的概念
```c
typedef struct aeEventLoop {
	//目前创建的最高的文件描述符
   
   
    int maxfd;   /* highest file descriptor currently registered */
   
   
    int setsize; /* max number of file descriptors tracked */
    //下一个时间事件id
    long long timeEventNextId;
    time_t lastTime;     /* Used to detect system clock skew */
    //3种事件类型
    aeFileEvent *events; /* Registered events */
    aeFiredEvent *fired; /* Fired events */
    aeTimeEvent *timeEventHead;
   
    //事件停止标志符
    int stop;
   
   
    //这里存放的是event API的数据，包括epoll，select等事件
    void *apidata; /* This is used for polling API specific ，可以指向select() 或者epoll() */ 后续指向后面的
   
   
   
   
   
    aeBeforeSleepProc *beforesleep;
} aeEventLoop;
/*
```

分别在ae_select.c、ae_epoll.c 和文件当中均定义了一个 aeApiState

```c
typedef struct aeApiState {
    int epfd;// 代表一个eventpoll 实例， 是有epoll_create 创建的
    struct epoll_event *events;
} aeApiState;
```


aeEventLoop d当中的apidata就是指向一个aeApiState结构体 


后面的基础使用环节部分
```c
// 分配一个数据结构体，
aeApiState *state = zmalloc(sizeof(aeApiState));
// epfd 就是指向epoll_create创建的对象
 state->epfd = epoll_create(1024); /* 1024 is just a hint for the kernel */
 ```


# ae.c
主要是使用来操作一个EventLoop对象的

主要的辅助代码情况
1. 创建一个EventLoop 结构体基本情况 *aeCreateEventLoop
2. 一个EventLoop当中有几个事件(监听了多少个event)aeGetSetSize，最大的文件描述符


