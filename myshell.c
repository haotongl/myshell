// 程序名: myshell
// 姓名:林浩通 学号:3170104908
#include "myshell.h"
#include "common.h"
#include "job.h"
#include "builtin.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <zconf.h>
#include <dirent.h>
#include <wait.h>


//定义外部命令路径作为一个字典树，在初始化shell，set_path时调用
trie_node_t* command_full_path;
//定义环境变量表
variable_t variable_table[MAX_VARIABLE_NUM];
//定义shell变量表
variable_t variable_local[MAX_VARIABLE_NUM];
int num_variable = 0;
int num_local = 0;

//myshell可能被作为后台进程调用，我们定义is_interactive表示其是否是交互的
int is_interactivte = 1;
//我们定义前台工作以及后台工作列表
job_t* forejob = NULL;
job_t* jobs = NULL;

// ============================ 命令执行相关 ===================================

//解析重定向命令
int do_redirect(command_t* command, int* fd) {
    int infile = STDIN_FILENO, outfile = STDOUT_FILENO;
    if (command->infile) {
        //如果输入重定向
        infile = open(command->infile, O_RDONLY);
        if (infile != -1)
            dup2(infile, STDIN_FILENO);
        else {
            perror("redirect error\n");
            return -1;
        }
    }
    if (command->outfile) {
        //如果输出重定向
        int flag = O_WRONLY | O_CREAT;
        if (command->outfile_type == 2) {
            //如果是>>, 则需要添加一个选项
            flag |= O_APPEND;
        }
        mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
        outfile = open(command->outfile ,flag, mode);
        if (outfile != -1)
            dup2(outfile, STDOUT_FILENO);
        else {
            perror("redirect error\n");
            return -1;
        }
    }
    if (infile == STDIN_FILENO)
        dup2(fd[0], STDIN_FILENO);
    if (outfile == STDOUT_FILENO && command->next)
        dup2(fd[1], STDOUT_FILENO);
    return 0;
}

//关闭管道，以防阻塞
void closepipe(int pipefd[2])
{
    if(pipefd[0]!=STDIN_FILENO)
        close(pipefd[0]);
    if(pipefd[1]!=STDOUT_FILENO)
        close(pipefd[1]);
}

pid_t execute_single(command_t* command, int* fd) {
    fflush(stdin);
    fflush(stdout);
    pid_t pid;
    //单个命令只需要考虑是内置还是外置命令即可
    if (is_builtin(command)) {
        //执行内部命令
        pid = exec_builtin(command, fd);
        return pid;
    }
    //如果是外部命令
    if ((pid = fork()) < 0)
        return pid;
    else if (pid == 0) {
        if (do_redirect(command, fd) == -1)
            exit(-1);
        //则创建子进程
        //首先查找命令路径
        //在执行命令
        char* path = search_trie(command_full_path, command->argv[0]);
        execv(path, command->argv);
        perror("command error!\n");
        printf("commdsd");
        exit(-1);
    }
    else{
        //父进程关闭管道以防阻塞
        closepipe(fd);
        return pid;
    }
    //todo: free
}

int execute_command(command_t* command) {
    //首先给命令串创建一个工作
    job_t* job = new_job(command);
    //定义两个管道，pipefd做框架
    int pipefd[2] = {STDIN_FILENO, STDOUT_FILENO};
    int fd[2];
    //命令串是否后台执行
    char foreground = command->foreground;
    //command是有一个空的头结点
    command = command->next;

    while (command) {
        //如果命令不空就继续执行下去
        fd[0] = pipefd[0];
        if (command->next) {
            //如果有下一条命令，则有管道
            pipe(pipefd);
        }
        else {
            //否则就没有管道
            pipefd[1] = STDOUT_FILENO;
        }
        fd[1] = pipefd[1];
        //执行单条命令
        pid_t pid = execute_single(command, fd);
        //设置job的组id，进程数目
        if (!job->pgid)
            job->pgid = pid;
        if (pid > 0) {
            setpgid(pid, job->pgid);
            job->process_num++;
        }
        command = command->next;
    }
    if (foreground) {
        //如果命令在前台执行
        forejob = job;
        wait_forejob();
        if (job == forejob) {
            //如果在等待过程中，前台进程没有被突然终止
            free(job);
            forejob = NULL;
        }
    }
    else {
        //添加工作到工作列表
        add_job(job);
    }
}
// ========================= 命令执行结束 ====================================

