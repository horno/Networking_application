# coding=utf-8
# #!/usr/bin/env python
import argparse
import socket
import threading
import struct
import random
import time
import sys
import os
import signal
import select
from ctypes import *

global equips_dat
quit = False
j = 2
k = 3
r = 3
l = 3 #queue length
w = 4

class udp_PDU(Structure):
    _fields_ = [
        ('tipus', c_ubyte),
        ('nom', c_char*7),
        ('MAC', c_char*13),
        ('aleatori', c_char*7),
        ('dades', c_char*50)
    ]
class tcp_PDU(Structure):
    _fields_ = [
        ('tipus', c_ubyte),
        ('nom', c_char*7),
        ('MAC', c_char*13),
        ('aleatori', c_char*7),
        ('dades', c_char*150)
    ]

def parse_arguments():
    parser = argparse.ArgumentParser(description='Server side of a client-server communication')
    parser.add_argument('-c', help = 'New route to the configuration file')
    parser.add_argument('-d', help = 'Activates the debugger flag', action = "store_true")
    parser.add_argument('-u', help = 'New route to the authorized clients file')
    args = parser.parse_args()
    return args

def extract_cfg_data():
    server_cfg = {'Nom': '', 'MAC': '', 'UDP-port': '', 'TCP-port': ''}
    if args.c:
        cfg_file = args.c
    else:
        cfg_file = "server.cfg"
    if not os.path.exists(cfg_file):
        print("File not found")
        exit(-1)

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
    if args.u:
        equips = args.u
    else:
        equips = "equips.dat"
    if not os.path.exists(equips):
        print("File not found")
        exit(-1)
    f = open(equips, 'r')
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

def superdebugger(debug_text): #Treure per entrega
    if args.d:
        print("SUPERDEBUGGER -----------------> "+debug_text)

def authorised(p):
    for i in equips_dat:
        if i['nom'] == p.nom:
            return True, i
    return False, {}

def nack_pdu_send(addr, udp_sock, message):
    nack_pdu = udp_PDU(tipus=0x02, nom='', MAC='', aleatori='', dades= message)
    udp_sock.sendto(nack_pdu, addr)

def correct_data(addr, udp_sock, p, equip_dat):
    rand = random.randint(1000001,1999999)
    rand = str(rand)[1:]
    ack_pdu = udp_PDU(tipus=0x01, nom=server_cfg['Nom'], MAC=server_cfg['MAC'], aleatori=rand, dades=server_cfg['TCP-port'])
    udp_sock.sendto(ack_pdu, addr)
    actualize_equips(p.nom, 'aleatori', rand)

def check_state(addr, udp_sock, p, equip_dat):
    if equip_dat['estat'] == 'DISCONNECTED':
        if p.MAC == equip_dat['MAC'] and p.aleatori == '000000':
            if 'ip' in equip_dat:
                if addr[0] == equip_dat['ip']:
                    correct_data(addr, udp_sock, p, equip_dat)
                    equip_dat['estat'] = 'REGISTERED'
                    actualize_equips(p.nom, 'estat', 'REGISTERED')
                    debugger(p.nom + ' passa a estat REGISTERED')
                    actualize_equips(p.nom, 'TTL', 0)
                    debugger("Creat thread per gestionar alive")
                    threading.Thread(target = ttl_register, args=(equip_dat['nom'],)).start()
                else:
                    message = 'Ip del primer missatge no concorda'
                    debugger('Petició de registre de ' + equip_dat['nom'] + ' denegada per:' + message)
                    nack_pdu_send(addr, udp_sock, message)
            else:
                equip_dat['ip'] = addr[0]
                actualize_equips(p.nom, 'ip', addr[0])
                actualize_equips(p.nom, 'estat', 'REGISTERED')  
                debugger(p.nom + ' passa a estat REGISTERED')
                correct_data(addr, udp_sock, p ,equip_dat)
                actualize_equips(p.nom, 'TTL', 0)
                debugger("Creat thread per gestionar alive")
                threading.Thread(target = ttl_register, args=(equip_dat['nom'],)).start()
        else:
            message = 'Mac no concorda o nombre aleatori no a zeros'
            debugger('Petició de registre de ' + p.nom + ' denegada per:' + message)
            nack_pdu_send(addr, udp_sock, message)
    else:
        if p.MAC == equip_dat['MAC']  and addr[0] == equip_dat['ip']:
            ack_pdu = udp_PDU(tipus=0x01, nom=server_cfg['Nom'], MAC=server_cfg['MAC'], aleatori=equip_dat['aleatori'], dades='9000')
            udp_sock.sendto(ack_pdu, addr)
            debugger(p.nom + ' segueeix en estat ' + equip_dat['estat'])
        else:
            message = 'IP, aleatori o MAC no concorden'
            debugger('Petició de registre de ' + p.nom + ' denegada per:' + message)            
            nack_pdu_send(addr, udp_sock, message)
    return equip_dat

