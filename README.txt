A client and server program that runs a JAR file to check a QR code and return the corresponding website url associated with it. The server handle multiple connections. To be run on a linux system due to use of UNIX commands. 

test1.png - A valid QR.
test2.png - Size is too large.
test3.png - A valid QR.
test4.png - An invalid QR (quality is too low) that results in a different message from the java.

The server ip is set at the top of QRClient.c under #define SERVER_ADDRESS.
Server options can be set with -PORT, -RATE_MSGS, -RATE_TIME, -MAX_USERS, and -TIMEOUT command line arguments.