// ========================= 设置路径相关 ====================================

//这里简化了外部命令的设计，所有的外部命令名称必须是全部字母
static int is_illegal(const char* name) {
    for (int i=0; i<strlen(name); i++) {
        if (name[i] < 'a' || name[i] > 'z')
            return 0;
    }
    return 1;
}

//设置路径
static void set_path(const char* new_path) {
    //首先设置环境变量PATH
    my_setenv(variable_table, &num_variable, "PATH", new_path, true);

    //其次要建立一棵字典树，方便快速查找路径
    command_full_path = create_trie_node();
    char* temp_path;
    struct dirent* temp_dir;

    //这里定义一个herepath方便对const 路径变量的改变
    char herepath[MAX_PATH_LENGTH];
    strcpy(herepath, new_path);
    temp_path = herepath;
    int length = strlen(new_path);
    //向字典树添加节点
    for (int i=0; i < length; i++) {
        if (new_path[i] == ':') {
            herepath[i] = 0;
            DIR* dir_stream = opendir(temp_path);
            int num = 0;
            //遍历目录文件
            while (temp_dir = readdir(dir_stream)) {
                //如果文件是可执行文件切文件名字全部为字母
                //则添加至字典树
                if(is_illegal(temp_dir->d_name) && temp_dir->d_type==DT_REG) {
                    char path[MAX_PATH_LENGTH];
                    path[0] = 0;
                    strcat(path, temp_path);
                    strcat(path, "/");
                    strcat(path, temp_dir->d_name);
                    insert_trie_node(command_full_path, temp_dir->d_name, path);
                }
                num++;
            }
            closedir(dir_stream);
            temp_path = (herepath+i+1);
        }
    }
    return ;
}

// ================================ 设置路径结束 ==================================

// ================================ 信号处理 ==============================
static void job_done(job_t* job)
{
    //从jobs列表删除已完成的job
    job_t *last = find_last(job);
    if(!last)
        jobs=job->next;
    else
        last->next = job->next;
    //打印工作结束信息
    dprintf(STDOUT_FILENO,"[%d] %d done:\t%s\n"
            ,job->id, job->pgid,job->cmd);
    //释放job的空间
    free(job);
}

static void process_done(job_t* job)
{
    //进程结束
    job->process_num--;
    //当一个job的进程数为0，这意味着job结束了
    if(job->process_num == 0)
        job_done(job);
}

// 检查更新前台进程执行状态
static void fore_stop()
{
    //如果前台进程为空，结束
    if(!forejob) return;

    static job_t *last_job = NULL;
    static pid_t job_count = 0;

    if(!last_job||last_job!=forejob)
    {
        //初始化last_job 与 job_count
        last_job = forejob;
        job_count = 0;
    }
    if(waitid(P_PGID,forejob->pgid,NULL,WNOHANG|WSTOPPED)==0
       &&++job_count == forejob->process_num)
    {
        //更新last_job与job_count
        job_count = 0;
        last_job = NULL;
        job_t* job = forejob;
        //结束前台等待进程
        forejob = NULL;
        dprintf(STDOUT_FILENO,"%c",'\n');
        //如果是前台进程被结束掉，需要拿一个job_id
        if(job->id==0)
            add_job(job);
        //设置terminal为前台控制
        tcsetpgrp(STDIN_FILENO,getpgrp());
    }
}

//检查所有的后台job, 如果有进程结束，立马返回
//否则返回-1
static pid_t wait_job()
{
    job_t *job = jobs;
    pid_t pid = 0;
    while(job)
    {
        pid = waitpid(-jobs->pgid,NULL,WNOHANG);
        if(pid > 0)
        {

            process_done(job);
            return pid;
        }
        //Next job.
        job = job->next;
    }
    return -1;

}

static void inthdlr(int signo)
{
    //给前台执行命令的进程结束操作
    if(forejob)
        kill(-forejob->pgid, SIGINT);
    else
        //否则结束myshell
        exit(1);
}

