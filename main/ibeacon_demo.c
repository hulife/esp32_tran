/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/



/****************************************************************************
*
* This file is for iBeacon demo. It supports both iBeacon sender and receiver
* which is distinguished by macros IBEACON_SENDER and IBEACON_RECEIVER,
*
* iBeacon is a trademark of Apple Inc. Before building devices which use iBeacon technology,
* visit https://developer.apple.com/ibeacon/ to obtain a license.
*
****************************************************************************/

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "nvs_flash.h"

#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt_defs.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"

#include "esp_ibeacon_api.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "mqtt_client.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "sniffer.h"
#include "wifi_tcp.h"

#include "esp_system.h"
#include "esp_event_loop.h"

#include "driver/uart.h"
#include "soc/uart_struct.h"
//#include "Ap_29demo.h"
#include "ar.h"
#include "led.h"

#include "list.h"
#include "base64.h"
#include "ble_data_bw_red_1024.h"
#include "esp_heap_caps.h"
#include "freertos/queue.h"

#include "cJSON.h"
#include "myjson.h"
static const int RX_BUF_SIZE = 1024*10;

#define TXD_PIN (GPIO_NUM_4)
#define RXD_PIN (GPIO_NUM_5)


int sendData(const char* logName, const char* data);

//#define LITTLE_EDIAN                                             //小端
#define MAX_COMPRESS_SIZE (234)									/*234 bytes: 压缩后最大size */
static const char *TAG = "LIST";
#define BLE_HEAD_SIZE (11)										/*11 bytes 包头size  */
#define MAX_PACKAGE_SIZE (MAX_COMPRESS_SIZE+13) 				/*(MAX_COMPRESS_SIZE+BLE_HEAD_SIZE+2)    2bytes：压缩后size的信�?*/
       unsigned int out_len=0;
            unsigned int out_len_r=0;
             unsigned char* temp_bw=NULL;
              unsigned char*  temp_red=NULL;

struct ble_send_t {
	char mac[20];  				
	char bda[20];  				
	long tm;					
	unsigned int length;
	unsigned int bw_length;
	unsigned int red_length;
	unsigned short send_count;
	unsigned short sn;
	unsigned short opt_handle;
//	gpointer chan;
    void*  chan;
	link_ble list;
	link_ble head_list;
	int isOk;
	int completed;
	int timeout;
	int write_error;
	int retry_count;	
	long id;			
};

typedef struct smarchit_context {
	char			szMac[20];
  //  config_t*       config;
   // GThreadPool*    g_gpool;
	//GQueue*			temp_tasks;
	//GQueue*			tasking_mac;
	//GMainContext* 	loop_context;
	char			cur_exe_path[1024];
} smarchit_context_t ;







int add_ble_data_to_list(link_ble my_list,  unsigned short start_sn, const unsigned char* ble_data, unsigned short length){

	const unsigned char * start_ptr, *end_ptr;
	unsigned int offset=0;
	start_ptr = ble_data;
	end_ptr = ble_data + length;

    int count=0;



	while (start_ptr + offset < end_ptr)
	{
		count++;
        item* ble_item;
		const unsigned char* temp;

		ble_item =  (char *)malloc(sizeof(item));
        	if (ble_item==NULL) {
			 ESP_LOGW(TAG,"no memory malloc!\n");
             ESP_LOGW(TAG,"%d\n",sizeof(item));
			return -1;
		}
      //  ESP_LOGI(TAG,"%d\n",count);


	
		memset((void*)ble_item, 0, sizeof(ble_item));
		ble_item->len = 0;

		int offset_package = 0;
		int cur_len = 0;
		//char command = 'L';
		int is_compress = 0;
		while (start_ptr + offset + offset_package < end_ptr) {
			int temp_len = 0;

			temp = start_ptr + offset + offset_package;

			if (*temp == 'I' || *temp == 'N' || *temp == 'L' || *temp == 'S')
				is_compress = 1;
			else
				is_compress = 0;
			
#ifdef LITTLE_EDIAN
			temp_len = *(temp + 1) + (*(temp + 2) << 8); //little endian
#else
			temp_len = (*(temp + 1) << 8) + *(temp + 2); //little endian
#endif
            //printf("temp_len =  %d\n",temp_len);//包长
			if (cur_len + temp_len > MAX_COMPRESS_SIZE) {
				break;
			}
			if (cur_len == 0)
				ble_item->sn = start_sn;
			start_sn++;

			if (cur_len == 0) /*第一个组包的第一个包 */
				ble_item->command = *temp;
			if (*temp == 'S' || *temp == 's') /*最后一个包 */
				ble_item->command = *temp;

          

			//ble_item->command = command;

			if (is_compress) {
				memcpy(ble_item->data + cur_len, temp + 1, temp_len + 2);
				cur_len += (temp_len + 2);
             // printf("cur_len =  %d\n",cur_len);//组包长
			}
			else {
				memcpy(ble_item->data + cur_len, temp + 3, temp_len);
				cur_len += (temp_len);
			}
			
			ble_item->len = cur_len;
			offset_package += (temp_len + 3);
		}

		add_tail(my_list, ble_item);//ble_item  have


   
		offset += offset_package;
	}
	return start_sn;
}


link_ble ble_build_list_2(const unsigned char* ble_data_bw, unsigned short length_bw, const unsigned char* ble_data_red, unsigned short length_red) {
	unsigned short sn;


	link_ble my_list = NULL;
	link_ble p = NULL;

	my_list = make_head(p);

	sn= add_ble_data_to_list(my_list, 0, ble_data_bw, length_bw);
  //printf("sn=++++%d",sn);黑包29
    printf(" length_red %d\n",length_red);

	if (length_red != 0 )
	    add_ble_data_to_list(my_list, sn, ble_data_red, length_red);
      

    char buff_item[250];
    char* message_data_item=buff_item;
    const unsigned char *image_data_item=my_list->next->data->data;

 //   printf(" ble_item->len =  %d\n",my_list->next->data->len);   
//    printf(" ble_item->sn  =  %d\n",my_list->next->data->sn); 
//    printf(" ble_item->com  =  %d\n",my_list->next->data->command); 

 //   for(int i=0;i<my_list->next->data->len;i++)
 //           {
 //     printf(" ble_item->data  =  %d\n",my_list->next->data->data[i]);
 //           }
 
    
	return my_list;
}

