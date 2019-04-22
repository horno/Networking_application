# coding=utf-8
#!/usr/bin/env python
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

# Classe que serveix com a estructura per guardar les dades de la PDU 
# que s'enviarà i rebrà pel canal UDP
class udp_PDU(Structure):
    _fields_ = [
        ('tipus', c_ubyte),
        ('nom', c_char*7),
        ('MAC', c_char*13),
        ('aleatori', c_char*7),
        ('dades', c_char*50)
    ]

# Classe que serveix com a estructura per guardar les dades de la PDU
# que s'enviarà i rebrà pel canal TCP
class tcp_PDU(Structure):
    _fields_ = [
        ('tipus', c_ubyte),
        ('nom', c_char*7),
        ('MAC', c_char*13),
        ('aleatori', c_char*7),
        ('dades', c_char*150)
    ]

# Obtenció de paràmetres d'entrada mitjançant la llibreria argparse
def parse_arguments():
    parser = argparse.ArgumentParser(description='Server side of a client-server communication')
    parser.add_argument('-c', help = 'New route to the configuration file')
    parser.add_argument('-d', help = 'Activates the debugger flag', action = "store_true")
    parser.add_argument('-u', help = 'New route to the authorized clients file')
    args = parser.parse_args()
    return args

# Extrau la informació del servidor del fitxer corresponent
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

# Extrau la informació dels equips autoritzats del fitxer corresponent i la
# fica en una llista de diccionaris
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

# Funció que permet mostrar per pantalla els missàtges de debug (passats per
# paràmetre)
def debugger(debug_text):
    if args.d:
        print("Debugger -> "+debug_text)

# Primer pas d'autenticació de client. Mira si el nom existeix en la 
# llista de clients autoritzats
def authorised(p):
    for i in equips_dat:
        if i['nom'] == p.nom:
            return True, i
    return False, {}

# Envia una PDU de tipus denegació de registre a l'adreça passada per paràmetre
# i amb el missatge passat per paràmetre com a dades del paquet
def nack_pdu_send(addr, udp_sock, message):
    nack_pdu = udp_PDU(tipus=0x02, nom='', MAC='', aleatori='', dades= message)
    udp_sock.sendto(nack_pdu, addr)

# Mètode cridat quan les dades d'un paquet de petició de registre són 
# correctes. Genera un nombre aleatori de 000001 a 999999 i envia la 
# PDU d'acceptació de registre amb el port TCP a les dades
def correct_data(addr, udp_sock, p, equip_dat):
    rand = random.randint(1000001,1999999)
    rand = str(rand)[1:]
    ack_pdu = udp_PDU(tipus=0x01, nom=server_cfg['Nom'], MAC=server_cfg['MAC'], 
                      aleatori=rand, dades=server_cfg['TCP-port'])
    udp_sock.sendto(ack_pdu, addr)
    actualize_equips(p.nom, 'aleatori', rand)

# Efectua el segon pas d'autenticació del paquet de petició de registre
# i respon en conseqüència
def check_state(addr, udp_sock, p, equip_dat):
    if equip_dat['estat'] == 'DISCONNECTED':
        if p.MAC == equip_dat['MAC'] and p.aleatori == '000000':
            if 'ip' in equip_dat:
                if addr[0] == equip_dat['ip']:
                    correct_data(addr, udp_sock, p, equip_dat)
                    equip_dat['estat'] = 'REGISTERED'
                    actualize_equips(p.nom, 'estat', 'REGISTERED')
                    print(p.nom + ' passa a estat REGISTERED')
                    actualize_equips(p.nom, 'TTL', 0)
                    debugger("Creat thread per gestionar alive")
                    thr = threading.Thread(target = ttl_register, args=(equip_dat['nom'],))
                    thr.setDaemon(True)
                    thr.start()
                else:
                    message = 'Ip del primer missatge no concorda'
                    debugger('Petició de registre de ' + equip_dat['nom'] + ' denegada per:' + message)
                    nack_pdu_send(addr, udp_sock, message)
            else:
                equip_dat['ip'] = addr[0]
                actualize_equips(p.nom, 'ip', addr[0])
                actualize_equips(p.nom, 'estat', 'REGISTERED')  
                print(p.nom + ' passa a estat REGISTERED')
                correct_data(addr, udp_sock, p ,equip_dat)
                actualize_equips(p.nom, 'TTL', 0)
                debugger("Creat thread per gestionar alive")
                thr = threading.Thread(target = ttl_register, args=(equip_dat['nom'],))
                thr.setDaemon(True)
                thr.start()
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