def actualize_equips(name, key, value):
    for i in equips_dat:
        if i['nom'] == name:
            i[key] = value            

def checkout_equip(name, key):
    for i in equips_dat:
        if i['nom'] == name:
            return i[key]

def register_petition(addr, udp_sock, p):
    auth, equip_dat = authorised(p)
    if auth:
        check_state(addr, udp_sock, p, equip_dat)
    else:
        debugger('Petició de registre de ' + p.nom + ' rebutjada, equip no autoritat')
        rej_pdu = udp_PDU(tipus=0x03, nom='', MAC='', aleatori='', dades='Equip no autoritzat')
        udp_sock.sendto(rej_pdu, addr)
    
def attend(data, addr, udp_sock):
    p = udp_PDU.from_buffer_copy(data)
    if p.tipus == 0x00:
        debugger('Rebuda petició de registre de ' + p.nom)
        register_petition(addr, udp_sock, p)
    elif p.tipus == 0x10:
        debugger('Rebut alive inf de ' + p.nom)
        alive_inf(addr, udp_sock, p)

def ttl_register(name):
    debugger('Comença temporitzacio de REGISTER a ALIVE de '+name)
    actualize_equips(name, 'TTL', 2)    
    for i in range(j):
        time.sleep(r)
    ttl = checkout_equip(name, 'TTL') - j
    actualize_equips(name, 'TTL', ttl)
    if ttl == 0:
        debugger(name + ' passa a estat DISCONNECTED')
        actualize_equips(name, 'estat', 'DISCONNECTED')

def ttl_alive(name):
    debugger('Comença temporitzacio ALIVE de '+name)
    actualize_equips(name, 'TTL', checkout_equip(name, 'TTL')+k)
    for i in range(k):
        time.sleep(r)
    ttl = checkout_equip(name, 'TTL') - k
    actualize_equips(name, 'TTL', ttl)
    if ttl == 0:
        debugger(name + ' passa a estat DISCONNECTED')
        actualize_equips(name, 'estat', 'DISCONNECTED')

def alive_inf(addr, udp_sock, p):
    auth,equip_dat = authorised(p)
    if (auth and (equip_dat['estat'] == 'ALIVE' or  equip_dat['estat'] == 'REGISTERED')
    and equip_dat['MAC'] == p.MAC and addr[0] == equip_dat['ip'] and 
    p.aleatori == equip_dat['aleatori'] ):
        alive_ack_pdu = udp_PDU(tipus=0x011, nom=server_cfg['Nom'], MAC=server_cfg['MAC'], aleatori=equip_dat['aleatori'], dades='')
        udp_sock.sendto(alive_ack_pdu, addr)
        if equip_dat['estat'] == 'REGISTERED':
            actualize_equips(p.nom, 'estat', 'ALIVE')
            debugger(p.nom + ' passa a estat ALIVE')

            #debugger("Creat thread per gestionar comunicació TCP")
            #threading.Thread(target = check_TCP_connexions, args = (addr ,p,)).start()


        threading.Thread(target = ttl_alive, args=(equip_dat['nom'],)).start()
    elif  not auth  or (equip_dat['estat'] != 'ALIVE' and  equip_dat['estat'] != 'REGISTERED'):
        message = 'Equip no autoritzat o no registrat'
        debugger('Alive_inf de '+p.nom+' rebutjat:' + message)
        alive_rej_pdu = udp_PDU(tipus=0x013, nom='', MAC='', aleatori='', dades=message)
        udp_sock.sendto(alive_rej_pdu, addr)
    elif addr[0] != equip_dat['ip'] or p.aleatori != equip_dat['aleatori']:
        message = 'Nombre aleatori o ip no concorden'
        debugger('Alive_inf de '+p.nom+' denegat:' + message)
        alive_nack_pdu = udp_PDU(tipus=0x012, nom='', MAC='', aleatori='', dades=message)
        udp_sock.sendto(alive_nack_pdu, addr)
    else:
        message = 'Paquet amb format incorrecte'
        debugger(message)
        err_pdu = udp_PDU(tipus=0x09, nom='', MAC='', aleatori='', dades=message)
        udp_sock.sendto(err_pdu, addr)