void pack_s(void)
{
//


}


char comd_end;

int new_compose_package(link_ble list, unsigned char* package, unsigned short* size) {

	unsigned short len;

	/*
	Packet Format
	--------------
	0               3               5               7         8
	+---------------+---------------+---------------+---------+-------------+---------+--------+--------+
	|   bytes(3)    |  uShort16BE   |  uShort16BE   | byte    |  Length-1   |  byte   |  byte  |  byte  |
	+---------------+---------------+---------------+---------+-------------+---------+--------+--------+
	|  0xAA0x550x7E |    SN         |  Length       | Command |  Data       |  SUM    |  XOR   |  0x0D  |
	+---------------+---------------+---------------+---------+-------------+---------+--------+--------+
	*/

	//初始��?	memset(package, 0, MAX_PACKAGE_SIZE);
//'I'    Initialize EPD before load data, first B/W packet use this command. 49
//'L'    Load data to EPD SRAM.                                              4C
//'N'    Switch RED part of image, first RED packet use this command.         4E
//'S'    Show/Refresh image after load data.                                    53
//'i'    Data uncompressed, Initialize EPD before load data, first B/W packet use this command.    69 
//'l'    Data uncompressed, Load data to EPD SRAM.                                                  6c
//'n'    Data uncompressed, Switch RED part of image, first RED packet use this command.               6e
//'s'    Data uncompressed, Show/Refresh image after load data.                                         73
	//package 起始标志 0xAA0x550x7E
	package[0] = 0xAA;
	package[1] = 0x55; 
	package[2] = 0x7E;

//list=list->next;
	//SN
#ifdef LITTLE_EDIAN
	//little edian
	package[3] = list->data->sn & 0x00ff;
	package[4] = (list->data->sn & 0xff00 ) >> 8;
#else
	//big edian
	package[4] = list->data->sn & 0x00ff;
	package[3] = (list->data->sn & 0xff00) >> 8;
#endif

	//Length
	len = list->data->len + 1; //2:压缩后长度也放在 data区， command也计算在length
		
#ifdef LITTLE_EDIAN
	//little edian
	package[5] = len  & 0x00ff;
	package[6] = (len & 0xff00) >> 8;
#else
	//big edian
	package[6] = len & 0x00ff;
	package[5] = (len & 0xff00) >> 8;
#endif

	//Command
	package[7] = list->data->command;
    comd_end= list->data->command;
	//Data
	memcpy(&package[8], list->data->data, list->data->len);

	//SUM and XOR Specification:
	//sum value = SUM({ Length, SN, Command, Data })
	//xor value = XOR({ Length, SN, Command, Data })
	unsigned char acc_sum = 0, acc_xor = 0;
	for (int i = 0; i < list->data->len + 5; i++) {
		unsigned char value = package[i + 3];
		acc_sum += value;
		acc_xor ^= value;
	}

	package[8 + list->data->len ] = acc_sum;
	package[8 + list->data->len + 1] = acc_xor;

	//结束标志 0x0D
	package[8 + list->data->len + 2] = 0x0D;

	*size = list->data->len + 11;

	return 0;
}

//11 12 4f 4b  OK
//int send_packages_test(struct ble_send_t* ble_send,struct ble_char_opt_t* ble_char_write, unsigned short* sn, int exec_count, smarchit_context_t* smt_ctx)
void test(void)
{

    static const char *TX_TASK_TAG = "连接指定MAC....................";
     esp_log_level_set(TX_TASK_TAG, ESP_LOG_INFO);
    // sendData(TX_TASK_TAG, "AT+CONN=5893D84FB2BB\r\n");
/*
    sendData(TX_TASK_TAG, "+++");
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    sendData(TX_TASK_TAG, "AT+CONNECT=,58:93:D8:4F:B2:C7\r\n");
    vTaskDelay(5000 / portTICK_PERIOD_MS);
   // sendData(TX_TASK_TAG, "AT+TRX_CHAN=1,17,17\r\n");
   // vTaskDelay(5000 / portTICK_PERIOD_MS);
    sendData(TX_TASK_TAG, "AT+EXIT\r\n");
    vTaskDelay(5000 / portTICK_PERIOD_MS);
*/
link_ble tmp;
unsigned char package[MAX_PACKAGE_SIZE];
unsigned short size, i=0;
int exec_count;	
uint8_t* data=NULL;
int TASK=0;
int rxBytes;
int count=0;

link_ble list = NULL;
list=ble_build_list_2(ble_data_bw,sizeof(ble_data_bw), ble_data_red, sizeof(ble_data_red));

//while (list != NULL) {
 flag:    
    list=list->next;
		tmp = list;
      //  printf("%02x ",data[11]);
      //  if((data[11]== '4f')&&(data[12]== '4b'))
       // {
		new_compose_package(tmp, package, &size);
      //  for(int i=0;i<size;i++)
     //   {
    //printf("0x%02x,",package[i]);
    // }
		printf("%d\n",size);
        usleep(200*1000);
        
       TASK= uart_write_bytes(UART_NUM_1,  ( char*)package, size);
         count++;
         if (TASK != 0)
         {          
            //printf("%x ",data[11]);
         
   goto  flag; 
                              
        }
   if (TASK != 44)
         {          
           
   goto  END; 
                              
        }

		if(comd_end=='S')
        {
    END:            
            printf("发送完成\n");
          //  break;
        }
		//send_a_package



//}

}

void test_task(void)
{

    static const char *TX_TASK_TAG = "连接指定MAC....................";
     esp_log_level_set(TX_TASK_TAG, ESP_LOG_INFO);
link_ble tmp;
unsigned char package[MAX_PACKAGE_SIZE];
unsigned short size, i=0;
int exec_count;	
uint8_t* data=NULL;
int TASK=0;
int rxBytes;
int count=0;

link_ble list = NULL;
list=ble_build_list_2(ble_data_bw,sizeof(ble_data_bw), ble_data_red, sizeof(ble_data_red));

while (1) {
   
    list=list->next;
		tmp = list;
		new_compose_package(tmp, package, &size);
		printf("%d\n",size);
       // usleep(200*1000);      
  uart_write_bytes(UART_NUM_1,  ( char*)package, size);
		if(comd_end=='S')
        {  
            printf("发送完成\n");
          // break;
        }
		//send_a_package

}

}



