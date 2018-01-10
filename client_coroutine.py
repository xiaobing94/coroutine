#coding: utf8
import socket
import sys
import threading
from time import ctime,sleep

threads = []

def connect_send():
    count = 0
    t = threading.current_thread()
    tid = t.ident
    res = "count:"+str(count)+"__tid:"+str(tid)
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect(('127.0.0.1', 8888))
    sock.send(res)
    szBuf = sock.recv(1024)
    print szBuf
    while(szBuf != 'exit'):
        count += 1
        res = "count:"+str(count)+"__tid:"+str(tid)
        sock.send(res)
        szBuf = sock.recv(1024)
        print("recv:" + szBuf)
        sleep(0.1)
        if(count > 10):
            break
    sock.close()

for i in xrange(100):
    t = threading.Thread(target=connect_send, args=())
    threads.append(t)

if __name__ == '__main__':
    for t in threads:
        t.start()