# Donat el nom d'un equip, el camp i el valor que se li vol assignar al camp,
# actualitza la llista d'equips
def actualize_equips(name, key, value):
    for i in equips_dat:
        if i['nom'] == name:
            i[key] = value            

# Donat el nom d'un equip i un camp, retorna el valor emmagatzemat en la
# llista de clients
def checkout_equip(name, key):
    for i in equips_dat:
        if i['nom'] == name:
            return i[key]

# Primer pas d'autenticació de client en el procés de registre del mateix
def register_petition(addr, udp_sock, p):
    auth, equip_dat = authorised(p)
    if auth:
        check_state(addr, udp_sock, p, equip_dat)
    else:
        debugger('Petició de registre de ' + p.nom + ' rebutjada, equip no autoritat')
        rej_pdu = udp_PDU(tipus=0x03, nom='', MAC='', aleatori='', dades='Equip no autoritzat')
        udp_sock.sendto(rej_pdu, addr)

# Crida a les funcions de gestió de Registre o Alive en funció de la petició  
def attend(data, addr, udp_sock):
    p = udp_PDU.from_buffer_copy(data)
    if p.tipus == 0x00:
        debugger('Rebuda petició de registre de ' + p.nom)
        register_petition(addr, udp_sock, p)
    elif p.tipus == 0x10:
        debugger('Rebut alive inf de ' + p.nom)
        alive_inf(addr, udp_sock, p)

# Mètode executat per un fil concurrent que controla el temps de vida
# d'un client des de que es registra fins que envia el primer alive. 
# Funciona de la mateixa manera que ttl_alive però només pel pas de
# registered a alive
def ttl_register(name):
    debugger('Comença temporitzacio de REGISTER a ALIVE de '+name)
    actualize_equips(name, 'TTL', 2)    
    for i in range(j):
        time.sleep(r)
    ttl = checkout_equip(name, 'TTL') - j
    actualize_equips(name, 'TTL', ttl)
    if ttl == 0:
        print(name + ' passa a estat DISCONNECTED')
        actualize_equips(name, 'estat', 'DISCONNECTED')

# Mètode executat per un fil concurrent que controla el temps de vida
# d'un client en funció d'un dels paquets alive rebuts. Restan un TTL
# per temps d'alive transcorregut. Si cap altre fil li augmenta el TTL
# és que no s'ha rebut cap Alive més i, per tant, es considera que hi
# ha hagut una fallada en la comunicació amb el client
def ttl_alive(name):
    debugger('Comença temporitzacio ALIVE de '+name)
    actualize_equips(name, 'TTL', checkout_equip(name, 'TTL')+k)
    for i in range(k):
        time.sleep(r)
    ttl = checkout_equip(name, 'TTL') - k
    actualize_equips(name, 'TTL', ttl)
    if ttl == 0:
        print(name + ' passa a estat DISCONNECTED')
        actualize_equips(name, 'estat', 'DISCONNECTED')

# Mètode principal del control d'Alive dels clients. Gestiona l'autenticitat
# dels paquets rebuts i respón en conseqüència 
def alive_inf(addr, udp_sock, p):
    auth,equip_dat = authorised(p)
    if (auth and (equip_dat['estat'] == 'ALIVE' or  equip_dat['estat'] == 'REGISTERED')
    and equip_dat['MAC'] == p.MAC and addr[0] == equip_dat['ip'] and 
    p.aleatori == equip_dat['aleatori'] ):
        alive_ack_pdu = udp_PDU(tipus=0x011, nom=server_cfg['Nom'], MAC=server_cfg['MAC'], aleatori=equip_dat['aleatori'], dades='')
        udp_sock.sendto(alive_ack_pdu, addr)
        if equip_dat['estat'] == 'REGISTERED':
            actualize_equips(p.nom, 'estat', 'ALIVE')
            print(p.nom + ' passa a estat ALIVE')
        thr = threading.Thread(target = ttl_alive, args=(equip_dat['nom'],))
        thr.setDaemon(True)
        thr.start()
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

