		-- Pràctica 1 de xarxes --


	-1 Printar canvis d'estat per stderr (o stdin), no sol
	   per debug

TODO CLIENT:
	-1 Mirar si envio el motiu de rebug en paquets de rebug
	   com NACKS i REJ
	-1 Ficar a la docu que el seu servidor al getconf envia
	   dos vegades la última línia
	-1 Comentar a la docu el limbo de la temporització del
	   alive
	-2 Preguntar si és necessari autoritzar en els SEND_ACK
	-3 Controlar buffer consola quan aquesta no està activada
	-3 Canvair a switch-case els ifs tant llagrs del client
TODO SERVIDOR:
	-1 Ficar paràmetre per canviar nom de equips.dat
	-1 Especificar a la docu que el listen es fa de 3 clients