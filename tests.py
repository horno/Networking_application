import argparse
import socket
import threading
import struct
import time

th1 = False
th2 = False
th3 = False

iterator = 0

def attend(th):
    for i in range(5):
        print(th + ' ' + str(i))
        time.sleep(1)

    if th == '1':
        global th1 
        th1 = False
    elif th == '2':
        global th2
        th2 = False
    else:
        global th3
        th3 = False

if __name__ == '__main__':
    threads = {'a':'1', 'b':'2', 'c':3}
    while True:
        if th1 == False:
            th1 = True
            thread_1 = threading.Thread(target = attend, args=(threads['a']))
            thread_1.start()
        elif th2 == False:
            th2  = True
            # thread_2 = threading.Thread(target = , args = ('2'))
            thread_2 = threading.Thread(target = attend, args=("2"))
            thread_2.start()
        elif th3 == False:
            th3 = True
            thread_3 = threading.Thread(target = attend, args = ('3'))
            thread_3.start()
