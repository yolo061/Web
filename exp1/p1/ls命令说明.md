# 实验 1 - 问题 3：ls 命令

> 参考教材：《Unix/Linux编程实践教程》（Bruce Molay 著，杨宗源、黄海涛译）
> 第 3 章「目录与文件属性：编写 ls」（书 P63–92，核心：3.5 节 P69–70、3.6 节 P71–82）
> 注：PDF 页码 = 书页 + 20

## 一、ls 是什么

`ls` 列出目录里的内容。它其实包含**两件不同的事**（P71）：

1. 列出目录的**内容**（有哪些名字）——目录里有；
2. 显示文件的**详细信息**（大小、属主、权限……）——目录里没有，要另想办法。

对应教材的两步走：先写 `ls1`（只有名字），再升级成 `ls2`（`ls -l` 的效果）。

`ls -l` 每行 7 个字段（P71–72 表格）：

| 字段 | 来自哪 |
|---|---|
| 模式 mode（10 字符：类型 1 + 权限 9） | `stat` 的 `st_mode` |
| 链接数 links | `st_nlink` |
| 文件所有者 owner（用户名） | `st_uid` → 转名字 |
| 组 group（组名） | `st_gid` → 转名字 |
| 大小 size | `st_size` |
| 最后修改时间 last-modified | `st_mtime` → `ctime` 转换 |
| 文件名 name | `readdir` 的 `d_name` |

## 二、版本 1：ls1 —— 只列名字（3.5，P69–70）

### 2.1 读目录的 API（P68）

目录虽然特殊，但**读目录和读文件形式类似**，有一组专用函数（`<dirent.h>`）：

| 项目 | 内容 |
|---|---|
| 打开目录 | `DIR *dir_ptr = opendir(const char *dir_name)` |
| 逐条读 | `struct dirent *direntp = readdir(DIR *dir_ptr)` |
| 关闭 | `closedir(DIR *dir_ptr)` |
| 返回值 | opendir 失败返回 NULL；readdir 读完返回 NULL |

每条记录是 `struct dirent`（P69），本问题只用到两个字段：`d_ino`（i-节点号）和 `d_name`（文件名，以 `'\0'` 结尾）。

### 2.2 ls 的算法与参考代码（P69–70）

```
main()
    opendir
    while (readdir)
        print d_name
    closedir
```

```c
/* ls1.c - list contents of directory or directories
 *   action: if no args, use . else list files in args
 */
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>

void do_ls(char[]);

main(int ac, char *av[])
{
    if ( ac == 1 )
        do_ls( "." );
    else
        while ( --ac )
        {
            printf( "%s:\n", * ++av );
            do_ls( *av );
        }
}

void do_ls( char dirname[] )
/* list files in directory called dirname */
{
    DIR *dir_ptr;          /* the directory */
    struct dirent *direntp;/* each entry */

    if ( ( dir_ptr = opendir( dirname ) ) == NULL )
        fprintf( stderr, "ls1: cannot open %s\n", dirname );
    else
    {
        while ( ( direntp = readdir( dir_ptr ) ) != NULL )
            printf( "%s\n", direntp->d_name );
        closedir( dir_ptr );
    }
}
```

编译运行 `cc ls1.c -o ls1 && ./ls1`，与标准 `ls` 对比：能列出来，但（P71"还能做什么"）——没排序、没分栏、把 `.` 开头的隐藏文件也列出来了、更没有 `-l` 详情。前三个是加分项（读进数组用 `qsort` 排序、`d_name[0]=='.'` 跳过），最后一个引出 ls2。

## 三、版本 2：ls2 —— 编写 ls -l（3.6，P71–82）

### 3.1 用 stat 得到文件信息（3.6.3，P72–74）

readdir 只给名字和 i-节点号，**大小、属主、权限都不在目录里**。取文件状态用 `stat`：

```c
#include <sys/stat.h>
int res = stat(const char *name, struct stat *buf);
/* 返回 -1 出错；成功把文件属性填进 buf */
```

