# 实验 1 - 问题 4：模拟简单 shell

> 参考教材：《Unix/Linux编程实践教程》（Bruce Molay 著，杨宗源、黄海涛译）
> 第 8 章「进程和程序：编写命令解释器 sh」（书 P228–257，核心：8.4 节 P234–249、8.5 节 P250–252）
> 注：PDF 页码 = 书页 + 20

## 一、shell 是什么

shell 是**命令行解释器**：打印提示符、读入你敲的命令、让系统运行它、再回到提示符。它本身也是个普通程序，靠**三个系统调用**的配合干活——模拟简单 shell，就是把这三个调用组装起来。

## 二、shell 是如何运行程序的（8.4，P234–249）

书上把"shell 如何运行程序"拆成 3 个问题：

| 问题 | 答案 | 书页 |
|---|---|---|
| 一个程序如何运行另一个程序 | `execvp`：用新程序**替换**当前进程（成功不返回） | P235–240 |
| 如何建立新的进程 | `fork`：把进程**复制**一份，返回两次 | P240–244 |
| 父进程如何等待子进程退出 | `wait`：挂起自己，直到子进程结束，拿到退出状态 | P244–249 |

**主循环模型（8.4.5，图 8.13）**——所有 Unix shell 的骨架：

```
打印提示符(write) → 读命令(read) → fork ─┬→ 子进程: exec → 新程序运行 → exit
                                          └→ 父进程: wait 等待 → 拿到状态 → 回到提示符
```

三个调用的关键语义：

**fork —— 复制进程**（P240–244）

```c
#include <unistd.h>
pid_t result = fork(void);
/* 调用一次、返回两次：
 *   子进程中返回 0；父进程中返回子进程的 pid；-1 出错 */
```

**execvp —— 换程序**（P235–240）

```c
#include <unistd.h>
execvp( const char *prog, char *const argv[] );
/* 在 PATH 里找 prog 并用它替换当前进程；
 * argv 是参数表，必须以 NULL 结尾。
 * 成功不返回；失败返回 -1（原程序还在） */
```

**wait —— 等子进程**（P249 API 表）

```c
#include <sys/types.h>
#include <sys/wait.h>
pid_t result = wait(int *statusptr);
/* 返回 -1 出错（如没有子进程）；否则为结束子进程的 pid，
 * 状态写入 statusptr */
```

## 三、书上参考代码：psh2.c（8.5，P250–252）

psh1 只能运行一条命令（exec 后自己就没了）；psh2 先 `fork`，shell 本体留在 `wait` 上，所以能循环服务。书上完整代码：

```c
/** prompting shell version 2
 **     Solves the 'one-shot' problem of version 1
 **     Uses execvp(), but fork()s first so that the
 **     shell waits around to perform another command
 **     New problem: shell catches signals. Run vi, press ^c.
 **/
#include    <stdio.h>
#include    <signal.h>

#define MAXARGS    20          /* cmdline args  */
#define ARGLEN     100         /* token length  */

main()
{
    char  *arglist[MAXARGS+1]; /* an array of ptrs  */
    int   numargs;             /* index into array  */
    char  argbuf[ARGLEN];      /* read stuff here   */
    char  *makestring();       /* malloc etc        */

    numargs = 0;
    while ( numargs < MAXARGS )
    {
        printf( "Arg[%d]? ", numargs );
        if ( fgets(argbuf, ARGLEN, stdin) && *argbuf != '\n' )
            arglist[numargs++] = makestring(argbuf);
        else
        {
            if ( numargs > 0 )            /* any args?    */
            {
                arglist[numargs]=NULL;    /* close list   */
                execute( arglist );       /* do it        */
                numargs = 0;              /* and reset    */
            }
        }
    }
    return 0;
}

execute( char *arglist[] )
/* use fork and execvp and wait to do it */
{
    int pid, exitstatus;       /* of child */

    pid = fork();              /* make new process */
    switch( pid ){
        case -1:
            perror( "fork failed" );
            exit(1);
        case 0:
            execvp(arglist[0], arglist);  /* do it */
            perror( "execvp failed" );
            exit(1);
        default:
            while( wait(&exitstatus) != pid )
                ;
            printf( "child exited with status %d,%d\n",
                    exitstatus>>8, exitstatus&0377 );
    }
}

char *makestring( char *buf )
/* trim off newline and create storage for the string */
{
    char *cp, *malloc();

    buf[strlen(buf)-1] = '\0';     /* trim newline */
    cp = malloc( strlen(buf)+1 );  /* get memory   */
    if ( cp == NULL ){             /* or die       */
        fprintf(stderr, "no memory\n");
        exit(1);
    }
    strcpy(cp, buf);               /* copy chars   */
    return cp;                     /* return ptr   */
}
```

