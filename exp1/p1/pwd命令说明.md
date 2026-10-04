# 实验 1 - 问题 2：pwd 命令

> 参考教材：《Unix/Linux编程实践教程》（Bruce Molay 著，杨宗源、黄海涛译）
> 第 4 章「文件系统：编写 pwd」（书 P96–120，核心：4.5 节 P113–115）
> 注：PDF 页码 = 书页 + 20

## 一、pwd 是什么

`pwd`（print working directory）用来显示**到达当前目录的完整路径**：

```
$ pwd
/home/yourname/experiments/demodir/c/d2
```

这条路径从根目录 `/` 开始，一层一层往下。看起来平常，但有一个反直觉的事实（P113）：**当前目录自己并不知道这份"名字链"**。把 pwd 自己写一遍，就是要解决"它怎么知道自己在哪"这个问题。

## 二、pwd 是如何工作的（4.5.1，P113–114）

### 2.1 难点：目录里没有存"完整路径"

目录的本质是一张**记录列表**，每条记录只有两样东西：**文件（或子目录）名 + i-节点号**。当前目录用名字 `"."` 表示自己，所以它只知道**自己的 i-节点号**，不知道自己的名字，更不知道从根到这里要经过哪些目录。

但反过来，**父目录的记录里存着"子目录名 → i-节点号"的映射**。这就是突破口。

### 2.2 算法：从当前目录一路往上爬（图 4.12）

1. 用 `stat(".")` 得到当前目录的 i-节点号，记为 n；
2. 用 `chdir("..")` 进入父目录；
3. 在父目录里用 `opendir`/`readdir` 逐条查找 i-节点号等于 n 的记录，记录的名字就是路径的最后一截。

对父目录重复这 3 步，名字一个个攒出来，直到到达树顶。

### 2.3 怎么判断到了树顶（P114 问题 1）

**根目录的 `"."` 和 `".."` 指向同一个 i-节点。** 所以当 `"."` 与 `".."` 的 i-节点号相同时，就说明已经站在根目录，算法结束。

（P114 问题 2：名字找到的顺序是从下往上的，怎么按正确顺序显示？书上用**递归**解决——先递归打印"上面"部分，再打印自己这一截。）

## 三、如何编写 pwd（4.5.2，P114–115）

书上参考代码 `pwd.c` 一共 4 个函数，各做一件事：

| 函数 | 职责 | 用到的调用 |
|---|---|---|
| `main` | 入口，打印路径 + 换行 | — |
| `get_inode(name)` | 返回某个文件的 i-节点号 | `stat` |
| `inum_to_name(ino, buf, len)` | 在**当前目录**里找 i-节点号对应的名字 | `opendir`/`readdir`/`closedir` |
| `printpathto(ino)` | 递归向上爬、按正确顺序打印路径 | `chdir` + 上面两个函数 |

完整参考代码（P114–115）：

```c
/* pwd.c - a simplified version of pwd
 *   starts in current directory and recursively
 *   climbs up to root of filesystem, prints top part
 *   then prints current part
 *   uses readdir() to get info about each thing
 *   bug: prints an empty string if run from "/"
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

ino_t get_inode(char *);
void printpathto(ino_t);
void inum_to_name(ino_t, char *, int);

int main()
{
    printpathto( get_inode( "." ) );   /* print path to here */
    putchar( '\n' );                   /* then add newline */
    return 0;
}

void printpathto( ino_t this_inode )
/* prints path leading down to an object with this inode
 * kindof recursive
 */
{
    ino_t my_inode;
    char its_name[BUFSIZ];

    if ( get_inode("..") != this_inode )          /* not at root yet */
    {
        chdir( ".." );                            /* up one dir */
        inum_to_name( this_inode, its_name, BUFSIZ ); /* get its name */
        my_inode = get_inode( "." );              /* print head */
        printpathto( my_inode );                  /* recursively */
        printf( "/%s", its_name );                /* now print name of this */
    }
}

void inum_to_name( ino_t inode_to_find, char *namebuf, int buflen )
/* looks through current directory for a file with this inode
 *   number and copies its name into namebuf
 */
{
    DIR *dir_ptr;                /* the directory */
    struct dirent *direntp;      /* each entry */

    dir_ptr = opendir( "." );
    if ( dir_ptr == NULL ) {
        perror( "." );
        exit(1);
    }

    /* search directory for a file with specified inum */
    while ( ( direntp = readdir( dir_ptr ) ) != NULL )
        if ( direntp->d_ino == inode_to_find )
        {
            strncpy( namebuf, direntp->d_name, buflen );
            namebuf[buflen-1] = '\0';
            closedir( dir_ptr );
            return;
        }
    closedir( dir_ptr );
    return;
}

ino_t get_inode( char *fname )
/* returns inode number of the file */
{
    struct stat info;

    if ( stat( fname, &info ) == -1 ) {
        fprintf( stderr, "Cannot stat " );
        perror( fname );
        exit(1);
    }
    return info.st_ino;
}
```

读懂递归的关键在 `printpathto` 的执行顺序。比如在 `/home/user/d2` 下运行：

1. `get_inode("..")`（即 `/home/user`）≠ 当前 inode → 还没到根；
2. `chdir("..")` 爬上一级；`inum_to_name` 在新的当前目录里找到自己的名字 `"d2"`；
3. `my_inode = get_inode(".")` 是**爬上一级之后**的 `"."`（即父目录的 inode），拿它去递归；
4. 递归一路到根后逐层返回，回来时才执行 `printf("/%s")`——所以打印顺序是**从根往下**的。

注意第 3 步：`get_inode(".")` 必须在 `chdir("..")` **之后**调用，取的已经是父目录的 inode。

## 四、编译与测试

```sh
$ cc pwd.c -o pwd          # 或 gcc -Wall pwd.c -o pwd
$ cd /usr/include/sys      # 找个深目录
$ ./pwd
/usr/include/sys
$ pwd                      # 与系统 pwd 对比
```

书上代码自带一个 bug（开头注释写了）：**在 `/` 下运行会打印空串**。因为到根时 `printpathto` 什么都不打印。想改进可以在 `main` 里特判：`get_inode("..") == get_inode(".")` 时直接 `printf("/\n")`。

## 五、现代 Linux 上的兼容性说明

- 需要的头文件：`dirent.h`（opendir/readdir）、`sys/stat.h`（stat）、`unistd.h`（chdir）；
- 4.6 节（P116–120）讲了这个简化版 pwd 的两个理论缺陷：**跨挂载点**（不同文件系统的 i-节点号会重复，比如 `/home` 单独分区时）、**符号链接**。严谨版要用 `stat` 拿到 `st_dev`，**同时比较 `st_dev + st_ino`** 才算同一个目录。作业基本版只比 ino 即可，但要知道为什么在多分区机器上可能出错；
- 在 ext4 上根目录的 `"."` 与 `".."` 的 inode 号都是 2，"相等即到根"的判定可用；
- `strncpy` 后手动补 `'\0'` 那行别省——书上加它是防 `d_name` 太长没截断。

## 六、小结

- 目录 = "名字 → i-节点号"的记录列表；当前目录不知道自己的名字，但**父目录知道**；
- pwd 的算法：`stat(".")` 取号 → `chdir("..")` 上爬 → `readdir` 查名字，循环直到 `"."` 与 `".."` 的 inode 相等（到达根）；
- 新学的系统调用/库函数：`chdir`（改变进程当前目录，P113 的 API 表）；
- 递归实现保证打印顺序从根到叶：**先递归打印上面，再打印自己这一截**。
