#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

void create_args(char *args[]) {
    for (int i=0; i<101; i++) {
        args[i] = "A"; // fill up to 100 arguments with filler char
    }
    args['A'] = "\x00";
    args['B'] = "\x20\x0a\x0d";
    args['C'] = "5001"; // addition for stage 5
    args[100] = NULL;
}

void create_file() {
    FILE *fp;

    fp = fopen("\x0a", "w");
    if (fp == NULL) {
        perror("fopen failed");
    }

    char data[] = {'\x00', '\x00', '\x00', '\x00'};

    if (fwrite(data, 4, 1, fp) != 1) {
        perror("Error writing to file");
    }

    fclose(fp);
}

int main(){

    char *args[101] = {};
    create_args(args);
    char path[] = "/home/yanisf/Documents/coding/pwnable.kr/input/input";

    // set environment variable - stage 3
    setenv("\xde\xad\xbe\xef", "\xca\xfe\xba\xbe", 1);
    extern char **environ;

    // write to file - stage 4
    create_file();

    pid_t childpid;
    int pipe_stdin[2];
    int pipe_stderr[2];

    //call pipe on both pipes
    if (pipe(pipe_stdin) < 0 || pipe(pipe_stderr) < 0) {
        perror("Pipe error");
        exit(1);
    }

    // fork the process
    if ((childpid = fork()) < 0) {
        perror("Fork error");
        exit(1);
    }

    // child process can close input side of pipe and write expected values
    if (childpid == 0) {
        /* Child process closes up input side of pipe */
        close(pipe_stdin[0]);
        close(pipe_stderr[0]);

        write(pipe_stdin[1], "\x00\x0a\x00\xff", 4);
        write(pipe_stderr[1], "\x00\x0a\x02\xff", 4);

        // Stage 5
        sleep(5);
        int sd, cd;
        struct sockaddr_in saddr;
        sd = socket(AF_INET, SOCK_STREAM, 0);

        if(sd == -1){
            printf("socket error, tell admin\n");
            return 0;
        }
        saddr.sin_family = AF_INET;
        saddr.sin_addr.s_addr = inet_addr("127.0.0.1");
        saddr.sin_port = htons(atoi(args['C']));

        if(connect(sd, (struct sockaddr *)&saddr, sizeof(saddr))<0)
        {
            printf("\n Error : Connect Failed \n");
            return 1;
        }

        write(sd, "\xde\xad\xbe\xef", 4);
        close(sd);
        return 0;
    } else {
        // parent process can close output side of pipe, connect it to stdin and stderr, and then close the input side and call
        close(pipe_stdin[1]);
        close(pipe_stderr[1]);

        dup2(pipe_stdin[0], 0);
        dup2(pipe_stderr[0], 2);

        close(pipe_stderr[0]);
        close(pipe_stdin[0]);

        execve(path, args, environ);
    }
}
