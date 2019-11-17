// 文件名: job.c
// 姓名:林浩通 学号:3170104908
#include "job.h"
#include <wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

extern job_t* forejob;
extern job_t* jobs;

//等待前台进程结束
void wait_forejob() {
    //首先交由前台工作控制标准输入输出
    tcsetpgrp(STDIN_FILENO, forejob->pgid);

    while (forejob && forejob->process_num > 0) {
        //waitpid, WNOHANG能够快速检查，不需要等待
        int temp = waitpid(-forejob->pgid, NULL, WNOHANG);
        if (temp>0) {
            //如果temp>0，说明有进程结束，temo为结束的pid
            //这时候，我们让进程数目--
            forejob->process_num--;
        }
    }
    //将标准输入输出交给myshell控制
    tcsetpgrp(STDIN_FILENO, getpgrp());
}


//新建一个job
job_t* new_job(command_t* command) {
    //首先申请内存
    job_t* job = (job_t*)malloc(sizeof(struct job));
    //再者复制第一条命令，标识信息
    job->cmd = (char*)malloc(sizeof(char)*(strlen(command->next->argv[0])+1));
    strcpy(job->cmd, command->next->argv[0]);
    //初始化job的变量
    //process_num=0尤其重要
    //调试时难以发现bug
    job->pgid = 0;
    job->next = NULL;
    job->id = 0;
    job->process_num = 0;
    return job;
}

//根据一个作业，我们找到其上一个作业
job_t* find_last(job_t* job) {
    //从第一个开始查找
    job_t* last = jobs;
    while (last) {
        //下一个相等则返回
        if (last->next == job)
            return last;
        last = last->next;
    }
    return NULL;
}
//根据作业id，查找作业
job_t* find_job(int id) {
    //从第一个开始查找
    job_t* job = jobs;
    while (job) {
        //jobid相等则返回作业
        if (job->id == id)
            return job;
        job = job->next;
    }
    return NULL;
}
//打印工作
void print_job(int id) {
    job_t* job;
    if (id == 0) {
        //如果id为0，则打印全部作业
        job = jobs;
        while (job) {
            dprintf(STDOUT_FILENO, "[%d] %d\t%s\n", job->id, job->pgid, job->cmd);
            job = job->next;
        }
    }
    else {
        //否则找到制定作业后打印
        job = find_job(id);
        if (job)
            dprintf(STDOUT_FILENO, "[%d] %d\t%s\n", job->id, job->pgid, job->cmd);
        else
            fputs("job dose not exist!\n", stderr);

    }
}
//添加作业到后台作业列表
void add_job(job_t* job) {
    job_t* last = jobs;
    int id = 0;
    //首先找到目前最大的作业id
    while (last) {
        if (id<last->id)
            id = last->id;
        last = last->next;
    }
    //找到后，即得到了自己的作业id
    job->id = id + 1;
    last = find_last(NULL);

    if (last == NULL) {
        //如果作业列表为空的话
        jobs = job;
    }
    else {
        last->next = job;
    }
    job->next = NULL;
    //添加到后台以后，我们通常选择打印以下
    //标识作业的开始
    print_job(job->id);
}
