#include "coroutine.h"

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

schedule* schedule_create()
{
    schedule *s = (schedule *)malloc(sizeof(schedule));
    s->running_coroutine = -1;
    s->max_index = 0;
    s->coroutines = (coroutine **)malloc(sizeof(coroutine *)*MAX_UTHREAD_SIZE);
    memset(s->coroutines, 0, sizeof(coroutine *)*MAX_UTHREAD_SIZE);
    s->max_index = 0;
    return s;
}

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

enum Coroutine_State coroutine_status(schedule *s, int id)
{
    if(id < 0 || id >= s->max_index)
    {
        return DEAD;
    }
    coroutine* c = s->coroutines[id];
    if(c == NULL)
    {
        return DEAD;
    }
    return c->state;
}

int schedule_finished(schedule *s)
{
    int i = 0;
    if(s->running_coroutine != -1)
    {
        return 0;
    }
    for(i=0; i < s->max_index; i++)
    {
        coroutine* c = s->coroutines[i];
        if(c != NULL && c->state != DEAD)
        {
            return 0;
        }
    }
    return 1;
}

int coroutine_running(schedule * s, int id)
{
    enum Coroutine_State state = coroutine_status(s, id);
    if(state == DEAD)
    {
        return 0;
    }
    coroutine* c = s->coroutines[id];
    c->state = RUNNING;
    s->running_coroutine = id;
    swapcontext(&(s->main), &(c->ctx));
}

int delete_coroutine(schedule *s, int id)
{
    if(id < 0 || id >= s->max_index)
    {
        return 0;
    }
    coroutine* c = s->coroutines[id];
    if(c != NULL)
    {
        s->coroutines[id] = NULL;
        free(c);
    }
    return 1;
}

void schedule_close(schedule *s)
{
    int i = 0;
    for( i = 0; i < s->max_index; i ++)
    {
        delete_coroutine(s, i);
    }
    free(s->coroutines);
    free(s);
}