// ls代码完成的操作
/*
打开dir，遍历dir当中的文件，关闭dir
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>

void do_ls(char []);

int main(int ac, char *av[]){
    if (ac == 1) do_ls(".");
    else 
        while (--ac){
            printf("%s:\n", *++av);
            do_ls(*av);
        }
}

void do_ls(char dirname[]){
    DIR *dir_ptr;
    struct dirent *direntp;

    if ((dir_ptr = opendir(dirname)) == NULL)
        fprintf(stderr, "ls1: cannot open %s\n", dirname);
    else {
        while ((direntp = readdir(dir_ptr)) != NULL)
            printf("%s\n", direntp -> d_name);
        closedir(dir_ptr);
    }
}