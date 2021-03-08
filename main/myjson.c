#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "myjson.h"

#include "cJSON.h"
static const char *TAG_J = "json";



/*解析json数据 只处理 解析 城市 天气 天气代码  温度  其他的自行扩展
* @param[in]   text  		       :json字符串
* @retval      void                 :无
* @note        修改日志 
*               Ver0.0.1:
*/
/*
{"identity":"5893D84FB2C7",
"type":"emp-epaper-V1(JDF)",
"command":"flush",
"data":{"width":648,
		"height":480,	
		"bw_length":1742,
		"red_length":0,
		"bw_buffer":"shuju"
		red_buffer: "shuju" 
		}
}

let pushData = {
        width: data.width,
        height: data.height,
        type: data.type,
        bw_length: 0,  //黑白图片压缩后大小
        red_length: 0,  //红色图片压缩后大小 , 注意不是base64 encode 后的大小
        bw_buffer: "",  //黑白压缩的 base64 encode
        red_buffer: ""  //红色压缩的 base64 encode
    }
*/




data_info data;

void parse_Json_test(char *text)
 {
	   char *index=strchr(text,'{');
    strcpy(text,index);
	//首先整体判断是否为一个json格式的数据
	cJSON *pJsonRoot = cJSON_Parse(text);
	//如果是否json格式数据
	if (pJsonRoot !=NULL) {

		char *s = cJSON_Print(pJsonRoot);
	//	printf("pJsonRoot: %s\r\n", s);
		cJSON_free((void *) s);
		//解析mac字段字符串内容
		cJSON *pMacAdress = cJSON_GetObjectItem(pJsonRoot, "identity");
		//判断identity字段是否json格式
		if (pMacAdress) {
			//判断mac字段是否string类型
			if (cJSON_IsString(pMacAdress))
				printf("get MacAdress:%s \n", pMacAdress->valuestring);
		} else
			printf("get MacAdress failed \n");

		//解析type字段char内容
		cJSON *ptype = cJSON_GetObjectItem(pJsonRoot, "type");
		//判断type字段是否存在
		if (ptype){
			if (cJSON_IsString(ptype))
			printf("get type:%s \n", ptype->valuestring);
		}
		else
			printf("get type failed \n");
			//解析command字段字符串内容
		cJSON *pcommand = cJSON_GetObjectItem(pJsonRoot, "command");
		//判断identity字段是否json格式
		if (pcommand) {
			//判断mac字段是否string类型
			if (cJSON_IsString(pcommand))
				printf("get command:%s \n", pcommand->valuestring);
		} else
			printf("get command failed \n");

		//解析value字段内容，判断是否为json
		cJSON *pValue = cJSON_GetObjectItem(pJsonRoot, "data");
		if (pValue) {
	
			//进一步剖析里面的height字段:注意这个根节点是 pValue
			cJSON *pwidth = cJSON_GetObjectItem(pValue, "width");
			if (pwidth)
				if (cJSON_IsNumber(pwidth))
					printf("get value->width : %d \n", pwidth->valueint);
			//进一步剖析里面的height字段:注意这个根节点是 pValue
			cJSON *pheight = cJSON_GetObjectItem(pValue, "height");
			if (pheight)
				if (cJSON_IsNumber(pheight))
					printf("get value->height : %d \n", pheight->valueint);
			

			//进一步剖析里面的bw_length字段:注意这个根节点是 pValue
			cJSON *pbw_length = cJSON_GetObjectItem(pValue, "bw_length");
			if (pbw_length)
				if (cJSON_IsNumber(pbw_length))
					printf("get value->bw_length : %d \n", pbw_length->valueint);
			//进一步剖析里面的red_length字段:注意这个根节点是 pValue
			cJSON *pred_length = cJSON_GetObjectItem(pValue, "red_length");
			if (pred_length)
				if (cJSON_IsNumber(pred_length))
					printf("get value->red_length : %d \n", pred_length->valueint);


			//进一步剖析里面的bw_buffer字段:注意这个根节点是 pValue
			cJSON *pbw_buffer= cJSON_GetObjectItem(pValue, "bw_buffer");
			if (pbw_buffer)
				if (cJSON_IsString(pbw_buffer))
				printf("get value->pbw_buffer	 : %s \n", pbw_buffer->valuestring);

				  sprintf(data.bw_buffer,"%s",pbw_buffer->valuestring);

				//进一步剖析里面的bw_buffer字段:注意这个根节点是 pValue
			cJSON *pred_buffer= cJSON_GetObjectItem(pValue, "red_buffer");
			if (pred_buffer)
				if (cJSON_IsString(pred_buffer))
				printf("get value->pred_buffer	 : %s \n",pred_buffer->valuestring);
				 sprintf(data.red_buffer,"%s",pred_buffer->valuestring);
		//剖析
		/*
		cJSON *pArry = cJSON_GetObjectItem(pJsonRoot, "hexArry");
		if (pArry) {
			//获取数组长度
			int arryLength = cJSON_GetArraySize(pArry);
			printf("get arryLength : %d \n", arryLength);
			//逐个打印
			int i ;
			for (i = 0; i < arryLength; i++)
				printf("cJSON_GetArrayItem(pArry, %d)= %d \n",i,cJSON_GetArrayItem(pArry, i)->valueint);
		*/
		}

	} else {
		printf("this is not a json data ... \n");
	}

	cJSON_Delete(pJsonRoot);
	printf("get freeHeap: %d \n\n", system_get_free_heap_size());
}

