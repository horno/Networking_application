import argparse
import socket
import threading
import time 

th1 = False
th2 = False
th3 = False

def test_th(th):
    for i in range(8):
        print("Thread:" + str(i))
        time.sleep(1)
    if th == "1":
         print("Thread 1 finnished")
         
    elif th == "2":
         print("Thread 2 finnished")
    elif th == "3":
         print("Thread 3 finnished")

if __name__ == '__main__':
    while True:
        #data, addr = sock.recvfrom(1024) #buffer lengths
        if th1 == False:
            th1 = True
            thread_1 = threading.Thread(target = test_th, args=("1"))
            thread_1.start()
        elif th2 == False:
            th2  = True
            thread_2 = threading.Thread(target = test_th, args=("2"))
            thread_2.start()
        elif th3 == False:
            th3 = True
            thread_3 = threading.Thread(target = test_th, args=("3"))
            thread_3.start()
