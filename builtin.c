//文件名:builtin.c
//姓名:林浩通 学号:3170104908
#include "builtin.h"
#include "job.h"
#include "myshell.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <dirent.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

extern job_t* forejob;
extern trie_node_t* command_full_path;
extern variable_t variable_table[];
extern int num_variable;
extern variable_t variable_local[];
extern int num_local;
//局部变量表，用来储存set参数
variable_t temp_table[MAX_VARIABLE_NUM];
int temp_num;

//内置命令枚举,
enum builtin_cmd_number {
    bg = 1,
    cd,
    clr,
    dir,
    echo,
    exec,
    exit_,
    environ,
    fg,
    help,
    jobs,
    pwd,
    quit,
    set,
    shift,
    test,
    time_,
    umask_,
    unset,
    number_builtin
};

//内置命令字符串，用于扫描检查
char* builtin_table[] = {
        "bg",
        "cd",
        "clr",
        "dir",
        "echo",
        "exec",
        "exit",
        "environ",
        "fg",
        "help",
        "jobs",
        "pwd",
        "quit",
        "set",
        "shift",
        "test",
        "time",
        "umask",
        "unset",
};

//检查是否为内置命令，同时返回命令编号
int is_builtin(command_t* command) {
    //遍历命令列表，检查是否匹配
    for (int i=0; i<number_builtin-1; i++) {

        if (strcmp(builtin_table[i], command->argv[0]) == 0)
            return i+1;
    }
    return 0;
}
// 打开命令，
int cd_cmd(command_t* command) {
    //如果有参数，则我们打开参数所指目录
    if (command->argv[1]) {
        int ret = chdir(command->argv[1]);
        if (ret == -1)
            perror("cd");
        char* PWD = getcwd(NULL, 0);
        my_setenv(variable_table, &num_variable,"PWD", PWD, 1);
        free(PWD);
        return ret;
    }
    //否则我们打开当前目录
    //相当于没有操作
    //这是由于为简化myshell操作，
    //我们并没有区分用户
    //因此没有主目录
    return 0;
    
}


int _bg_cmd(int id) {
    //后台执行命令，我们需要找到工作
    //并且给工作的组pid发送继续信号
    //如果没有，则返回-1
    job_t* job = find_job(id);
    if (job)
        return kill(-job->pgid, SIGCONT);
    else
        return -1;
}

int _fg_cmd(int id) {
    //前台运行命令
    //首先找到作业
    //然后使用tcsetpgrp函数移交控制
    //同时检查作业是否能够继续
    //如果不能需要输出继续错误
    job_t* job = find_job(id);
    if (job != NULL) {
        tcsetpgrp(STDIN_FILENO,job->pgid);
        if(kill(-job->pgid,SIGCONT)<0)
        {
            fputs("Error continue job \n",stderr);
            return -1;
        }
        //否则设置前台进程
        //并且等待前台进程结束
        forejob = job;
        wait_forejob();
        return 0;
    }
    else {
        //如果工作不存在，直接返回
        return -1;
    }
}


int bg_cmd(command_t* command) {
    //这里做了一个简单的解析功能
    //检查了输入的正确性
    //将参数转化为数字
    if (command->argc > 1)
        return _bg_cmd(atoi(command->argv[1]));
    return -1;
}

int fg_cmd(command_t* command) {
    //这里做了一个简单的解析功能
    //检查了输入的正确性
    //将参数转化为数字
    if (command->argc>1)
        return _fg_cmd(atoi(command->argv[1]));
    return -1;
}

int jobs_cmd(command_t* command, int* fd) {
    //这里由于需要进行输出
    //可能用于管道操作
    //为了避免设计的复杂性
    //我们选择创建子进程
    
    pid_t pid = fork();
    //如果子进程创建失败
    if (pid < 0) return pid;
    
    if (pid == 0) {
        //这里是子进程
        do_redirect(command, fd);
        
        //如果没有参数，这代表打印所有工作
        if (!command->argv[1]) {
            print_job(0);
        }
        else {
            //否则打印制定工作
            print_job(atoi(command->argv[1]));
        }
        exit(0);
    }
    else if (pid>0) {
        //关闭管道，以防阻塞
        closepipe(fd);
        return pid;
    }
}
pid_t echo_cmd(command_t* command, int* fd) {
    //这里由于需要进行输出
    //可能用于管道操作
    //为了避免设计的复杂性
    //我们选择创建子进程
 
    pid_t pid = fork();
    if (pid == 0) {
        //首先做重定向j
        do_redirect(command, fd);
        //获取参数
        char** arg = command->argv+1;
        while (*arg) {
            //然后接连打印即可
            //由于参数可能是变量
            //我们需要简单检查
            char* real = *arg;
            if ((*arg)[0] ==  '$')
                 real = my_getenv(variable_table, num_variable, *arg+1);
            if (strcmp(real, "") == 0)
                real = my_getenv(variable_local, num_local, *arg+1);
            dprintf(STDOUT_FILENO, "%s ", real);
            arg++;
        }
        //最后我们需要输出换行j
        dprintf(STDOUT_FILENO, "\n");
        exit(0);
    }
    else if (pid > 0) {
        //父进程关闭管道
        closepipe(fd);
        return pid;
    }
}
pid_t clr_cmd() {
    //清屏
    //这里打印清屏
    //回到行首
    //回到列首即可
    static char clear[] = "\033[1A\033[2J\033[H";
    ssize_t result = write(STDOUT_FILENO,clear,strlen(clear));
    return 0;
}

