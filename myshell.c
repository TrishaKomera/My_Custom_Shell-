/********************************************************************************************
This is a template for assignment on writing a custom Shell. 

Students may change the return types and arguments of the functions given in this template,
but do not change the names of these functions.

Though use of any extra functions is not recommended, students may use new functions if they need to, 
but that should not make code unnecessorily complex to read.

Students should keep names of declared variable (and any new functions) self explanatory,
and add proper comments for every logical step.

Students need to be careful while forking a new process (no unnecessory process creations) 
or while inserting the single handler code (should be added at the correct places).

Finally, keep your filename as myshell.c, do not change this name (not even myshell.cpp, 
as you not need to use any features for this assignment that are supported by C++ but not by C).
*********************************************************************************************/
/* BT20CSE074   ---- Tarun Landa  */
#include <stdio.h>
#include <string.h>
#include <sys/wait.h> // wait()
#include <stdlib.h>	  // exit()
#include <fcntl.h>	  // close(), open()
#include <unistd.h>	  // fork(), getpid(), exec()
#include <signal.h>	  // signal()

#define NumberOfTokens 200

void parseInput(char *input_command, char *command_tokens[NumberOfTokens], int *isParallel, int *isSequential, int *isRedirected);
void executeCommand(char *command_tokens[NumberOfTokens]);
void executeParallelCommands(char *command_tokens[NumberOfTokens]);
void executeSequentialCommands(char *command_tokens[NumberOfTokens]);
void executeCommandRedirection(char *command_tokens[NumberOfTokens]);


void parseInput(char *input_command, char *command_tokens[NumberOfTokens], int *isParallel, int *isSequential, int *isRedirected)
{
    // This function will parse the input string into multiple commands or a single command with arguments depending on the delimiter (&&, ##, >, or spaces).
	char *token;
	int i = 0;
	// Extracting tokens by spaces
	while((token = strsep(&input_command, " ")) != NULL)
	{
		command_tokens[i] = token;
		// Setting respective flags if tokens match
		if (strcmp(command_tokens[i], "##") == 0) *isSequential = 1;
		if (strcmp(command_tokens[i], "&&") == 0) *isParallel = 1;
		if (strcmp(command_tokens[i], ">") == 0) *isRedirected = 1;
		i++;
	}
	command_tokens[i - 1] = strsep(&command_tokens[i - 1], "\n");//to remove \n character from command_tokens at the last character.
	command_tokens[i] = NULL;//Adding NULL to for no option commands and to iterate through multiple command tokens.
}
/*parseinput function takes input as user command string and also pointers to where we need to parse and store data
tokens as a 2d char array to store tokens seperated from input_command by delimiter " " using strsep function
## or && or > token is searched, if found its respective flag is set for appropriate execution in main.*/

void executeCommand(char *command_tokens[NumberOfTokens])
{
    // This function will fork a new process to execute a command
	if(strcmp(command_tokens[0], "cd") == 0)// No forking for cd command, using chdir directory is changed.
		{
			if(command_tokens[1] != NULL) 
			{
				int status = chdir(command_tokens[1]);
				if(status == -1)
				{
					printf("bash: cd: %s: No such file or directory\n",command_tokens[1]);
				}
			}
			// To go root home directory on cd command
			else 
			{
				chdir(getenv("HOME"));//chdir will change path to HOME.
			}
		}
	else
	{
		int id = fork();
		if(id < 0)//case when fork failed(happens in very rare case )
		{
			exit(1);
		}
		else if(id == 0)//new child process will go through this loop if fork is succecced
		{
			// restoring signals to default handling
			signal(SIGINT, SIG_DFL);
			signal(SIGTSTP, SIG_DFL);

			execvp(command_tokens[0], command_tokens);
			// Commands under execvp run only if Error from execvp else no.
			printf("Shell: Incorrect command\n");
			exit(1);
		}
		else//parent process will go through this loop
		{
			int status;
			int id_wait = waitpid(WAIT_ANY,&status,WUNTRACED);
			if(WIFSTOPPED(status))
			{
				if(WSTOPSIG(status)==SIGTSTP) kill(id_wait,SIGKILL);// SIGTSTP is manually handled to terminate process from stopped mode using SIGKILL
			}
		}
	}
}

