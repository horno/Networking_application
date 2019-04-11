import argparse
import socket
import threading
import struct

th1 = False
th2 = False
th3 = False


def parse_arguments():
    parser = argparse.ArgumentParser(description='Server side of a client-server communication')
    parser.add_argument('-c', help = 'New route to the configuration file')
    parser.add_argument('-d', help = 'Activates the debugger flag', action = "store_true")
    args = parser.parse_args()
    return args

class client:
    estat = ""
    aleatori = ""
    ip = ""

    def __init__(self, nom, MAC):
        self.nom = nom
        self.mac = MAC

def extract_cfg_data():
    server_cfg = {'Nom': '', 'MAC': '', 'UDP-port': '', 'TCP-port': ''}
    if args.c:
        cfg_file = args.c
    else:
        cfg_file = "server.cfg"
    f = open(cfg_file, 'r')
    line = f.readline()
    while line:
        words = line.split()
        if words[0] == 'Nom':
            server_cfg['Nom'] = words[1]
        elif words[0] == 'MAC':
            server_cfg['MAC'] = words[1]
        elif words[0] == 'UDP-port':
            server_cfg['UDP-port'] = words[1]
        elif words[0] == 'TCP-port':
            server_cfg['TCP-port'] = words[1]

        line = f.readline()

    f.close()
    return server_cfg

def extract_equips_dat():
    f = open('equips.dat', 'r')
    i = 0
    equips_dat = []
    line = f.readline()
    while line:
        words = line.split()
        equips_dat.append({'Nom':words[0], 'MAC':words[1]})
        line = f.readline()
        i = i +1
    f.close()
    return equips_dat

def debugger(debug_text):
    if args.d:
        print("Debugger -> "+debug_text)

def attend(data, addr, th):
    # print(data)


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
    args = parse_arguments()
    server_cfg = extract_cfg_data()
    equips_dat = extract_equips_dat()

    UDP_IP = '127.0.0.1'
    UDP_PORT = int(server_cfg['UDP-port'])
    debugger("Socket UDP obert")
    
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((UDP_IP, UDP_PORT))
    
    debugger("Inici de bucle de servei infinit")
    while True:
        data, addr = sock.recvfrom(1024) #buffer lengths
        if th1 == False:
            th1 = True
            thread_1 = threading.Thread(target = attend, args=(data, addr, '1'))
            thread_1.start()
        elif th2 == False:
            th2  = True
            thread_2 = threading.Thread(target = attend, args = (data, addr, '2'))
            thread_2.start()
        elif th3 == False:
            th3 = True
            thread_3 = threading.Thread(target = attend, args = (data, addr, '3'))
            thread_3.start()

    sock.close()