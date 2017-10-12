#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/types.h>
#include <fcntl.h> //for solaris
      

#define MAXARGS 100 // a variable which is from measuring the array arguments and can be changed from here 
int split_pipes(char *buffer, char **arguments) { //this function tokenize the string buffer as a pipe and put the results into arguments, through arg array.If the string is not a pipe (|) nothing happens and the string stays the same.
    int n=0;
    // Cut apart based on pipes and files equally.  Also remove \r and \n from end of line
    char *arg=strtok(buffer, "|\r\n");
    while(arg != NULL)
    {
        arguments[n++]=arg;
        arg=strtok(NULL, "|");        //this while is a typical way of tokenizing strings with strtok function
	
    }
    
    return(n);
}


int create_process(char *part, int  pipes[][2], int pipenum) { // in this function is where the process of fork and having children happens.Takes appointed an array of the parts of the command, one double array pipes where is (64x2)(until 64 pipes,2 positions each pipe read and write)

    void (*Fixer)(); //two functions Fixer and SuperFixer which are responsible for control the incoming signals ctrl C and term.
    void (*SuperFixer)();
    int pid;
    char *arguments[MAXARGS];
    int argc=0, n;
    char *arg=strtok(part, " \t\n"); //i tokenize the array part in spaces mainly to extract the initial command from its arguments and have separately. 
    
    while(arg != NULL)
    {
        arguments[argc++]=arg;
        arg=strtok(NULL, " \t\n"); 
	
    }

    arguments[argc++]=NULL; //putting NULL at EOF to make sure that the commands will work and not have an infinite loop when i execute them.

    for(n=0; arguments[n] != NULL; n++)
    {
        fprintf(stderr, "\t\targ %02d: %s\n", n, arguments[n]); //printing the arguments of every command in stdout , for better understanding of this program.
    }
    //fork
    pid = fork();
    //Error checking to see if fork works        
    if (pid == -1) {
           perror("fork");
           	exit(1);
    }
    //kill(pid,SIGTERM); //this is in a comment, is to check that with the signal term the program doesn't stop here and ignores it.
    if (pid != 0) {  /* parent process */
	
	printf("Parent Process: PID=%d \n", getpid());         				
			
    }else { /*child process */
        int m;
	int counter=0; //a counter to count the index of arguments array
	int fdOut;
	printf("Child Process: PID=%d, PPID=%d \n",getpid(),getppid());
	printf("\n");
	signal(SIGINT,Fixer); // fixing the ignored signals (which i have ignored in main ) cause we are inside a child.
	signal(SIGTERM,SuperFixer);     
	while(arguments[counter] != NULL){	//looping all the arguments array 
		if(strcmp(arguments[counter], ">") == 0) { // if in arguments i found (>) this symbol,meanss that the user wants to type the command into a file
         		if(arguments[counter+1] != NULL) { //if the next index of > is not EOF
         			fdOut = open(arguments[counter+1], O_CREAT | O_WRONLY | O_TRUNC, 0600); // i open a file with the name of the counter+1 (counter == >) 
           			dup2(fdOut, STDOUT_FILENO); // and with dup2 function i redirect the output to a file from stdout
         		}	
         		else {
           		printf("No output file specified"); // if counter+1 is EOF the user hasn't type any file so error
         		}
         		arguments[counter] = 0;
       		}
	
       		counter++; //i increase the number of counter to get to the next index 
		   
		//while(1){ //again this two comments are for controling the incoming signals and act depending the situation
		//kill(pid,SIGTERM);	

		// now we are in pipe position. If there is possibility(|) in the array pipes if the elements that has in the first index is >= 0 i use dup2 to redirect the reading , and i do the same with output.The creations of pipes happens in main function.
		//Otherwise if pipes are still -1 it means that no (|) symbol has been spotted and nothing happens from pipes procpective.

		if(pipes[pipenum][STDIN_FILENO] >= 0) { 
			dup2(pipes[pipenum][STDIN_FILENO], STDIN_FILENO); // FD 0
		}
        	if(pipes[pipenum][STDOUT_FILENO] >= 0) {
			dup2(pipes[pipenum][STDOUT_FILENO], STDOUT_FILENO); // FD 1
		}
	
        	// Close all pipes
        	for(m=0; m<64; m++) {
        
            		if(pipes[m][STDIN_FILENO] >= 0) {
		 		close(pipes[m][STDIN_FILENO]);
			}
            		if(pipes[m][STDOUT_FILENO] >= 0) {
				close(pipes[m][STDOUT_FILENO]);
			}       
	 	}
		//}
	}
        execvp(arguments[0], arguments); //execute the actual command
        fprintf(stderr, "COMMAND NOT FOUND\n");
        exit(404);
    }

    return(pid);
}

int main(void)
{
    pid_t children[64];  //these four array is as some above.Children holds how many children are created and in every child its PID
    int pipes[64][2]; 	//for pipes see above
    char *partsOfCmds[64]; //this array is the same as array part
    char buffer[BUFSIZ]; 
    int linenum=0; //a variable that counts in what line we in.Lines i call how many times the program will reappeal the prompt
    void (*Fixer)();
    void (*SuperFixer)();
    while(1){ // a loop that only stops when the user type close in prompt
   	char *log;// an array that holds the login name of the user
	log=getlogin();
	fprintf(stdout,"\n%s:#-> ",log);       //print out the actuall prompt   
   	//get input
        fgets(buffer,BUFSIZ,stdin);              /*   read in the command line     */
    	Fixer = signal(SIGINT,SIG_IGN);//ignore these two signals if they appear
	SuperFixer = signal(SIGTERM,SIG_IGN);
        int m, n;
        n=split_pipes(buffer, partsOfCmds); // Split apart line into 'parts'
	
       
	if(strcmp(buffer,"close") == 0) { // the only way to stop the loop while
		exit(1);
	}
        // Clear out any pipes from last loop
        for(m=0; m<64; m++) {
          pipes[m][STDIN_FILENO] = - 1; 
	  pipes[m][STDOUT_FILENO] = -1;
        }

        fprintf(stderr, "Line %d\n", ++linenum); //print in what line we are
	
        // For n processes, create n-1 pipes // i prefer to use STDOUT_FILENO and STDIN_FILENO rather than 1 and 0. I have searched and i 
        for(m=0; m<(n-1); m++) {									//found that its better.
            int p[2];
            pipe(p);
            pipes[m][STDOUT_FILENO] = p[STDOUT_FILENO]; // Process N writes to the pipe
            pipes[m+1][STDIN_FILENO] = p[STDIN_FILENO]; // Process N+1 reads from the pipe
        }
	
		
       	for(m=0; m<n; m++) { // if there is no pipe in the command the program run from here and execute normaly the given command.
            
		fprintf(stderr, "\tpart %02d: %s\n", m, partsOfCmds[m]);
	        children[m]=create_process(partsOfCmds[m], pipes, m);
	           
	}

        // Close all pipes
        for(m=0; m<64; m++) {
        
            	if(pipes[m][STDIN_FILENO] >= 0) {
			close(pipes[m][STDIN_FILENO]);
	    	}	
           	if(pipes[m][STDOUT_FILENO] >= 0) {
			close(pipes[m][STDOUT_FILENO]);
	    	}        
	}

        for(m=0; m<n; m++) {
               	int status;
		/*wait for every child to exit*/
      		waitpid(children[m], &status, 0);
	      	fprintf(stderr, "Child %d with PID : %d terminated :  exit code=%d\n",m,children[m], status>>8);
       	 }	 	
              
			        
    }	    
}

