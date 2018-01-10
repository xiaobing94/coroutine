## 自己实现协程（一）

### 引言
在使用socket编程时，我们会用到accept、connect、recv、send等函数，这些函数在没有数据到达时，会阻塞等待IO数据的到达。
这不利于我们处理多个连接并快速响应。一种方案是，服务端每accept一个连接，就创建一个新的线程用来处理这个连接。这会导致线程过多，
而且线程之前切换开销很大。这就可以使用到协程了。当然不止socket这种可以使用协程，IO密集型都可以使用协程，无论是网络IO还是其他IO。
### 协程
协程可以理解为用户态的轻量级的非抢占式的线程。

### 特点
用户态:协程是在用户态实现调度。
轻量级:协程不用内核调度，内核态与用户态之间切换。
非抢占:协程是由用户自己实现调度，并且同一时间只能有一个协程在执行，协程自己主动交出CPU的。

### 优缺点
优点：
1. 协程切换的时候开销小，用户态且轻量
2. 非抢占式，不用加很多锁，减小复杂度，不用很复杂的处理线程同步问题。
缺点：
    协程不能利用多核，只能使用单核，因为同时只有一个协程在运行。

### 适用场景
IO密集型。
在IO密集的情况下，协程的开销比线程小，能快速实现调度。
协程不适用于计算密集型，协程不能很好的利用多核cpu。

### ucontext组件
linux下在头文件< ucontext.h >提供了getcontext(),setcontext(),makecontext(),swapcontext()四个函数和mcontext_t和ucontext_t结构体。

其中mcontext_t与机器相关。ucontext_t结构体如下（一般在/usr/include下）：
```
typedef struct ucontext
{
    unsigned long int uc_flags;
    struct ucontext *uc_link;
    stack_t uc_stack;
    mcontext_t uc_mcontext;
    __sigset_t uc_sigmask;
    struct _libc_fpstate __fpregs_mem;
} ucontext_t;
```
其中uc_link指向下文，及当前上下文（可以理解为执行状态）执行完了，恢复运行的上下文;
uc_sigmask为该上下文中的阻塞信号集合;
uc_stack为该上下文中使用的栈;
uc_mcontext保存的上下文的特定机器表示，包括调用线程的特定寄存器等。
通过栈式计算机原理，我们可以知道，保存栈区和所有通用寄存器的值就可以保存程序运行的状态。这里uc_mcontext就是用来保存所有的寄存器的值的。而我们把栈设置到uc_stack所指向的内存，
uc_stack就保存了栈的状态。

### ucontext的4个函数介绍

```
int getcontext(ucontext_t *ucp);
```
获取当前上下文，初始化ucp结构体，将当前的上下文保存到ucp中。如果执行成功，返回0。执行失败返回-1。
```
int setcontext(const ucontext_t *ucp);
```
设置当前上下文，设置当前的上下文为ucp, 恢复ucp的执行状态。如果ucp执行完了，会恢复到uc_link所指向的上下文，若uc_link为NULL，则线程退出。如果执行成功，不返回。执行失败返回-1。
```
void makecontext(ucontext_t *ucp, void (*func)(), int argc, ...);
```
创建上下文，修改通过getcontext取得的上下文ucp, 然后给该上下文指定一个栈空间ucp->stack，设置后继的上下文ucp->uc_link。
```
int swapcontext(ucontext_t *oucp, ucontext_t *ucp);
```
切换上下文，保存当前上下文到oucp结构体中，然后激活upc上下文。 如果执行成功，不返回。执行失败返回-1。

### ucontext简单试用
下面一段简单代码体验一下获取和恢复上下文:
```
#include <stdio.h>
#include <ucontext.h>
#include <unistd.h>

int main(int argc, const char *argv[]){
	ucontext_t context;
	
	getcontext(&context);
	puts("Hello world");
	sleep(1);
	setcontext(&context);
	return 0;
}
```
保存并编译，执行结果:
```
Hello world  
Hello world  
Hello world  
Hello world  
Hello world  
Hello world  
Hello world  
^C  
```
如果不主动ctrl+c结束程序，会不断执行下去。第8行哪里getcontext保存了上下文，相当于按了一个暂停并保存起来放到了context变量里面，程序继续执行，到第11行的时候，恢复了原来的上下文，相当于把context变量里面保存的东西恢复了，恢复到第8行的状态。所以程序会不断执行下去。

### 函数之前切换
下面一段简单的代码体验一下不同函数间切换和恢复上下文：
```
#include <stdio.h>
#include <ucontext.h>
#include <unistd.h>


void fun1()
{
    printf("func1\n");
}
void fun2(ucontext_t *ctx)
{
    ucontext_t t_ctx;
    printf("func2\n");
    swapcontext(&t_ctx, ctx);
}

int main(int argc, char *argv[]) {
    ucontext_t context, context2, main_ctx;
    char stack[1024];                                                                                                                         
    getcontext(&context);
    getcontext(&context2);
    context.uc_stack.ss_sp = stack;
    context.uc_stack.ss_size = 1024;
    context.uc_link = &main_ctx;
    context2.uc_stack.ss_sp = stack;
    context2.uc_stack.ss_size = 1024;
    makecontext(&context,(void (*)(void))fun1, 0);
    swapcontext(&main_ctx, &context);
    puts("main");
    makecontext(&context2,(void (*)(void))fun2,1,&main_ctx);
    swapcontext(&main_ctx, &context2);
    puts("main over");
    return 0;
}
```
执行结果:
```
func1
main
func2
main over
```
这里第28行，先保存上下文到main_ctx，切换到context上下文执行，就调用了fun1函数，fun1函数执行完后恢复uc_link指向的上下文main_ctx。然后接着执行输出了main。第31行保存上下文到main_ctx恢复context2，执行fun2函数，并传一个函数main_ctx,执行到14行后保存上下文到gCtx,然后恢复ctx(ctx是通过指针传过来的mainc_ctx)；即恢复到了32行的状态最后main函数执行完成。
### 总结
linux系统，为我们提供了获取当前执行状态的函数、恢复当前上下文的函数和构造当前上下文的函数，所有栈的区域我们都可以自己分配一段空间来做协程的栈，每个协程都有自己独立的空间，我们就可以自己在用户态实现上线文切换，实现一个“用户态的线程”，即协程。