两个必须理解的细节：

1. **`case 0` 里 execvp 失败必须 `exit(1)`**：execvp 失败时子进程还活着，如果不 exit，它会掉回主循环——shell 被复制成两个，提示符越敲越多。这是本章最经典的坑。
2. **`arglist[numargs] = NULL`**：execvp 靠 NULL 判断参数表到哪结束，忘了这步就是段错误/随机崩溃。

`exitstatus>>8` 是子进程的退出码，`&0377` 是杀死它的信号号（P249）。现代写法用 `<sys/wait.h>` 的宏更清楚：`WIFEXITED(status)` 判断是否正常退出、`WEXITSTATUS(status)` 取退出码。

## 四、建议的写法：整行读取 + 切词（更接近真 shell）

psh2 逐个参数提问（`Arg[0]?` `Arg[1]?`…），用起来别扭。真正的 shell 是读**一整行**再按空白切词——这正是第 9 章 smsh1 的思路（P263，`smsh1——命令行解析`）。建议这样组装：

1. `printf("$ "); fflush(stdout);` 打印提示符（不换行，记得刷新缓冲）；
2. `fgets(buf, sizeof(buf), stdin)` 读一整行；返回 NULL（Ctrl-D）就退出循环；
3. 去掉行尾 `'\n'`，用 `strtok(buf, " \t")` 把整行切成 `argv[]`，末尾补 `NULL`；
4. 切出 0 个词（空行/纯空格）就 `continue` 回到提示符；
5. `fork` + `switch` 三分支 + `wait`（照抄 psh2 的 `execute`）；
6. 回到第 1 步。

切词部分参考：

```c
/* 把 buf 按空白切成 argv，返回参数个数 */
int n = 0;
char *arg = strtok(buf, " \t");
while ( arg != NULL && n < MAXARGS ) {
    argv[n++] = arg;
    arg = strtok(NULL, " \t");
}
argv[n] = NULL;
```

（psh2 的 `makestring`/malloc 那套是给"逐个提问"用的；整行切词时 `strtok` 返回的指针就指在 `buf` 里，不用再拷贝。）

## 五、编译与测试

```sh
$ cc smsh.c -o smsh        # 或 gcc -Wall smsh.c -o smsh
$ ./smsh
$ ls                       # 试试各命令
$ date
$ ps                       # 能看到 smsh 和它的子进程 ps
$ nosuchcmd                # 故意输错：应打印 execvp failed...
$ Ctrl-D                   # 退出
```

测试清单：普通命令正常跑、退出码能观察（`exitstatus` 一行）、输错命令 shell 不死、Ctrl-D 能退出、连续执行多条命令提示符不重复。

## 六、现代 Linux 上的兼容性说明

- 头文件：`unistd.h`（fork/execvp）、`sys/wait.h`（wait 及状态宏）、`stdio.h`、`string.h`；
- 书上 `main()` 无参数原型能编译，现代规范写 `int main(void)` / `int main(int ac, char *av[])`；
- 书上 `char *malloc()` 这种老式声明**不要抄**，直接 `#include <stdlib.h>`；
- `strtok` 会改动原串（把空格改成 `\0`），`buf` 不需要保留就无所谓；想保留原始输入先 `strcpy` 一份；
- `wait` 循环写法 `while(wait(&st) != pid)` 是防"有多个子进程"的稳妥版，单子进程 `wait(&st)` 一次即可；
- psh2 开头注释那句 "New problem: shell catches signals. Run vi, press ^c"——**这正是下一个任务（信号处理）的伏笔**：现在 shell 和它运行的程序同属一个进程组，Ctrl-C 会连 shell 一起杀。

## 七、小结

- 模拟 shell = 一个死循环 + 三件套：`fork`（复制）→ 子进程 `execvp`（换程序）→ 父进程 `wait`（收尸 + 拿状态）；
- shell 本体从不 exec，所以它永远活着等你下一条命令；execvp 成功不返回，是"替换"不是"调用"；
- 三个易错点：execvp 失败不 exit（shell 越来越多）、argv 忘 NULL 结尾、wait 忘了写；
- 学到的调用：`fork` / `execvp` / `wait`，以及退出状态的两种读法（`>>8` 老式位运算、`WEXITSTATUS` 现代宏）。