static void chldhdlr(int sig)
{
    //检查前台进程
    fore_stop();
    //重复检查直到没有进程停止
    while(wait_job()>0);
}

// ============================ 信号处理结束 =====================================

void init(int argc, char** argv) {

    //初始化信号
    //子进程结束信号
    signal(SIGCHLD, chldhdlr);
    //ctrl+c信号
    signal(SIGINT, inthdlr);
    //其他常用信号忽略或默认
    signal(SIGTSTP, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);

    fflush(stdout);
    if (argc > 1) {
        //如果参数不只myshell的话，考虑重定向和后台执行命令
        if (argv[argc-1][0] == '&') {
            //如果此shell是后台执行的，则is_interactive=0
            is_interactivte = 0;
        }
        //重定向表准输入到脚本
        //这里直接重定向即可，在parse时会做解析
        int fd = open(argv[1], O_RDONLY);
        if (fd == -1)
            exit(1);
        dup2(fd, STDIN_FILENO);
    }

    //设置初始化路径, 这里面会建一棵命令路径字典树
    set_path("/bin:/usr/bin:./:");

    //设置参数，将参数设置到shell变量表上
    //$0~$9
    char** arg = argv;
    char temp[2];
    for (int i=0; i<10 && *arg; i++) {
        sprintf(temp, "%d", i);
        temp[1] = 0;
        my_setenv(variable_local, &num_local, temp, *arg, true);
        arg++;
    }
    //$#
    sprintf(temp, "#");
    temp[1] = 0;
    char value[2];
    sprintf(value, "%d", argc);
    value[1] = 0;
    my_setenv(variable_local, &num_local, temp, value, true);

    //初始化环境变量
    //包括USER, HOST, PWD等等常用环境变量
    my_setenv(variable_table, &num_variable, "USER", "haotongl", true);
    my_setenv(variable_table, &num_variable, "HOST", "ubuntu", true);
    char* current_path = getenv("PWD");
    my_setenv(variable_table, &num_variable, "PWD", current_path, true);

    //打印代表初始化完毕，可以开始使用
    printf("----------------------------\n");
    printf("- 名称: myshell             -\n");
    printf("- 作者: 林浩通               -\n");
    printf("-                          -\n");
    printf("- 欢迎使用！                 -\n");
    printf("----------------------------\n");
}


// 打印提示符
void print_prompt() {
    //首先查看PS1是否被设置，如果设置，则直接打印PS1
    char* PS1 = my_getenv(variable_table, num_variable,"PS1");
    if (strcmp(PS1, "") != 0) {
        printf("%s", PS1);
    }
    else {
        //否则需要先获取PWD, 当前路径
        char* PWD=my_getenv(variable_table, num_variable,"PWD");
        //模仿bash配色，打印myshell:path$
        printf("\033[1;32mmyshell");
        printf("\033[0m:\033[1;34m%s", PWD);
        printf("\033[0m$ ");
    }
}







//============================== 变量管理相关 ============================
char* my_getenv(variable_t* variable_table, int num_variable, const char* name) {
    //遍历查询对比变量名，
    //存在即返回变量值
    //不存在返回""
    for (int i=0; i<num_variable; i++) {
        if (strcmp(name, variable_table[i].name) == 0) {
            return variable_table[i].value;
        }
    }
    return "";
}
void my_setenv(variable_t* variable_table, int* num_variable, const char* name, const char* value, int replace) {
    //设置某个变量
    //我们首先检查此变量是否存在
    //如果存在，根据replace位决定是否替换或者保留

    //获取变量
    const char* old_value = my_getenv(variable_table, *num_variable, name);

    if (strcmp(old_value, "") == 0) {
        // 如果之前没有，则直接设置即可
        strcpy(variable_table[*num_variable].name, name);
        strcpy(variable_table[*num_variable].value, value);
        (*num_variable) = *num_variable + 1;
    }
    else if(replace == 1) {
        // 否则，我们替换此变量
        for (int i=0; i<*num_variable; i++) {
            if (strcmp(name, variable_table[i].name) == 0) {
                strcpy(variable_table[i].value, value);
                break;
            }
        }
    }
    //亦或者什么都不做，如果replace = 0
    return;
}
// =============================== 变量管理结束 ================================
