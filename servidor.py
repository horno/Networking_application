# coding=utf-8
import argparse
import socket
import threading
import struct
import random
import time
import sys
import os
from ctypes import *

global equips_dat
quit = False
j = 2
k = 3
r = 3

def parse_arguments():
    parser = argparse.ArgumentParser(description='Server side of a client-server communication')
    parser.add_argument('-c', help = 'New route to the configuration file')
    parser.add_argument('-d', help = 'Activates the debugger flag', action = "store_true")
    args = parser.parse_args()
    return args

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
        equips_dat.append({'nom':words[0], 'MAC':words[1], 'estat': "DISCONNECTED"})
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

def authorised(p):
    for i in equips_dat:
        if i['nom'] == p.nom:
            return True, i    

def nack_pdu_send(addr, sock, message):
    nack_pdu = PDU(tipus=0x02, nom='', MAC='', aleatori='', dades= message)
    sock.sendto(nack_pdu, addr)

def correct_data(addr, sock, p, equip_dat):
    rand = random.randint(1000001,1999999)
    rand = str(rand)[1:]
    ack_pdu = PDU(tipus=0x01, nom=server_cfg['Nom'], MAC=server_cfg['MAC'], aleatori=rand, dades='9000')
    sock.sendto(ack_pdu, addr)
    actualize_equips(p.nom, 'aleatori', rand)


def check_state(addr, sock, p, equip_dat):
    if equip_dat['estat'] == 'DISCONNECTED':
        if p.MAC == equip_dat['MAC'] and p.aleatori == '000000':
            if 'ip' in equip_dat:
                if addr[0] == equip_dat['ip']:
                    correct_data(addr, sock, p, equip_dat)
                    equip_dat['estat'] = 'REGISTERED'
                    actualize_equips(p.nom, 'estat', 'REGISTERED')
                    debugger(p.nom + ' passa a estat REGISTERED')
                else:
                    message = 'Ip del primer missatge no concorda'
                    debugger('Petició de registre de ' + equip_dat['nom'] + ' denegada per:' + message)
                    nack_pdu_send(addr, sock, message)
            else:
                equip_dat['ip'] = addr[0]
                actualize_equips(p.nom, 'ip', addr[0])
                actualize_equips(p.nom, 'estat', 'REGISTERED')  
                debugger(p.nom + ' passa a estat REGISTERED')
                correct_data(addr, sock, p ,equip_dat)
                actualize_equips(p.nom, 'TTL', 0)
                threading.Thread(target = ttl_register, args=(equip_dat['nom'],)).start()
        else:
            message = 'Mac no concorda o nombre aleatori no a zeros'
            debugger('Petició de registre de ' + p.nom + ' denegada per:' + message)
            nack_pdu_send(addr, sock, message)
    else:
        if p.MAC == equip_dat['MAC']  and addr[0] == equip_dat['ip']:
            ack_pdu = PDU(tipus=0x01, nom=server_cfg['Nom'], MAC=server_cfg['MAC'], aleatori=equip_dat['aleatori'], dades='9000')
            sock.sendto(ack_pdu, addr)
            debugger(p.nom + ' segueeix en estat ' + equip_dat['estat'])

        else:
            message = 'IP, aleatori o MAC no concorden'
            debugger('Petició de registre de ' + p.nom + ' denegada per:' + message)            
            nack_pdu_send(addr, sock, message)
    return equip_dat

def actualize_equips(name, key, value):
    for i in equips_dat:
        if i['nom'] == name:
            i[key] = value            

def checkout_equips(name, key):
    for i in equips_dat:
        if i['nom'] == name:
            return i[key]

def register_petition(addr, sock, p):
    auth, equip_dat = authorised(p)
    if auth:
        check_state(addr, sock, p, equip_dat)
    else:
        debugger('Petició de registre de ' + p.nom + ' rebutjada, equip no autoritat')
        rej_pdu = PDU(tipus=0x03, nom='', MAC='', aleatori='', dades='Equip no autoritzat')
        sock.sendto(rej_pdu, addr)
    

def attend(data, addr, sock):
    p = PDU.from_buffer_copy(data)
    if p.tipus == 0:
        debugger('Rebuda petició de registre de ' + p.nom)
        register_petition(addr, sock, p)
    elif p.tipus == 0x10:
        debugger('Rebut alive inf de ' + p.nom)
        alive_inf(addr, sock, p)