def console():
    quit = False
    debugger('Terminal activa')
    while not quit:
        try:
            readable, [], [] = select.select([sys.stdin],[],[],0.0)
        except ValueError:
            exit(-1)
        if readable != []:    
            try:
                line = sys.stdin.readline()
            except ValueError:
                exit(-1)

            word = line.split()
            if(word != [] and word[0] == 'quit'):
                debugger("El servidor es tancarà en uns instants")
                quit = True
            elif word != [] and word[0] == 'list':
                cp=('-NOM-','-MAC-','-ESTAT-','-IP-','-ALEATORI-')
                print ('{0:>0} {1:>10} {2:>16} {3:>11} {4:>13}'.format(*cp))
                for i in equips_dat:
                    if i['estat'] == 'REGISTERED' or i['estat'] == 'ALIVE':
                        cp = (i['nom'],i['MAC'],i['estat'],i['ip'],i['aleatori'])
                        print ('{0:>0} {1:>14} {2:>14} {3:>11} {4:>8}'.format(*cp))
                    elif i['estat'] == 'DISCONNECTED':
                        cp = (i['nom'],i['MAC'],i['estat'])
                        print ('{0:>0} {1:>14} {2:>14}'.format(*cp))
            else:
                print('Comanda incorrecta')

    os._exit(1)

def sigint_handler(signum, frame):
    global quit
    quit = True
    udp_sock.close()
    exit(-1)

def send_file(connex_sock, t, tcp_addr, sending, getting):
    debugger("Rebuda petició d'enviament d'arxiu de configuració")
    if (authorised(t) and checkout_equip(t.nom,'estat') == 'ALIVE' and 
    checkout_equip(t.nom,'MAC') == t.MAC and t.aleatori == checkout_equip(t.nom,'aleatori') and
    tcp_addr[0] == checkout_equip(t.nom, 'ip') and not sending and not getting):
        debugger("Tot correcte per SEND_CONF!")
        ack_tcp_pdu = tcp_PDU(tipus=0x21, nom=server_cfg['Nom'], MAC=server_cfg['MAC'], aleatori=t.aleatori, dades= t.nom+".cfg")
        connex_sock.send(ack_tcp_pdu)
        return True
    elif (not authorised(t) or checkout_equip(t.nom,'MAC') != t.MAC or 
          checkout_equip(t.nom,'estat') == 'ALIVE'):
        message = "Paquet rebutjat per equip no autoritzat, MAC no corresponent o equip no registrat"
        debugger(message)
        rej_tcp_pdu = tcp_PDU(tipus=0x23, nom='', MAC='000000000000', aleatori='000000', dades= message)
        connex_sock.send(rej_tcp_pdu)
        return False
    else:
        message = "Paquet denegat, ip no concorda, error en el número aleatori o aquest quip ja 'està efectuant una operació similar"
        debugger(message)
        nack_tcp_pdu = tcp_PDU(tipus=0x22, nom='', MAC='000000000000', aleatori='000000', dades= message)
        connex_sock.send(nack_tcp_pdu)
        return False

def get_file(connex_sock, t, tcp_addr, sending, getting):

    if (authorised(t) and checkout_equip(t.nom,'estat') == 'ALIVE' and 
    checkout_equip(t.nom,'MAC') == t.MAC and t.aleatori == checkout_equip(t.nom,'aleatori') and
    tcp_addr[0] == checkout_equip(t.nom, 'ip') and not sending and not getting):
        debugger("TOT CORRECTE PER GET_CONF!")
        ack_tcp_pdu = tcp_PDU(tipus=0x31, nom=server_cfg['Nom'], MAC=server_cfg['MAC'], aleatori=t.aleatori, dades= t.nom+".cfg")
        connex_sock.send(ack_tcp_pdu)
        return True
    elif (not authorised(t) or checkout_equip(t.nom,'MAC') != t.MAC or 
          checkout_equip(t.nom,'estat') == 'ALIVE'):
        message = "Paquet rebutjat per equip no autoritzat, MAC no corresponent o equip no registrat"
        debugger(message)
        rej_tcp_pdu = tcp_PDU(tipus=0x33, nom='', MAC='000000000000', aleatori='000000', dades= message)
        connex_sock.send(rej_tcp_pdu)
        return False
    else:
        message = "Paquet denegat, ip no concorda, error en el número aleatori o aquest quip ja 'està efectuant una operació similar"
        debugger(message)
        nack_tcp_pdu = tcp_PDU(tipus=0x32, nom='', MAC='000000000000', aleatori='000000', dades= message)
        connex_sock.send(nack_tcp_pdu)
        return False

