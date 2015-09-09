#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <pwd.h>
#include <fcntl.h>
#include <sys/types.h>

#define buffer_size 1024
#define token_size 64
#define token_delim " \n\t\r\a"
#define IN 1
#define OUT 2

int shelly_cd (char **args);
int shelly_help (char **args);
int shelly_exit (char **args);
int shelly_history (char **args);
int shelly_easter_eggs (char **args);

int redirect;
int his_pos;

char cmd_history[500][100];

char *builtin_str[] = {"cd","help","history","exit"};

int (*builtin_func[]) (char**) = {&shelly_cd,&shelly_help,&shelly_history,&shelly_exit};

int num_builtins () {
	return sizeof(builtin_str) / sizeof(char*);
}

int shelly_cd (char **args) {
	if (args[1] == NULL) {
		fprintf(stderr,"shelly: excpected argument to \"cd\"\n");
	}
	else {
		if (chdir(args[1]) != 0) {
			perror("shelly");
			printf("\n");
		}
	}
	return 1;
}

int shelly_help (char **args) {
	printf("shelly by Nalin Gupta\n");
	printf("The following are the builtin functions:\n");
	for (int i=0;i<num_builtins();i++) {
		printf(" %s\n",builtin_str[i]);
	}
	return 1;
}

int shelly_exit (char **args) {
	return 0;
}

int shelly_history (char **args) {
	int in=0;
	if (his_pos>500) {
		in = his_pos - 500;
	}
	for (int i=in;i<his_pos-1;i++) {
		printf(" %d  %s",i+1,cmd_history[i]);
	}
	return 1;
}

void add_history (char *line) {
	strcpy(cmd_history[his_pos], line);
	his_pos++;
}

void signal_handler (int sig) {
	signal(SIGINT,signal_handler);
	printf("\nSHELLY> ");
	fflush(stdout);
}

char *get_username () {
	register struct passwd *pw;
	register uid_t uid;

	uid = geteuid ();
	pw = getpwuid (uid);

	if (pw)
	{
		return pw->pw_name;
	}

	fprintf (stderr,"shelly: cannot find username for UID %u\n",(unsigned) uid);
	exit (EXIT_FAILURE);
}

char *shelly_read_line () {
	char *line = NULL;
	ssize_t buffer = 0;
	getline(&line, &buffer, stdin);
	return line;
}

char **shelly_split_line (char *line) {
	int buffer = token_size;
	int pos =0;
	char **tokens = malloc(buffer * sizeof(char*));
	char *token;

	redirect = 0;

	if (!tokens) {
		fprintf(stderr, "shelly: Allocation error\n");
		return (EXIT_FAILURE);
	} 

	token = strtok(line, token_delim);
	while (token != NULL) {
		tokens[pos] = token;
		pos++;

		if (pos >= token_size) {
			buffer += buffer;
			tokens = realloc(tokens,buffer*sizeof(char*));

			if (!tokens) {
				fprintf(stderr, "shelly: Allocation error\n");
				return (EXIT_FAILURE);
			} 
		}
		token = strtok(NULL, token_delim);
	}
	tokens[pos] = NULL;
	return tokens;
}

int shelly_launch (char **args) {
	pid_t pid, wpid;
	int status;

	pid = fork();
	if (pid==0) {
		if (execvp(args[0],args)==-1) {
			perror("shelly");
			printf("\n");
		}
		return (EXIT_FAILURE);
	}
	else if (pid < 0 ) {
		perror("shelly");
		printf("\n");
	}
	else {
		do {
			wpid = waitpid(pid, &status, WUNTRACED);
		} while (!WIFEXITED(status) && !WIFSIGNALED(status));
	}
	return 1;
}

int shelly_execute (char **args) {
	if (args[0] == NULL) {
		return 1;
	}

	for (int i=0;i<num_builtins();i++) {
		if (strcmp(args[0],builtin_str[i])==0){
			return (*builtin_func[i])(args);
		}
	}

	return shelly_launch(args);
}

void shelly_loop () {
	char *line;
	char **args;
	int status = 1;

	signal(SIGINT,signal_handler);
	printf("\n");
	do {
		printf("shelly: %s$ ", get_username());
		line = shelly_read_line();
		add_history (line);
		args = shelly_split_line(line);
		status = shelly_execute(args);

		free(line);
		free(args);
	} while(status);
}

int main(int argc, char *argv[]) {
	his_pos = 0;
	shelly_loop();
	return 0;
}