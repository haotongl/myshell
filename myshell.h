//程序名:myshell
//姓名:林浩通 学号:3170104908
#ifndef MYSHELL_MYSHELL_H
#define MYSHELL_MYSHELL_H

#include "tries.h"
#include "common.h"

//初始化myshell
void init(int argc, char** argv);
//打印提示符
void print_prompt();
//执行命令串作为job
int execute_command(command_t* command);

//关闭管道，避免管道阻塞
void closepipe(int pipefd[2]);
//做重定向
int do_redirect(command_t* command, int* fd);

//环境变量以及shell变量管理
char* my_getenv(variable_t* variable_table, int num_variable, const char* name);
void my_setenv(variable_t* variable_table, int* num_variable, const char* name, const char* value, int replace);

#endif
