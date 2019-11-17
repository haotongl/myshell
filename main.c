// 程序名: myshell
// 姓名: 林浩通 学号:3170104908
#include "common.h"
#include "myshell.h"
#include "parse.h"
#include <stdlib.h>


int main(int argc, char** argv) {
    //首先定义命令，给命令申请空间
    command_t* command = (command_t*)malloc(sizeof(struct command));
    //作为临时读入缓冲
    char line[MAX_LINE];
    //bash执行标志
    int should_run = 1;
    //初始化shell
    init(argc, argv);

    while (should_run) {
        //打印提示符
        print_prompt();
        //读入命令
        read_command(line);
        if (strlen(line) <= 1)
            continue;
        //解析命令为命令串，
        //例如 pwd | ls | wc > test.txt & 为一个命令串
        //将会被解析为三条命令，以管道为分解符，
        //是否有重定向是单个命令的属性
        //是否后台执行是命令串的性质
        //这样一个命令串也被当做一个job, 每个命令（外部）将被看成一个process
        parse(line, command);
        //执行命令串
        execute_command(command);

    }
    free(command);
}
