#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"


char*
fmtname(char *path)
{
  static char buf[DIRSIZ+1];
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  // Return blank-padded name.
  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  memset(buf+strlen(p), ' ', DIRSIZ-strlen(p));
  return buf;
}

char * rstrip(char * path){
    char *p;
    for(p=path+strlen(path); p >= path && *p != ' '; p--)
        ;
    p++;
    *p = 0;
    return path;
}
void find(char* path,char *f){
    char buf[512], *p;
    char tmp[DIRSIZ+1];
    memmove(tmp,fmtname(f),strlen(fmtname(f)));
    int fd;
    struct dirent de;
    struct stat st;

    if((fd = open(path, 0)) < 0){
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    if(fstat(fd, &st) < 0){
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    switch(st.type){
    case T_FILE:
        fprintf(2, "find: %s is not a dir\n", path);
        break;

    case T_DIR:
        if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
        printf("find: path too long\n");
        break;
        }
        strcpy(buf, path);
        p = buf+strlen(buf);
        *p++ = '/';
        while(read(fd, &de, sizeof(de)) == sizeof(de)){
        if(de.inum == 0)
            continue;
        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = 0;
        if(stat(buf, &st) < 0){
            printf("find: cannot stat %s\n", buf);
            continue;
        }

        /*printf("%s %d %d %d\n", fmtname(buf), st.type, st.ino, st.size);*/
        if(st.type == T_FILE){
            /*printf("%s %s\n",fmtname(buf),f);*/
            if(strcmp(fmtname(buf),tmp) ==0)
                printf("%s\n",buf);
        }

        else if(st.type == T_DIR && de.name[0] != '.'){
            /*printf("%s %s %d\n",buf,de.name,100);*/
            find(buf,f);
        }
        
        }
        break;
    }
    close(fd);   
    
}

int main(int argc, char *argv[]){
    if(argc < 3){
        fprintf(2, "Usage: find ...\n");
        exit(1);
    }
    else{
        find(argv[1],argv[2]);
    }
    exit(0);
}