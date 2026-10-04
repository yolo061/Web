#include <stdio.h>
#include <fcntl.h>
#include <utmp.h>


int main(){
    struct utmp cr;
    int reclen = sizeof(cr);
    
    int utmfd = open(UTMP_FILE, O_RDWR);
    // 检验文件是否打开
    if (utmfd == -1) {
        perror(UTMP_FILE);
        exit(1);
    }

    
}