# Mètode que gestiona la consola. Fins que quit no sigui cert, llegirà de 
# l'entrada estàndard les comandes 'quit' i 'list'. Si llegeix una comanda
# no reconeguda, ho informarà. 
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
    udp_sock.close()
    tcp_sock.close()
    os._exit(1)

# Gestiona (per si calgués) la senyal sigint (CTRL + C)
def sigint_handler(signum, frame):
    global quit
    quit = True
    tcp_sock.close()
    udp_sock.close()
    exit(-1)

# Gestiona l'autenticitat dels paquets rebuts per tcp amb petició d'enviament
# dades de configuració i respón en conseqüència
def send_file(connect_sock, t, tcp_addr, sending, getting):
    debugger("Rebuda petició d'enviament d'arxiu de configuració")
    if (authorised(t) and checkout_equip(t.nom,'estat') == 'ALIVE' and 
    checkout_equip(t.nom,'MAC') == t.MAC and t.aleatori == checkout_equip(t.nom,'aleatori') and
    tcp_addr[0] == checkout_equip(t.nom, 'ip') and not sending and not getting):
        debugger("Tot correcte per SEND_CONF!")
        ack_tcp_pdu = tcp_PDU(tipus=0x21, nom=server_cfg['Nom'], MAC=server_cfg['MAC'], aleatori=t.aleatori, dades= t.nom+".cfg")
        connect_sock.send(ack_tcp_pdu)
        return True
    elif (not authorised(t) or checkout_equip(t.nom,'MAC') != t.MAC or 
          checkout_equip(t.nom,'estat') == 'ALIVE'):
        message = "Paquet rebutjat per equip no autoritzat, MAC no corresponent o equip no registrat"
        debugger(message)
        rej_tcp_pdu = tcp_PDU(tipus=0x23, nom='', MAC='000000000000', aleatori='000000', dades= message)
        connect_sock.send(rej_tcp_pdu)
        return False
    else:
        message = "Paquet denegat, ip no concorda, error en el número aleatori o aquest quip ja 'està efectuant una operació similar"
        debugger(message)
        nack_tcp_pdu = tcp_PDU(tipus=0x22, nom='', MAC='000000000000', aleatori='000000', dades= message)
        connect_sock.send(nack_tcp_pdu)
        return False

# Gestiona l'autenticitat dels paquets rebuts per tcp amb petició d'obtenció
# de dades de configuració i respón en conseqüència
def get_file(connect_sock, t, tcp_addr, sending, getting):

    if (authorised(t) and checkout_equip(t.nom,'estat') == 'ALIVE' and 
    checkout_equip(t.nom,'MAC') == t.MAC and t.aleatori == checkout_equip(t.nom,'aleatori') and
    tcp_addr[0] == checkout_equip(t.nom, 'ip') and not sending and not getting):
        debugger("TOT CORRECTE PER GET_CONF!")
        ack_tcp_pdu = tcp_PDU(tipus=0x31, nom=server_cfg['Nom'], MAC=server_cfg['MAC'], aleatori=t.aleatori, dades= t.nom+".cfg")
        connect_sock.send(ack_tcp_pdu)
        return True
    elif (not authorised(t) or checkout_equip(t.nom,'MAC') != t.MAC or 
          checkout_equip(t.nom,'estat') == 'ALIVE'):
        message = "Paquet rebutjat per equip no autoritzat, MAC no corresponent o equip no registrat"
        debugger(message)
        rej_tcp_pdu = tcp_PDU(tipus=0x33, nom='', MAC='000000000000', aleatori='000000', dades= message)
        connect_sock.send(rej_tcp_pdu)
        return False
    else:
        message = "Paquet denegat, ip no concorda, error en el número aleatori o aquest quip ja 'està efectuant una operació similar"
        debugger(message)
        nack_tcp_pdu = tcp_PDU(tipus=0x32, nom='', MAC='000000000000', aleatori='000000', dades= message)
        connect_sock.send(nack_tcp_pdu)
        return False