void test_nontask(void)
{

    static const char *TX_TASK_TAG = "连接指定MAC....................";
     esp_log_level_set(TX_TASK_TAG, ESP_LOG_INFO);


    static const char *RX_TASK_TAG = "RX_TASK";
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    uint8_t*data_2 = (uint8_t*) malloc(RX_BUF_SIZE+1);
    // sendData(TX_TASK_TAG, "AT+CONN=5893D84FB2BB\r\n");
/*
    sendData(TX_TASK_TAG, "+++");
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    sendData(TX_TASK_TAG, "AT+CONNECT=,58:93:D8:4F:B2:C7\r\n");
    vTaskDelay(5000 / portTICK_PERIOD_MS);
   // sendData(TX_TASK_TAG, "AT+TRX_CHAN=1,17,17\r\n");
   // vTaskDelay(5000 / portTICK_PERIOD_MS);
    sendData(TX_TASK_TAG, "AT+EXIT\r\n");
    vTaskDelay(5000 / portTICK_PERIOD_MS);
*/
link_ble tmp;
unsigned char package[MAX_PACKAGE_SIZE];
unsigned short size, i=0;
int exec_count;	
//uint8_t* data_2=NULL;
int count=0;

link_ble list = NULL;
list=ble_build_list_2(ble_data_bw,sizeof(ble_data_bw), ble_data_red, sizeof(ble_data_red));

//while (list != NULL) {
flag:   
        list=list->next;
		tmp = list;
		new_compose_package(tmp, package, &size);     
		printf("%d\n",size);
       // usleep(300*1000); 
       // vTaskDelay(300 / portTICK_PERIOD_MS); 
        uart_write_bytes(UART_NUM_1,  ( char*)package, size);
          count++;
        const int rxBytes = uart_read_bytes(UART_NUM_1, data_2, RX_BUF_SIZE,10 / portTICK_RATE_MS)               ;//1000 / portTICK_RATE_MS
        if (rxBytes > 0)
        {

            printf("%x ",data_2[11]);
            printf("%x ",data_2[12]);
        goto  flag; 
        }
/*
        if (rxBytes > 0)
         {
            //data_2[rxBytes] = 0;   
            if(count>=1)//6777
            {  
            printf("%x ",data_2[11]);
            printf("%x ",data_2[12]);
   goto  flag; 
            }  
                          
         if((data_2[11]==79)&&(data_2[12]== 75))//4f4b7975
                 {
            printf("%d ",data_2[11]);
            printf("%d ",data_2[12]);     
      goto  flag; 
                 }
          if(count>1)//4f4b7975
                 {
            printf("%02x ",data_2[11]);
            printf("%02x ",data_2[12]);

      goto  flag; 
                 } 
        
        }
 */
          
//        }
    
		if(comd_end=='S')
        {
            printf("发送完成\n");
          // break;
        }
		



//}

}

#define UART_CTS   (GPIO_NUM_6)  
#define UART_RTS   (GPIO_NUM_7)  



