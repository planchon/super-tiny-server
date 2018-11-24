//        _                  _                   _       
//       | |                | |                 (_)      
//  _ __ | | __ _ _ __   ___| |__   ___  _ __    _  ___  
// | '_ \| |/ _` | '_ \ / __| '_ \ / _ \| '_ \  | |/ _ \ 
// | |_) | | (_| | | | | (__| | | | (_) | | | |_| | (_) |
// | .__/|_|\__,_|_| |_|\___|_| |_|\___/|_| |_(_)_|\___/ 
// | |                                                   
// |_|                                                   

//            Super Tiny Server - StS v1

//   a very compact web server created by Paul Planchon
// in order to host planchon.io on the smallest webserver
//       possible. All code under MIT licence.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT    9191
#define VERSION 1
#define MAX_WAITING_CONNEXION 64
#define BUFSIZE 8096

#define CURRENT_DIR "static/"

struct {
	char * ext;
	char * filetype;
} extensions_mime [] = {
	{"gif", "image/gif" },  
	{"jpg", "image/jpg" }, 
	{"jpeg","image/jpeg"},
	{"png", "image/png" },  
	{"ico", "image/ico" },  
	{"zip", "image/zip" },  
	{"gz",  "image/gz"  },  
	{"tar", "image/tar" },  
	{"htm", "text/html" },  
	{"html","text/html" },
	{"css","text/css"   },
	{0,0}
};

void server(int socket, int hit) {
	long ret;
	int webRequest, file, buflen;
	char * fstr;
	static char buffer[BUFSIZE + 1];
	
	webRequest = read(socket, buffer, BUFSIZE);
	
	if (!strncmp(&buffer[0], "GET /\0", 6) || !strncmp(&buffer[0], "get /\0", 6) || !strncmp(&buffer[0], "GET / ", 6)) {
		(void) strcpy(buffer, "GET /index.html");
	}

	for(int i=4;i<BUFSIZE;i++) { /* null terminate after the second space to ignore extra stuff */
		if(buffer[i] == ' ') { /* string is "GET URL " +lots of other stuff */
			buffer[i] = 0;
			break;
		}
	}

	buflen=strlen(buffer);
	fstr = (char *)0;
	for(int i=0;extensions_mime[i].ext != 0;i++) {
		long lenExt = strlen(extensions_mime[i].ext);
		if( !strncmp(&buffer[buflen-lenExt], extensions_mime[i].ext, lenExt)) {
			fstr = extensions_mime[i].filetype;
			break;
		}
	}
	
	if ((file = open(&buffer[5], O_RDONLY)) == -1) {
		printf("ERROR (%s) : The file (%s) doesnt exist, or cant be opened.\n", strerror(errno), &buffer[5]);
		fflush(stdout);
		exit(3);
	}
	
	long len = lseek(file, (off_t)0, SEEK_END);

	(void) lseek(file, (off_t)0, SEEK_SET);

	(void) sprintf(buffer,"HTTP/1.1 200 OK\nServer: StS/%d.0\nContent-Length: %ld\nConnection: close\nContent-Type: %s\n\n", VERSION, len, fstr);
	
	(void) write(socket, buffer, strlen(buffer));

	while ((ret = read(file, buffer, BUFSIZE)) > 0) {
		(void) write(socket, buffer, ret);
	}

	sleep(1);
	close(socket);
	exit(1);
}

int main(int argc, char** argv) {
	int listenSocket, pip, clientSocket;
	static struct sockaddr_in server_addr;
	static struct sockaddr_in client_addr;
	socklen_t clientLength = sizeof(client_addr);

	// We are in the main fork, so the pip cant be 0
	if (fork() != 0)
		return 0;

	printf("\n\tSuper Tiny Server - StS v%d\n\n", VERSION);
	
	if((listenSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("ERROR (%d) : Can't create the listen socket. Closing.\n", errno);
		exit(-1);
	}

	if((chdir(CURRENT_DIR)) == -1) {
		printf("ERROR: Error during the directory changing.\n");
		exit(4);
	}
	
	(void) signal(SIGCLD, SIG_IGN);
	(void) signal(SIGHUP, SIG_IGN);
	
	(void) setpgrp();
	
	server_addr.sin_family      = AF_INET;           // We are using ipv4
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // We want our server has an IP, any tho
	server_addr.sin_port        = htons(PORT);       // Converting the port to network byte structure

	if (bind(listenSocket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		printf("ERROR (%d) : Can't bind server socket. Closing.\n", errno);
		exit(-1);
	}

	if (listen(listenSocket, MAX_WAITING_CONNEXION) < 0) {
		printf("ERROR: Can't listen to the server socket. Closing.\n");
		exit(-1);		
	}
	
	for (int hit = 1; ; hit++) {
		
		if ((clientSocket = accept(listenSocket, (struct sockaddr *) &client_addr, &clientLength)) < 0) {
			printf("ERROR (%d) : Can't accept the socket. Closing on hit %d.\n", errno, hit);
			exit(3);		
		}

		if ((pip = fork()) < 0) {
			printf("ERROR: Can't create the child process. Closing. \n");
			exit(3);
		} else {
			if (pip == 0) { // We are in the child program
				(void) close(listenSocket);
				server(clientSocket, hit);
			} else {        // We are in the main program, so we wait for an other request.
				(void) close(clientSocket);
			}
		}
	}
}
