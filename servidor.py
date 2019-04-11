import argparse
import socket
import threading
import struct
from ctypes import * 

#encoding: utf-8

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

class PDU(Structure):
    _fields_ = [
        ('tipus', c_ubyte),
        ('nom', c_char*7),
        ('MAC', c_char*13),
        ('aleatori', c_char*7),
        ('dades', c_char*50)
    ]


def attend(data, addr, sock):
    p = PDU.from_buffer_copy(data)
    

    p = PDU(t = 1)
    p = PDU(d = '')

    sock.sendto(p,addr)


if __name__ == '__main__':
    args = parse_arguments()
    server_cfg = extract_cfg_data()
    equips_dat = extract_equips_dat()

    UDP_IP = '127.0.0.1'
    UDP_PORT = int(server_cfg['UDP-port'])
    debugger("Socket UDP obert")
    
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(("", UDP_PORT))
    
    debugger("Inici de bucle de servei infinit")
    # try:
    #     while True:
    data, addr = sock.recvfrom(78) #buffer lengths
    attend(data,addr, sock)
            
    # finally:
    #     sock.close()