void init() {
    const uart_config_t uart_config = {
        .baud_rate = 230400,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    // We won't use a buffer for sending data.
    uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
}

int sendData(const char* logName, const char* data)
{
    const int len = strlen(data);
    const int txBytes = uart_write_bytes(UART_NUM_1, data, len);
    ESP_LOGI(logName, "Wrote %d bytes", txBytes);
    return txBytes;
}

//61440/160=384
uint16_t leng=0;
int count2=0;
char buff_s2[160];
char* message_bw=buff_s2;
const unsigned char *image_bw=gImage_ar;//

void test2(void)
{
    static const char *TX_TASK_TAG = "TX_TASK";
    esp_log_level_set(TX_TASK_TAG, ESP_LOG_INFO);
while (1)
	{
usleep(20*1000);
	 memcpy(message_bw, image_bw,160);
		if(count2<300)
		{
	  image_bw+= 160;
		leng=160;	
		}
	count2++;
  //  sendData(TX_TASK_TAG, message_bw);
  // vTaskDelay(2000 / portTICK_PERIOD_MS);
    uart_write_bytes(UART_NUM_1, message_bw, leng);
	memset(message_bw,0,160);
	if(count2>=300)
	{
printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@%d@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@",count2);
        count2=0;

	break;

	}
	}

}

static void tx_task()
{
    static const char *TX_TASK_TAG = "TX_TASK";
    esp_log_level_set(TX_TASK_TAG, ESP_LOG_INFO);
    while (1) {
        sendData(TX_TASK_TAG, "AT+CONN=E7A93335DEAC\r\n");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

 int flist=0;   
             
static void rx_task(unsigned short bw_len, unsigned short red_len)
{
    static const char *RX_TASK_TAG = "RX_TASK";
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    uint8_t*data = (uint8_t*) malloc(RX_BUF_SIZE+1);

link_ble tmp;
unsigned char package[MAX_PACKAGE_SIZE];
unsigned short size, i=0;
int count=0;
link_ble list = NULL;
//list=ble_build_list_2(ble_data_bw,sizeof(ble_data_bw), ble_data_red, sizeof(ble_data_red));

list=ble_build_list_2(temp_bw,bw_len, temp_red, red_len);

list=list->next;
tmp = list;
new_compose_package(tmp, package, &size);     
printf("%d\n",size);
uart_write_bytes(UART_NUM_1,  ( char*)package, size);

    while (list!= NULL) {
        const int rxBytes = uart_read_bytes(UART_NUM_1, data, RX_BUF_SIZE, 30 / portTICK_RATE_MS);
        if (rxBytes > 0) {
            data[rxBytes] = 0;
            ESP_LOGI(RX_TASK_TAG, "Read %d bytes: '%s'", rxBytes, data);

       
        if((data[11]!=79)&&(data[12]!= 75))//4f4b7975ok       
        {
               
      //  bzero(data,0);
		new_compose_package(tmp, package, &size);     
		printf("%d\n",size);
        uart_write_bytes(UART_NUM_1,  ( char*)package, size);
        }
         if((data[11]==79)&&(data[12]== 75))
         {
                
       // bzero(data,0);
 
            new_compose_package(tmp, package, &size);   
                     list=list->next;
	        tmp = list;    
		printf("%d\n",size);
        uart_write_bytes(UART_NUM_1,  ( char*)package, size);

         }


      
       //    	if( flist==2)
     //   {  
     //       printf("发送完成\n");
      //     flist=0;
    //       break;
     //   }
     
            ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, data, rxBytes, ESP_LOG_INFO);
        }
          	if(comd_end=='S')
        {  

            printf("发送完成\n");
           flist=1;
        }
     
    }
    free(data);
     printf("跳出\n");
}

void app_uart()
{
  

    xTaskCreate(rx_task, "uart_rx_task", 1024*2, NULL, configMAX_PRIORITIES, NULL);
    //test();
   // test2();
   // static const char *TX_TASK_TAG = "TX_TASK";
   // sendData(TX_TASK_TAG, "99999\r\n");
    //vTaskDelay(2000 / portTICK_PERIOD_MS);
   // sendData(TX_TASK_TAG, "AT+SEND=666666\r\n");
 //   xTaskCreate(test_task, "uart_tx_task", 1024*2, NULL, configMAX_PRIORITIES-1, NULL);
}



/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about one event 
 * - are we connected to the AP with an IP? */
const int WIFI_CONNECTED_BIT = BIT0;


static const char* DEMO_TAG = "BLE_DISCOVERY";
static const char* BT_DIS = "BT_DISCOVERY";
static const char* WIFI_DIS = "WIFI_DISCOVERY";

extern esp_ble_ibeacon_vendor_t vendor_config;

static esp_mqtt_client_handle_t g_mqtt_client=NULL;
static char g_bluetooth_mac[18];

#define MQTT_HOST			 		"192.168.21.139"
#define MQTT_PORT			 		1883
#define TOPIC						"iBeacon"
#define IBEACON_ID					"fda50693"
#define THRESHOLD					(-85)
#define SPLIT_CHAR					"{$}"


///Declare static functions
static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);

//#if (IBEACON_MODE == IBEACON_RECEIVER)
static esp_ble_scan_params_t ble_scan_params = {
    .scan_type              = BLE_SCAN_TYPE_ACTIVE,
    .own_addr_type          = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval          = 0x40,
    .scan_window            = 0x40,
    .scan_duplicate         = BLE_SCAN_DUPLICATE_DISABLE
};

//#elif (IBEACON_MODE == IBEACON_SENDER)
static esp_ble_adv_params_t ble_adv_params = {
    .adv_int_min        = 0x20,
    .adv_int_max        = 0x40,
    .adv_type           = ADV_TYPE_NONCONN_IND,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};
//#endif


typedef enum {
    APP_GAP_STATE_IDLE = 0,
    APP_GAP_STATE_DEVICE_DISCOVERING,
    APP_GAP_STATE_DEVICE_DISCOVER_COMPLETE,
} app_gap_state_t;

typedef struct {
    bool dev_found;
    uint8_t bdname_len;
    uint8_t eir_len;
    uint8_t rssi;
    uint32_t cod;
    uint8_t eir[ESP_BT_GAP_EIR_DATA_LEN];
    uint8_t bdname[ESP_BT_GAP_MAX_BDNAME_LEN + 1];
    esp_bd_addr_t bda;
    app_gap_state_t state;
} app_gap_cb_t;

static app_gap_cb_t m_dev_info;
static char g_bluetooth_mac[18];

static const uint8_t esp_module_mac[32][3] = {
    {0x54, 0x5A, 0xA6}, {0x24, 0x0A, 0xC4}, {0xD8, 0xA0, 0x1D}, {0xEC, 0xFA, 0xBC},
    {0xA0, 0x20, 0xA6}, {0x90, 0x97, 0xD5}, {0x18, 0xFE, 0x34}, {0x60, 0x01, 0x94},
    {0x2C, 0x3A, 0xE8}, {0xA4, 0x7B, 0x9D}, {0xDC, 0x4F, 0x22}, {0x5C, 0xCF, 0x7F},
    {0xAC, 0xD0, 0x74}, {0x30, 0xAE, 0xA4}, {0x24, 0xB2, 0xDE}, {0x68, 0xC6, 0x3A},
};

/* The device num in ten minutes */
int s_device_info_num           = 0;
station_info_t *station_info    = NULL;
station_info_t *g_station_list  = NULL;

extern int tcpsock_1, tcpsock_2;

static void send_data_to_socket(int socket, int16_t rssi, const char* uuid_str, const char* type)
{
	char buf[256];
	memset(buf, 0, sizeof(buf));

	sprintf(buf, "{\"%s\":\"%s\",\"%s\":\"%s\",\"%s\":\"%s\",\"%s\":%d,\"%s\":\"%s\"}%s", "bluetoothSite",g_bluetooth_mac, "peopleId", uuid_str, "bluetoothType", "gateway", "signalIntensity", rssi, "type", type, SPLIT_CHAR);

	int err = send(socket, buf, strlen(buf), 0);
	if (err < 0) {
        ESP_LOGE(DEMO_TAG, "Error occurred during sending: err %d", err);
	}

	return;
}

static void send_data_to_mqtt(int16_t rssi, const char* uuid_str, const char* type)
{
	char buf[256];
	memset(buf, 0, sizeof(buf));

	sprintf(buf, "{\"%s\":\"%s\",\"%s\":\"%s\",\"%s\":\"%s\",\"%s\":%d,\"%s\":\"%s\"}", "bluetoothSite",g_bluetooth_mac, "peopleId", uuid_str, "bluetoothType", "gateway", "signalIntensity", rssi, "type", type);
	esp_mqtt_client_publish(g_mqtt_client, TOPIC, buf, strlen(buf), 0, 0);


	return;
}

static char *bda2str(esp_bd_addr_t bda, char *str, size_t size)
{
    if (bda == NULL || str == NULL || size < 18) {
        return NULL;
    }

    uint8_t *p = bda;
    sprintf(str, "%02x:%02x:%02x:%02x:%02x:%02x",
            p[0], p[1], p[2], p[3], p[4], p[5]);
    return str;
}


static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    esp_err_t err;

    switch (event) {
    case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:{
//#if (IBEACON_MODE == IBEACON_SENDER)
        esp_ble_gap_start_advertising(&ble_adv_params);
//#endif
        break;
    }
    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
//#if (IBEACON_MODE == IBEACON_RECEIVER)
        //the unit of the duration is second, 0 means scan permanently
        uint32_t duration = 0;
        esp_ble_gap_start_scanning(duration);
//#endif
        break;
    }
    case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
        //scan start complete event to indicate scan start successfully or failed
        if ((err = param->scan_start_cmpl.status) != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(DEMO_TAG, "Scan start failed: %s", esp_err_to_name(err));
        }
        break;
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        //adv start complete event to indicate adv start successfully or failed
        if ((err = param->adv_start_cmpl.status) != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(DEMO_TAG, "Adv start failed: %s", esp_err_to_name(err));
        }
        break;
    case ESP_GAP_BLE_SCAN_RESULT_EVT: {
        esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;
		char head_test[100];
        char mac_test[12];
        int  count=0;
        switch (scan_result->scan_rst.search_evt) {
        	case ESP_GAP_SEARCH_INQ_RES_EVT:
	            /* Search for BLE iBeacon Packet */
               
		        //test
	//			sprintf(head_test, "%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x", 
	//			scan_result->scan_rst.ble_adv[0],scan_result->scan_rst.ble_adv[1],scan_result->scan_rst.ble_adv[2],scan_result->scan_rst.ble_adv[3],
	//						scan_result->scan_rst.ble_adv[4],scan_result->scan_rst.ble_adv[5],scan_result->scan_rst.ble_adv[6],scan_result->scan_rst.ble_adv[7],scan_result->scan_rst.ble_adv[8],
	//						scan_result->scan_rst.ble_adv[9],scan_result->scan_rst.ble_adv[10],scan_result->scan_rst.ble_adv[11],scan_result->scan_rst.ble_adv[12],scan_result->scan_rst.ble_adv[13],							
	//						scan_result->scan_rst.ble_adv[14],scan_result->scan_rst.ble_adv[15],scan_result->scan_rst.ble_adv[16],scan_result->scan_rst.ble_adv[17],scan_result->scan_rst.ble_adv[18],														
	//						scan_result->scan_rst.ble_adv[19],scan_result->scan_rst.ble_adv[20],scan_result->scan_rst.ble_adv[21],scan_result->scan_rst.ble_adv[22],scan_result->scan_rst.ble_adv[23],																					
	//						scan_result->scan_rst.ble_adv[24],scan_result->scan_rst.ble_adv[25],scan_result->scan_rst.ble_adv[26],scan_result->scan_rst.ble_adv[27],scan_result->scan_rst.ble_adv[28],
	//						scan_result->scan_rst.ble_adv[29]
	//			);
	//			ESP_LOGI(DEMO_TAG, "Head: %s",head_test);
				//test end
		
               
        
                if((scan_result->scan_rst.ble_adv[9]==0x41)&&(scan_result->scan_rst.ble_adv[10]==0x52)&&(scan_result->scan_rst.ble_adv[11]==0x43)&&(scan_result->scan_rst.ble_adv[12]==0x48))
            {
                 count++;
        
    			sprintf(mac_test, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", scan_result->scan_rst.ble_adv[13],scan_result->scan_rst.ble_adv[14],
															scan_result->scan_rst.ble_adv[15],scan_result->scan_rst.ble_adv[16],scan_result->scan_rst.ble_adv[17],scan_result->scan_rst.ble_adv[18],scan_result->scan_rst.ble_adv[19],scan_result->scan_rst.ble_adv[20],
                                                            scan_result->scan_rst.ble_adv[21],scan_result->scan_rst.ble_adv[22],scan_result->scan_rst.ble_adv[23],scan_result->scan_rst.ble_adv[24]);
				//ESP_LOGI(DEMO_TAG, "MAC: %s",mac_test);
                char bda_str[18];
                bda2str(scan_result->scan_rst.bda, bda_str, 18);
              //  ESP_LOGI(DEMO_TAG, "mac: %s",bda_str);


                send_data_to_mqtt(scan_result->scan_rst.rssi, mac_test, "ble");
             

          
            }

             break;



 #ifdef  test       
    	        if (esp_ble_is_ibeacon_packet(scan_result->scan_rst.ble_adv, scan_result->scan_rst.adv_data_len))
    	        {
					char head[30];
    	        	char uuid[32];
					char ibeacon_id[10];
					memset(ibeacon_id, 0,10);
					memset(head, 0,30);
					sprintf(ibeacon_id, "%02x%02x%02x%02x", scan_result->scan_rst.ble_adv[9],scan_result->scan_rst.ble_adv[10],
															scan_result->scan_rst.ble_adv[11],scan_result->scan_rst.ble_adv[12]);
					sprintf(head, "%02x %02x %02x %02x %02x %02x %02x %02x %02x", scan_result->scan_rst.ble_adv[0],scan_result->scan_rst.ble_adv[1],scan_result->scan_rst.ble_adv[2],scan_result->scan_rst.ble_adv[3],
								scan_result->scan_rst.ble_adv[4],scan_result->scan_rst.ble_adv[5],scan_result->scan_rst.ble_adv[6],scan_result->scan_rst.ble_adv[7],scan_result->scan_rst.ble_adv[8]
					);
					
					if (!strcmp(IBEACON_ID,ibeacon_id) && scan_result->scan_rst.rssi > THRESHOLD )
					{
						memset(uuid, 0,32);
						sprintf(uuid, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
													scan_result->scan_rst.ble_adv[13], scan_result->scan_rst.ble_adv[14], scan_result->scan_rst.ble_adv[15],
													scan_result->scan_rst.ble_adv[16], scan_result->scan_rst.ble_adv[17], scan_result->scan_rst.ble_adv[18],
													scan_result->scan_rst.ble_adv[19], scan_result->scan_rst.ble_adv[20], scan_result->scan_rst.ble_adv[21],
													scan_result->scan_rst.ble_adv[22], scan_result->scan_rst.ble_adv[23], scan_result->scan_rst.ble_adv[24]);

	        	        esp_ble_ibeacon_t *ibeacon_data = (esp_ble_ibeacon_t*)(scan_result->scan_rst.ble_adv);
		                ESP_LOGI(DEMO_TAG, "----------iBeacon Found----------");
						ESP_LOGI(DEMO_TAG, "len=%d,Data: %s",scan_result->scan_rst.adv_data_len,uuid);
						ESP_LOGI(DEMO_TAG, "Head: %s",head);
		                esp_log_buffer_hex("IBEACON_DEMO: Device address:", scan_result->scan_rst.bda, ESP_BD_ADDR_LEN );
		                esp_log_buffer_hex("IBEACON_DEMO: Proximity UUID:", ibeacon_data->ibeacon_vendor.proximity_uuid, ESP_UUID_LEN_128);

		                uint16_t major = ENDIAN_CHANGE_U16(ibeacon_data->ibeacon_vendor.major);
		                uint16_t minor = ENDIAN_CHANGE_U16(ibeacon_data->ibeacon_vendor.minor);
		                ESP_LOGI(DEMO_TAG, "Major: 0x%04x (%d)", major, major);
		                ESP_LOGI(DEMO_TAG, "Minor: 0x%04x (%d)", minor, minor);
		                ESP_LOGI(DEMO_TAG, "Measured power (RSSI at a 1m distance):%d dbm", ibeacon_data->ibeacon_vendor.measured_power);
		                ESP_LOGI(DEMO_TAG, "RSSI of packet:%d dbm", scan_result->scan_rst.rssi);

//						char bda_str[18];
//						bda2str(scan_result->scan_rst.bda, bda_str, 18);
						send_data_to_mqtt(scan_result->scan_rst.rssi, uuid, "ble");
		//				send_data_to_socket(tcpsock_1,scan_result->scan_rst.rssi, uuid, "ble");
						//send_data_to_socket(tcpsock_2,scan_result->scan_rst.rssi, uuid, "ble");
					}
	            }
              
	            break;
     #endif 
	        default:
	            break;
        }
        break;
    }

    case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
        if ((err = param->scan_stop_cmpl.status) != ESP_BT_STATUS_SUCCESS){
            ESP_LOGE(DEMO_TAG, "Scan stop failed: %s", esp_err_to_name(err));
        }
        else {
            ESP_LOGI(DEMO_TAG, "Stop scan successfully");
        }
        break;

    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        if ((err = param->adv_stop_cmpl.status) != ESP_BT_STATUS_SUCCESS){
            ESP_LOGE(DEMO_TAG, "Adv stop failed: %s", esp_err_to_name(err));
        }
        else {
            ESP_LOGI(DEMO_TAG, "Stop adv successfully");
        }
        break;

    default:
        break;
    }
}


