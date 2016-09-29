#include "csapp.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {

	if( argc < 3 ) {
		printf( "Too few arguments, a server and port need to be specified" );
		exit(0);
	}

	char * server 	= argv[1];			// first argument (second in list) is the server ip
	int port 		= atoi( argv[2] );	// second argument (3rd in list) is the port number

	char * username[40];
	char * password[40];

	printf( "Username: " );
	Fgets( username, 40, stdin );

	printf( "Password: " );
	Fgets( password, 40, stdin );

	// open up a connection with the server
	int connection = Open_clientfd( server, port );
	rio_t io;
	rio_readinitb( &io, connection );

	// now send the username and password data to the server
	rio_written( connection, username, strlen(username) );
	rio_written( connection, password, strlen(password) );

	// if the server rejects the connection, it will send back a response
	char * response[128];
	Rio_readlineb( &io, response, 128 );

	printf( "%s", response ); // print the response the server returned to the user

	bool login_approved = false;
	if( strcmp(response, "Login Approved\n") == 0 ){
		login_approved = true;
	}
	char * input[128];
	while( login_approved ){
		printf("rrsh> "); // prompt the user for input (commands)
		Fgets( input, 128, stdin ); // read that input
		int compare = strcmp( input, "quit\n" );
		if( compare == 0 ){ // if the user wants to quit, quit
			break;
		}
		else {
			compare = strcmp( input, "\n" ); // empty line, don't execute
			if( !(compare == 0) ){ // send commands that aren't empty lines to the server for execution
				rio_written( connection, input, strlen(input) );
				rio_readlineb( &io, response, MAXLINE );
				printf( "%s", response );
			}
		}
	}
	close( connection );
}