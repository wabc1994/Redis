# 结构体

epoll 使用到的数据结构：

1. epoll_event  (这个结构体后面会给出来里面具体的内容是怎样的情况)
2. eventpoll （epoll instance 其实就是epolll_create 创建的epoll 实例）
3. epitem (每一个用户fd对应内核eventpoll红黑树当中的一个节点)
4. epoll_entry



## 1. epoll_event
```c
typedef union epoll_data { 
                void *ptr; 
                int fd;  // 这个套接字描述符是客户端和服务器端连接后，一般来讲， 这个文件描述符都是套接字绑定后生成的
                __uint32_t u32; 
                __uint64_t u64; 
        } epoll_data_t; 


// epoll_data 主要有四个属性

struct epoll_event { 
                __uint32_t events;      /* epoll events  要监听的事件类型， 注意在这里是复数的情况 */ 
                epoll_data_t data;      /* user data variable */ 
        }; 
```
// epoll_event 主要有两个属性， 一个是要监听的发生事件， 另一个是要对谁进行监听， 要为谁进行监听， 监听成功后为谁而返回



结构体epoll_event   被用于 注册所感兴趣的事件和回传所发生待处理的事件 **就是后面调用epooll_wait后要进行处理的处理， 主要是处理里面的events 事件体**)

events 取值主要有如下几个： 
- EPOLLIN : 表示对应的文件描述符可以读
- EPOLLOUT：表示对应的文件描述符可以写
- EPOLLPRI：表示对应的文件描述符有紧急的数据可读（这里应该表示有带外数据到来）； 
- EPOLLERR：表示对应的文件描述符发生错误； 
- EPOLLHUP：表示对应的文件描述符被挂断； 
- EPOLLET： 将EPOLL设为边缘触发(Edge Triggered)模式，这是相对于水平触发(Level Triggered)来说的。 
- EPOLLONESHOT：只监听一次事件，当监听完这次事件之后，如果还需要继续监听这个socket的话，需要再次把这个socket加入到EPOLL队列里
  

## 2. epitem

当向系统中添加一个fd时，就创建一个epitem结构体(红黑树当中的每个节点就是一个epitem 对象)，这是内核管理epoll的基本数据结构：

（删除一个fd, 就从黑红树当中删除一个节点， 充分利用了红黑树快速的搜索，删除和添加时间复杂度方面的基础优势）

```c
struct epitem {
    
    struct rb_node rbn;          //代表红黑树当中的节点
   
    struct list_head rdllink;    //rdllink记录eventpoll当中就绪链表头rdllist 当中的头结点， 方便将后续准备好的插入
    

    struct epitem *next;         //用于主结构体中的链表
    struct epoll_filefd ffd;     //这个结构体对应的被监听的文件描述符信息
    int nwait;                   //poll操作中事件的个数
    struct list_head pwqlist;    //双向链表，保存着被监视文件的等待队列，功能类似于select/poll中的poll_table
    struct eventpoll *ep;        //该项属于哪个主结构体（多个epitm从属于一个eventpoll）
    struct list_head fllink;     //双向链表，用来链接被监视的文件描述符对应的struct file。因为file里有f_ep_link,用来保存所有监视这个文件的epoll节点
    struct epoll_event event;    //注册的感兴趣的事件,也就是用户空间的epoll_event,  又要使用到上面的关系是
}
```

   **回调函数ep_poll_callback会干什么了**
   
>红黑树上的收到event的epitem（代表每个fd）插入ep->rdllist(就绪队列当中，双向链表)中，这样，当epoll_wait返回时，rdllist里就都是就绪的fd了 


## 3. eventpoll

一个黑红树根节点rb_root rbr 和一个list_head双向链表rdllist(read)