void ble_ibeacon_appRegister(void)
{
    esp_err_t status;

    ESP_LOGI(DEMO_TAG, "register callback");

    //register the scan callback function to the gap module
    if ((status = esp_ble_gap_register_callback(esp_gap_cb)) != ESP_OK) {
        ESP_LOGE(DEMO_TAG, "gap register error: %s", esp_err_to_name(status));
        return;
    }

}

void ble_ibeacon_init(void)
{
    esp_bluedroid_init();
    esp_bluedroid_enable();
    ble_ibeacon_appRegister();
}

static inline uint32_t sniffer_timestamp()
{
    return xTaskGetTickCount() * (1000 / configTICK_RATE_HZ);
}
/* The callback function of sniffer */
void wifi_sniffer_cb(void *recv_buf, wifi_promiscuous_pkt_type_t type)
{
    wifi_promiscuous_pkt_t *sniffer = (wifi_promiscuous_pkt_t *)recv_buf;
    sniffer_payload_t *sniffer_payload = (sniffer_payload_t *)sniffer->payload;

    /* Check if the packet is Probo Request  */
    if (sniffer_payload->header[0] != 0x40) {
        return;
    }

    if (!g_station_list) {
        g_station_list = malloc(sizeof(station_info_t));
        g_station_list->next = NULL;
    }

    /* Check if there is enough memory to use */
    if (esp_get_free_heap_size() < 60 * 1024) {
        s_device_info_num = 0;

        for (station_info = g_station_list->next; station_info; station_info = g_station_list->next) {
            g_station_list->next = station_info->next;
            free(station_info);
        }
    }
    /* Filter out some useless packet  */
    for (int i = 0; i < 32; ++i) {
        if (!memcmp(sniffer_payload->source_mac, esp_module_mac[i], 3)) {
            return;
        }
    }
    /* Traversing the chain table to check the presence of the device */
    for (station_info = g_station_list->next; station_info; station_info = station_info->next) {
        if (!memcmp(station_info->bssid, sniffer_payload->source_mac, sizeof(station_info->bssid))) {
            return;
        }
    }
    /* Add the device information to chain table */
    if (!station_info) {
        station_info = malloc(sizeof(station_info_t));
        station_info->next = g_station_list->next;
        g_station_list->next = station_info;
    }

    station_info->rssi = sniffer->rx_ctrl.rssi;
    station_info->channel = sniffer->rx_ctrl.channel;
    station_info->timestamp = sniffer_timestamp();
    memcpy(station_info->bssid, sniffer_payload->source_mac, sizeof(station_info->bssid));
    s_device_info_num++;


	if ( sniffer->rx_ctrl.rssi > THRESHOLD ) {
		char bda_str[18];
		ESP_LOGI(WIFI_DIS, "----------Wifi device Found----------");
		ESP_LOGE(WIFI_DIS, "Current device num = %d", s_device_info_num);
		ESP_LOGE(WIFI_DIS, "WIF_MAC: 0x%02X.0x%02X.0x%02X.0x%02X.0x%02X.0x%02X, The time is: %d, The rssi = %d\n", station_info->bssid[0], station_info->bssid[1], station_info->bssid[2], station_info->bssid[3], station_info->bssid[4], station_info->bssid[5], station_info->timestamp, station_info->rssi);
		bda2str(station_info->bssid, bda_str, 18);
		send_data_to_mqtt(sniffer->rx_ctrl.rssi, bda_str, "wifi");
		//send_data_to_socket(tcpsock_1,sniffer->rx_ctrl.rssi, bda_str, "wifi");
		//send_data_to_socket(tcpsock_2,sniffer->rx_ctrl.rssi, bda_str, "wifi");

	}
	
}


static esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id) {
    case SYSTEM_EVENT_STA_START:
		ESP_LOGI(WIFI_DIS, "esp_wifi_connect");
        esp_wifi_connect();
        break;
	
    case SYSTEM_EVENT_STA_GOT_IP:
		
		ESP_LOGI(DEMO_TAG, "SYSTEM_EVENT_STA_GOT_IP\n");
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);

	//	tcp_init_client();
		
/*		if (!g_station_list) {
			g_station_list = malloc(sizeof(station_info_t));
			g_station_list->next = NULL;
			ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(wifi_sniffer_cb));
			ESP_ERROR_CHECK(esp_wifi_set_promiscuous(1));
		}
暂时不要wifi*/		
        break;
        
    case SYSTEM_EVENT_STA_DISCONNECTED:
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
        break;
    default:
        break;
    }
    return ESP_OK;
}

char*buffer=NULL;
int flag=0;
int len;

static void wifi_init(void)
{
	tcpip_adapter_init();
	wifi_event_group = xEventGroupCreate();
	ESP_ERROR_CHECK(esp_event_loop_init(wifi_event_handler, NULL));
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	wifi_config_t wifi_config = {
		.sta = {
			.ssid = "Archempower",
			.password = "60662155",
//		.ssid = "arch.face.ipc1",
//		.password = "arh123@ipc",

		},
	};
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
	ESP_LOGI(DEMO_TAG, "start the WIFI SSID:[%s] password:[%s]", wifi_config.sta.ssid, "******");
	ESP_ERROR_CHECK(esp_wifi_start());
	ESP_LOGI(DEMO_TAG, "Waiting for wifi");
	xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);
}

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{   
    int msg_id;
    //esp_mqtt_client_handle_t client = event->client;
    switch (event->event_id) {
	case MQTT_EVENT_BEFORE_CONNECT:
		break;
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(DEMO_TAG, "MQTT_EVENT_CONNECTED");

         msg_id = esp_mqtt_client_subscribe(g_mqtt_client, "/topic/qos0", 0);
         ESP_LOGI(DEMO_TAG, "sent subscribe successful, msg_id=%d", msg_id);

        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(DEMO_TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(DEMO_TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(DEMO_TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(DEMO_TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(DEMO_TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
       // printf("DATA=%.*s\r\n", event->data_len, event->data);
        if( event->data!=NULL)
        {     
             //  buffer = (char *)malloc(1024*10);
            // sprintf(buffer,"%s",event->data);//"HelloWorld\n"写入a指向的地址
             //  memcpy(buffer, event->data, strlen(event->data));
             //     parse_Json_test(buffer);
             //strcpy(buffer,event->data);
             // parse_Json(event->data);
            parse_Json_test(event->data);
         //   printf("++++++++++++++++++++=%.*s\r\n", event->data_len, data.bw_buffer);
     
              temp_bw=base64_decode((const unsigned char*)data.bw_buffer, strlen(data.bw_buffer),&out_len);//base_64转换
              temp_red=base64_decode((const unsigned char*)data.red_buffer, strlen(data.red_buffer),&out_len_r);//base_64转换
             for(len=0;len<2;len++)
             {
             printf("*********************=%02x\r\n",temp_bw[len]);
             }
              printf("=%d\r\n",out_len);
               printf("=%d\r\n",out_len_r);
            flag=1;
      if(flag==1)
    {
  
          //  app_uart();
            rx_task(out_len, out_len_r);
          //  tx_task();
           
    }
            
        }
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(DEMO_TAG, "MQTT_EVENT_ERROR");
        break;
    }
    return ESP_OK;
}


static void mqtt_app_start(void)
{
    const esp_mqtt_client_config_t  mqtt_cfg = {
        .event_handle = mqtt_event_handler,
        .host            = MQTT_HOST,
        .port            = MQTT_PORT,
//       .client_id       = "test",
        .username        = "",
        .password        = "",
        .keepalive       = 120,
        .lwt_topic       = "/lwt",
        .lwt_msg         = "offline",
        .lwt_qos         = 0,
        .lwt_retain      = 0
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_start(client);
	g_mqtt_client = client;	
}


static void update_device_info(esp_bt_gap_cb_param_t *param)
{
    char bda_str[18];
    uint32_t cod = 0;
    int16_t rssi = -129; /* invalid value */
    esp_bt_gap_dev_prop_t *p;

    ESP_LOGI(BT_DIS, "Device found: %s", bda2str(param->disc_res.bda, bda_str, 18));
    for (int i = 0; i < param->disc_res.num_prop; i++) {
        p = param->disc_res.prop + i;
        switch (p->type) {
        case ESP_BT_GAP_DEV_PROP_COD:
            cod = *(uint32_t *)(p->val);
            ESP_LOGI(BT_DIS, "--Class of Device: 0x%x", cod);
            break;
        case ESP_BT_GAP_DEV_PROP_RSSI:
            rssi = *(int8_t *)(p->val);
            ESP_LOGI(BT_DIS, "--RSSI: %d", rssi);
            break;
        case ESP_BT_GAP_DEV_PROP_BDNAME:
        default:
            break;
        }
    }

	if ( rssi > THRESHOLD ) {
		send_data_to_mqtt(rssi, bda_str, "bt");
	//	send_data_to_socket(tcpsock_1,rssi, bda_str, "bt");
	//	send_data_to_socket(tcpsock_2,rssi, bda_str, "bt");
	}

    /* search for device with MAJOR service class as "rendering" in COD */
    app_gap_cb_t *p_dev = &m_dev_info;
    if (p_dev->dev_found && 0 != memcmp(param->disc_res.bda, p_dev->bda, ESP_BD_ADDR_LEN)) {
        return;
    }

    if (!esp_bt_gap_is_valid_cod(cod) ||
            !(esp_bt_gap_get_cod_major_dev(cod) == ESP_BT_COD_MAJOR_DEV_PHONE)) {
        return;
    }

    memcpy(p_dev->bda, param->disc_res.bda, ESP_BD_ADDR_LEN);
    p_dev->dev_found = true;
    for (int i = 0; i < param->disc_res.num_prop; i++) {
        p = param->disc_res.prop + i;
        switch (p->type) {
        case ESP_BT_GAP_DEV_PROP_COD:
            p_dev->cod = *(uint32_t *)(p->val);
            break;
        case ESP_BT_GAP_DEV_PROP_RSSI:
            p_dev->rssi = *(int8_t *)(p->val);
            break;
        case ESP_BT_GAP_DEV_PROP_BDNAME: {
            uint8_t len = (p->len > ESP_BT_GAP_MAX_BDNAME_LEN) ? ESP_BT_GAP_MAX_BDNAME_LEN :
                          (uint8_t)p->len;
            memcpy(p_dev->bdname, (uint8_t *)(p->val), len);
            p_dev->bdname[len] = '\0';
            p_dev->bdname_len = len;
            break;
        }
        case ESP_BT_GAP_DEV_PROP_EIR: {
            memcpy(p_dev->eir, (uint8_t *)(p->val), p->len);
            p_dev->eir_len = p->len;
            break;
        }
        default:
            break;
        }
    }

 //   if (p_dev->eir && p_dev->bdname_len == 0) {
 //       get_name_from_eir(p_dev->eir, p_dev->bdname, &p_dev->bdname_len);
 //       ESP_LOGI(GAP_TAG, "Found a target device, address %s, name %s", bda_str, p_dev->bdname);
 //       p_dev->state = APP_GAP_STATE_DEVICE_DISCOVER_COMPLETE;
 //       ESP_LOGI(GAP_TAG, "Cancel device discovery ...");
 //       esp_bt_gap_cancel_discovery();
 //   }
}


void bt_app_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    app_gap_cb_t *p_dev = &m_dev_info;
    switch (event) {
	    case ESP_BT_GAP_DISC_RES_EVT: {
	        update_device_info(param);
	        break;
	    }
	    case ESP_BT_GAP_DISC_STATE_CHANGED_EVT: {
	        ESP_LOGE(DEMO_TAG, "%d", p_dev->state);
	        if(p_dev->state == APP_GAP_STATE_IDLE){
	            ESP_LOGE(BT_DIS, "discovery start ...");
			    ESP_LOGI(BT_DIS, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
	            p_dev->state = APP_GAP_STATE_DEVICE_DISCOVERING; 
	        }else if(p_dev->state == APP_GAP_STATE_DEVICE_DISCOVERING){
	            ESP_LOGE(BT_DIS, "discovery timeout ...");
	            p_dev->state = APP_GAP_STATE_DEVICE_DISCOVER_COMPLETE;
	        }else{
	            ESP_LOGE(BT_DIS, "discovery again ...");
	            esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 0x30, 0);
	            p_dev->state = APP_GAP_STATE_IDLE;
	        }
	        break;
	    }
	    case ESP_BT_GAP_RMT_SRVCS_EVT: {
	        break;
	    }
	    case ESP_BT_GAP_RMT_SRVC_REC_EVT:
	    default: {
	        break;
	    }
    }
    return;
}



void bt_app_gap_start_up(void)
{
    char *dev_name = "ESP_GAP_INQRUIY";
    esp_bt_dev_set_device_name(dev_name);

    /* set discoverable and connectable mode, wait to be connected */
    esp_bt_gap_set_scan_mode(ESP_BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE);

    /* register GAP callback function */
    esp_bt_gap_register_callback(bt_app_gap_cb);

    /* inititialize device information and status */
    app_gap_cb_t *p_dev = &m_dev_info;
    memset(p_dev, 0, sizeof(app_gap_cb_t));

    /* start to discover nearby Bluetooth devices */
	p_dev->state = APP_GAP_STATE_IDLE;
    esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 0x30, 0);
} 




	char* jsonRoot ="{\"mac\":\"84:f3:eb:b3:a7:05\", \"number\":2,\"value\":{\"name\":\"xuhongv\",\"age\":18,\"blog\":\"https://blog.csdn.net/xh870189248\"},\"hexArry\":[51,15,63,22,96]}";


#define   MAIN
#define   mqtt

#ifdef  MAIN
void app_main(void)
{
//app_pwm();
//app_led();

init();
//app_uart();//串口发送任务
//test_nontask();

//send_packages_test();
//test();//52832

 //parse_Json(jsonRoot);//json解析

#ifdef  mqtt  





printf("\n\n-------------------------------- Get Systrm Info------------------------------------------\n");
//获取从未使用过的最小内存
printf("     esp_get_minimum_free_heap_size : %d  \n", esp_get_minimum_free_heap_size());

	esp_err_t ret;
	uint8_t mac[6];

    ESP_LOGI(DEMO_TAG, "[APP] Startup..");
    ESP_LOGI(DEMO_TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());//获取芯片可用内存
    ESP_LOGI(DEMO_TAG, "[APP] IDF version: %s", esp_get_idf_version());

	ret=esp_read_mac(mac, 2);//0:wifi station, 1:wifi softap, 2:bluetooth, 3:ethernet
	ESP_ERROR_CHECK(ret);

    ESP_LOGI(DEMO_TAG, "[APP] ESP32 bluetooth mac addres is %s", bda2str(mac, g_bluetooth_mac, 18));

//  ESP_ERROR_CHECK(nvs_flash_init()); 
    //避免 ESP_ERR_NVS_NO_FREE_PAGES ，一直重启
	ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
		   ESP_ERROR_CHECK(nvs_flash_erase());
		   ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK( ret );
 // ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BTDM));
 /*
    If the mode is ESP_BT_MODE_BTDM, then it may be useful to call API esp_bt_mem_release(ESP_BT_MODE_BTDM) instead, 
    which internally calls esp_bt_controller_mem_release(ESP_BT_MODE_BTDM) and additionally releases the BSS and data 
    consumed by the BT/BLE host stack to heap. For more details about usage please refer to the documentation 
    of esp_bt_mem_release() function
 */
// ESP_ERROR_CHECK(esp_bt_mem_release(ESP_BT_MODE_BTDM));
/****************
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_bt_controller_init(&bt_cfg);
    
    esp_bt_controller_enable(ESP_BT_MODE_BTDM);
  ******************/
    wifi_init();
    mqtt_app_start();
//    tcp_init_client();
/****************************************
    ble_ibeacon_init();

   //  set scan parameters 
//#if (IBEACON_MODE == IBEACON_RECEIVER)
    esp_ble_gap_set_scan_params(&ble_scan_params);

//#elif (IBEACON_MODE == IBEACON_SENDER)
    esp_ble_ibeacon_t ibeacon_adv_data;
    esp_err_t status = esp_ble_config_ibeacon_data (&vendor_config, &ibeacon_adv_data);
    if (status == ESP_OK){
        esp_ble_gap_config_adv_data_raw((uint8_t*)&ibeacon_adv_data, sizeof(ibeacon_adv_data));
    }
    else {
        ESP_LOGE(DEMO_TAG, "Config iBeacon data failed: %s\n", esp_err_to_name(status));
    }
     ************************************** */
#endif
 
//#endif

	/*
	 *  class_bt discovery
	 */

//	bt_app_gap_start_up();


}
#endif
