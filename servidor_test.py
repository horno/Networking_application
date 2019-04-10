import threading
import multiprocessing
import time


def threading_test_1():
    for i in range(5):
        print(str(i) + "Thread 1!")
        time.sleep(1)
    return "1- Is this gonna arrive?"

def threading_test_2():
    for i in range(10):
        print(str(i) + "Thread 2!")
        time.sleep(1)
    return "2- Is this gonna arrive?"

if __name__ == '__main__':
    thread_1 = multiprocessing.Process(target=threading_test_1)
    thread_2 = multiprocessing.Process(target=threading_test_2)
    thread_1.start()    
    thread_2.start()
    thread_1.join()
    # thread_2.join()
    print("PROGRAM END")