#include "coroutine.h"

#include <stdio.h>

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