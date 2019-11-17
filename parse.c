//文件名:parse.c
//姓名:林浩通 学号:3170104908
#include "parse.h"
#include <stdbool.h>

//这里会更新arg的位置
//使其指向下一个参数
//同时会返回当前参数的长度
static int get_arg(const char* cmd, char** arg) {
    int start = 0, i;
    int flag = true;//没有找到参数


    for (i=0; cmd[i]!=0 && cmd[i]!='\n'; i++) {
        if (cmd[i] == ' ' || cmd[i] == '\t') {
            if (flag == false) {
                break;
            }
            flag = true;
        }
        else {
            //如果找到参数
            //则首先记录位置
            if (flag == true) {
                start = i;
            }
            flag = false;
        }
    }
    
    if (flag == false) {
        //返回长度
        *arg = cmd + start;
        return i-start;
    }
    arg = NULL;
    return 0;
}

//数一下参数
static int count_arg(const char* line) {
    int num_arg = 0;
    char* arg;
    //通过不断的get_arg
    //往后找参数
    //num_arg累加
    //即可获得参数数目
    int length = get_arg(line, &arg);
    while (length > 0) {
        num_arg++;
        line = arg+length;
        length = get_arg(line, &arg);
    }
    return num_arg;
}
//parse 一个命令， 所有由空格制表符分隔开的
//均被当做一个参数parse
static void _parse(const char* line, command_t* cmd) {
    cmd->argc = count_arg(line);
    cmd->argv = NULL;
    char* arg;
    if (cmd->argc > 0) {
        cmd->argv = (char**)malloc((cmd->argc+1)*sizeof(char*));
        for (int i=0; i<cmd->argc; i++) {
            //首先得到当前参数的长度
            //同时arg指针移步下一个参数
            //
            int arg_length = get_arg(line, &arg);
            line = arg+arg_length;
            //给参数申请空间
            cmd->argv[i] = (char*)malloc(arg_length+1);
            for (int j=0; j<arg_length; j++) {
                //复制arg 
                cmd->argv[i][j] = arg[j];
            }
            cmd->argv[i][arg_length] = 0;
        }
        cmd->argv[cmd->argc] = NULL;
    }
    //之后需要处理重定向
    //我们假设重定向符号均在命令的最后
    //我们直接检查倒数第二个参数是否是重定向符号
    //如果是的话我们释放掉后面两个参数
    //并且设置infile, outfile, outfile_type即可
    //
    //
    //首先遍历检查
    cmd->outfile_type = 0;
    int argc = cmd->argc;
    for (int i=0; i<argc; i++) {
        //首先是<
        if (strcmp(cmd->argv[i], "<") == 0) {
            cmd->argc -= 2;
            cmd->infile = (char*)malloc(strlen(cmd->argv[i+1]));
            stpcpy(cmd->infile, cmd->argv[i+1]);
            free(cmd->argv[i]);
            free(cmd->argv[i+1]);
            cmd->argv[i] = NULL;
        }
        else if (strcmp(cmd->argv[i], ">") == 0) {
            //再者是>
            cmd->argc -= 2;
            cmd->outfile = (char*)malloc(strlen(cmd->argv[i+1]));
            stpcpy(cmd->outfile, cmd->argv[i+1]);
            cmd->outfile_type = 1;
            free(cmd->argv[i]);
            free(cmd->argv[i+1]);
            cmd->argv[i] = NULL;
        }
        else if (strcmp(cmd->argv[i], ">>") == 0) {
            //最后是>>
            //思路均与前述一致
            //固不做赘述
            cmd->argc -= 2;
            cmd->outfile = (char*)malloc(strlen(cmd->argv[i+1]));
            stpcpy(cmd->outfile, cmd->argv[i+1]);
            cmd->outfile_type = 2;
            free(cmd->argv[i]);
            free(cmd->argv[i+1]);
            cmd->argv[i] = NULL;
        }
    }
    cmd->next = NULL;
}

//解析命令
void parse(char* line, command_t* command) {
    char copy_line[MAX_LINE];
    //首先定义command->next为NULL
    command->next = NULL;
    if (line[strlen(line)-2] == '&') {
        //如果是&, 则此作业是后台执行
        command->foreground = 0;
        //并把它去掉
        line[strlen(line)-2] = 0;
    }
    else
        //否则就是前台执行
        command->foreground = 1;

    //解析注释
    for (int i=0; i<strlen(line); i++) {
        if (line[i] == '#')
            line[i] = 0;
    }
    strcpy(copy_line, line);
    // 这里要吧管道符号换成空，然后只需要parse一个命令
    int last_start = 0;
    for (int i=0; i<strlen(line); i++) {
        if (line[i] == '|' || line[i+1] == 0) {
            if (line[i] == '|')
                copy_line[i] = 0;
            command->next = (command_t*)malloc(sizeof(struct command));
            command = command->next;
            command->infile = NULL;
            command->outfile = NULL;
            //parse一个命令
            _parse(copy_line+last_start, command);
            last_start = i+1;
        }
    }
    command->next = NULL;
}


void read_command(char* line) {
    //如果读取到文件末端
    //exit
    if( fgets(line, MAX_LINE, stdin) == NULL)
        exit(0);
}