`struct stat` 里本问题用到的成员：`st_mode`（类型+权限）、`st_nlink`、`st_uid`、`st_gid`、`st_size`、`st_mtime`。

### 3.2 把模式字段转换成字符（3.6.6，P75–78）

`st_mode` 是 16 位二进制数（图 3.4）：前 4 位文件类型，中间 3 位特殊属性（set-uid 等，3.7 节，选做），后 9 位是三组 rwx 权限。解码用**掩码**：与 `&` 把不要的位清 0。

- 类型判断直接用 `<sys/stat.h>` 的宏：`S_ISDIR(m)`、`S_ISCHR(m)`、`S_ISBLK(m)`（P77–78，比手写 `((m & 0170000) == 0040000)` 清晰）；
- 9 个权限位各有掩码：`S_IRUSR/S_IWUSR/S_IXUSR`、`S_IRGRP/...`、`S_IROTH/...`。

书上参考函数 `mode_to_letters`（P78）：

```c
/* takes a mode value and a char array, and puts into
 * the char array the file type and the nine letters
 * that correspond to the bits in mode.
 * NOTE: It does not code setuid, setgid, and sticky
 */
void mode_to_letters( int mode, char str[] )
{
    strcpy( str, "----------" );       /* default = no perms */

    if ( S_ISDIR(mode) ) str[0] = 'd'; /* directory? */
    if ( S_ISCHR(mode) ) str[0] = 'c'; /* char devices */
    if ( S_ISBLK(mode) ) str[0] = 'b'; /* block device */

    if ( mode & S_IRUSR ) str[1] = 'r'; /* 3 bits for user */
    if ( mode & S_IWUSR ) str[2] = 'w';
    if ( mode & S_IXUSR ) str[3] = 'x';

    if ( mode & S_IRGRP ) str[4] = 'r'; /* 3 bits for group */
    if ( mode & S_IWGRP ) str[5] = 'w';
    if ( mode & S_IXGRP ) str[6] = 'x';

    if ( mode & S_IROTH ) str[7] = 'r'; /* 3 bits for other */
    if ( mode & S_IWOTH ) str[8] = 'w';
    if ( mode & S_IXOTH ) str[9] = 'x';
}
```

### 3.3 将用户/组 ID 转换为字符串（3.6.7，P79–81）

`st_uid`/`st_gid` 是数字，`ls -l` 要显示名字。查名字用库函数（不用自己扫 `/etc/passwd`）：

```c
#include <pwd.h>                        /* /etc/passwd 相关 */
struct passwd *getpwuid(uid_t uid);     /* 成员 pw_name 即用户名 */

#include <grp.h>                        /* /etc/group 相关 */
struct group *getgrgid(gid_t gid);      /* 成员 gr_name 即组名 */
```

书上参考函数（P80–81）：

```c
/* returns a username associated with the specified uid
 * NOTE: does not work if there is no username
 */
char *uid_to_name( uid_t uid )
{
    struct passwd *pw_ptr;
    static char numstr[10];

    if ( ( pw_ptr = getpwuid( uid ) ) == NULL ) {
        sprintf( numstr, "%d", uid );   /* 没有对应用户名时打印数字 */
        return numstr;
    }
    else
        return pw_ptr->pw_name;
}

char *gid_to_name( gid_t gid )
{
    return getgrgid( gid )->gr_name;
}
```

P80 专门讲了为什么 `getpwuid` 可能返回 NULL：文件属主的账号后来被删了。简单版照书直接 `getpwuid(uid)->pw_name` 也能跑，但 NULL 检查是标准 `ls` 的做法（书上明确说了这点）。

### 3.4 组装 ls2.c（3.6.8，P81–83）

