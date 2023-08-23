#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <getopt.h>
#include <sys/wait.h>
#include <time.h>

#define TRUE 1
#define FALSE 0
#define SERVER_SIZE_LIMIT 8192

//Struct for keeping count of requests per timeframe
struct Request {
	time_t request_time;
	int count;
} Request_default = {0,0};

typedef struct Request Request;

int main(int argc, char *argv[])
{
	//Setup Variables
	const char *port = "2012";		/* available port */
	const char *welcome_msg = "Type 'close' to disconnect; 'shutdown' to stop\n";
	const int hostname_size = 32;
	char hostname[hostname_size];
	char buffer[8192];
	const int backlog = 10;			/* also max connections */
	char connection[backlog][hostname_size];	/* storage for IPv4 connections */
	socklen_t address_len = sizeof(struct sockaddr);
	struct addrinfo hints, *server;
	struct sockaddr address;
	int r,max_connect,fd,done;
	fd_set main_fd,read_fd;
	int serverfd,clientfd;
	
	char PORT[20] = "2012";
	int RATEreq = 2, RATEsec = 60;
	struct timeval TIME_OUT = {1,0};
	int MAX_USERS = 3;
	pid_t pid;
	int status;
	FILE *logger;
	char time_buff[30];
	struct tm *logTime;
	time_t nowTime = time(NULL);
	
	time_t timer = time(0);
	int user_count = 0;
	int MAX_TIMEOUT = 80;

	
	//Struct for getopt_long_only options
	static struct option long_options[] =
	{
		{"PORT", required_argument, NULL, 'p'},
		{"RATE_MSGS", required_argument, NULL, 'r'},
		{"RATE_TIME", required_argument, NULL, 's'},
		{"MAX_USERS", required_argument, NULL, 'u'},
		{"TIME_OUT", required_argument, NULL, 't'},
	};
	
	int indexptr, c;
	//Read the option arguments, and get/set the values for the server variables
	while((c = getopt_long_only(argc, argv, "p:r:s:u:t:", long_options, &indexptr)) != -1){
		switch(c){
			case 'p':
				//Set PORT;
				strcpy(PORT, optarg);
				break;
			case 'r':
				//Set RATE_MSGS;
				RATEreq = atoi(optarg);
				break;
			case 's':
				//Set RATE_TIME;
				RATEsec = atoi(optarg);
				break;
				
			case 'u':
				//Set MAX_USERS
				MAX_USERS = atoi(optarg);
				break;
			case 't':
				//Set TIME_OUT
				MAX_TIMEOUT = atoi(optarg);
				break;
			
		}

	}

	//Setup array for keeping track of user activity for timeouts
	time_t activityTimes[MAX_USERS+1];
	for(int u=0;u<=MAX_USERS+1;u++){
		activityTimes[u] = time(NULL);
	}
	
	//Setup array for keeping track of user requests per timeframe.
	Request requestArray[MAX_USERS+1];
	for(int q=0;q<MAX_USERS+1;q++){
		requestArray[q].count = 0;
		requestArray[q].request_time = 0;
	}
	//Open Logfile.
	logger = fopen("sever.log", "w");
	
	logTime = gmtime(&timer);
	strftime (time_buff, sizeof(time_buff), "%Y-%m-%d %H:%M:%S", logTime);
	fprintf(logger, "%s Server Variables:\n Port: %s\n RATE: %d requests per %d seconds\n MAX USERS: %d\n TIME OUT: %d seconds\n", time_buff, PORT, RATEreq, RATEsec, MAX_USERS, MAX_TIMEOUT);
	//printf("Server Variables:\n Port: %s\n RATE: %d requests per %d seconds\n MAX USERS: %d\n TIME OUT: %ld seconds\n", PORT, RATEreq, RATEsec, MAX_USERS, TIME_OUT.tv_sec);
	logTime = gmtime(&timer);
	strftime (time_buff, sizeof(time_buff), "%Y-%m-%d %H:%M:%S", logTime);
	fprintf(logger, "%s Starting Server.\n", time_buff);


	/* configure the server */
	memset( &hints, 0, sizeof(struct addrinfo));	/* use memset_s() */
	hints.ai_family = AF_INET;			/* IPv4 */
	hints.ai_socktype = SOCK_STREAM;	/* TCP */
	hints.ai_flags = AI_PASSIVE;		/* accept any */
	r = getaddrinfo( 0, PORT, &hints, &server );
	if( r!=0 )
	{
		perror("failed");
		exit(1);
	}

	/* create a socket */
	serverfd = socket(server->ai_family,server->ai_socktype,server->ai_protocol);
	if( serverfd==-1 )
	{
		perror("failed");
		exit(1);
	}

	/* bind to a port */
	r = bind(serverfd,server->ai_addr,server->ai_addrlen);
	if( r==-1 )
	{
		perror("failed");
		exit(1);
	}

	/* listen for a connection*/
	fputs("TCP Server is listening...", logger);
	r = listen(serverfd,backlog);
	if( r==-1 )
	{
		perror("failed");
		exit(1);
	}

	/* deal with multiple connections */
	max_connect = backlog;		/* maximum connections */
	FD_ZERO(&main_fd);			/* initialize file descriptor set */
	FD_SET(serverfd, &main_fd);	/* set the server's file descriptor */
	/* endless loop to process the connections */
	done = FALSE;
	
	//Main Loop
	while(!done)
	{
		/* backup the main set into a read set for processing */
		read_fd = main_fd;

		/* scan the connections for any activity */
		r = select(max_connect+1,&read_fd, NULL, NULL, &TIME_OUT);
		if( r==-1 )
		{
			perror("failed");
			exit(1);
		}

		/* process any connections */
		for( fd=1; fd<=max_connect; fd++)
		{
			//Current time to check activity
			nowTime = time(NULL);
			/* filter only active or new clients */
			if( FD_ISSET(fd,&read_fd) )	/* returns true for any fd waiting */
			{

				//Timeout Check.
				if(fd != serverfd && (nowTime - activityTimes[fd]) > MAX_TIMEOUT){

					char timeoutMessage[300] = "You have been timed-out due to inactivity.\n";
					send(fd, timeoutMessage, strlen(timeoutMessage), 0);
					
					FD_CLR(fd, &main_fd);
					close(fd);
					//update the screen
					logTime = gmtime(&timer);
					strftime (time_buff, sizeof(time_buff), "%Y-%m-%d %H:%M:%S", logTime);
					fprintf(logger, "%s %s has been timed-out.\n", time_buff,connection[fd]);
					user_count--;
					continue;
				}
				/* filter out the server, which indicates a new connection */
				if( fd==serverfd )
				{
					/* add the new client */
					clientfd = accept(serverfd,&address,&address_len);
					if( clientfd==-1 )
					{
						perror("failed");
						exit(1);
					}
					/* connection accepted, get name */
					r = getnameinfo(&address,address_len,hostname,hostname_size,0,0,NI_NUMERICHOST);
					/* update array */
					strcpy(connection[clientfd],hostname);
					logTime = gmtime(&timer);
					strftime (time_buff, sizeof(time_buff), "%Y-%m-%d %H:%M:%S", logTime);
					fprintf(logger, "%s New connection from %s\n",time_buff, connection[clientfd]);

					//Check if there's room to add the client
					if(user_count < MAX_USERS){

					/* add new client socket to the master list */
					FD_SET(clientfd, &main_fd);

					/* respond to the connection */
					strcpy(buffer,"0 - Hello to ");
					strcat(buffer,connection[clientfd]);
					strcat(buffer,"!\n");
					strcat(buffer,welcome_msg);
					send(clientfd,buffer,strlen(buffer),0);
					//Increase user count, set activity time, and reset request count. 
					user_count++;
					activityTimes[clientfd] = time(NULL);
					requestArray[clientfd].count = 0;
					
					}
					else{	//No room for new users
						strcpy(buffer, "Sorry, server at capacity. Please try again later\n");
						send(clientfd, buffer, strlen(buffer),0);
						close(clientfd);
						logTime = gmtime(&timer);
						strftime (time_buff, sizeof(time_buff), "%Y-%m-%d %H:%M:%S", logTime);
						fprintf(logger, "%s Server at capacity, new client could not join.\n", time_buff);
					}
				} /* end if, add new client */
				/* the current fd has incoming info - deal with it */
				else
				{
					//Set activity time for user
					activityTimes[fd] = time(NULL);
					/* read the input buffer for the current fd */
					r = recv(fd,buffer,8192,0);
					/* if nothing received, disconnect them */
					if( r<1 )
					{
						/* clear the fd and close the connection */
						FD_CLR(fd, &main_fd);	/* reset in the list */
						close(fd);				/* disconnect */
						/* update the screen */
						logTime = gmtime(&timer);
						strftime (time_buff, sizeof(time_buff), "%Y-%m-%d %H:%M:%S", logTime);
						fprintf(logger, "%s %s closed\n", time_buff,connection[fd]);
						//Reduce User Count
						user_count--;
					}
					/* something has been received */
					else
					{
						/* if 'shutdown' sent... */
						if( strcmp(buffer,"shutdown")==0 )
							done = TRUE;		/* terminate the loop */
						/* otherwise, handle user request */
						else{
							//Check if user has made too many requests in the current timeframe.
							if(requestArray[fd].count >= RATEreq){
								time_t currentTime = time(NULL);
								if((currentTime - requestArray[fd].request_time) < RATEsec){
									//Too many requests have been made in the timeframe.
									char too_many_requests[200];
									char i_to_string1[20];
									sprintf(i_to_string1, "%ld", RATEsec - (currentTime - requestArray[fd].request_time));
									strcpy(too_many_requests, "3 - You have exceeded your request limit. Please try again in ");
									strcat(too_many_requests, i_to_string1);
									strcat(too_many_requests," seconds.\n");
									send(fd, too_many_requests, strlen(too_many_requests), 0);
									memset(buffer, 0, strlen(buffer));
									continue;
								}
								else{
									//Reset the count since enough time has passed.
									requestArray[fd].count = 0;
								}
								
							}
							
							logTime = gmtime(&timer);
							strftime (time_buff, sizeof(time_buff), "%Y-%m-%d %H:%M:%S", logTime);
							fprintf(logger, "%s Size Comparing.\n", time_buff);
							fprintf(logger, "%s Size: %d\n", time_buff, atoi(buffer));
							//File Size Handling
							if(atoi(buffer) > SERVER_SIZE_LIMIT){ //File too big
								char not_OK[20] = "File too large.";
								int o = send(fd, not_OK, strlen(not_OK), 0);
							}
							else{ //File size is fine.

								logTime = gmtime(&timer);
								strftime (time_buff, sizeof(time_buff), "%Y-%m-%d %H:%M:%S", logTime);
								fprintf(logger, "%s File size is fine.\n", time_buff);
								char* is_OK = "OK";
								send(fd, is_OK, strlen(is_OK), 0);


								int fileSize = atoi(buffer);
								int k;
								char file_transfer[1024];
								memset(buffer, 0, strlen(buffer));
								logTime = gmtime(&timer);
								strftime (time_buff, sizeof(time_buff), "%Y-%m-%d %H:%M:%S", logTime);
								fprintf(logger, "%s File size: %d\n", time_buff, fileSize);

								
								//Create Filepath for incoming image
								char filepath[400];
								char i_to_string2[20];
								sprintf(i_to_string2, "%d", fd);
								strcpy(filepath, "test_img_");
								strcat(filepath, i_to_string2);
								strcat(filepath, ".png");

								
								//Create file
								FILE *image;
								image = fopen(filepath, "wb");
								while(fileSize > 0){
									k = recv(fd, file_transfer, 1024,0);
									fwrite(file_transfer, 1, k, image);
									fileSize -= k;
									memset(file_transfer, 0, strlen(file_transfer));					
								}
					
								fclose(image);
								
								
								
								char* java_list[] = {"java", "-cp", "javase.jar:core.jar", "com.google.zxing.client.j2se.CommandLineRunner", filepath, NULL};

								int output_pipe[2];
								pipe(output_pipe);
								char exec_output[4000];
							
								//Running the Exec
								pid = fork();
								if (pid == 0) { /* this is child process */
									dup2(output_pipe[1], STDOUT_FILENO);
									close(output_pipe[0]);
									close(output_pipe[1]);
									//Calling execvp using aptr from createarray
									execvp(java_list[0], java_list);
									printf("If you see this statement then execl failed ;-(\n");
									perror("execvp");
									exit(-1);
								} 
								else if (pid > 0) { /* this is the parent process */
									wait(&status); /* wait for the child process to terminate */
									if (WIFEXITED(status)) { /* child process terminated normally */
										close(output_pipe[1]);
										read(output_pipe[0], exec_output, sizeof(exec_output));
										char *temp;
										//now exec_output should have the whole output. so we need to read through it and get the output we want.
										char *output_line = strtok_r(exec_output, "\n", &temp);
										int count = 1;
										while((output_line = strtok_r(NULL, "\n", &temp)) != NULL){
											if(strcmp(output_line, "Raw result:") == 0){
												//Get it and send it!
												output_line = strtok_r(NULL, "\n", &temp);
												logTime = gmtime(&timer);
												strftime (time_buff, sizeof(time_buff), "%Y-%m-%d %H:%M:%S", logTime);
												fprintf(logger, "%s output_line: %s\n", time_buff, output_line);
												count++;
												send(fd, output_line, strlen(output_line), 0);
												memset(output_line, 0, strlen(output_line));
												break;
											}									
										}
										if(count == 1){	//If the exec couldn't return a website name
											send(fd,exec_output, strlen(exec_output), 0);
											memset(exec_output, 0, strlen(exec_output));
										}
										//If this was the first request by the user, set this time as the start of the request timeframe
										if(requestArray[fd].count == 0){
											requestArray[fd].request_time = time(NULL);
										}
										//Increment Number of requests made by the user
										requestArray[fd].count++;
									}
									else { /* child process did not terminate normally */
									//printf("Child process did not terminate normally!\n");
									}
								}
								else { /* we have an error */
									perror("fork"); /* use perror to print the system error message */
									exit(EXIT_FAILURE);
								}
							
							}
								
						}
					}
				} /* end else to send/recv from client(s) */
			} /* end if */
			else{

			}
		} /* end for loop through connections */
	} /* end while */
	//Shut server down.
	puts("TCP Server shutting down");
	fclose(logger);
	close(serverfd);
	freeaddrinfo(server);
	return(0);
}
