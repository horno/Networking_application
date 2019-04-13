import argparse
import socket
import threading
import struct
import time



if __name__ == '__main__':
    threads = {'a':'1', 'b':'2', 'c':3}
    thread_list = []

    thread_list.append({'a':'3', 'b':'4', 'c':5})
    thread_list.append(threads)

    for i in thread_list:
        print(i)  
