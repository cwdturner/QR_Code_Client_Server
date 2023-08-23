#include <unistd.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <strings.h>
#include <errno.h>

#define SERVER_ADDRESS "127.0.0.1"

int check_if_image(const char* path) {
    const char *last_dot = strrchr(path, '.');
    if (last_dot) {
        if (strcmp(last_dot, ".jpeg") == 0) return 1;
        if (strcmp(last_dot, ".jpg") == 0) return 1;
        if (strcmp(last_dot, ".png") == 0) return 1;
    } 
    return 0;
}

int main(int argc, char *argv[]){

	//Setup Variables
	char PORT[10];
	strcpy(PORT, "2012");
	int my_socket, r;
	struct addrinfo hints,*host;
	FILE *picture;
	char buffer[1024];
	char file_size[256];
	
	//Read Port Value from Arguments
	if(argc == 2){
		strcpy(PORT, argv[1]);
		printf("PORT: %s\n", PORT);
	}
	
	memset( &hints, 0, sizeof(hints) );		/* use memset_s() */
	hints.ai_family = AF_INET;				/* IPv4 */
	hints.ai_socktype = SOCK_STREAM;		/* TCP */
	r = getaddrinfo(SERVER_ADDRESS, PORT, &hints, &host );
	

	//Create Socket
	my_socket = socket(host->ai_family,host->ai_socktype,host->ai_protocol);
	printf("Creating Socket\n");
	if(my_socket == -1){
		printf("socket() failed.\n");
		exit(0);
	}
	
	//Connect To Server
	r = connect(my_socket, host->ai_addr,host->ai_addrlen);
	if(r != 0){
		printf("connect() failed.\n");
		close(my_socket);
		exit(0);
	}
	else{
		printf("Connected!");
	}


	printf("Port: %s\n",PORT);

	//Variables for the main loop.
	char p[100];
	int done = 0;
	int done2 = 0;
	int bytes_sent;
	while(done == 0){
		
		r= recv(my_socket,buffer,1024,0);
		if( r<1 )
		{
			printf("Connection closed by peer\n");
			memset(buffer, 0, strlen(buffer));
			break;
		}
		if(strcmp(buffer, "Sorry, server at capacity. Please try again later\n") == 0){
			printf("%s", buffer);
			memset(buffer, 0, strlen(buffer));
			break;
		}
		printf("%s", buffer);
		memset(buffer, 0, strlen(buffer));
		
		//Second Loop to handle input from user
		while(done2 == 0){
			printf("\nEnter 'close' to disconnect, 'shutdown' to turn off server, or a qr code filename to get the URL:\n");
			scanf("%s", p);
			if(strcmp(p, "close") == 0){	//Close Client
				done = 1;
				done2 = 1;
			}
			else if (strcmp(p, "shutdown") == 0){	//Close Client and tell server to shutdown
				if((bytes_sent = send(my_socket, p, strlen(p)+1,0)) == -1){
					printf("error = %s\n", strerror(errno));
					memset(p, 0, strlen(p));
				}
				printf("Sent shutdown message. Closing Program.\n");
				done = 1;
				done2 = 1;
			}
			else{

				if(check_if_image(p) == 1){	//Check if user provided an image
					if((picture = fopen(p, "r"))){	//Read through image to get size
						int size;
						fseek(picture, 0, SEEK_END);
						size = ftell(picture);
						printf("Size of image is %d bytes.\n",size);
						sprintf(file_size, "%d", size);
						fseek(picture, 0, SEEK_SET);

						
						//Send size to server
						bytes_sent = send(my_socket, file_size, strlen(file_size),0);
						memset(file_size, 0, strlen(file_size));
						
						//Receive message from server
						r= recv(my_socket,buffer,1024,0);
						if( r<1 )
						{
							printf("Connection closed by peer\n");
							memset(buffer, 0, strlen(buffer));
							fclose(picture);
							done = 1;
							break;
						}
						if(strcmp(buffer, "OK") == 0){	//File size is not a problem. OK to send.
							//printf("OK\n");
							char img_buffer[1024];

        						while((bytes_sent = fread(img_buffer, 1, sizeof(img_buffer), picture))>0){
        							send(my_socket, img_buffer, bytes_sent, 0);
        							memset(img_buffer,0,sizeof(img_buffer));
        							//printf("Looping Send, %d bytes sent.\n", bytes_sent);
        						}
							//printf("Finished sending file.\n");
							
							fclose(picture);
							memset(buffer, 0, strlen(buffer));
							break;
						}
						else if(buffer[0] == '3'){	//Exceeded allowed requests per timeframe to the server
							printf("%s", buffer);
							fclose(picture);
							memset(buffer, 0, strlen(buffer));					
						}
						else if(strcmp(buffer, "You have been timed-out due to inactivity.\n") == 0){	//Timed out
							printf("%s", buffer);
							fclose(picture);
							memset(buffer, 0, strlen(buffer));
							done = 1;
							done2 = 1;
						}
						else{
							printf("File too large. Please send another.\n");
							//printf("%s\n", buffer);
							fclose(picture);
							memset(buffer, 0, strlen(buffer));
						}
					}
					else{
						printf("Either file does not exist, or filepath was incorrect.\n");
					}
				}
				else{
					printf("Either unrecognized command or file was not an image.\n");
				}
			}
	
		}	
	}
	
	//Close Socket
	close(my_socket);
	freeaddrinfo(host);
}
