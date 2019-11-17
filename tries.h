// 文件名: tries,h
// 姓名:林浩通 学号: 3170104908
#ifndef MYSHELL_TRIES_H
#define MYSHELL_TRIES_H

#include "common.h"
//字典树节点
//储存路径
typedef struct trie_node {
    char path[MAX_PATH_LENGTH];
    struct trie_node* next[26];
} trie_node_t;

//包括创建节点
trie_node_t* create_trie_node();
//插入节点
int insert_trie_node(trie_node_t* root, const char* name, const char* path);
//搜索节点
char* search_trie(trie_node_t* root, const char* name);
//销毁字典树
void destroy_trie(trie_node_t* root);

#endif //MYSHELL_TRIES_H
