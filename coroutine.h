#ifndef COROUTINE_H
#define COROUTINE_H

#include <ucontext.h>

#define STACK_SZIE (1024*128)
#define MAX_UTHREAD_SIZE   1024
#define NULL 0

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

typedef struct schedule_
{
    ucontext_t main;
    int running_coroutine;
    coroutine **coroutines;
    int max_index;
}schedule;

// 用于执行目标函数
static void main_func(schedule *s);
// 创建调度器
schedule* schedule_create();
// 删除协程
int delete_coroutine(schedule *s, int id);
// 关闭调度器
void schedule_close(schedule *s);
// 查看状态
enum Coroutine_State coroutine_status(schedule *s, int id);
// 创建协程
int coroutine_create(schedule *s, coroutine_func func, void *args);
// 切换协程
void coroutine_yield(schedule *s);
// 恢复协程
void coroutine_resume(schedule * s, int id);
// 检查是否完全执行完成
int schedule_finished(schedule *schedule);
// 运行协程
int coroutine_running(schedule * s, int id);
#endif 