def ttl_register(name):
    debugger('Comença temporitzacio de REGISTER de '+name)
    actualize_equips(name, 'TTL', 2)    
    for i in range(j):
        time.sleep(r)
    ttl = checkout_equips(name, 'TTL') - j
    actualize_equips(name, 'TTL', ttl)
    if ttl == 0:
        debugger(name + ' passa a estat DISCONNECTED')
        actualize_equips(name, 'estat', 'DISCONNECTED')

def ttl_alive(name):
    debugger('Comença temporitzacio ALIVE de '+name)
    actualize_equips(name, 'TTL', checkout_equips(name, 'TTL')+k)
    for i in range(k):
        time.sleep(r)
    ttl = checkout_equips(name, 'TTL') - k
    actualize_equips(name, 'TTL', ttl)
    if ttl == 0:
        debugger(name + ' passa a estat DISCONNECTED')
        actualize_equips(name, 'estat', 'DISCONNECTED')

def alive_inf(addr, sock, p):
    auth,equip_dat = authorised(p)
    if (auth and (equip_dat['estat'] == 'ALIVE' or  equip_dat['estat'] == 'REGISTERED')
    and equip_dat['MAC'] == p.MAC and addr[0] == equip_dat['ip'] and 
    p.aleatori == equip_dat['aleatori'] ):
        alive_ack_pdu = PDU(tipus=0x011, nom=server_cfg['Nom'], MAC=server_cfg['MAC'], aleatori=equip_dat['aleatori'], dades='')
        sock.sendto(alive_ack_pdu, addr)
        if equip_dat['estat'] == 'REGISTERED':
            actualize_equips(p.nom, 'estat', 'ALIVE')
            debugger(p.nom + ' passa a estat ALIVE')
        threading.Thread(target = ttl_alive, args=(equip_dat['nom'],)).start()
    elif  not auth  or (equip_dat['estat'] != 'ALIVE' and  equip_dat['estat'] != 'REGISTERED'):
        message = 'Equip no autoritzat o no registrat'
        debugger('Alive_inf de '+p.nom+' rebutjat:' + message)
        alive_rej_pdu = PDU(tipus=0x013, nom='', MAC='', aleatori='', dades=message)
        sock.sendto(alive_rej_pdu, addr)
    elif addr[0] != equip_dat['ip'] or p.aleatori != equip_dat['aleatori']:
        message = 'Nombre aleatori o ip no concorden'
        debugger('Alive_inf de '+p.nom+' denegat:' + message)
        alive_nack_pdu = PDU(tipus=0x012, nom='', MAC='', aleatori='', dades=message)
        sock.sendto(alive_nack_pdu, addr)
    else:
        message = 'Paquet amb format incorrecte'
        debugger(message)
        err_pdu = PDU(tipus=0x09, nom='', MAC='', aleatori='', dades=message)
        sock.sendto(err_pdu, addr)

def console():
    quit = False
    debugger('Terminal activa')
    while not quit:
        line = sys.stdin.readline()
        word = line.split()
        if(word != [] and word[0] == 'quit'):
            quit = True
        elif word != [] and word[0] == 'list':
            cp=('-NOM-','-MAC-','-ESTAT-','-IP-','-ALEATORI-')
            print ('{0:>0} {1:>10} {2:>16} {3:>11} {4:<10}'.format(*cp))
            for i in equips_dat:
                if i['estat'] == 'REGISTERED' or i['estat'] == 'ALIVE':
                    cp = (i['nom'],i['MAC'],i['estat'],i['ip'],i['aleatori'])
                    print ('{0:>0} {1:>14} {2:>14} {3:>11} {4:<10}'.format(*cp))
                elif i['estat'] == 'DISCONNECTED':
                    cp = (i['nom'],i['MAC'],i['estat'])
                    print ('{0:>0} {1:>14} {2:>14}'.format(*cp))
        else:
            debugger('Comanda incorrecta')

    os._exit(1)

if __name__ == '__main__':
    args = parse_arguments()
    server_cfg = extract_cfg_data()
    global equips_dat
    equips_dat = extract_equips_dat()
    threading.Thread(target=console).start()
    UDP_IP = '127.0.0.1'
    UDP_PORT = int(server_cfg['UDP-port'])
    debugger("Socket UDP obert")

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(("", UDP_PORT))

    debugger("Inici de bucle de servei infinit")
    try:
         while True:
            data, addr = sock.recvfrom(78)
            attend(data,addr, sock)
    finally:
        sock.close()
