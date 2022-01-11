#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char buf[512];
char tmp[30];

int findpos(char * path,int begin){
    int len  = strlen(path);
    while(begin < len && path[begin]!= '\n')
        begin++;
    return begin;
}

void copy(char *dst,char*src,int n){
    while (n--)
    {
        *(dst++) = *(src++);
    }
    
}


int main(int argc, char *argv[]){
    int n,i,len=0,j=0;
    char* lineSplit[32];
    char *p = tmp;
    char cmd[20] = "/bin/";
    strcpy(cmd+5,argv[1]);
    for(i=1;i<argc;i++){
        lineSplit[j++] = argv[i];
    }

    while((n = read(0,buf,sizeof(buf))) > 0){
        for(i=0;i<n;i++){
            if(buf[i] == '\n'){
                tmp[len] = 0;
                lineSplit[j++] = p;
                lineSplit[j] = 0;
                if(fork() ==0){
                    exec(argv[1],lineSplit);
                }
                wait(0);
                len = 0;
                j = argc -1;
                p = tmp;
            }
            else if(buf[i] == ' '){
                tmp[len++] = 0;
                lineSplit[j++] = p;
                p = &tmp[len];
            }
            else{
                tmp[len++] = buf[i];
            }
        }
    }
    if(n < 0){
        fprintf(2, "cat: read error\n");
        exit(1);
    }
    
    exit(0);
    
    

}