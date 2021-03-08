#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "led.h"

static const char *TAG = "Led";


void app_pwm(void)
{
    //1. PWM: 定时器配置
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_13_BIT, // resolution of PWM duty
        .freq_hz = 5000,                      // frequency of PWM signal
        .speed_mode = LEDC_HS_MODE,           // timer mode
        .timer_num = LEDC_HS_TIMER,            // timer index
      //  .clk_cfg = LEDC_AUTO_CLK,              // Auto select the source clock
    };

    ledc_timer_config(&ledc_timer);
    //2. PWM:通道配置
    ledc_channel_config_t ledc_channel= {
        .channel    = LEDC_HS_CH0_CHANNEL,
        .duty       = 0,
        .gpio_num   = LEDC_HS_CH0_GPIO, //这里是SDK带的呼吸灯案例， 2表示你板子上的那个灯 2引脚的那个
        .speed_mode = LEDC_HS_MODE,
        .hpoint     = 0,
        .timer_sel  = LEDC_HS_TIMER   
    };
    ledc_channel_config(&ledc_channel);
    //3.PWM:使用硬件渐变
    ledc_fade_func_install(0);
    //4. 输出PWM信号控制灯
    while (1) {
        //灯：由灭慢慢变亮
        printf("1. LEDC fade up to duty = %d\n", LEDC_TEST_DUTY);
        ledc_set_fade_with_time(ledc_channel.speed_mode,
        ledc_channel.channel, LEDC_TEST_DUTY, LEDC_TEST_FADE_TIME);
        ledc_fade_start(ledc_channel.speed_mode,
        ledc_channel.channel, LEDC_FADE_NO_WAIT);
        vTaskDelay(LEDC_TEST_FADE_TIME / portTICK_PERIOD_MS);
        //灯：由亮慢慢变灭
        printf("2. LEDC fade down to duty = 0\n");
        ledc_set_fade_with_time(ledc_channel.speed_mode,
        ledc_channel.channel, 0, LEDC_TEST_FADE_TIME);
        ledc_fade_start(ledc_channel.speed_mode,
        ledc_channel.channel, LEDC_FADE_NO_WAIT);
        vTaskDelay(LEDC_TEST_FADE_TIME / portTICK_PERIOD_MS);
    }

}



void app_led(void)
{
	//选择IO
    gpio_pad_select_gpio(LED_R_IO);
  
    //设置IO为输出
    gpio_set_direction(LED_R_IO, GPIO_MODE_OUTPUT);
    while(1) {
        //只点亮红灯
        gpio_set_level(LED_R_IO, 0);
        vTaskDelay(100 / portTICK_PERIOD_MS);
        gpio_set_level(LED_R_IO, 1);
        vTaskDelay(100 / portTICK_PERIOD_MS);
        gpio_set_level(LED_R_IO, 0);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}
