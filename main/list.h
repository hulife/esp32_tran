
#ifndef _LIST_H
#define _LIST_H

//#include "../mqtt/action_cb.h"

typedef struct item_t{
	unsigned short sn;	
	char command;
	unsigned short len;
	unsigned char data[234];
}item;

typedef struct node *link_ble; //定义节点指针
typedef struct node //节点定义
{
	item* data; //数据域
	link_ble next; //链域
}NODE;

link_ble make_head(link_ble l); //生成新空链表L
int is_empty(link_ble l); //判断链表是否为空
void add_tail(link_ble p, item* data); //在链表的尾部添加
//void print_link(link_ble p); //打印整个链表
link_ble search(link_ble l, unsigned short sn);  //查询内容序号为sn为data的链表的位置
//void insert(link_ble pos, item data, item value); //插入
//void modify(link_ble l, item data, item value);   //修改
//void delete(link_ble l, item data);  //删除
link_ble destory(link_ble l);   //摧毁整个链表

#endif