def client_sending(connex_sock, t):
    f = open(t.nom+".cfg", 'w')
    while True:
        readable, [], [] = select.select([connex_sock],[],[],w)
        if readable != []:    
            tcp_data = connex_sock.recv(178)
            rebut = tcp_PDU.from_buffer_copy(tcp_data)
            if rebut.tipus == 0x24:
                debugger("Obtinguda línia de configuració")
                f.write(rebut.dades)
            elif rebut.tipus == 0x25:
                debugger("Obtingut final d'enviament de configuració")
                f.close()
                return True
            else:
                debugger("Error al obtenir paquet TCP")
                f.close()
                return False
        else:
            debugger("El temps d'espera per rebre dades de configuració ha expirat")
            f.close()
            return False

def client_getting(connex_sock, t):
    f = open(t.nom+".cfg", 'r')
    lines = f.readlines()

    for line in lines:
        data_tcp_pdu = tcp_PDU(tipus=0x34, nom=server_cfg['Nom'], MAC=server_cfg['MAC'], aleatori=t.aleatori, dades= line)
        connex_sock.send(data_tcp_pdu)
        debugger("Enviada línia de l'arxiu de configuració a " + t.nom)

    
    data_tcp_pdu = tcp_PDU(tipus=0x35, nom=server_cfg['Nom'], MAC=server_cfg['MAC'], aleatori=t.aleatori, dades= '')
    connex_sock.send(data_tcp_pdu)
    debugger("Enviat GET_END a " + t.nom)
    f.close()


def attend_TCP_connexion():
    connex_sock, tcp_addr = tcp_sock.accept()
    debugger("Connexió TCP acceptada")
    data = connex_sock.recv(178)        
    t = tcp_PDU.from_buffer_copy(data)
    sending = False
    getting = False
    if t.tipus == 0x20:
        sending = send_file(connex_sock, t, tcp_addr, sending, getting)
        if not sending:
            debugger("Tancant connexió TCP")
            connex_sock.close()
            return 0
        if not client_sending(connex_sock, t):
            debugger("Tancant connexió TCP")
            connex_sock.close()
            return 0

    elif t.tipus == 0x30:
        debugger("Rebuda petició d'obtenció d'arxiu de configuració")
        if not os.path.exists(t.nom+".cfg"):
            message = "No existeix arxiu de configuració d'aquest equip!"
            debugger(message)
            rej_tcp_pdu = tcp_PDU(tipus=0x33, nom='', MAC='000000000000', aleatori='000000', dades= message)
            connex_sock.send(rej_tcp_pdu)
            connex_sock.close()
            return 0
        else:
            getting = get_file(connex_sock, t, tcp_addr, sending, getting)
            if not getting:
                debugger("Tancant connexió TCP")
                connex_sock.close()
                return 0
            if not client_getting(connex_sock, t):
                debugger("Tancant connexió TCP")
                connex_sock.close()

    else:
        debugger("PAQUET TCP NO ACCEPTAT")
        rej_tcp_pdu = tcp_PDU(tipus=0x09, nom='', MAC='', aleatori='', dades= "message")
        connex_sock.send(rej_tcp_pdu)


    debugger("Tancant connexió TCP")
    connex_sock.close()

def check_TCP_connexions():
    try:
        readable, [], [] = select.select([tcp_sock],[],[],0.0)
    except ValueError:
            exit(-1)
    if readable != []:    
        debugger("Creat thread per gestionar comunicació TCP")
        threading.Thread(target= attend_TCP_connexion).start()
def check_UDP_buffer(udp_sock):
    readable, [], [] = select.select([udp_sock],[],[],0.0)
    if readable != []:    
        data, addr = udp_sock.recvfrom(78)
        attend(data,addr, udp_sock)

if __name__ == '__main__':
    args = parse_arguments()
    server_cfg = extract_cfg_data()
    global equips_dat
    equips_dat = extract_equips_dat()


    udp_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    udp_sock.bind(("", int(server_cfg['UDP-port'])))
    debugger("Socket UDP obert")

    tcp_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    tcp_sock.bind(("", int(server_cfg['TCP-port'])))
    debugger("Socket TCP obert")
    a = tcp_sock.listen(l)

    signal.signal(signal.SIGINT, sigint_handler)
    threading.Thread(target=console).start()

    debugger("Inici de bucle de servei infinit")
    try:
         while True:
            check_UDP_buffer(udp_sock)
            check_TCP_connexions()

    finally:
        quit = True
        udp_sock.close()
        tcp_sock.close()