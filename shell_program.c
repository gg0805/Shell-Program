#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_LINE 80 
#define MAX_ARGS (MAX_LINE / 2 + 1) 

char *history = NULL; 
int history_available = 0; 


void parse_command(char *input, char **args) {
    int i = 0;
    for (i = 0; i < MAX_ARGS; i++) {
        args[i] = strsep(&input, " ");
        if (args[i] == NULL) break;  
        if (strlen(args[i]) == 0) i--; 
    }
}


int is_background(char **args) {
    int i = 0;
    while (args[i] != NULL) {
        if (strcmp(args[i], "&") == 0) {
            args[i] = NULL; 
            return 1;
        }
        i++;
    }
    return 0;
}


int has_redirection(char **args, char **file, int *redir_type) {
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], ">") == 0) {
            *redir_type = 1; 
            *file = args[i + 1];
            args[i] = NULL; 
            return 1;
        } else if (strcmp(args[i], "<") == 0) {
            *redir_type = 0; 
            *file = args[i + 1];
            args[i] = NULL; 
            return 1;
        }
    }
    return 0;
}


void execute_command(char **args, int background, char *input_file, char *output_file) {
    pid_t pid = fork();

    if (pid == 0) { 
        
        if (input_file) {
            int fd = open(input_file, O_RDONLY);
            if (fd < 0) {
                perror("Failed to open input file");
                exit(1);
            }
            dup2(fd, STDIN_FILENO); 
            close(fd);
        }

        
        if (output_file) {
            int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                perror("Failed to open output file");
                exit(1);
            }
            dup2(fd, STDOUT_FILENO); 
            close(fd);
        }

        
        if (execvp(args[0], args) == -1) {
            perror("exec failed");
            exit(1);
        }
    } else if (pid > 0) { 
        if (!background) {
            wait(NULL); 
        }
    } else {
        perror("fork failed");
    }
}


void execute_pipe(char **first_command, char **second_command) {
    int fd[2];
    if (pipe(fd) == -1) {  
        perror("Pipe failed");
        exit(1);
    }

    pid_t pid1 = fork();
    if (pid1 == -1) {
        perror("fork failed");
        exit(1);
    }

    if (pid1 == 0) {  
        close(fd[0]);  
        dup2(fd[1], STDOUT_FILENO);  
        close(fd[1]);

        if (execvp(first_command[0], first_command) == -1) {
            perror("exec failed for first command");
            exit(1);
        }
    }

    pid_t pid2 = fork();
    if (pid2 == -1) {
        perror("fork failed");
        exit(1);
    }

    if (pid2 == 0) {  
        close(fd[1]);  
        dup2(fd[0], STDIN_FILENO);  
        close(fd[0]);

        if (execvp(second_command[0], second_command) == -1) {
            perror("exec failed for second command");
            exit(1);
        }
    }

    
    close(fd[0]);
    close(fd[1]);

    wait(NULL);  
    wait(NULL);
}

int main(void) {
    char *args[MAX_ARGS]; 
    int should_run = 1;   

    while (should_run) {
        printf("osh> ");
        fflush(stdout);

        char input[MAX_LINE];
        fgets(input, MAX_LINE, stdin);

        
        input[strcspn(input, "\n")] = '\0';

        
        if (strcmp(input, "exit") == 0) {
            should_run = 0;
            continue;
        }

        
        if (strcmp(input, "!!") == 0) {
            if (!history_available) {
                printf("No commands in history.\n");
                continue;
            }
            strcpy(input, history); 
            printf("osh> %s\n", input);   
        } else {
            
            if (history != NULL) {
                free(history);
            }

            
            history = (char *)malloc((strlen(input) + 1) * sizeof(char));
            if (history == NULL) {
                perror("malloc failed");
                continue;
            }
            strcpy(history, input);
            history_available = 1;
        }

        
        char *pipe_pos = strchr(input, '|');
        if (pipe_pos) {
            *pipe_pos = '\0';  
            pipe_pos++;  

            char *first_command[MAX_ARGS];
            char *second_command[MAX_ARGS];

            parse_command(input, first_command);  
            parse_command(pipe_pos, second_command);  

            
            execute_pipe(first_command, second_command);
            continue;
        }

        
        parse_command(input, args);

        
        int background = is_background(args);

        
        char *file = NULL;
        int redir_type = -1; 
        if (has_redirection(args, &file, &redir_type)) {
            if (redir_type == 1) { 
                execute_command(args, background, NULL, file);
            } else if (redir_type == 0) { 
                execute_command(args, background, file, NULL);
            }
        } else {
            
            execute_command(args, background, NULL, NULL);
        }
    }

    
    if (history != NULL) {
        free(history);
    }

    return 0;
}
