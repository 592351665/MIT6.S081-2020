## Uthread

目的：实现用户态下线程切换（协程）

流程：

1）旧进程的用户线程陷入内核态，将用户程序的寄存器保存在trapframe中，进入内核态；

2）调用thread_yield()让出cpu，调用thread_schedule()进行线程切换；

3）实际切换就一行：thread_switch((uint64)&t->context,(uint64)&next_thread->context)

将旧线程的 被调用者保存寄存器（callee-saved registers） 保存到t->context中，新线程的上下文next_thread->context加载到寄存器中，返回寄存器ra；调用者保存寄存器（caller-saved registers）在调用switch函数前会隐式地自动保存到线程栈中；

4）usertrapret()，返回用户空间；

##  Using threads

1.pthread_mutex_t lock[NBUCKET] = {PTHREAD_MUTEX_INITIALIZER};

为每个BUCKET定义一个互斥锁，并通过宏进行静态初始化，互斥锁初始化为未锁定状态；

2.互斥锁动态初始化，结束前要销毁锁

for (int i = 0; i < NBUCKET; ++i)    pthread_mutex_init(&lock[i], NULL);

for (int i = 0; i < NBUCKET; ++i)    pthread_mutex_destroy(&lock[i]);

3.降低锁的粒度，不是每个哈希表一个锁，而是每个bucket一个锁；

4.get不修改，put会调用insert修改，在insert前后加上锁；

##  Barrier
