		-- Pràctica 1 de xarxes --

	-0 Comentar problema de getconf i REUSEADDR
	-0 Comentar dilema entre sleep i select d's segons al registre 
	-0 Mirar TODOS 

TODO CLIENT:
	-0 Comentar a la docu el limbo de la temporització del
	   alive
	-0 Ficar a la docu que el seu servidor al getconf envia
	   dos vegades la última línia
	-0 Mencionar lo del buffer consola quan aquesta no està activada
	-1 Mirar si envio el motiu de rebug en paquets de rebug
	   com NACKS i REJ
	-1 Mencionar a la docu que he triat com a llargada màxima del 
		nom del servidor 25 (24 + \0) caràcters
	-2 Comentar que havia fet binding del socket UDP al principi
TODO SERVIDOR:
	-1 Especificar a la docu que el listen es fa de 3 clients