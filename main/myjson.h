
#ifndef _MYJSON_H
#define _MYJSON_H


typedef struct 
{	char identity[20];		//5893D84FB2C7
	char type[20];			//emp-epaper-V1(JDF)
	char command[20];		//flush

    int width;			//640
    int height;		//480
//	char type[20];			
    int bw_length;
    int red_length;
	char bw_buffer[6000];
    char red_buffer[6000];
}data_info;



extern data_info data;




void cjson_to_struct_info(char *text);

void parse_Json(char *text);
void parse_Json_test(char *text);
void creatJson(void);


#endif