typedef struct 
{
    char cit[20];
    char weather_text[20];
    char weather_code[2];
    char temperatur[3];
}weather_info;

weather_info weathe;
void cjson_to_struct_info(char *text)
{
    cJSON *root,*psub;
    cJSON *arrayItem;
    //截取有效json
    char *index=strchr(text,'{');
    strcpy(text,index);

    root = cJSON_Parse(text);
    
    if(root!=NULL)
    {
        psub = cJSON_GetObjectItem(root, "results");
        arrayItem = cJSON_GetArrayItem(psub,0);

        cJSON *locat = cJSON_GetObjectItem(arrayItem, "location");
        cJSON *now = cJSON_GetObjectItem(arrayItem, "now");
        if((locat!=NULL)&&(now!=NULL))
        {
            psub=cJSON_GetObjectItem(locat,"name");
            sprintf(weathe.cit,"%s",psub->valuestring);
            ESP_LOGI(TAG_J,"city:%s",weathe.cit);

            psub=cJSON_GetObjectItem(now,"text");
            sprintf(weathe.weather_text,"%s",psub->valuestring);
            ESP_LOGI(TAG_J,"weather:%s",weathe.weather_text);
            
            psub=cJSON_GetObjectItem(now,"code");
            sprintf(weathe.weather_code,"%s",psub->valuestring);
            //ESP_LOGI(HTTP_TAG,"%s",weathe.weather_code);

            psub=cJSON_GetObjectItem(now,"temperature");
            sprintf(weathe.temperatur,"%s",psub->valuestring);
            ESP_LOGI(TAG_J,"temperatur:%s",weathe.temperatur);

            //ESP_LOGI(TAG_J,"--->city %s,weather %s,temperature %s<---\r\n",weathe.cit,weathe.weather_text,weathe.temperatur);
        }
    }
    //ESP_LOGI(TAG_J,"%s 222",__func__);
    cJSON_Delete(root);
}

	/*
//解析以下一段json数据
{
	 "mac":"84:f3:eb:b3:a7:05",
	 "number":2,
	 "value":{"name":"半颗心脏",
              "age":18 ,
	          "blog":"https://blog.csdn.net/xh870189248"
	          },
	 "hex":[51,15,63,22,96]
}
     */

