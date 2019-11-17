//文件名: builtin.h
//姓名:林浩通 学号:3170104908
#ifndef MYSHELL_BUILTIN_H
#define MYSHELL_BUILTIN_H
#include "common.h"

// 对外命令只有检查是否为内置命令
// 以及内置命令
// 检查内置命令需要参数命令
int is_builtin(command_t* command);
// 执行内置命令需要管道，输入、输出描述符
// 用以完成重定向、管道任务
pid_t exec_builtin(command_t* command, int* fd);

#endif //MYSHELL_BUILTIN_H
