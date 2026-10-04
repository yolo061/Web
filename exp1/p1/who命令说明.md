# 实验 1 - 问题 1：who 命令

> 参考教材：《Unix/Linux编程实践教程》（Bruce Molay 著，杨宗源、黄海涛译）
> 第 2 章「用户、文件操作与联机帮助：编写 who 命令」（书 P25–44）

## 一、什么是 who 命令

`who` 是 Unix 的一个标准命令，用来回答一个问题：**现在都有谁在使用这台系统？**

在命令行输入 `who`，输出如下（教材 P27 示例）：

```
$ who
hecker1  ttyp1 Jul 21 19:51 (tide75.surfcity.com)
nlopez   ttyp2 Jul 21 18:11 (roam163-141.student.ivy.edu)
dgsullivan ttyp3 Jul 21 14:18 (h004005a8bd64.ne.mediaone.net)
...
```

**每一行代表一个已经登录的用户**，共 4 列：

| 列 | 含义 |
|---|---|
| 第 1 列 | 用户名（登录名） |
| 第 2 列 | 终端名（该用户在哪台终端/ttyn 上） |
| 第 3 列 | 登录时间 |
| 第 4 列 | 用户的登录地址（远程主机名），某些版本的 who 默认不显示 |

其他常见形式：`who am i`（显示自己是谁）、`whoami`（只显示用户名）。查看完整用法用联机帮助：`man who`。

另外，Unix 中几乎所有的命令（包括 who、ls）都是人编写的普通程序，大多是 C 语言写的。对命令的功能不满意，完全可以自己编写一个自己的 `who`——这正是本实验要做的。

## 二、who 命令是如何工作的（原理）

联机帮助 `man who` 的描述（P28–29）说明：已登录用户的信息存放在 **utmp 文件**中（教材的 SunOS 系统里是 `/var/adm/utmp`；在现代 Linux/Ubuntu 上由 `UTMP_FILE` 宏指定，即 `/var/run/utmp`）。每个用户登录时，系统就往该文件里写入一条记录。

记录的结构定义在头文件 `/usr/include/utmp.h` 的 `struct utmp` 中，主要成员（P32）：

| 成员 | 含义 |
|---|---|
| `ut_name`（即 `ut_user`） | 登录名 |
| `ut_line` | 设备名（终端类型，如 ttyp1） |
| `ut_time` | 登录时间（从 1970-01-01 00:00 起的秒数，time_t 类型） |
| `ut_host` | 用户用于登录的远程计算机名 |
| `ut_type` | 记录类型（见下表） |

`ut_type` 的取值（P39，定义在 utmp.h 中）：

```c
#define EMPTY         0
#define RUN_LVL       1
#define BOOT_TIME     2
#define OLD_TIME      3
#define NEW_TIME      4
#define INIT_PROCESS  5   /* Process spawned by "init" */
#define LOGIN_PROCESS 6   /* A "getty" process waiting for login */
#define USER_PROCESS  7   /* A user process（已登录的用户） */
#define DEAD_PROCESS  8   /* 已退出的进程 */
```

**who 的工作流程**（P32 图 2.2）很简单：

```
打开 utmp → 读取记录 → 显示记录 → 关闭 utmp
```

utmp 中的结构数组存放着每个用户的信息，把记录一个一个地读出来并显示即可。

## 三、如何编写 who（教材 P32–44）

编写 who 程序只有两件事要做：

1. **从文件中读取数据结构**
2. **将结构中的信息以合适的形式显示出来**

具体动手时按下面 6 步走：

| 步骤 | 做什么 |
|---|---|
| 第 1 步 | 弄清数据从哪来：utmp 文件位置（`UTMP_FILE`）和 `struct utmp` 结构（见上文"二"） |
| 第 2 步 | 学习 `open` / `read` / `close` 三个系统调用的用法（见 3.1） |
| 第 3 步 | 写版本 1（who1.c）：打开 utmp → 循环按 `sizeof(struct utmp)` 整条读 → 打印字段 → 关闭（见 3.2） |
| 第 4 步 | 编译运行 who1，与系统 `who` 输出对比，找出两个问题：空白记录、时间是整数（见 3.2 末尾） |
| 第 5 步 | 改进为版本 2（who2.c）：用 `ut_type == USER_PROCESS` 过滤空白记录；用 `ctime` 格式化时间（见 3.3） |
| 第 6 步 | 重新编译运行，与系统 `who` 对比验证，输出一致即完成 |

### 3.1 读取数据结构：open / read / close 三个系统调用

不能用 `getc` 一个字节一个字节地读（太慢），要用一次读出一整个结构的方法。通过联机帮助（`man -k file | grep read`、`man 2 read` → SEE ALSO → `man 2 open`、`man 2 close`）可以找到这 3 个文件操作系统调用：

**open —— 打开一个文件**（P35）

```c
#include <fcntl.h>
int fd = open(char *name, int how);
/* how: O_RDONLY 只读 / O_WRONLY 只写 / O_RDWR 可读可写
 * 返回值: -1 遇到错误；成功返回文件描述符 fd */
```

**read —— 把数据读入缓冲区**（P35）

