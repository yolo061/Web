#include <fcntl.h> // 打开文件
#include <stdio.h>
#include <stdlib.h>
#include <utmp.h>
#include <unistd.h>
#include <time.h>
#define SHOWHOST

// 将整数位的时间格式化
char* format_time(long timeval){
    return ctime(&timeval) + 4;
}


void show_info(struct utmp *utbufp)
{
    if (utbufp -> ut_type != USER_PROCESS) return; // 屏蔽非用户进程
    printf("%-8.8s", utbufp->ut_name);
    printf(" ");                     
    printf("%-8.8s", utbufp->ut_line);
    printf(" ");                     
    printf("%12.12s", format_time(utbufp->ut_time));
    
    printf(" ");                      
#ifdef SHOWHOST
    printf("(%s)", utbufp->ut_host);      /* the host */
#endif
    printf("\n");                         /* newline */
}


int main(){
    // 打开文件
    struct utmp current_record;

    int reclen = sizeof(current_record);
    int utmfd = open(UTMP_FILE, O_RDONLY);
    if (utmfd == -1){
        perror(UTMP_FILE);
        exit(1);
    }

    while (read(utmfd, &current_record, reclen) == reclen) show_info(&current_record);


    // 关闭文件
    close(utmfd);
    return 0;
}