void executeParallelCommands(char *command_tokens[NumberOfTokens])
{
	// This function will run multiple commands in parallel
	int i = 0;
	// iterating over all tokens 
	while(command_tokens[i] != NULL)
	{
		char *individual_command_tokens[NumberOfTokens];
		int j = 0;
		//creating seperate command tokens to execute according to the style of shell
		while(command_tokens[i] != NULL && strcmp(command_tokens[i], "&&") != 0)
		{
			individual_command_tokens[j++] = command_tokens[i++];
		}
		if(command_tokens[i]!=NULL) i++;
		individual_command_tokens[j] = NULL;
		//no forking needed for cd command,using chdir directory is changed.
		if(strcmp(individual_command_tokens[0], "cd") == 0)
		{
			if(individual_command_tokens[1] != NULL) 
			{
				int status = chdir(individual_command_tokens[1]);
				if(status == -1)
				{
					printf("bash: cd: %s: No such file or directory\n",individual_command_tokens[1]);
				}
			}
			// To go root home directory on cd command
			else 
			{
				chdir(getenv("HOME"));//chdir will change path to HOME.
			}
		}
		else
		{
			int id = fork();
			if(id < 0)//case when fork failed(happens in very rare case )
			{
				exit(1);
			}
			else if(id == 0)
			{
				// restoring signals to default handling
				signal(SIGINT, SIG_DFL);
				signal(SIGTSTP, SIG_DFL);

				execvp(individual_command_tokens[0], individual_command_tokens);
				// Commands under execvp run only if Error from execvp else no.
				printf("Shell: Incorrect command\n");
				exit(1);
			}
		}
	}
	// we have to wait for parent to execute untill all parallel child processes terminate.
	int status;
	int id_wait;
	while ((id_wait = waitpid(WAIT_ANY,&status,WUNTRACED)) > 0) 
	{
		if(WIFSTOPPED(status))
		{
			if(WSTOPSIG(status)==SIGTSTP) kill(id_wait,SIGKILL);// SIGTSTP is manually handled to terminate process from stopped mode using SIGKILL
		}
	}
}

/* This Function is to execute multiple commands with/without options in sequential way seperated by token ##.
Parent waits untill childs termination gurantees serial execution of commands from user input. */

void executeSequentialCommands(char *command_tokens[NumberOfTokens])
{
	// This function will run multiple commands in parallel
	int i = 0;
	while (command_tokens[i] != NULL)
	{
		char *individual_command_tokens[NumberOfTokens];
		int j = 0;
		//creating seperate command tokens to execute according to the style of shell
		while(command_tokens[i] != NULL && strcmp(command_tokens[i], "##") != 0)
		{
			individual_command_tokens[j++] = command_tokens[i++];
		}
		if(command_tokens[i]!=NULL) i++;
		individual_command_tokens[j] = NULL;
		// No forking for cd command, using chdir directory is changed.
		if(strcmp(individual_command_tokens[0], "cd") == 0)
		{
			if(individual_command_tokens[1] != NULL) 
			{
				int status = chdir(individual_command_tokens[1]);
				if(status == -1)
				{
					printf("bash: cd: %s: No such file or directory\n",individual_command_tokens[1]);
				}
			}
			// To go root home directory on cd command
			else 
			{
				chdir(getenv("HOME"));//chdir will change path to HOME.
			}
		}
		else if(strcmp(individual_command_tokens[0], "exit") == 0)
		{
			printf("Exiting shell...\n");
			exit(0);
		}
		else
		{
			int id = fork();
			if(id < 0)//case when fork failed(happens in very rare case )
			{ 
				exit(1);
			}
			// new child process
			else if(id == 0)//new child process will go through this loop if fork is succecced
			{ 
				// restoring signals to default handling
				signal(SIGINT, SIG_DFL);
				signal(SIGTSTP, SIG_DFL);

				execvp(individual_command_tokens[0], individual_command_tokens);
				// Commands under execvp run only if Error from execvp else no.
				printf("Shell: Incorrect command\n");
				exit(1);
			}
			else//parent process will go through this loop
			{
                // to not to be deterministic in nature will call wait in the parent process so that until last child process exits
				// waiting helps us in executing commands in serial as parent has to wait
				// for last child to terminate before executing the next command in sequence.
				int status;
				int id_wait = waitpid(WAIT_ANY,&status,WUNTRACED);
				// SIGINT terminates child process, so we handle it here to return from function
				// to avoid execution of next commands in sequence
				if(WIFSIGNALED(status))
				{
					if(WTERMSIG(status)==SIGINT) return;
				}
				// SIGTSTP is manually handled to terminate process from stopped mode using SIGKILL,
				// control is returned and next commands in sequence wont execute
				else if(WIFSTOPPED(status))
				{
					if(WSTOPSIG(status)==SIGTSTP)
					{
						kill(id_wait,SIGKILL);
						return;
					}
				}
			}
		}
	}
}

