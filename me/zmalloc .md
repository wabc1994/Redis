# zmalloc 分配操作

对不同平台下面的malloc和free 当中的代码进行一个封装，



先看下面定义的各种API

```c
void *zmalloc(size_t size);  //调用zmalloc申请size个大小的空间
void *zcalloc(size_t size);   //调用系统函数calloc函数申请空间
void *zrealloc(void *ptr, size_t size);//原内存重新变化为空间size 的大小

```

