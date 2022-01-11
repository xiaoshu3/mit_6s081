#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]){
    int i;
    if(argc < 2){
    fprintf(2, "Usage: sleep ...\n");
    exit(1);
    }
    else{
        i = atoi(argv[1]);
        sleep(i);
    }
    exit(0);
}