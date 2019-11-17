//文件名:tries.h
//姓名:林浩通 学号:3170104908
#include "tries.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//创建节点
trie_node_t* create_trie_node() {
    trie_node_t* tree_node = NULL;
    //默认为NULL
    tree_node = (trie_node_t* )malloc(sizeof(struct trie_node));
    //申请空间
    if (tree_node == NULL) {
        printf("节点创建错误");
    }
    memset(tree_node->next, 0, sizeof(tree_node->next));
    return tree_node;
}

//插入节点
int insert_trie_node(trie_node_t* root, const char* name, const char* path) {
    trie_node_t* temp_node = root;
    int index;
    // 仅仅查找对应位置后，插入
    for(int i=0; i<strlen(name); i++) {
        if (name[i] < 'a' && name[i]>'z') {
            printf("不合法的命令名称");
            return -1;
        }
        index = name[i] - 'a';
        if (temp_node->next[index] != NULL) {
            temp_node = temp_node->next[index];
            continue;
        }
        //插入下一个字母的节点
        temp_node->next[index] = create_trie_node();
        temp_node = temp_node->next[index];
    }
    //到达位置，将路径复制上去
    strcpy(temp_node->path, path);
    return 0;
}

//搜索字典树
char* search_trie(trie_node_t* root, const char* name) {
    trie_node_t* temp_node = root;
    int index;
    if (root == NULL || name == NULL) {
        return NULL;
    }
    //按照字母顺序查找到对应位置
    for (int i=0; i<strlen(name); i++) {
        index = name[i] - 'a';
        if (temp_node->next[index] == NULL) {
            return NULL;
        }
        temp_node = temp_node->next[index];
    }
    return temp_node->path;
}

//销毁字典树
void destroy_trie(trie_node_t* root) {
    if (root == NULL) {
        return;
    }
    //递归销毁字典树
    for (int i=0; i<26; i++) {
        if (root->next[i] != NULL) {
            destroy_trie(root->next[i]);
        }
    }
    free(root);
    return;
}
