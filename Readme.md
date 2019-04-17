		-- Pràctica 1 de xarxes --

	-0 Tinc problemes de tancar el socket TCP al servidor
	-1 Printar canvis d'estat per stderr (o stdin), no sol
	   per debug

TODO CLIENT:
	-1 Mirar si envio el motiu de rebug en paquets de rebug
	   com NACKS i REJ
	-1 Ficar a la docu que el seu servidor al getconf envia
	   dos vegades la última línia
	-1 Comentar a la docu el limbo de la temporització del
	   alive
	-2 Mencionar lo del buffer consola quan aquesta no està activada
	-2 Mencionar a la docu que he triat com a llargada màxima del 
		nom del servidor 25 (24 + \0) caràcters
	-3 Comentar que havia fer binding del socket UDP al principi
TODO SERVIDOR:
	-1 Ficar paràmetre per canviar nom de equips.dat
	-1 Especificar a la docu que el listen es fa de 3 clients