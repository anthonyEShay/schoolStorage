#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */

 
/**
 * setup() reads in the next command line, separating it into distinct tokens
 * using whitespace as delimiters. setup() sets the args parameter as a
 * null-terminated string.
 */

void setup(char inputBuffer[], char *args[],int *background)
{
    int length, /* # of characters in the command line */
        i,      /* loop index for accessing inputBuffer array */
        start,  /* index where beginning of next command parameter is */
        ct;     /* index of where to place the next parameter into args[] */
   
    ct = 0;

    /* read what the user enters on the command line */
    length = read(STDIN_FILENO, inputBuffer, MAX_LINE); 

    start = -1;
    if (length == 0)
        exit(0);            /* ^d was entered, end of user command stream */
    if (length < 0){
        perror("error reading the command");
        exit(-1);           /* terminate with error code of -1 */
    }

    /* examine every character in the inputBuffer */
    for (i=0;i<length;i++) {
        switch (inputBuffer[i]){
          case ' ':
          case '\t' :               /* argument separators */
            if(start != -1){
                    args[ct] = &inputBuffer[start];    /* set up pointer */
                ct++;
            }
            inputBuffer[i] = '\0'; /* add a null char; make a C string */
            start = -1;
            break;
          case '\n':                 /* should be the final char examined */
            if (start != -1){
                    args[ct] = &inputBuffer[start];    
                ct++;
            }
                inputBuffer[i] = '\0';
                args[ct] = NULL; /* no more arguments to this command */
            break;
          default :             /* some other character */
            if (start == -1)
                start = i;
            if (inputBuffer[i] == '&'){
                *background  = 1;
                start = -1;
                inputBuffer[i] = '\0';
            }
          }
     }   
     args[ct] = NULL; /* just in case the input line was > 80 */
}

/**
Converts all characters in a string to uppercase and returns it
*/
char* compToUpper(char toEdit[]){
	int counter = 0;
	while(toEdit[counter] != '\0'){
		toEdit[counter] = toupper(toEdit[counter]);
		++counter;
	}
	return toEdit;
}

/**
Prints out all arguments except the first to stdout, but in all caps
*/
void yeller(char *args[]){
	int count = 1;
	while(args[count] != NULL){
		printf("%s ", compToUpper(args[count]));
		++count;
	}
	printf("\n");
}

/**
Prints out some statistics to stdout and then exits
*/
void exiter(){
	char systemPrint[] = "ps -p ";
	char pidNumb[32];
	sprintf(pidNumb, "%d", getpid() );
	strcat(systemPrint, pidNumb);
	strcat(systemPrint, " -o pid,ppid,pcpu,pmem,etime,user,command");
	system(systemPrint);
	exit(0);
}

int main(void)
{
	
char inputBuffer[MAX_LINE];      /* buffer to hold the command entered */
    int background;              /* equals 1 if a command is followed by '&' */
    char *args[(MAX_LINE/2)+1];  /* command line (of 80) has max of 40 arguments */
 
    int promptNumber = 1;
    int pid = getpid();
    printf("\nWelcome to ASshell. My pid is %d\n", pid);

    while (1){            /* Program terminates normally inside setup */
       background = 0;
       printf(" ASshell[%d]:\n", promptNumber);
       ++promptNumber;
       setup(inputBuffer,args,&background);       /* get next command */

       if(strcmp(args[0], "yell") == 0){
	 yeller(args);
       } else
	if(strcmp(args[0], "exit") == 0){
		exiter();
	}else {
	pid_t isChild = fork();
	if(isChild == 0){
		execvp(args[0], NULL);
		exit(0);
	}
	}

      /* the steps are:
       (0) if built-in command, handle internally
       (1) if not, fork a child process using fork()
       (2) the child process will invoke execvp()
       (3) if background == 0, the parent will wait,
            otherwise returns to the setup() function. */
    }
}
