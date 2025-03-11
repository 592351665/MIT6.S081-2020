## trace

1.按照hints中的前两条，添加函数、存根等；

2.系统调用全流程：
1）trace.c中调用trace(int)，用户态系统调用头文件user.h中加入了，调用usys.pl中跳板函数。

2）生成usys.S文件（汇编），系统调用的**参数**传入了a0寄存器中，系统调用编号传入了a7寄存器中，调用ecall，转入**内核态**

3）所有系统调用都会跳到syscall.c中，根据传入的系统调用编号num查找对应的调用函数执行，执行结果存放在寄存器a0当中；

先来到sysproc.c中的sys_trace()，实现系统调用的具体功能。trace的功能是：从a0中取出系统调用的参数（即mask），放入进程的PCB中，用以让当前进程获取到mask;

内核与用户进程的页表不同、寄存器也不互通，所以参数无法直接通过 C 语言参数的形式传过来，而是需要使用 argaddr、argint、argstr 等系列函数，从进程的 trapframe 中读取用户进程寄存器a0中的参数。

argint()函数，接收trace的参数，也就是32、2147483647等，存放入进程的trace_mask中。

struct proc *p = myproc()   该指令不是创建新进程，而是 p是指向进程控制块的指针，对a0和a7的操作是在同一个PCB中。

4）进行掩码判断，是否是要追踪的系统调用，((1 << num) & p->trace_mask)，num是调用函数的SYS号，p->trace_mask是内核进程中接收到的；

实例分析：trace 2147483647 grep hello README

其中2147483647二进制为 低31位全为1，进行&（与）操作后不改变，意味着跟踪全部的系统调用；首先是trace，完成参数接收，再后面的系统调用也都是跟该数&,到read的时候（1<<5）&2147483647，结果仍是32，跟踪；

5）hints 4：fork()产生子进程后，应该复制父进程的trace_mask，proc.c中修改；

6）proc.c的allocproc中，创建新进程的时候(执行不同指令)，为新添加的 trace_mask 附上默认值 0（否则初始状态下可能会有垃圾数据）。

## Sysinfo

### 1.sysinfo声明和系统调用接口

1.user/sysinfotest.c中调用sysinfo（）

2.用户态系统调用头文件中添加，user/user.h，int sysinfo(struct sysinfo *);参数是结构体，要在上面声名；

3.user.pl中加入跳板函数 entry("sysinfo");

----------从用户态跳转到内核态-----------

4.到达统一的内核态系统调用 kernel/syscall.c，用 extern 全局声明新的内核调用函数，并加入到 syscalls 映射表中；

5.kernel/sysproc.c中实现sysinfo具体代码，打印；

### 2.sys_sysinfo具体实现

功能：获取空闲内存与进程数量，并返回给用户空间；分为三部分；

#### 1.返回信息到用户空间

copyout(a,b,c,d)函数：把内核地址从c开始的d大小的数据拷贝到逻辑地址b在页表a中映射的物理地址中；

有四个参数：

b:  argaddr(0, &addr)   sysinfo系统调用从用户态传入的参数保存在寄存器a0中，是一个指针，所以通过argaddr()获取；RISC-V系统是64位，作为参数的指针是8字节（64位），所以定义的addr是uint64（无符号64位整数）；

通过argaddr()将寄存器a0中的作为参数的指针保存在地址addr中；

a:进程页表，p->pagetable ，该进程是用户进程，addr通过此页表找到对应的物理地址，以便复制数据；

c:  要复制的内容；

d: sizeof()；

#### 2.空闲内存的字节数

**常见的记录空闲页的方法有：空闲表法、空闲链表法、位示图法（位图法）、成组链接法**， xv6 采用的是空闲链表法

#### 3.进程数量

遍历proc数组，不是UNUSED的即使用的进程；
