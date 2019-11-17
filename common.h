//文件名:common.h
//姓名:林浩通 学号:3170104908
#ifndef MYSHELL_COMMON_H
#define MYSHELL_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <zconf.h>
#include <dirent.h>
#include <wait.h>

#define MAX_LINE 80
#define MAX_VARIABLE_NAME 55
#define MAX_VARIABLE_VALUE 255
#define MAX_VARIABLE_NUM 512
#define MAX_PATH_LENGTH 255

//这里定义了一些公共结构以及宏定义
typedef struct command {
    int argc;            //参数数量
    char** argv;         //参数
    char* infile;        //重定向读入文件
    char* outfile;       //重定向输出文件
    char outfile_type;   //重定向输出类型 >>=2
    char foreground;     //命令组在前台或者后台执行
    struct command* next;//链表
} command_t;

typedef struct {
    char name[MAX_VARIABLE_NAME];
    char value[MAX_VARIABLE_VALUE];
} variable_t;

#endif