```c
#include <unistd.h>
ssize_t numread = read(int fd, void *buf, size_t qty);
/* 从 fd 读 qty 字节到 buf
 * 返回值: -1 错误；0 文件结束；否则为实际读取的字节数 numread */
```

因为每次要读入一个数据结构，所以用 `sizeof(struct utmp)` 指定每次读取的字节数。

**close —— 关闭一个文件**（P36）

```c
#include <unistd.h>
int result = close(int fd);
/* 返回值: -1 错误；0 成功 */
```

### 3.2 版本 1：who1.c（P36–37）

```c
/* who1.c - a first version of the who program
 *          open, read UTMP file, and show results
 */
#include <stdio.h>
#include <utmp.h>
#include <fcntl.h>
#include <unistd.h>

#define SHOWHOST              /* include remote machine on output */

int main()
{
    struct utmp current_record;   /* read info into here */
    int utmpfd;                   /* read from this descriptor */
    int reclen = sizeof(current_record);

    if ((utmpfd = open(UTMP_FILE, O_RDONLY)) == -1) {
        perror(UTMP_FILE);        /* UTMP_FILE is in utmp.h */
        exit(1);
    }

    while (read(utmpfd, &current_record, reclen) == reclen)
        show_info(&current_record);

    close(utmpfd);
    return 0;                     /* went ok */
}
```

显示函数 `show_info`（P37，把结构内容按定宽格式打印，与系统 who 输出对齐）：

```c
show_info( struct utmp *utbufp )
{
    printf("%-8.8s", utbufp->ut_name);    /* the logname */
    printf(" ");                          /* a space */
    printf("%-8.8s", utbufp->ut_line);    /* the tty */
    printf(" ");                          /* a space */
    printf("%10ld", utbufp->ut_time);     /* login time */
    printf(" ");                          /* a space */
#ifdef SHOWHOST
    printf("(%s)", utbufp->ut_host);      /* the host */
#endif
    printf("\n");                         /* newline */
}
```

编译运行 `cc who1.c -o who1 && ./who1` 后能列出记录，但对照系统 who 发现**两个问题**（P38–39）：

- 显示了大量**空白记录**（utmp 里还存有未使用的终端、系统启动等记录）
- 登录时间显示为一个大整数（如 `952601411`），**不是人类可读的时间**

### 3.3 版本 2：who2.c —— 解决两个问题（P39–43）

**（1）消除空白记录**：靠过滤用户名为空的记录不可靠（控制台 LOGIN 记录用户名也不为空）。正确方法是利用 `ut_type`：只有 `ut_type == USER_PROCESS`（值为 7）才是真正已登录的用户。

```c
if ( utbufp->ut_type != USER_PROCESS )    /* users only */
    return;
```

**（2）正确显示登录时间**：Unix 中时间是一个整数（time_t，即 long int），表示从 1970 年 1 月 1 日 0 时起的秒数。用 `ctime()` 把它转换成字符串：

```c
cp = ctime(&timeval);
/* 字符串形如 "Mon Feb 4 00:46:40 EST 1991"
 *              0123456789012345        */
printf("%12.12s", cp + 4);   /* 从第 4 个字符开始取 12 个字符 → "Feb 4 00:46" */
```

最终版完整代码见本目录 `who.c`（即 who2.c），要点：

- `main`：`open(UTMP_FILE, O_RDONLY)` → 循环 `read` 整条记录 → `show_info` → `close`
- `show_info`：过滤 `ut_type != USER_PROCESS`；定宽打印用户名、终端；调 `showtime` 显示时间
- `showtime`：`ctime` 转字符串后用 `%12.12s` 截取 `cp+4` 起的 12 个字符

### 3.4 编译与测试（本实验环境：Ubuntu）

```sh
$ cc who.c -o who        # 或 gcc -Wall who.c -o who
$ ./who                  # 与系统 who 的输出对比
$ who
```

who2 与系统的 who 对比（P43–44），除了某些字段宽度略有不同，其余完全一致。若想显示远程主机名，编译时加 `#define SHOWHOST`。

## 四、现代 Linux 上的兼容性说明

教材基于早期 Unix（utmp 记录较小）。在现代 Linux 的 `/usr/include/utmp.h` 中：

- `ut_name` 是 `ut_user` 的兼容宏，`ut_time` 是 `ut_tv.tv_sec` 的兼容宏，书中代码**可以原样编译**；
- utmp 文件路径由 `UTMP_FILE` 宏给出（Ubuntu 上为 `/var/run/utmp`）；
- 图形界面登录的记录 `ut_line` 可能是 `:0` 这样的显示名，属于正常现象。

## 五、小结（P44 回顾与展望）

本章从「Unix 的 who 命令是如何工作的」这个简单问题开始，分 3 步走：

1. 首先弄清 who 的功能（列出已登录用户：用户名、终端、登录时间、主机）；
2. 然后通过联机帮助（`man`）和头文件（`utmp.h`）知道它的工作原理——读取 utmp 文件，并学会用 `open/read/close` 系统调用按结构读取文件；
3. 最后通过编写自己的 who 来检验对知识的掌握程度（who1.c → who2.c：过滤非用户记录、用 ctime 格式化时间）。

其中学到的联机帮助、头文件、utmp 结构、Unix 时间表示、结构化读取文件的方法，是掌握 Unix 编程的重要基础。
