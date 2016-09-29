#include "csapp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// allow for booleans
typedef int bool;
#define true 1
#define false 0

// initialize some variables needed later
int port;
int listener;
int connection;

struct sockaddr_in address;

// declare functions used in main
void run_commands(rio_t io, int connection, char *username);

int main(int argc, char **argv) {

	// the port is the first argument passed, but a check needs to be made that it was in fact passed
	if( argc < 2 ){
		printf( "No port was specified\n" );
		exit(0);
	}
	else if( argc == 2 ){
		port = atoi( argv[1] );
	}

	// listen on the specified slot
	listener = open_listenfd( port );

	// we need the user's password and username from the client
	rio_t rio;
	rio_readinitb(&rio, connection);

	
	bool done = false;
	while( !(done) ){

		connection = Accept(listener, (SA *)&address, sizeof(address) );

		rio_t io; 		// struct for input/output
		// we need to ask the user to log in
		char username[40]; 	// the username can be up to 40 characters
		char password[40]; 	// same for password
		

		rio_readinitb(&io, connection); 	// initialize connection
		printf( "User: " );						// prompt for username, no newline
		rio_readlineb(&io, username, sizeof(username) ); 	// get username
		printf( "Password: " );					// prompt for password, no newline
		rio_readlineb(&io, password, sizeof(password) ); 	// get password

		// now we have credentials, we need to check them
		// user credentials are stored in 'rrshuser.txt'
		FILE * cred_file = fopen("rrshusers.txt", "r");  

		bool found_user = false;		// changed to 1 if the username matches
		bool found_pass = false; 	// changed to 1 if the username has a password match
		char pos_username[40];	// Possible username, per line
		char pos_password[40];	// Possible password, per line
		bool login_approved = false;	// changed to true if login was approved
				
    	while( fscanf(cred_file,"%s %s", &pos_username, &pos_password)>0 ) {
        	// now compare to see if the username and password
        	// passed to the server by the client is on this line 
        	// of the cred_file, and iterate through the whole
        	// file to see if the user is in the file
    		int compare = strcmp( pos_username, username );
    		if( compare == 0 ){
    			found_user = true;
    			compare = strcmp( pos_password, password );
    			if( compare == 0 ){
    				found_pass = true;
    				printf( "Login Approved\n" );
    				rio_writen(connection, "Login Approved\n", strlen("Login Approved\n"));
    			}
    		}
    	}
    	if( (found_user) && !(found_pass) ){
    		printf( "Login Failed, user exists, but that is an incorrect password\n" );
    		rio_writen( connection, "Login Failed, user exists, but that is an incorrect password\n", strlen("Login Failed, user exists, but that is an incorrect password\n") );
    	}
    	else {
    		printf( "Login Failed\n" );
    		rio_writen( connection, "Login Failed\n", strlen("Login Failed\n") );
    	}

    	// execute commands if the login was approved
    	run_commands(io, connection, username);

    	close(connection);

    	done = true;
    }
    exit(0);
}

void run_commands(rio_t io, int connection, char *username) {
	// commands need to be checked against this file before running
	FILE * command_file = fopen("rrshusers.txt", "r"); 
	// read lines from the rrsh client as needed
	char input[MAXLINE];
	while( rio_readlineb(&io, input, MAXLINE) > 0 ){
		// use the parser from parser.c in assignment 4
		struct command * current_command = parse_command( input );
		bool found_command = false; // whether the command is in the file or not

		if( current_command->out_redir != NULL || current_command->in_redir != NULL ){
			// we can't allow for input or output redirection for security reasons. Tell the client this
			rio_writen( connection, "Input and output redirection is not allowed\n", strlen("Input and output redirection is not allowed\n") ); 
		}
		else { // other commands are allowed to be run, if they are contained in 'rrshcommands.txt'
			
			char * pos_command;
			 
			while( fscanf(command_file,"%s",  &pos_command)>0 ) {
				int compare = strcmp( pos_command, current_command->args[0] );
				if( compare == 0 ){ // found the command
					found_command = true;
				}
			}
		}
		if( found_command ){
			pid_t pid = fork(); // fork to execute the command
			if( pid == 0 ){ // child process
				dup2( connection, 1 );
				execv( current_command->args[0], current_command->args );
				exit(0);
			}
			else if( pid > 0 ){ // parent process
				// do nothing for now
				int i = 0;
			}
			int status;
			waitpid( pid, &status, 0 );
			rio_writen( connection, "RRSH COMMAND COMPLETED\n", strlen("RRSH COMMAND COMPLETED\n") );
			found_command = false;
		}
		else {
			printf( "the command '%s' is not allowed.\n", input );
			rio_writen( connection, "Cannot execute\n", strlen("Cannot execute\n") );
			exit(0);
		}
	}
	fclose( command_file );
}