pid_t dir_cmd(command_t* command, int* fd) {
    //浏览目录
    //这里由于需要进行输出
    //可能用于管道操作
    //为了避免设计的复杂性
    //我们选择创建子进程
 
    pid_t pid = fork();
    
    if (pid == 0) {
        //首先重定向
        do_redirect(command, fd);
        DIR* dir;
        //然后调用opendir, readdir
        //即可遍历浏览目录下文件
        //如果有参数
        //则打开指定目录
        //若无参数
        //浏览挡墙目录即可
        struct dirent* point_dir;
        if (command->argv[1])
            dir = opendir(command->argv[1]);
        else
            dir = opendir(my_getenv(variable_table, num_variable,"PWD"));
        if (dir == NULL) {
            perror("dir error");
            exit(-1);
        }
        while ((point_dir = readdir(dir)) != NULL) {
            //打印文件名称
            dprintf(STDOUT_FILENO, "%s\n", point_dir->d_name);
        }
        closedir(dir);
        exit(0);
    }
    else if (pid > 0) {
        //父进程关闭通道
        closepipe(fd);
        return pid;
    }
}

void exec_cmd(command_t* command, int* fd) {
    //执行程序
    //由于exec族是直接替代进程
    //所以我们不必创建子进程
    if (command->argv[1]) {
        //做好重定向后
        do_redirect(command, fd);
        //查询路径
        char* path = search_trie(command_full_path, command->argv[1]);
        //执行即可
        execv(path, command->argv+1);
        perror("exec error!\n");
        exit(-1);
    }
}

pid_t environ_cmd(command_t* command, int* fd) {
    //这里由于需要进行输出
    //可能用于管道操作
    //为了避免设计的复杂性
    //我们选择创建子进程
    
    pid_t pid = fork();
   
    if (pid == 0) {
        //从重定向开始做起
        do_redirect(command, fd);

        for (int i=0; i<num_variable; i++) {
            //之后遍历打印环境变量列表即可
            dprintf(STDOUT_FILENO, "%s: %s\n", variable_table[i].name, variable_table[i].value);
        }
        exit(0);
    }
    else {
        //主进程关闭管道
        closepipe(fd);
        return pid;
    }

}

pid_t pwd_cmd(command_t* command, int* fd) {
    //这里由于需要进行输出
    //可能用于管道操作
    //为了避免设计的复杂性
    //我们选择创建子进程

    pid_t pid = fork();

    if (pid == 0) {

        do_redirect(command, fd);
        //打印变量PWD即可
        char* pwd = my_getenv(variable_table, num_variable, "PWD");
        dprintf(STDOUT_FILENO, "%s\n", pwd);
        exit(0);
    }
    else if (pid > 0) {
        //父进程关闭管道，以防阻塞
        closepipe(fd);
        return pid;
    }
}

pid_t test_cmd(command_t* command) {
   //test命令必须有两个额外的参数
   if (command->argc != 3)
       return -1;
    //test命令比较两个参数是否一样，一样输出true
    if (strcmp(command->argv[1], command->argv[2]) == 0)
        dprintf(STDOUT_FILENO, "true");
    //不一样输出false
    else
        dprintf(STDOUT_FILENO, "false");
    return 0;
}

//打印当前时间
pid_t time_cmd(command_t* command, int* fd) {
    //这里由于需要进行输出
    //可能用于管道操作
    //为了避免设计的复杂性
    //我们选择创建子进程


    //创建子进程
    pid_t pid = fork();

    //如果是子进程
    if (pid == 0) {
        if (command->argv[1] == NULL) {
            //重定向
            do_redirect(command, fd);
            char current_time[MAX_LINE];
            memset(current_time, 0,  MAX_LINE);
            time_t now;
            //通过time函数获取当前时间
            //之后转化为固定格式后打打印
            time(&now);
            strftime(current_time, MAX_LINE, "%Y-%m-%d %H:%M:%S", gmtime(&now));
            dprintf(STDOUT_FILENO, "%s\n", current_time);
            exit(0);
        }
    }
    else if (pid > 0) {
        closepipe(fd);
        //父进程关闭管道
        return pid;
    }
}

pid_t unset_cmd(command_t* command) {
    //设置环境变量为空
    //即取消环境变量
    my_setenv(variable_local, &num_local ,command->argv[1], "", 1);
    return 0;
}