void parse_Json(char *text)
 {

	//首先整体判断是否为一个json格式的数据
	cJSON *pJsonRoot = cJSON_Parse(text);

	if(pJsonRoot == NULL){
	printf("json pack into cjson error…");

}

	//如果是否json格式数据
	if (pJsonRoot !=NULL) {

		char *s = cJSON_Print(pJsonRoot);
		printf("pJsonRoot: %s\r\n", s);
		cJSON_free((void *) s);
		//解析mac字段字符串内容
		cJSON *pMacAdress = cJSON_GetObjectItem(pJsonRoot, "mac");
		//判断mac字段是否json格式
		if (pMacAdress) {
			//判断mac字段是否string类型
			if (cJSON_IsString(pMacAdress))
				printf("get MacAdress:%s \n", pMacAdress->valuestring);
		} else
			printf("get MacAdress failed \n");

		//解析number字段int内容
		cJSON *pNumber = cJSON_GetObjectItem(pJsonRoot, "number");
		//判断number字段是否存在
		if (pNumber){
			if (cJSON_IsNumber(pNumber))
			printf("get Number:%d \n", pNumber->valueint);
		}
		else
			printf("get Number failed \n");
		//解析value字段内容，判断是否为json
		cJSON *pValue = cJSON_GetObjectItem(pJsonRoot, "value");
		if (pValue) {
			//进一步剖析里面的name字段:注意这个根节点是 pValue
			cJSON *pName = cJSON_GetObjectItem(pValue, "name");
			if (pName)
				if (cJSON_IsString(pName))
					printf("get value->Name : %s \n", pName->valuestring);

			//进一步剖析里面的age字段:注意这个根节点是 pValue
			cJSON *pAge = cJSON_GetObjectItem(pValue, "age");
			if (pAge)
				if (cJSON_IsNumber(pAge))
					printf("get value->Age : %d \n", pAge->valueint);

			//进一步剖析里面的blog字段:注意这个根节点是 pValue
			cJSON *pBlog= cJSON_GetObjectItem(pValue, "blog");
			if (pBlog)
				if (cJSON_IsString(pBlog))
				printf("get value->pBlog	 : %s \n", pBlog->valuestring);
		}
		//剖析
		cJSON *pArry = cJSON_GetObjectItem(pJsonRoot, "hexArry");
		if (pArry) {
			//获取数组长度
			int arryLength = cJSON_GetArraySize(pArry);
			printf("get arryLength : %d \n", arryLength);
			//逐个打印
			int i ;
			for (i = 0; i < arryLength; i++)
				printf("cJSON_GetArrayItem(pArry, %d)= %d \n",i,cJSON_GetArrayItem(pArry, i)->valueint);
		}

	} else {
		printf("this is not a json data ... \n");
	}

	cJSON_Delete(pJsonRoot);
	printf("get freeHeap: %d \n\n", system_get_free_heap_size());
}
char tempMessage[6]="555555555";

void creatJson(void)
{

/*
  {
	 "mac":"84:f3:eb:b3:a7:05",
	 "number":2,
	 "value":{"name":"xuhongv",
              "age":18 ,
	          "blog":"https://blog.csdn.net/xh870189248"
	          },
	 "hex":[51,15,63,22,96]
}
 */
	//取一下本地的station的mac地址


	cJSON *pRoot = cJSON_CreateObject();
	cJSON *pValue = cJSON_CreateObject();


	cJSON_AddStringToObject(pRoot,"mac",tempMessage);
	cJSON_AddNumberToObject(pRoot,"number",2);

	cJSON_AddStringToObject(pValue,"mac","xuhongv");
	cJSON_AddNumberToObject(pValue,"age",18);
	cJSON_AddStringToObject(pValue,"mac","https://blog.csdn.net/xh870189248");

	cJSON_AddItemToObject(pRoot, "value",pValue);

    int hex[5]={51,15,63,22,96};
	cJSON *pHex = cJSON_CreateIntArray(hex,5);
	cJSON_AddItemToObject(pRoot,"hex",pHex);

	char *s = cJSON_Print(pRoot);
	printf("\r\n creatJson : %s\r\n", s);
	cJSON_free((void *) s);

	cJSON_Delete(pRoot);
}
