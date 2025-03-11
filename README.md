1.在调用sbrk()增大堆空间时，不给其实际分配物理内存，当访问到这段地址发生缺页中断时，再进行分配物理内存、进行映射；

sys_sbrk()系统调用：通过内核增大用户空间堆的大小；p->sz 指向堆顶，xv6中，堆从低地址向高地址增长；

2.usertrap中加入判断，解决来自用户空间的page fault，如果缺页报错为懒分配地址，为其分配实际物理内存并页表映射；

1）is_lazy_addr()判断是否为懒分配地址：

va<p->trapframe->sp&&va>=p->trapframe->sp - PGSIZE ：如果地址在page guard保护页中，不进行懒分配；p->trapframe->sp为栈顶；

va>MAXVA：超过用户空间边界；

pte && (*pte & PTE_V)：页表项存在且映射有物理页；

va>=p->sz：p->sz 指向堆顶，超过堆空间；

2）lazy_alloc函数：

实际分配物理页，并在页表中添加页（用户空间）到页帧/页框（物理内存）的映射；

3.上述实现解决来自用户空间的page fault，但是当一些系统调用（如read、write）访问用户态虚拟地址，当访问到lazy alloction的地址时，会引起报错，这是来自内核态的缺页报错；

write()系统调用 参数传递过程：

使用copyin()将用户空间传来的数据复制到内核空间，调用walkaddr()找到虚拟地址对应的pte，找到返回地址，未找到返回0；

现在要在内核态中处理缺页报错，

walkaddr()中，在pte = walk(pagetable, va, 0)找va地址对应的pte之前，进行条件判断if(is_lazy_addr(va))，其中也排除已经分配的lazy页面，只对处于合适位置，并且还未映射的页面进行lazy_alloc(va)；

这样就解决了内核访问用户虚拟地址懒分配页时的page fault；
