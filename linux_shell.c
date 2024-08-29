#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <limits.h>
#include <regex.h>
#include <stdbool.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#define TRUE 1
void start_shell();
char * get_dir(char *path);

void convert_to_argl(char *argl);

void get_argl(char *argl[], int i, char **token_input);

int tokenize_input(char **token_input, size_t input_size, char *input, char *delim);

int contains_cd_or_exit(char *input);

int contains_regex(char *input, char *delim);

int count_redirection(char **token_input, int k);

int get_symbol_pos(char **token_input, char *str, int k);

void execute_syscall(char *input);

bool has_cd_or_exit = false;

size_t input_size = 1000;


int main() {
    start_shell();
    return EXIT_SUCCESS;
}

void start_shell() {
    while(TRUE) {
        char *input;
        
        int loop;
        char **token_input_with_pipes;
        
        input = (char *)malloc(input_size*sizeof(char *));
        char *pth = (char *)malloc(1024*sizeof(char *));;
        int a_length;
        getcwd(pth,1024);
        char *dir = get_dir(pth);
        printf("[sh %s]$ ", dir);
        fflush(stdout);
        a_length = getline(&input, &input_size, stdin);
        if(a_length == -1) {
            if(feof(stdin)) {
                exit(0);
            }
            else {
                continue;
            }
        }
        if(*input == '|' || *input=='<' || *input=='>' || (*input=='>' && *(input+1)=='>')) {
                fprintf(stderr, "Error: invalid command\n");
                continue;
        }
        
        *(input+(a_length-1)) = '\0';
        if(contains_regex(input, "|")) {
            token_input_with_pipes = (char **) malloc(input_size*sizeof(char *)+1);
            loop = tokenize_input(token_input_with_pipes,input_size, input, "|");
            has_pipes = true;
            //printf("%d\n", loop);
        }
        else {
            loop = 1;
        }
        int fds[2*(loop-1)];
        char **token_input;
        int i;
        int no_loops;
        int c;
        for(c=0;c<loop-1;c++) {
            if(pipe(fds+c*2)<0) {
                exit(0);
            }
        }

        for(no_loops=0;no_loops<loop;no_loops++) {
            bool has_input_redir = false;
            bool has_output_redir = false;
            bool has_double_output_redir = false;
            if(has_pipes) {
                if(no_loops>0) {
                    input = *(token_input_with_pipes+no_loops);
                    input = input+1;
                }
                else {
                    input = *(token_input_with_pipes+no_loops);
                }
                if(no_loops != loop-1)
                    *(input + (strlen(input)-1)) = '\0';
                
            }
            
            if(contains_cd_or_exit(input)) {
                has_cd_or_exit = true;
            }



            token_input = (char **) malloc(input_size*sizeof(char *)+1);
            i = tokenize_input(token_input, input_size, input, " ");

            if(has_cd_or_exit) {
                if(strcmp(*token_input, "cd") == 0) {
                    if(i!=2) {
                        fprintf(stderr, "Error: invalid command\n");
                    }
                    else {
                        int err = chdir(*(token_input+1));
                        if(err == -1) {
                            fprintf(stderr, "Error: invalid directory\n");
                        }
                    }
                    
                }
                else if (strcmp(*token_input, "exit") == 0){
                    if(i!=1) {
                        fprintf(stderr, "Error: invalid command\n");
                    }
                    else {
                        exit(0);
                    }
                    

                }
                
                else if(strcmp(*token_input, "jobs") == 0) {

                }
                else if(strcmp(*token_input, "fg") == 0) {

                }
                has_cd_or_exit = false;
                continue;
            }
            int new_token_input;
            char **temp_token_input;
            int pos_input;
            int pos_output;
            if(contains_regex(input, ">") || contains_regex(input, "<") || contains_regex(input, ">>")) {
                if(count_redirection(token_input,i) == 1) {
                    fprintf(stderr, "Error: invalid command");
                }
                else {
                    if(i>3) {
                        fprintf(stderr, "Error: invalid command");
                        continue;
                    }
                    else {
                        
                        
                        if(contains_regex(input, "<")) {
                            pos_input = get_symbol_pos(token_input, "<",i);
                            if(pos_input+1==i) {
                                fprintf(stderr, "Error: invalid command\n");
                                continue;
                            }
                            has_input_redir = true;
                        }

                        if(contains_regex(input, ">")) {
                            if(contains_regex(input, ">>")) {
                                
                                pos_output = get_symbol_pos(token_input, ">>",i);
                                
                                has_double_output_redir = true;
                            }
                            else {
                                pos_output = get_symbol_pos(token_input, ">",i);
                                has_output_redir = true;
                            }
                            if(pos_output+1==i) {
                                    fprintf(stderr, "Error: invalid command\n");
                                    continue;
                            }
                        }
                        
                        temp_token_input = (char **) malloc(input_size*sizeof(char *)+1);
                        
                        for(new_token_input=0;new_token_input<i;new_token_input++) {
                            if(strcmp(*(token_input+new_token_input), ">") ==0|| strcmp(*(token_input+new_token_input), "<") ==0 || strcmp(*(token_input+new_token_input), ">>")==0)
                                break;
                            *(temp_token_input+new_token_input) = strdup(*(token_input+new_token_input));
                        }
                        *(temp_token_input+new_token_input) = NULL;
                    }
                }
        


            }

            
            char *argl[i];
            
            if(has_input_redir || has_output_redir || has_double_output_redir) {
                if(has_input_redir && (has_output_redir || has_double_output_redir))
                    execute_syscall(input, temp_token_input, new_token_input, has_input_redir, has_output_redir, has_double_output_redir, *(token_input+pos_input+1), *(token_input+pos_output+1), has_pipes, fds, no_loops, loop);
                else if(has_input_redir) 
                    execute_syscall(input, temp_token_input, new_token_input, has_input_redir, has_output_redir, has_double_output_redir, *(token_input+pos_input+1), NULL, has_pipes, fds, no_loops, loop);
                else
                    execute_syscall(input, temp_token_input, new_token_input, has_input_redir, has_output_redir, has_double_output_redir, NULL, *(token_input+pos_output+1), has_pipes, fds, no_loops, loop);
            }
            else {
                execute_syscall(input, token_input, i, has_input_redir, has_output_redir, has_double_output_redir, NULL, NULL, has_pipes, fds, no_loops, loop);
            }

        
        execute_syscall(input);
        
            
            
        
        free(input);
        free(pth);
        free(token_input);
        free(dir);
        
    }

}


char * get_dir(char *path) {
    int last_root = 0;
    if(strlen(path) == 1) {
        printf("%s\n", path);
        char *ret = malloc(10*sizeof(char));
        *ret = *path;
        *(ret+1) = '\0';
        return ret;
    }
    int i=0;
    while(*(path+i) != '\0') {
        if(*(path+i)=='/') {
            last_root=i;
        }
        i++;
    }
    char *ret = malloc(sizeof(char)*(i-last_root-1));
    last_root++;
    i=0;
    while(*(path+last_root) != '\0') {
        *(ret+i) = *(path+last_root);
        i++;
        last_root++;
    }

    return ret;


}

void get_argl(char *argl[], int i, char **token_input) {
    int j;
    for(j=0;j<=i;j++) {
        argl[j] = *(token_input+j);
    }
}

int tokenize_input(char **token_input, size_t input_size, char *input, char *delim) {
    char *str = (char *)malloc(input_size*sizeof(char *)) ;
    str = strcpy(str, input);
    const char *delim = " ";
    char *subtoken;
    char **token_input = (char **) malloc(input_size*sizeof(char *)+1);
    int i;
    char *saveptr = malloc(sizeof(char *));

        
    for(i=0; ;i++,str = NULL) {
            subtoken = strtok_r(str, delim, &saveptr);
            
            if(subtoken == NULL) {
                break;
            }
            *(subtoken+strlen(subtoken)) = '\0';
            *(token_input+i) = strdup(subtoken);
    }
    *(token_input+i) = NULL;
    free(str);
    free(saveptr);
    return i;
}

int contains_cd_or_exit(char *input) {
    regex_t regex;
    int ret;

    ret = regcomp(&regex, "(cd|exit|fg|jobs)", REG_EXTENDED);
    if (ret) {
        fprintf(stderr, "Could not compile regex\n");
        exit(1);
    }

    ret = regexec(&regex, input, 0, NULL, 0);
    if (!ret) {
        regfree(&regex);
        return 1;
    } else if (ret == REG_NOMATCH) {
        regfree(&regex);
        return 0;
    } 

    return 0;
}

int contains_regex(char *input, char *delim) {
    regex_t regex;
    int ret;

    ret = regcomp(&regex, delim, REG_EXTENDED);
    if (ret) {
        fprintf(stderr, "Could not compile regex\n");
        exit(1);
    }

    ret = regexec(&regex, input, 0, NULL, 0);
    if (!ret) {
        regfree(&regex);
        return 1;
    } else if (ret == REG_NOMATCH) {
        regfree(&regex);
        return 0;
    } 
    

    return 0;
}

int count_redirection(char **token_input, int k) {
    int countLessThan = 0, countGreaterThan = 0, countAppend = 0;
    
    int i;
    for(i=0;i<k;i++) {
        if(strcmp(*(token_input+i), ">") == 0) {
           countGreaterThan++;
        }
        if (strcmp(*(token_input+i), "<")==0)
        {
           countLessThan++;
        }
        if (strcmp(*(token_input+i), ">>")==0)
        {
           countAppend++;
        }
        
    }

    if (countLessThan>1 || countGreaterThan>1) {
        return 1;
    }

    return 0; 
}

int get_symbol_pos(char **token_input, char *str, int k) {
    int i;
    for(i=0;i<k;i++) {
        if(strcmp(*(token_input+i), str) == 0) {
            break;
        }
    }
    return i;
}

void execute_syscall(char *input) {
        int loop;
        bool has_pipes = false;
        char **token_input_with_pipes;
        if(contains_regex(input, "|")) {
            token_input_with_pipes = (char **) malloc(input_size*sizeof(char *)+1);
            loop = tokenize_input(token_input_with_pipes,input_size, input, "|");
            has_pipes = true;
        }
        else {
            loop = 1;
        }
        int fds[2*(loop-1)];
        char **token_input;
        int i;
        int no_loops;
        int c;
        for(c=0;c<loop-1;c++) {
            if(pipe(fds+c*2)<0) {
                exit(0);
            }
        }
        int pid;
        for(no_loops=0;no_loops<loop;no_loops++) {
            pid = fork();
            
            if(pid == 0) {
            bool has_input_redir = false;
            bool has_output_redir = false;
            bool has_double_output_redir = false;
            if(has_pipes) {
                if(no_loops>0) {
                    input = *(token_input_with_pipes+no_loops);
                    input = input+1;
                }
                else {
                    input = *(token_input_with_pipes+no_loops);
                }
                
                
            }
            
            if(contains_cd_or_exit(input)) {
                has_cd_or_exit = true;
            }



            token_input = (char **) malloc(input_size*sizeof(char *)+1);
            i = tokenize_input(token_input, input_size, input, " ");

            if(has_cd_or_exit) {
                if(strcmp(*token_input, "cd") == 0) {
                    if(i!=2) {
                        fprintf(stderr, "Error: invalid command\n");
                    }
                    else {
                        int err = chdir(*(token_input+1));
                        if(err == -1) {
                            fprintf(stderr, "Error: invalid directory\n");
                        }
                    }
                    
                }
                else if (strcmp(*token_input, "exit") == 0){
                    if(i!=1) {
                        fprintf(stderr, "Error: invalid command\n");
                    }
                    else {
                        exit(0);
                    }
                    

                }
                
    
                has_cd_or_exit = false;
                continue;
            }
            int new_token_input;
            char **temp_token_input;
            int pos_input;
            int pos_output;
            if(contains_regex(input, ">") || contains_regex(input, "<") || contains_regex(input, ">>")) {
                if(count_redirection(token_input,i) == 1) {
                    fprintf(stderr, "Error: invalid command");
                }
                else {
                    
                        
                        
                        if(contains_regex(input, "<")) {
                            pos_input = get_symbol_pos(token_input, "<",i);
                            if(pos_input+1==i) {
                                fprintf(stderr, "Error: invalid command\n");
                                continue;
                            }
                            has_input_redir = true;
                        }

                        if(contains_regex(input, ">")) {
                            if(contains_regex(input, ">>")) {
                                
                                pos_output = get_symbol_pos(token_input, ">>",i);
                                
                                has_double_output_redir = true;
                            }
                            else {
                                pos_output = get_symbol_pos(token_input, ">",i);
                                has_output_redir = true;
                            }
                            if(pos_output+1==i) {
                                    fprintf(stderr, "Error: invalid command\n");
                                    continue;
                            }
                        }
                        
                        temp_token_input = (char **) malloc(input_size*sizeof(char *)+1);
                        
                        for(new_token_input=0;new_token_input<i;new_token_input++) {
                            if(strcmp(*(token_input+new_token_input), ">") ==0|| strcmp(*(token_input+new_token_input), "<") ==0 || strcmp(*(token_input+new_token_input), ">>")==0)
                                break;
                            *(temp_token_input+new_token_input) = strdup(*(token_input+new_token_input));
                        }
                        *(temp_token_input+new_token_input) = NULL;
                    
                }
        


            }
            
            char *argl[i];
            
             
                if(has_input_redir) {
                    int fl = open(*(token_input+pos_input+1), O_RDONLY);
                    if(fl==-1) {
                        fprintf(stderr, "Error: invalid file\n");
                        exit(0);
                    }
                    dup2(fl, STDIN_FILENO);
                    close(fl);
                }
                if(has_output_redir || has_double_output_redir) {
                    int fl;
                    if(has_double_output_redir) {
                        
                        fl = open(*(token_input+pos_input+1), O_CREAT|O_WRONLY|O_APPEND);
                    }
                    else {
                        
                        fl = open(*(token_input+pos_input+1), O_CREAT|O_WRONLY|O_TRUNC);
                    }
                    
                    if(fl==-1) {
                        fprintf(stderr, "Error: invalid file\n");
                        exit(0);
                    }
                    dup2(fl, STDOUT_FILENO);
                    close(fl);
                }
                

                
                
                


                if(contains_regex(input, ">") || contains_regex(input, "<") || contains_regex(input, ">>")) { 
                    token_input = temp_token_input;
                }
                get_argl(argl, i, token_input);
                
                if(contains_regex(input, "/") != 1) {
                    char *temp_str = malloc(100*sizeof(char *));
                    strcpy(temp_str, "/usr/bin/");
                    strcat(temp_str, *token_input);
                    *token_input = temp_str;
                    

                }
                else if(contains_regex(input, "/") == 1 && contains_regex(input, "^/") != 1 && contains_regex(input, "^.") != 1) {
                    char *temp_str = malloc(100*sizeof(char *));
                    strcpy(temp_str, "./");
                    strcat(temp_str, *token_input);
                    *token_input = temp_str;
                }
                printf("%d\n", has_pipes);
                if(has_pipes) {
                    if(no_loops > 0) {
                        printf("In %d\n", (pipe_no-1)*2);
                        if(dup2(fds[(no_loops-1)*2], 0)<0) {

                            printf("Error");
                        }
                        close(fds[0]);
                    }
                    
                    if(no_loops<loop-1) {
                       
                        close(fds[0]);
                        if(dup2(fds[no_loops*2 + 1], 1)<0) {

                            printf("Error");
                        }
                        close(fds[1]);
                    }
                    
                }
                int c;
                for(c=0;c<2*(loop-1);c++) {
                        close(fds[c]);
                }
                int err = execv(*token_input, argl);
                if(err == -1) {
                    fprintf(stderr, "Error: invalid program\n");
                    exit(0);
                }
            
                
            }
        
            
        }

        for(int i=0;i<loop;i++) {
                    wait(NULL);
   
                 }

    for(int c=0;c<2*(loop-1);c++) {
                close(fds[c]);
    }

}
