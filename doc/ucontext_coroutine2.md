## 自己实现协程（二）
### 基本思路
  上一篇已经提到如何使用ucontext组件来实现上下文切换。而我们要实现协程就需要实现一个调度器用来调度管理协程、协程恢复、协程切换等功能。用调度器来管理当前执行的协程，及并在调度器，并通过调度器管理所有协程的列表。

### 协程
协程需要执行目标函数的功能，并具备保存自身栈区、上下文和执行状态的功能。则协程结构体如下：
```
enum Coroutine_State{DEAD, READY, RUNNING, SUSPEND};

struct schedule_;

typedef void (*coroutine_func)(struct schedule_* s, void *args);

typedef struct coroutine_
{
    coroutine_func func;
    void *args;
    ucontext_t ctx;
    enum Coroutine_State state;
    char stack[STACK_SZIE];
}coroutine;
```
其中coroutine_func func为被执行的函数， void*args为函数的参数struct schedule_* s为调度器，ucontext_t ctx为当前协程上下文，enum Coroutine_State state为协程状态， char stack[STACK_SZIE]为函数的栈区。大小为STACK_SZIE宏。

### 调度器
调度器需要具有管理所有协程的功能，并需要具有保存主逻辑上下文的功能，并且要能保存当前执行的协程。则结构体如下：
```
typedef struct schedule_
{
    ucontext_t main;
    int running_coroutine;
    coroutine **coroutines;
    int max_index;
}schedule;

```
其中main是重来保存主逻辑上下文的，running_coroutine指向当前运行的协程id，coroutines是协程指针列表，max_index，为当前使用的最大id。

### 协程函数执行

为了便于管理协程的状态，通过一个函数来执行协程。函数如下：
```
void main_func(schedule *s)
{
    int id = s->running_coroutine;
    if(id != -1)
    {
        coroutine* c = s->coroutines[id];
        c->func(s, c->args);
        c->state = DEAD;
        s->running_coroutine = -1;
    }

}
```
当func目标函数执行完成了协程也就结束了到DEAD状态，当前也没有在执行的协程了，running_coroutine设置为-1。
### 协程创建
```
int coroutine_create(schedule *s, coroutine_func func, void *args)
{
    coroutine *c = NULL;
    int i = 0;
    for(i=0; i < s->max_index; i++)
    {
        c = s->coroutines[i];
        if(c->state == DEAD)
        {
            break;
        }
    }
    if(s->max_index == i || c == NULL)
    {
        s->coroutines[i] = (coroutine *)malloc(sizeof(coroutine));
    }
    c = s->coroutines[i];
    c->state = READY;
    c->func = func;
    c->args= args;
    s->max_index ++;
    getcontext(&(c->ctx));

    c->ctx.uc_stack.ss_sp = c->stack;
    c->ctx.uc_stack.ss_size = STACK_SZIE;
    c->ctx.uc_stack.ss_flags = 0;
    c->ctx.uc_link = &(s->main);

    makecontext(&(c->ctx),(void (*)(void))main_func, 1, s);
    return i;
}
```
获取并初始化上下文信息，并设置栈区等信息，且设置方法为main_func。
### 协程切换
```
void coroutine_yield(schedule *s)
{
    if(s->running_coroutine != -1)
    {
        coroutine* c = s->coroutines[s->running_coroutine];
        c->state = SUSPEND;
        s->running_coroutine = -1;

        swapcontext(&(c->ctx), &(s->main));
    }
}
```
保存当前运行的协程上下文，并切回主调度逻辑。
### 协程恢复
```
void coroutine_resume(schedule * s, int id)
{
    if(id >= 0 && id < s->max_index)
    {
        coroutine *c = s->coroutines[id];
        if(c != NULL && c->state == SUSPEND)
        {
            c->state = RUNNING;
            s->running_coroutine = id;
            swapcontext(&(s->main), &(c->ctx));
        }
    }
}

```
恢复指定id的协程，保存主调度逻辑，恢复对应id的协程。

### 简单测试
```
#include <stdio.h>
#include "coroutine.h"

void func1(schedule *s, void *args)
{
    puts("11");
    coroutine_yield(s);
    int *a = (int *)args;
    printf("func1:%d\n", a[0]);

}

void func2(schedule *s, void *args)
{
    puts("22");
    coroutine_yield(s);
    puts("22");
    int *a = (int *)args;
    printf("func2:%d\n", a[0]);
}

int main()
{
    int args1[] = {1};
    int args2[] = {2};
    schedule *s = schedule_create();
    int id1 = coroutine_create(s, func1, (void *)args1);
    int id2 = coroutine_create(s, func2, (void *)args2);
    coroutine_running(s, id2);
    coroutine_running(s, id1);
    while(!schedule_finished(s)){
        coroutine_resume(s, id2);
        coroutine_resume(s, id1);
    }
    puts("main over");
    schedule_close(s);
    return 0;
}
```
结果:
```
22
11
22
func2:2
func1:1
main over
```
### 总结
本文主要介绍了C语言实现简单协程库的实现，主要用于学习协程，实际使用情况比较复杂，这里为了学习简化了很多。主要从协程函数执行、协程创建、协程恢复等功能介绍了协程的实现。
项目代码我已经上传到github:https://github.com/xiaobing94/coroutine