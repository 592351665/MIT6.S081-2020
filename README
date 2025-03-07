1.思路：fork出的子进程不会完全复制一份父进程的用户空间，而是共享父进程内存；两个页表项都加上了新标志位PTE_C，清除了PTE_W；当有进程写时，会发生page fault，进入usertrap进行处理，会复制共享页帧，给引发报错的进程添加新映射，同时有写权限；

2.uvmcopy()函数：原作用是复制父进程的用户空间给子进程；修改后：

1）清除父进程页表项 PTE_W，加上PTE_C（在页表项的保留位中加入新的标志位）；

2）将子进程va映射到父进程的页帧中，形成共享，带有PTE_C，没有PTE_W；

3）pa多了一个进程引用，refcnt_inc(pa)增加引用计数；

3.当要写入时，触发tore page fault（r_scause() == 15）；

1）函数uncopied_cow()，判断是否为合理的cow页；

2）函数cowalloc()，清除页表项的PTE_C，加上PTE_W；

先复制原页帧，再取消对原页帧的映射，加上对新页帧的映射；

3.引用计数reference count

![img](https://ttzytt.com/img/xv6/note/kernel_pagetable.png)

struct {

 struct spinlock lock;

 int count[PHYSTOP / PGSIZE];    //数组大小为PHYSTOP / PGSIZE

}refc;

1）封装refc_cnt 计算物理地址pa对应页帧的引用数；封装refc_inc 增加引用计数；

2）kinit()：初始化引用计数锁，调用freerange（）,会调用kfree()，使得count[]减1，所以要初始化为1；

3）kfree（）:引用计数-1，进行判断，==0则执行具体释放；

4）kalloc（）：当新分配物理页帧后，只有当前进程对其进行引用，refc.count[(uint64)r / PGSIZE] = 1，也需要上锁；

4.copyout()中修改

1）copyin()不需要修改是因为 页表项有PTE_R标志位，可以从共享页帧中读取内容；

2）在pa0 = walkaddr(pagetable, va0) 前加上

if(uncopied_cow(pagetable,va0)==0){

   pa0 = (uint64)cowalloc(pagetable,va0); }
