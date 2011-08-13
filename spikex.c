
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>


int main(int argc, char **argv) {
    char *script;
    char assembly[256];
    pid_t pid;
    int out, status;
    
    if (argc < 2) {
        fprintf(stderr, "usage: %s SCRIPT [ARG]...\n", argv[0]);
        return 1;
    }
    
    script = argv[1];
    sprintf(assembly, "%s.s", script);
    out = open(assembly, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    
    pid = fork();
    if (!pid) {
        dup2(out, 1);
        execl("./cspk", "./cspk", "rtl/rtl.spk", script, NULL);
        perror("execl");
        return 1;
    }
    
    close(out);
    wait(&status);
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
        return 1;
    
    pid = fork();
    if (!pid) {
        execlp("gcc", "gcc", "-g",
               assembly,
               "rtl/Array.s",
               "rtl/CFunction.s",
               "rtl/CObject.s",
               "rtl/Char.s",
               "rtl/error.s",
               "rtl/Function.s",
               "rtl/Integer.s",
               "rtl/main.s",
               "rtl/Object.s",
               "rtl/rot.s",
               "rtl/send.s",
               "rtl/singletons.s",
               "rtl/String.s",
               "rtl/test.s",
               "rtl/lookup.c",
               NULL);
        perror("execlp");
        return 1;
    }
    
    wait(&status);
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
        return 1;
    
    execv("a.out", argv + 1);
    perror("execv");
    return 1;
}