struct eventpoll在epoll_create时创建。 (也就是epdf） 
```
struct eventpoll {
    spin_lock_t lock;            //对本数据结构的访问，本质上面是自旋锁， 实现了多线程环境下面的安全，  
    struct mutex mtx;            //防止使用时被删除
    wait_queue_head_t wq;         //sys_epoll_wait()使用的等待队列
    wait_queue_head_t poll_wait;  //file->poll()使用的等待队列
    struct list_head rdllist;     //事件满足条件的链表 /*双链表中则存放着将要通过epoll_wait返回给用户的满足条件的事件*/   双向链表
    struct rb_root rbr;          //用于管理所有fd的红黑树（树根）/*红黑树的根节点，这颗树中存储着所有添加到epoll中的需要监控的事件*/    红黑树的root
    struct epitem *ovflist;      //将事件到达的fd进行链接起来发送至用户空间

    struct file *file;  //eventpoll 在内核当中注册的文件系统    //epoll文件系统中构建的虚拟文件
}
```

上述结构体 eventpoll和epitem之间的关系如下面所示

![eventpoll和epitem的基本关系](https://github.com/wabc1994/InterviewRecord/blob/master/io%E5%A4%8D%E7%94%A8/picture/eventpollandepitem.jpg)




## 等待队列

[参考链接](http://guojing.me/linux-kernel-architecture/posts/wait-queue/)

主要作用是是实现Linux内核当中的异步通知调用时间，等待队列（wait queue）用于使进程等待某一特定的事件发生而无需频繁的轮询，进程在等待期间睡眠，在某件事发生时由内核自动唤醒。

wait_queue_head_t  wq poll_wait

等待队列 waitqueue
 * 我们简单解释一下等待队列:
 * 队列头(wait_queue_head_t)往往是资源生产者,
 * 队列成员(wait_queue_t)往往是资源消费者,
 * 当头的资源ready后, 会逐个执行每个成员指定的回调函数,  
 * 来通知它们资源已经ready了, 等待队列大致就这个意思.



等待队列的定义是放在/include/linux/wait.h， 他通常有一个等待队列和等待task 的头， task_list 上面存放的是在这里面进行等待的进程，包括等待队列头wait_queue_head_t 和等待队列项wait_queue_t. 等待队列头和等待队列项都包含一个list_head类型的域 ，作为"连接件"， 它通过一个双链表和等待task的头， 和等待的进程列表链接起来，


### 等待队列头

1. 结构体
```c
struct _wait_queue_head{
    spinlock_t lock;   //在对task_list操作的过程当中， 使用该锁实现对task_list的互斥访问
    struct list_head task_list;  //双向循环队列 ， 存放等待的进程
}
// 定义为等待队列的头
typedef struct _wait_queue_head wait_queue_head_t 
```

2. 使用
```c
wait_queue_head_t my_queue;  
init_waitqueue_head(&my_queue);  //会将自旋锁初始化为未锁，等待队列初始化为空的双向循环链表。
//宏名用于定义并初始化，相当于"快捷方式"
DECLARE_WAIT_QUEUE_HEAD (my_queue);  
```


### 等待队列项

/*__wait_queue，该结构是对一个等待任务的抽象。每个等待任务都会抽象成一个wait_queue，并且挂载到wait_queue_head上。该结构定义如下：*/


```c
struct _wait_queue{
    unsigned int flags;   // 该进程是互斥还是非互斥状态
    #define WQ_FLAG_EXCLUSIVE   0x01
    // 声明一个无类型的指针， 
    void *private;   // void型指针变量功能强大，你可以赋给它你需要的结构体指针。一般赋值为**task_struct**类型的指针，也就是说指向一个进程；
    wait_queue_func_t func;    func：函数指针，指向该等待队列项的唤醒函数； 
    // 等待唤醒进程
    struct list_head task_list;   // 主要是为了记录使用， 使用一个变量使用，可以直接访问一个进程在等待队列头当中的位置信息
      /* 挂入wait_queue_head的挂载点 * 也就是当前wait_queue_t放在wait_queue_head当前的位置/
}
typedef struct _wait_queue  wait_queue_t
```

### 将进程加入等待队列

为了将使当前进程加入到一个等待队列中， 使用add_wait_queue()函数， 这个函数获得自旋锁， 将工作委托给_add_wait_queue

```c
static inline void __add_wait_queue(
    wait_queue_head_t *head;
    wait_queue_t *new{
        // 将链表插入到等待队列头的尾部
        list_add(& new->task_list, & head->task_list)
    }
)
```
![](https://github.com/wabc1994/InterviewRecord/blob/master/Linux/picture/wait_queu.jpeg)

### wait_queue_head_t和wait_queue_t的关系
1. 等待队列头的task_list域链接的成员就是等待队列类型的(wait_queue_t)。
2. wait_queue_t当中的task_list当中存放的就是进程
3. 所以基本的使用状态就是: 先将进程current加入等待队列wait_queue_t ，然后再讲wait_queue_t添加到wait_queue_head_t 当中的task_list后面即可
4. Linux当前的执行进程都是使用current来代表


将进程加入到等待队当中后， 进程还没有真正进入睡眠状态， 只有当中调用schedule(); //进入真正的休眠状态（CPU资源让给别的任务）



### 等待时间
```c
#define wait_event(wq, condition)                  
do {          
       // 如果满足的话就不进行等待                         
        if (condition)                         
         break;     
         // b'n                    
        __wait_event(wq, condition);                   
} while (0)
wait_event(queue,condition);   //等待以queue为等待队列头等待队列被唤醒，condition必须满足，否则阻塞  
wait_event_interruptible(queue,condition);  //可被信号打断  
wait_event_timeout(queue,condition,timeout);    //阻塞等待的超时时间，时间到了，不论condition是否满足，都要返回  
wait_event_interruptible_timeout(queue,condition,timeout) 
```

### 进程睡眠
比如下面将一个进程加入等待队列并进行睡眠
```c
// 指明等待队里
void __sched sleep_on(wait_queue_head_t *q)
{  
  sleep_on_common(q, TASK_UNINTERRUPTIBLE, MAX_SCHEDULE_TIMEOUT);
}

static long __sched
// 指明等待队列， 进程的状态，  state 设置为TASK_INT 或者TASK_UNINTER 两种状态
sleep_on_common(wait_queue_head_t *q, int state, long timeout)
{
        unsigned long flags;
        // 定一个等待队列项
        wait_queue_t wait;
        // 初始化 ，将当前进程加入
        init_waitqueue_entry(&wait, current);
        __set_current_state(state);
        // 获得自旋锁
        spin_lock_irqsave(&q->lock, flags);
        // 将等待队列项加入等待队列头
        __add_wait_queue(q, &wait);
        spin_unlock(&q->lock);
        timeout = schedule_timeout(timeout);
        //
        spin_lock_irq(&q->lock);
        // 从等待队列当中移除
        __remove_wait_queue(q, &wait);
        spin_unlock_irqrestore(&q->lock, flags); 
        return timeout;
}
```

上述代码解释
>函数的作用是定义一个等待队列(wait)，并将当前进程添加到等待队列中(wait)，然后将当前进程的状态置为 TASK_UNINTERRUPTIBLE，并将等待队列(wait)添加到等待队列头(q)中。之后就被挂起直到资源可以获取，才被从等待队列头(q) 中唤醒，从等待队列头中移出。在被挂起等待资源期间，该进程不能被信号唤醒。


### 初始化一个等待队列项
```c
static inline void init_waitqueue_entry(wait_queue_t *q, struct task_struct *p)
{

        q->flags = 0;   //非互斥状态
        q->private = p; // current代表当前进程
        q->func = default_wake_function;  //
}
```


### 将等待队列当中的进程唤醒
进程唤醒是由内核来完成的， 满足下面两个状态会触发进程唤醒
1. 数据准备好读写
2. 接受到信号

wake_up函数完成




## struct file
每一个文件描述fd在Linux内核当都有一个struct file结构体与之对应关系，struct file结构体定义在include/linux/fs.h中定义。文件结构体代表一个打开的文件，系统中的每个打开的文件在内核空间都有一个关联的 struct file。它由内核在打开文件时创建，并传递给在文件上进行操作的任何函数。所以在epoll_clt 打开一个文件

可以看下struct file 在epoll() 当中是如何使用的,
1. epoll_create() 对应的sys_epoll_create()

```c
asmlinkage long sys_epoll_create(int size)
{
    int error, fd;
    struct inode *inode;  
    struct file *file;  // 直接就定义了一个file 结构体

    DNPRINTK(3, (KERN_INFO "[%p] eventpoll: sys_epoll_create(%d)\n",
             current, size));

    /* Sanity check on the size parameter */
    error = -EINVAL;
    if (size <= 0)
        goto eexit_1;

    /*
     * Creates all the items needed to setup an eventpoll file. That is,
     * a file structure, and inode and a free file descriptor.
     */
    error = ep_getfd(&fd, &inode, &file);
    if (error)
        goto eexit_1;

    /* Setup the file internal data structure ( "struct eventpoll" ) */
    error = ep_file_init(file);
    if (error)
        goto eexit_2;


    DNPRINTK(3, (KERN_INFO "[%p] eventpoll: sys_epoll_create(%d) = %d\n",
             current, size, fd));

    return fd;

eexit_2:
    sys_close(fd);
eexit_1:
    DNPRINTK(3, (KERN_INFO "[%p] eventpoll: sys_epoll_create(%d) = %d\n",
             current, size, error));
    return error;
}

```

create第一次调用时候创建了新的inode、file、fd，Linux遵循一切皆文件的原则，一切都是文件操作，返回的也是一个fd。这样做还有一个好处，指针的指向并不好判断资源的有效性，但是fd就可以通过current->files->fd_array[]找到。

## 自旋锁和互斥锁

>简要结论就是epoll是通过锁来保证线程安全的, epoll中粒度最小的自旋锁ep->lock(spinlock)用来保护就绪的队列, 互斥锁ep->mtx用来保护epoll的重要数据结构红黑树


关于这里面为何选择一个

自旋锁主要使用在内核当这中， 互斥锁主要是使用在用户当中(比如用户程序调用epoll_ctl操作添加一个fd,这时候就涉及到往红黑树当中添加一个epitem 节点, )，自旋锁主要是使用在内核态，当一个文件描述符 
    - 如果线程的上下文切换时间小于持有锁的时间， 那么就使用互
    - 如果线程持有锁的时间是比较短的， 直接是自旋锁就行了， 这样线程就做一个空循环，  不用进入睡眠状态


# 函数流程API



在下面函数流程当中就需要使用到fd,和event 结构体了，epoll句柄

首先注意如何在Linux系统中如何进行标志一个epoll 实体， 那就是使用 epfd 了， 在后面两个程序函数epoll_ctl和epoll_wait 当中第一个函数参数就是该epdf, 

## 创建
1. epfd= epoll_create(10), 返回一个epoll句柄 epdf 也就是event_poll  对象

该函数的作用流程图如下面所示
![epoll_create基础流程图](https://github.com/wabc1994/InterviewRecord/blob/master/io%E5%A4%8D%E7%94%A8/picture/epoll_create.jpg)


## 添加
2. epoll_ctl(**epfd**， EPOLL_CTL_ADD(添加操作，删除),**socketfd**, &event） 在socketfd文件描述符上面进行注册, 以及要监听什么事件(event)， 注测要监听的事件和文件描述符(注册到一个黑红树当中去)；poll的事件注册函数，它不同于select()是在监听事件时
   告诉内核要监听什么类型的事件，而是在这里先注册要监听的事件类型。(事件类型注册机制)

> 执行epoll_ctl时，如果增加socket套接字1，则检查在红黑树中是否存在，存在立即返回，不存在则添加到树干上，然后向内核注册回调函数，用于当中断事件来临时向准备就绪链表中插入数据

 > 其中第二个参数 表示动作
>1. EPOLL_CTL_ADD：注册新的fd到epfd中； 
>2. EPOLL_CTL_MOD：修改已经注册的fd的监听事件； 
>3. EPOLL_CTL_DEL：从epfd中删除一个fd；

## 返回
3. epoll_wait(**epfd**, struct epoll_event ***wait_event**, num,-1 ) 在就绪链表list 当中观察，返回准备就绪的文件描述符即可



在上述函数3当中， epoll_event * wait_event  放在该位置， 当epoll_wait（）返回时， 就会把套接字的信息(监听成功的套接字信息放在这个结构体当中), **注意只返回成功相关的情况**


并且在epoll_ctl 这个地方中event 当中是地址传递， 代表对这个东西进行一定的修改，

## 注意事项情况
**参数异同**
注意在上面这个例子当中，  struct epoll_event  event 和 struct epoll_evetn * wait_event 的区别， 一个是单独就是一个结构体， 一个是结构体指针(可以代表很过个这样的数据， 一排地址的首地址， 代表很多结构体当中第一个)


struct epoll_event eventList[MAX_EVENTS] 声明为一个数组的形式也是可以的，其中 存储的是从内核返回的就绪的状态


**为何这样， 解释**
一开始的时候我们只是创建了一个epoll句柄， 并且定义了一个epoll_event(包括文件描述符和监听的时间类型)， 但是在后面我们还可以通过各种途径进行一个添加

文件描述符(服务器还在会在不同的端口进行一个监听， 如果连接成功的话，就会不断加入进行了)

这就是为什么我们要增加有第二个步骤， epoll_ctl(int opt监听， 修改，增加这三种情况)， 在后期还要处理新的连接情况

# 建立连接后的判断
```C


// 判断主要是通过相应的位置操作
if(wait_event.data.fd==socked && (wait_event.events &EPOLLIN==EPOLLIN))
//  如果返回结果当中有套接字sockfd ， 并且相应的时间等于EPOOL_IN , 然后就可以执行相应的代码
{
    执行相应的代码体
    
}
```


# 与套接字的结合
- 1.  绑定端口号和ip 地址
- 2. sockert :套接字函数返回一个地址(ip: port)和一个文件描述符，
- 3. Listen : 在相应的文件描述符上面进行相应的监听， 查看是否有链接


# epoll的高效之处
## 特点
epoll的高效就在于，当我们调用epoll_ctl往里塞入百万个句柄时，epoll_wait仍然可以飞快的返回，并有效的将发生事件的句柄给我们用户。

## 原因
1. epoll 文件系统file  黑红树
2. 句柄就绪链表list
3. 函数回调机制call_back  这里面又涉及到Linux当中的函数处理方面的
4. 少量的cache高速缓存, 快速访问文件，减少大量内核态到用户态的

![双向链表和黑红树之间的基本关系](https://github.com/wabc1994/InterviewRecord/blob/master/io%E5%A4%8D%E7%94%A8/picture/relationshi.png)


## 就绪链表是如何工作的呢？
**双向链表当中存储的东西**
双向链表当中的节点其实就是epitem结构当中的rdllink;




**双向链表是如何工作的**
这是由于我们在调用epoll_create时，内核除了帮我们在epoll文件系统里建了个file结点，在内核cache里建了个红黑树用于存储以后epoll_ctl传来的socket外，还会给内核中断处理程序注册一个回调函数，**告诉内核， 如果这个句柄的中断到了， 就把这个文件描述符放在准备就绪双向链表当中**所以，当一个socket上有数据到了，内核在把网卡上的数据copy到内核中后就来把socket插入到准备就绪链表里了。


所以上述东西的关键  如何进行监听准备就绪的套接字， 一个很重要的概念就是进行回调机制

>所以ep_poll_callback函数主要的功能是将被监视文件的等待事件就绪时，将文件对应的epitem实例添加到就绪队列中，当用户调用epoll_wait()时，内核会将就绪队列中的事件报告给用户。



还会再建立一个list链表，用于存储准备就绪的事件，

当epoll_wait调用时，仅仅观察这个list链表里有没有数据即可。有数据就返回，没有数据就sleep，等到timeout时间到后即使链表没数据也返回。所以，epoll_wait非常高效。



# 函数原理代码实现解析


关于epoll源码实现部分细节, 具体可以参考这个 博客的基本内容
 [参考链接](https://blog.csdn.net/lixungogogo/article/details/52226479)




epoll的实现主要依赖于一个迷你文件系统：eventpollfs。此文件系统通过eventpoll_init初始化。在初始化的过程中，eventpollfs create两个slub专用的高速缓存分别是：
1. epitem (每一个用户态的文件描述符fd对内核当中的epitem，所以内核当中监控的是对应的epitem)
2. eppoll_entry。





epoll_create() 创建一个eventpoll 实例epfd时同时调用eventpoll_init


```c
static int __init eventpoll_init(void)
{
	... ...
 
	/* Allocates slab cache used to allocate "struct epitem" items */

    // 创建专用的cache高速缓存, 这些高速缓存是直接建立在连续的物理内存之上的，然后利用slab分类器进行管理即可

      kmen_cache_create 其实就是slab分配器 对于这块内容可以查看后面的linux 内存管理 伙伴分配器 和slab 分类器的东西， 
	epi_cache = kmem_cache_create("eventpoll_epi", sizeof(struct epitem),
			0, SLAB_HWCACHE_ALIGN|EPI_SLAB_DEBUG|SLAB_PANIC,
			NULL, NULL);
 
	/* Allocates slab cache used to allocate "struct eppoll_entry" */
	pwq_cache = kmem_cache_create("eventpoll_pwq",
			sizeof(struct eppoll_entry), 0,
			EPI_SLAB_DEBUG|SLAB_PANIC, NULL, NULL);
 
 ... ...


```

## slab分类器
**补充下linux当中的高速缓存slab分类器**

一个高速缓存当中有很多个slab,每个slab又有很多细分为一类对象(这里面的对象是内存)， 一个高速缓存当中的所有是slab是通过链表串连在一起的， 




1. 创建一个高速缓存
关于这部分内容是来自201页《Linux内核实现与设计》


每个高速缓存只负责一种对象类型，比如进程描述符对象 task_struct 和 struct inode  索引节点对象。每个高速缓存当中的slab 的数目是不一样的， 这与已经使用的页数目、对象长度和被管理对象的数目是一样的。

>另外， 系统当中所有的缓存都保存在一个双链表中。这使得内核有机会遍历所有的缓存， 这是很有必要的。比如在内存发生不足时， 内核可能需要缩减分配给缓存的内存数量。

slab层高速缓存(专用高速缓存)与通用高速换是不一样的概念



**kmem_cache**
slab高速缓存使用一个结构体kmem_cache 来标记, 有两个比较重要的成员
    - 指向一个数组的指针， 其保存了各个CPU最后释放的对象
    - kmem_list3 结构， 这个结构用来管理高速缓存当中的slab链表
        - 第一个链表是完全使用完毕的链表 slabs_full
        - 第二个链表是部分使用部分空闲的链表 slabs_partial
        - 第三个链表是空闲未使用过的链表 slab_empty

简单过程图可以查看下，所有的kmem_cache 对象被组织成为以cache_chain为对头的一个双向循环链表
![kmem_cache](https://github.com/wabc1994/InterviewRecord/blob/master/Linux/picture/kmem_cache.png)


一个高速缓存是通过以下函数创建的

```
struct kmem_cache*  kmem_cache_create(const char *name, size_t size, size_t align, unsigned long){
       const char * name, size_t size, size_t align, unsigned long slags, void(*)


}

```
具体参数解释
   1. 第一个参数是字符串， 存放高速缓存的名字；
   2. 第二个参数是高速缓存内中每个对象的大小；
   3. slab内第一个对象的偏移，确保在叶内进行对其。通常情况下设置为0表示标准对齐，
   4. 设置高速缓存的行为， 一般是标志位运算
   5. 第五个是高速缓存的构造函数， 现在一般不是使用了，在新的页

返回值 
   1. 如果是创建成功的话， 就会返回一个指向高速缓存的指针， 否则直接返回NULL；

2. 销毁一个高速缓存

>int kmem_cache_destory(struct kmem_cache * cachep)
销毁给定的高速缓存， 要满足下面两个条件才能销毁一个高速缓存
    - 高速缓存当中所有的slab都必须为空。 不管哪个slab当中， 只要还有一个对象被分配出去，并且还在使用当中的话，怎么可能撤销拿？
    - 调用这个函数之后， 不再访问这个高速缓存

3. 从高速缓存中获取对象

kmem_cache_alloc 用于从特定的缓存中获取对象(一小块内存)，类似于malloc 函数， 
>void * kmem_cache_alloc(struct kmem_cache* cachep， gfp_t flags)

该函数从指定的高速缓存当中返回一个指向对象的指针。 如果slab高速缓存当中没有空闲的对象了，那么slab 层必须通过kmem_getpages() 获取新的页，flags的值传递给_get_free_pages(). slab层的高速缓存不够用的时候，主要是向伙伴系统请求分配页， 

4. 释放一个对象

这里的释放一个对象是指将内存归还给slab层，与销毁高速缓存的概念是不一样的
>void kmem_cache_free(struct kmem_cache *cachep, void * objp)
调用上述语句我们就能将cachep当中的objp 标记为NULL


**slab描述符**

一个高速缓存当中有很多个slab, slab分配器使用struct slab数据结构(slab) 来描述其状态以及进行管理。 
slab 可以分类为
```c
struct slab{
    struct list_head list; // 满， 部分或者空闲的链表
    unsigned long colouroff; // slab着色的偏移量
    void   * s_mem; // 在slab当中的第一个对象
    unsigned int     inuse; // 在slab当中 已经使用的对象数
    kmem_bufctl_t free  ;     // 第一个空闲对象， 如果存在的话

}
```


## slab 着色
**slab着色原理介绍**
slab虽然名为高速缓存，但是其实只是主存RAM当中的一块区域，与硬件高速缓存是两个概念，硬件高速缓存是按照行来划分的。

**slab缓存与硬件高速缓存之**
 
slab代表RAM主存， 硬件高速缓存 主要为了解决CPU和RAM之间速度不匹配的问题

 类似一个矩阵的关系，  不同的slab内存块(对象)映射到硬件高速缓存当中， CPU直接访问cache ， 不用访问RAM, 加快访问速度

 硬件高速缓存当中存储的东西就是对RAM当中内存的一种简单映射(包括slab当中的分配的对象内存)

 **slab对象映射到硬件高速缓存有什么特点呢？**
 
 不同slab块当中相同大小的slab对象倾向于映射到硬件高速缓存当中相同的行， 


**硬件高速缓存的基础结构体**
                  
                 slab1    slab2    slab3
cache line 0 

cache line 1 

cache line 3

着色说白就是添加一定的偏移量, 这个偏移量其实是一个slab当中剩余的空间， 使对象大小一致映射到不同的cache line当中