pid_t umask_cmd(command_t* command, int* fd) {
    //获取掩码
    //设置掩码
    //如果没有参数
    //我们直接调用umask函数设置即可
    if (command->argv[1]) {
        umask(strtol(command->argv[1], NULL, 8));
        return 0;
    }
    else {
        //否则我们需要打印掩码
        //存在打印的地方
        //就存在管道重定向的可能
        //所以我们创建了一个子进程
        //
        //创建子进程
        pid_t pid = fork();
        if (pid == 0) {
            //子进程做重定向
            do_redirect(command, fd);
            mode_t mode;
            //同时两遍设置即可完成获取并不改变
            umask((mode = umask(0)));
            //之后打印
            dprintf(STDOUT_FILENO, "%04o\n", mode);
            exit(0);
        }
        else if (pid>0) {
            //父进程关闭管道以防阻塞
            closepipe(fd);
            return pid;
        }
    }
}

void set_cmd(command_t* command) {
    //set，将后面的参数设置到临时变量表
    //并将前10个设置到local变量表
    int i = 1;
    char** arg = command->argv + 1;
    while (*arg) {
        char temp[4];
        sprintf(temp, "%d", i);
        strcpy(temp_table[i].name, temp);
        strcpy(temp_table[i].value, *arg);
        //set 前十个
        if (i<10)
            my_setenv(variable_local, &num_local, temp, *arg, 1);

        arg++; i++;
    }
    //更新$#
    temp_num = i;
    char temp[4];
    sprintf(temp, "%d", temp_num);
    my_setenv(variable_local, &num_local, "#", temp, 1);

}

void shift_cmd(command_t* command) {
    //移动参数
    //这里只需将临时变量表中的参数
    //前移即可
    //并讲前10个设置在local表中
    if (temp_num <= 1) {

        perror("argc must be greater than 1");
        return ;
    }
    temp_num = temp_num - 1;
    for (int i=0; i<temp_num; i++) {
        char temp[4];
        sprintf(temp, "%d", i);
        strcpy(temp_table[i].name, temp);
        //设置后面的为前面的
        //即完成了前移
        strcpy(temp_table[i].value, temp_table[i+1].value);
        my_setenv(variable_local, &num_local, temp, temp_table[i].value, 1);
    }
    //我们也要完成$#的更新
    char temp[4];
    sprintf(temp, "%d", temp_num);
    my_setenv(variable_local, &num_local, "#", temp, 1);
}

void help_cmd() {
    //这里定义帮助信息后打印
    static char help_info[] = "myshell beta v1.0 \n\
内建命令             简介     \n\
bg                   后台执行命令 \n\
cd                   更改当前工作目录 \n\
clr                  清屏 \n\
dir                  列出当前目录 \n\
echo                 输出参数并且换行 \n\
exec                 执行程序替换当前进程 \n\ 
exit                 退出myshell\n\
environ              列出所有环境变量\n\
fg                   前台执行命令\n\
help                 显示帮助信息\n\
jobs                 查看当前所有在执行的工作\n\
pwd                  显示当前目录\n\
quit                 退出myshell\n\
set                  设置参数到0~9，更多的可以使用shift查看\n\
shift                移动参数\n\
test                 测试表达式\n\
time                 显示当前时间\n\
umask                显示/设置当前掩码\n\
unset                取消变量\n\
\n\
外部命令支持/bin、/usr/bin以及./下所有的可执行程序\n\
\n\
我们也支持管道，重定向，后台执行命令的任意的多重组合。\n\
\n\
更多信息参见README, 谢谢\n";

    printf("%s", help_info);


}


//退出myshell
void quit_cmd() {
    //退出时，需要检查是否还有工作在运行
    //若存在，则不能退出
    job_t* j = jobs;
    if (jobs == NULL)
        exit(0);   
    perror("Error, there are jobs running\n");
} 





pid_t exec_builtin(command_t* command, int* fd) {
    int ret = -1;
    //默认返回值为-1
    //根据is_builtin解析的命令类型
    //调用对应函数即可
    switch(is_builtin(command)){
        case bg: ret = bg_cmd(command); break;
        case cd: ret = cd_cmd(command); break;
        case fg: ret = fg_cmd(command); break;
        case jobs: ret = jobs_cmd(command, fd); break;
        case echo: ret = echo_cmd(command, fd); break;
        case clr: ret = clr_cmd(); break;
        case quit: exit(0); break;
        case exit_: exit(0); break;
        case dir: ret = dir_cmd(command, fd); break;
        case exec: exec_cmd(command, fd); break;
        case environ: ret = environ_cmd(command, fd); break;
        case help: help_cmd(); ret = 0; break;
        case pwd: ret = pwd_cmd(command, fd); break;
        case test: ret = test_cmd(command); break;
        case time_: ret = time_cmd(command, fd); break;
        case unset: ret = unset_cmd(command); break;
        case umask_: ret = umask_cmd(command, fd); break;
        case set: set_cmd(command); ret = 0; break;
        case shift: shift_cmd(command); ret = 0; break;
        default: break;
    }
    return ret;
}

