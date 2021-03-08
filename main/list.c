#include <stdio.h>
#include <stdlib.h>
//#include <malloc.h>
#include "list.h"
#include "esp_log.h"
static const char *TAG1 = "LIST1";
link_ble make_head(link_ble l)
{
	static const char *TAG = "LIST";
	l = (link_ble)malloc(sizeof(NODE));
	if (l==NULL)
	{
		 ESP_LOGW(TAG,"no memory ________MAKEHEAD\n");   
	}


	{
		/* code */
	}
	
	l->next = NULL;
	return l;
}

link_ble search(link_ble l, unsigned short sn)
{
	while (l != NULL)
	{
		if (l->data->sn == sn)
			break;
		l = l->next;
	}
	if (l == NULL)
	{
		 printf("not found sn=%d\n",sn);
		return NULL;
	}
	else
	{
		//printf("found sn=%d\n", l->data->sn);
		return l;
	}
}
/*
void delete(link l, item data)
{
	link tmp;
	item d_data;
	if (!l)
	{
		printf("当前链表为空\n");
		return;
	}
	while (l != NULL)
	{
		if (l->next != NULL && l->next->data == data)
		{
			tmp = l->next;
			l->next = tmp->next;
			d_data = tmp->data;
			free(tmp);
			break;
		}
		l = l->next;
	}
	if (l == NULL)
	{
		printf("当前链表中没有找到数值为%d的数据块!\n", data);
		printf("更新失败！\n");
	}
	else
	{
		printf("删除数据项%d成功!\n", d_data);
	}

}
void modify(link l, item data, item value)
{
	if (!l)
	{
		printf("当前链表为空\n");
		return;
	}
	while (l != NULL)
	{
		if (l->data == data)
		{
			l->data = value;
			break;
		}
		l = l->next;
	}
	if (l == NULL)
	{
		printf("当前链表中没有找到数值为%d的数据块!\n", data);
		printf("更新失败！\n");
	}
	else
	{
		printf("更新成功!\n");
	}

}
void insert(link pos, item data, item value)
{
	if (!pos)
	{
		printf("当前链表为空\n");
		return;
	}
	while (pos != NULL)
	{
		if (pos->data == data)
		{
			link temp = (link)malloc(sizeof(NODE));
			temp->data = value;
			temp->next = pos->next;
			pos->next = temp;
			break;
		}
		pos = pos->next;
	}
	if (pos == NULL)
	{
		printf("当前链表中没有找到数值为%d的数据块!\n", data);
		printf("插入失败！\n");
	}
	else
	{
		printf("插入成功!\n");
	}

}
void print_link(link p)
{
	while (p->next != NULL)
	{
		p = p->next;
		printf("%d ", p->data);
	}
	printf("\n");
}
*/
void add_tail(link_ble p, item* data)
{
	if (!p)
		return;
	while (p->next != NULL)
		p = p->next;
	link_ble temp = (link_ble)malloc(sizeof(NODE));
	temp->data = data;
	temp->next = p->next;
	p->next = temp;
}
int is_empty(link_ble l)
{
	return l->next == NULL;
}
link_ble destory(link_ble l)
{
	link_ble tmp;
	while (l->next != NULL)
	{
		tmp = l->next;
		l->next = tmp->next;
		free(tmp->data);
		free(tmp);
	}
	return l;
}