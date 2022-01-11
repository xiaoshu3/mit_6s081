#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]){
    int p1[2];
    int p2[2];
    pipe(p1);
    pipe(p2);
    char *tmp = "c\n";

    if(fork() ==0 ){
        read(p1[0],tmp,1);
        printf("%d: received ping\n",getpid());
        close(p1[0]);
        close(p1[1]);
        write(p2[1],tmp,1);
        close(p2[0]);
        close(p2[1]);
        exit(0);
    }
    else{
        close(p1[0]);
        write(p1[1],tmp,1);
        close(p1[1]);
        close(p2[1]);
        read(p2[0],tmp,1);
        printf("%d: received pong\n",getpid());
        close(p2[1]);
    }
    exit(0);
}