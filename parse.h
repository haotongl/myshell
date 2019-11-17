//文件名:parse.h
//姓名:林浩通 学号:3170104908
#ifndef MYSHELL_PARSE_H
#define MYSHELL_PARSE_H

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "common.h"

//读取命令
void read_command(char* line);
//解析命令
void parse(char* line, command_t* command);


#endif 
