// exit()、perror() 所需
#include <stdlib.h>
// printf、putchar 等标准输入输出
#include <stdio.h>
// chdir() 改变当前目录
#include <unistd.h>
// strncpy 字符串拷贝
#include <string.h>
// ino_t 类型（i-节点号）的定义
#include <sys/types.h>
// struct stat 结构体和 stat() 系统调用
#include <sys/stat.h>
// opendir/readdir/closedir 读目录
#include <dirent.h>

// 三个函数的声明：查 i-节点号 / 递归打印路径 / 由 i-节点号查名字
ino_t get_inode(char *);
void printpathto(ino_t);
void inum_to_name(ino_t, char *, int);

int main(){
    // 从当前目录的 i-节点号开始，递归打印从根到这里的完整路径
    printpathto(get_inode("."));
    // 路径打完补一个换行
    putchar('\n');
    return 0;
}

// 递归函数：打印通往 i-节点号为 this_inode 的目录的路径
void printpathto(ino_t this_inode){
    // 父目录的 i-节点号，作为下一层递归的参数
    ino_t my_inode;
    // 存放查到的当前目录名字
    char its_name[BUFSIZ];

    // 取 ".."（父目录）的号来比较：不相等说明还没到根
    // （根目录的 "." 和 ".." 指向同一个 i-节点，相等即到根）
    if (get_inode("..") != this_inode){
        // 还没到根：先爬上一层，进入父目录
        chdir("..");
        // 在父目录里找 i-节点号等于 this_inode 的记录，拿到自己的名字
        inum_to_name(this_inode, its_name, BUFSIZ);

        // 取新的当前目录（即父目录）的 i-节点号
        my_inode = get_inode(".");
        // 先递归打印更上面的部分，保证输出顺序从根往下
        printpathto(my_inode);
        // 递归返回后才打印自己这一截名字
        printf("/%s", its_name);
    }

}

// 在当前目录里查找 i-节点号为 inode_to_find 的记录，
// 把它的名字拷进 namebuf（最多 buflen 字节）
void inum_to_name(ino_t inode_to_find, char *namebuf, int buflen){
    // 目录流句柄，由 opendir 返回
    DIR *dir_ptr;
    // readdir 逐条返回的目录项指针
    struct dirent *direntp;

    // 打开当前目录
    dir_ptr = opendir(".");
    if (dir_ptr == NULL){
        // 打不开就报错退出
        perror(".");
        exit(1);
    }


    // 逐条读目录记录，读完（返回 NULL）为止
    while((direntp = readdir(dir_ptr)) != NULL)
        // 记录的 i-节点号 == 要找的号 → 就是这条
        if (direntp -> d_ino == inode_to_find){
            // 拷出名字，并保证字符串以 '\0' 结尾
            strncpy(namebuf, direntp->d_name, buflen);
            namebuf[buflen-1] = '\0';
            closedir(dir_ptr);
            return ;   // 找到了，结束
        }
    // 循环走完 = 找遍了都没有，关目录返回（namebuf 保持原样）
    closedir(dir_ptr);
    return ;

}

// 返回文件 fname 的 i-节点号
ino_t get_inode(char *fname){
    // stat 会把文件属性填进这个结构体
    struct stat info;
    if (stat(fname, &info) == -1){
        // stat 失败：报错并退出
        fprintf(stderr, "Cannot stat");
        perror(fname);
        exit(1);
    }
    // 从填好的结构体里取出 i-节点号
    return info.st_ino;
}
