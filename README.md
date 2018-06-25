# 协程的简单实现
## 目的
学习协程原理，并记录了学习过程，以供有相同需求的同学参考
## 以博客的形式描述了大概思路
1. [准备知识](doc/ucontext_coroutine1.md)
2. [实现简单协程库](doc/ucontext_coroutine2.md)
3. [使用协程库并实现简单非阻塞socket服务器](doc/ucontext_coroutine3.md)
## 文件说明
- coroutine.h  协程头文件
- coroutine.c  协程实现 
- test_simple.c  简单测试
- test_socket.c  非阻塞socket测试
- client.py  单线程socket，用户输入测试
- client_thread.py  多线程socket测试
