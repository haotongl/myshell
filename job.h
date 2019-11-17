// 文件名称: job.h
// 姓名: 林浩通 学号:3170104908
#ifndef MYSHELL_JOB_H
#define MYSHELL_JOB_H

#include "common.h"

#include <fcntl.h>
#include <zconf.h>

//job结构
typedef struct job {
    pid_t pgid;        //作业组pid
    int process_num;   //活跃进程数目
    int id;            //jobid
    char* cmd;         //命令文本
    struct job* next;  //链表
} job_t;

//等待前台作业结束
void wait_forejob();
//新建作业
job_t* new_job(command_t* command);
//根据作业，找到其上一个作业
job_t* find_last(job_t* job);
//根据作业id找到这个作业
job_t* find_job(int id);
//打印作业
void print_job(int id);
//添加作业列表
void add_job(job_t* job);

#endif 
