all: QRServer QRClient

QRServer: QRServer.c
	gcc -o QRServer QRServer.c

QRClient: QRClient.c
	gcc -o QRClient QRClient.c
	
clean:
	rm QRServer QRClient