/* This Function is to execute only single commands with/without options with redirection
New child is forked, File descriptors are changed to output to a file as per command token. */

void executeCommandRedirection(char *command_tokens[NumberOfTokens])
{
	char *individual_command_tokens[NumberOfTokens];
	int i = 0, j = 0;
	// Extract command to be executed (tokens before > )
	while(strcmp(command_tokens[i], ">") != 0)
	{
		individual_command_tokens[j++] = command_tokens[i++];
	}
	i++;
	individual_command_tokens[j] = NULL;
	// No forking for cd command, using chdir directory is changed.
	if(strcmp(individual_command_tokens[0], "cd") == 0)
	{
			if(individual_command_tokens[1] != NULL) 
			{
				int status = chdir(individual_command_tokens[1]);
				if(status == -1)
				{
					printf("bash: cd: %s: No such file or directory\n",individual_command_tokens[1]);
				}
			}
			// To go root home directory on cd command
			else 
			{
				chdir(getenv("HOME"));//chdir will change path to HOME
			}
	}
	else
	{
		int id = fork();
		if(id < 0)//case when fork failed(happens in very rare case )
		{
			exit(1);
		}
		else if(id > 0)//parent process will go through this loop
		{
			int status;
			int id_wait = waitpid(WAIT_ANY,&status,WUNTRACED);
			if(WIFSTOPPED(status))
			{
				if(WSTOPSIG(status)==SIGTSTP) kill(id_wait,SIGKILL);// SIGTSTP is manually handled to terminate process from stopped mode using SIGKILL
			}
		}
		else//new child process will go through this loop if fork is succecced
		{
			// Restoring signals to default handling
			signal(SIGINT, SIG_DFL);
			signal(SIGTSTP, SIG_DFL);

			// Opening file for redirection and closing standard ouput
			close(STDOUT_FILENO);
			open(command_tokens[i], O_CREAT | O_WRONLY | O_APPEND);

			execvp(individual_command_tokens[0], individual_command_tokens);
			// Commands under execvp run only if Error from execvp else no.
			printf("Shell: Incorrect command\n");
			exit(1);
		}
	}
}

int main()
{
	// Ignoring Signals
	signal(SIGCHLD,SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);

	char *currentWorkingDirectoryPath;
	char *command_tokens[NumberOfTokens];
	char *input_command;
	size_t len = 0;

	while(1)
	{
		/* Print the prompt in format - currentWorkingDirectory$
		glibc's getcwd() : allocates the buffer dynamically, In this case size is zero, so buf is allocated as big as necessary.  */
		currentWorkingDirectoryPath = getcwd(NULL, 0);
		printf("%s$", currentWorkingDirectoryPath);
		free(currentWorkingDirectoryPath);

		// accept input with 'getline()'
		getline(&input_command, &len, stdin);

		int isParallel = 0, isSequential = 0, isRedirected = 0;

		// Parse input with 'strsep()' for different symbols (&&, ##, >) and for spaces.
		parseInput(input_command, command_tokens, &isParallel, &isSequential, &isRedirected);

		// When user uses exit command.
		if(strcmp(command_tokens[0], "exit") == 0)
		{
			printf("Exiting shell...\n");
			break;
		}

		// To execute user commands based on cases
		if(command_tokens != NULL && command_tokens[0] != NULL && strcmp(command_tokens[0],"") != 0)
		{
			// This function is invoked when user wants to run multiple commands sequentially (commands separated by ##)
			if(isSequential == 1)
			{
				executeSequentialCommands(command_tokens); 
			}
			// This function is invoked when user wants to run multiple commands in parallel (commands separated by &&)
			else if(isParallel == 1)
			{
				executeParallelCommands(command_tokens);
			}
			// This function is invoked when user wants redirect output of a single command to and output file specificed by user
			else if(isRedirected == 1)
			{
				executeCommandRedirection(command_tokens); 
			}
			// This function is invoked when user wants to run a single commands
			else
			{
				executeCommand(command_tokens); 
			}	
		}
	}
	return 0;
}
