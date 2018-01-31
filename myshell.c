#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */
#define BUFFER_SIZE 50

static char buffer[BUFFER_SIZE];

struct hiComm{
	int commNumb;
	char userInput[MAX_LINE];
};

static struct hiComm historyA[10];
static int historyLength;

/* the signal handler function */
void handle_SIGQUIT() {
      write(STDOUT_FILENO,buffer,strlen(buffer));
	int x;
	char toPrint[90] = "";
	for(x = 0; x < historyLength; ++x){
		char numb[32];
		sprintf(numb, "%d", historyA[x].commNumb);
		strcat(toPrint, numb);
		strcat(toPrint, ")  ");
		strcat(toPrint, historyA[x].userInput);
		write(STDOUT_FILENO,toPrint,strlen(toPrint));
		write(STDOUT_FILENO,"\n",strlen("\n"));
		strcpy(toPrint, "");
	}
}


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

/**
Pushes all the history structs back one so the latest struct can be put in the correct position
*/
void pushBack(){
	int x;
	for(x = 1; x < 10; ++x){
		historyA[x-1].commNumb = historyA[x].commNumb;
		strcpy(historyA[x-1].userInput, historyA[x].userInput);
	}
}

/**
Merges the array of string pointers back into the original command so that it may be stored in
the history structure
*/
void argsMerger(char * toMerge[], char finalString[]){
	strcpy(finalString, "");
	int count = 0;
	while(toMerge[count] != NULL){
		strcat(finalString, toMerge[count]);
		strcat(finalString, " ");
		++count;
	}
	
}


int main(void)
{


/* set up the signal handler */
      struct sigaction handler;
      handler.sa_handler = handle_SIGQUIT;
      handler.sa_flags = SA_RESTART;
      sigaction(SIGQUIT, &handler, NULL);

      strcpy(buffer,"History : \n");
	
char inputBuffer[MAX_LINE];      /* buffer to hold the command entered */
    int background;              /* equals 1 if a command is followed by '&' */
    char *args[(MAX_LINE/2)+1];  /* command line (of 80) has max of 40 arguments */
 
	historyLength = 0;
	char finalString[90] = "";
    int promptNumber = 1;
    int pid = getpid();
    printf("\nWelcome to ASshell. My pid is %d\n", pid);

    while (1){            
       background = 0;
       printf("\n ASshell[%d]:\n", promptNumber);
       setup(inputBuffer,args,&background);       /* get next command */
	argsMerger(args, finalString);
	if(historyLength > 9){
		pushBack();
		historyA[9].commNumb = promptNumber;
		strcpy(historyA[9].userInput, finalString);
	}else{
		historyA[historyLength].commNumb = promptNumber;
		strcpy(historyA[historyLength].userInput, finalString);
		++historyLength;
	}
	++promptNumber;

       if(strcmp(args[0], "yell") == 0){
	 yeller(args);
       } else
	if(strcmp(args[0], "exit") == 0){
		exiter();
	}else {
	pid_t isChild = fork();
	char *toPrint;
	if(isChild != 0){
		if(background == 1){
			toPrint = "TRUE";
		}else{
			toPrint = "False";
		}
		printf("[Child pid = %d, background = %s]\n", isChild, toPrint);
	}else{
		execvp(args[0], args);
		exit(0);
	}
	if(background == 0){
		int status = 0;
		waitpid(isChild, &status, 0);
		printf("Child process complete\n");
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