```c
/* ls2.c - list contents of directory or directories
 *   action: if no args, use . else list files in args
 *   note: uses stat and pwd.h and grp.h
 * BUG: try ls2 /tmp
 */
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>

void do_ls(char[]);
void dostat(char *);
void show_file_info(char *, struct stat *);
void mode_to_letters(int, char[]);
char *uid_to_name(uid_t);
char *gid_to_name(gid_t);

main(int ac, char *av[])
{
    if ( ac == 1 )
        do_ls( "." );
    else
        while ( --ac )
        {
            printf( "%s:\n", * ++av );
            do_ls( *av );
        }
}

void do_ls( char dirname[] )
{
    DIR *dir_ptr;
    struct dirent *direntp;

    if ( ( dir_ptr = opendir( dirname ) ) == NULL )
        fprintf( stderr, "ls2: cannot open %s\n", dirname );
    else
    {
        while ( ( direntp = readdir( dir_ptr ) ) != NULL )
            dostat( direntp->d_name );
        closedir( dir_ptr );
    }
}

void dostat( char *filename )
{
    struct stat info;

    if ( stat( filename, &info ) == -1 )
        perror( filename );
    else
        show_file_info( filename, &info );
}

void show_file_info( char *filename, struct stat *info_p )
{
    char modestr[11];

    mode_to_letters( info_p->st_mode, modestr );

    printf( "%s"    , modestr );
    printf( "%4d "  , (int) info_p->st_nlink );
    printf( "%-8s " , uid_to_name( info_p->st_uid ) );
    printf( "%-8s " , gid_to_name( info_p->st_gid ) );
    printf( "%8ld " , (long) info_p->st_size );
    printf( "%.12s ", 4 + ctime( &info_p->st_mtime ) );
    printf( "%s\n"  , filename );
}
```

`show_file_info` 各格式符的含义：`%4d` 链接数右对齐、`%-8s` 名字左对齐占 8 列、`%8ld` 大小右对齐、`%.12s` 从 `ctime` 结果**第 4 个字符起取 12 个**（跳过星期，只显示 "月 日 时刻"，且 `ctime` 返回串自带 `'\n'`，截 12 个正好把它去掉）。

## 四、编译与测试

```sh
$ cc ls1.c -o ls1 && ./ls1         # 第一步：能列出名字
$ cc ls2.c -o ls2 && ./ls2         # 第二步：7 列详情
$ ls -l                            # 与标准 ls 对比
```

书上代码故意留了个 bug（开头注释 `BUG: try ls2 /tmp`）——**试试 `./ls2 /tmp` 就知道**：`dostat` 拿到的 `d_name` 不带路径，对当前目录碰巧能用，换个目录就 stat 失败。修复方法：

```c
char fullname[1024];
sprintf( fullname, "%s/%s", dirname, direntp->d_name );
dostat( fullname );
```

这也是本问题最容易踩的坑，务必自己改一遍并测试。

## 五、现代 Linux 上的兼容性说明

- 需要的头文件：`dirent.h`、`sys/stat.h`、`sys/types.h`、`pwd.h`、`grp.h`、`string.h`（strcpy）、`time.h`（ctime）；
- `st_size` 是 `off_t`（64 位），printf 用 `%ld` 并强转 `(long)`（书上就是这么写的），`-Wall` 才不会警告；
- readdir 返回顺序**不保证有序**，和标准 `ls` 排序输出对不上是正常现象（排序进数组 + `qsort` 是可选加分项）；
- `.` 和 `..` 会被列出来；标准 `ls` 对 `d_name[0]=='.'` 的条目默认不显示，可自己加 `-a` 逻辑；
- `mode_to_letters` 没处理 set-uid/set-gid/sticky 位（书 3.7 节 P86–87，`s`/`t` 字符），作业基本版可以不做。

## 六、小结

- `ls` = 两件事：读目录（`opendir`/`readdir`/`closedir`，得名字）+ 读文件属性（`stat`，得详情）；
- `st_mode` 解码：掩码 `&` + `<sys/stat.h>` 的 `S_ISDIR` 宏和 `S_R/W/X USR/GRP/OTH` 位，组装成 10 字符串；
- 数字 ID 转名字：`getpwuid`/`getgrgid`，注意 NULL（属主账号已删）；
- 两个经典坑：`stat` 忘了拼目录路径（书上的 BUG）、`ctime` 结果自带换行（用 `%.12s` 截断）。
