#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void func(int cfd,int rfd){
    int num,i,len;
    int p[2];
    close(cfd);

    len = read(rfd,&num,sizeof(int));
    if(len > 0){
        printf("prime %d\n",num);
        pipe(p);
        if(fork() ==0){
            func(p[1],p[0]);
        }
        else{
            close(p[0]);
            while(read(rfd,&i,sizeof(int))){
                if(i % num)
                    write(p[1],&i,sizeof(int));
            }
            close(p[1]);
            wait(0);
        }
        
    }
    close(rfd);
    exit(0);

}

int main(int argc, char *argv[]){
    int p[2],i;
    pipe(p);
    if(fork() == 0){
        func(p[1],p[0]);
    }
    else{
        close(p[0]);
        for(i=2;i<36;i++){
            write(p[1],&i,sizeof(int));
        }
        close(p[1]);
    }
    
    wait(0);
    exit(0);

}