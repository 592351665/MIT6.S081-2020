## Backtrace


![在这里插入图片描述](https://i-blog.csdnimg.cn/direct/b948b04ce793444380549e3a0626c9b0.png#pic_center)

1.函数调用栈从高地址向低地址增长，每一块是一个函数的栈帧，当前的栈帧在最下面；fp栈帧指针（又叫帧指针）指向栈帧的基部（高地址），fp保存在s0/fp寄存器；sp栈指针指向栈帧顶部（低地址）；

2.fp向下偏移8B是执行完当前函数后的返回地址，fp向下偏移16B是上一个栈帧的栈帧指针；

偏移实现：frame[-1]等价于*(frame - 1)，向下偏移8字节；

3.调用栈的大小为1页（4KB），使用PGROUNDDOWN(fp)和PGROUNDUP(fp)确保地址正确；

## Alarm


开始alarm前熟悉系统调用trap的流程：

![在这里插入图片描述](https://i-blog.csdnimg.cn/direct/1c4b4911f23646ab90cc59c929786f8f.png#pic_center)

**1.应用程序shell调用write() ->usys.S ->ecall指令**

1）如果trap是设备中断，并且SIE位被清空，不执行下列操作

2）清除SIE位以禁用中断

3）User mode->supervisor mode

4）let sepc = pc （保存发生trap时的程序计数器pc）

5）let pc = stvec ，jump to pc（跳转到地址trampoline.S执行uservec函数）

**2.uservec函数：（trampoline.S）**

1）将用户程序的32个通用寄存器保存在trapframe结构体（proc.h）对应位置

2）将trapframe中的内核页表、内核栈、cpu id、usertrap地址保存到寄存器中

3）跳转到usertrap()函数

**3.usertrap函数：**

1）改变stvec为kernelvec，代表处理内核空间中的trap；还可能会修改sepc值

2）根据ecall指令设置的r_scause的值判断trap是系统调用（会进入syscall函数直接对应程序）、中断（设备、page fault等）、异常（直接kill进程）

**4.usertrapret函数：**

1）将内核页表、内核栈、cpu id、usertrap地址保存到trapframe中，以便下一次从用户态转到内核态使用

2）恢复stevc和sepc的值

3）调用userret

**5.userret指令（对应ecall指令）**

1）恢复trapframe中32个通用寄存器的值

2）把用户空间的page table、用户空间的stack装载到寄存器里

3）调用sret返回到用户空间，从sepc处的指令继续执行用户程序，重新打开中断



**Alarm:**

1.alarmtest.c中调用sigalarm(2, periodic)：要求内核每隔两个滴答强制调用periodic()；sigalarm（）系统调用通过跳板函数进入内核，
``sigalarm:`
 `li a7, SYS_sigalarm`
 `ecall`
 ret

2.ecall：进入内核；将当前指令的pc(程序计数器)保存在sepc中；let pc=stvec；jump to pc；清除**SIE**以禁用中断；设置`scause`以反映产生陷阱的原因；将当前模式（用户或管理）保存在状态的**SPP**位中。

3.trampoline.S/uservec：保存32个通用寄存器；把内核的page table、内核的stack、当前执行该进程的CPU号装载到寄存器里；跳转到trap.c/usertrap（）;

4.usertrap()：将sepc保存在p->trapframe->epc；进trap时中断关闭（ecall的作用），不会进行进程调度，在sepc被保存后，会重新打开中断。if_else判断是 系统调用/中断/异常中的哪一个，本题为系统调用，p->trapframe->epc += 4，从ecall->ret；intr_on()，中断打开；syscall()，系统调用；

5.syscall.c/sysproc.c/sys_sigalarm()，将sigalarm(2, periodic)传入的两个参数接收，proc.h/strcut proc中加入三个字段，ticks：几个滴答后执行handler，handler：处理函数，ticks_cnt：计数几次滴答；

<u>tips：</u>内核态无法调用用户函数handler，需要返回到用户态再执行；

6.if(which_dev == 2)：每一次滴答，会强制执行一次时钟中断；
在此处加判断，if（p->ticks!=0&&p->ticks_cnt==p->ticks），执行要handler；修改返回用户态的执行地址，p->trapframe->epc = p->handler，修改成handler的地址；

7.usertrapret（）：填入了trapframe的内容，这样下一次从用户空间转换到内核空间时可以用到这些数据；w_sepc(p->trapframe->epc)，将修改为handler后的地址保存到sepc中；

8.trampoline.S/userret：恢复通用寄存器；把用户空间的page table、用户空间的stack装载到寄存器里。

9.sret：将pc设置为sepc中的值，即handler（）地址；重新打开中断。