# Gestiona la recepció de dades de configuració del client. Si rep dades
# les guarda al fitxer corresponent, si rep SEND_END deixa de llegir, 
# si la cadència d'informació és menor a w deixa de rebre. Retorna una
# booleana en funció de l'exit de la transacció
def client_sending(connect_sock, t):
    f = open(t.nom+".cfg", 'w')
    while True:
        readable, [], [] = select.select([connect_sock],[],[],w)
        if readable != []:    
            tcp_data = connect_sock.recv(178)
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

# Obre el fitxer de configuració (si existeix) del client indicat i envia
# l'arxiu de configuració línia per línia culminant en un paquet GET_END  
def client_getting(connect_sock, t):
    f = open(t.nom+".cfg", 'r')
    lines = f.readlines()

    for line in lines:
        data_tcp_pdu = tcp_PDU(tipus=0x34, nom=server_cfg['Nom'], MAC=server_cfg['MAC'], 
                               aleatori=t.aleatori, dades= line)
        connect_sock.send(data_tcp_pdu)
        debugger("Enviada línia de l'arxiu de configuració a " + t.nom)

    
    data_tcp_pdu = tcp_PDU(tipus=0x35, nom=server_cfg['Nom'], MAC=server_cfg['MAC'], aleatori=t.aleatori, dades= '')
    connect_sock.send(data_tcp_pdu)
    debugger("Enviat GET_END a " + t.nom)
    f.close()

# Accepta la connexió TCP i atén a la comanda demanada depenent del tipus
# de paquet que rebi
def attend_TCP_connection():
    connect_sock, tcp_addr = tcp_sock.accept()
    debugger("Connexió TCP acceptada")
    data = connect_sock.recv(178)        
    t = tcp_PDU.from_buffer_copy(data)
    sending = False
    getting = False
    if t.tipus == 0x20:
        sending = send_file(connect_sock, t, tcp_addr, sending, getting)
        if not sending:
            debugger("Tancant connexió TCP")
            connect_sock.close()
            return 0
        if not client_sending(connect_sock, t):
            debugger("Tancant connexió TCP")
            connect_sock.close()
            return 0

    elif t.tipus == 0x30:
        debugger("Rebuda petició d'obtenció d'arxiu de configuració")
        if not os.path.exists(t.nom+".cfg"):
            message = "No existeix arxiu de configuració d'aquest equip!"
            debugger(message)
            rej_tcp_pdu = tcp_PDU(tipus=0x33, nom='', MAC='000000000000', aleatori='000000', dades= message)
            connect_sock.send(rej_tcp_pdu)
            connect_sock.close()
            return 0
        else:
            getting = get_file(connect_sock, t, tcp_addr, sending, getting)
            if not getting:
                debugger("Tancant connexió TCP")
                connect_sock.close()
                return 0
            client_getting(connect_sock, t)

    else:
        debugger("PAQUET TCP NO ACCEPTAT")
        rej_tcp_pdu = tcp_PDU(tipus=0x09, nom='', MAC='', aleatori='', dades= "message")
        connect_sock.send(rej_tcp_pdu)


    debugger("Tancant connexió TCP")
    connect_sock.close()

# Comprova si hi ha alguna petició de connexió disponible per al canal
# TCP. Si n'hi ha, crea un thread no daemon (per evitar problemes de 
# fitxers oberts) per tractar la connexió 
def check_TCP_connections():
    readable, [], [] = select.select([tcp_sock],[],[],0.0)
    if readable != []:    
        debugger("Creat thread per gestionar comunicació TCP")
        threading.Thread(target= attend_TCP_connection).start()


# Comprova si es pot llegir per el canal UDP. Si es pot, atén la consulta
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
    tcp_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1) #TODO: Treure-ho
    tcp_sock.bind(("", int(server_cfg['TCP-port'])))
    debugger("Socket TCP obert")
    a = tcp_sock.listen(l)

    signal.signal(signal.SIGINT, sigint_handler)
    thr = threading.Thread(target=console)
    thr.setDaemon(True)
    thr.start()

    debugger("Inici de bucle de servei infinit")
    try:
        while True:
            check_UDP_buffer(udp_sock)
            check_TCP_connections()

    finally:
        quit = True
        udp_sock.close()
        tcp_sock.close()
