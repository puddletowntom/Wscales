#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "rom/gpio.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "lvgl.h"
#include "board.h"
#include "esp_timer.h"
#include "ui/ui.h"
#include "ADS1230/ADS1230.h"
#define TAG "MAIN"
//#define ADS1230_HOST    SPI1_HOST
#define ADS1230_HOST    HSPI_HOST
char dispTxt[20];
float weightVal = 0.0L;

int32_t map(int32_t x) {
    return (int32_t) ((x*100)/3000); 
}

void getADCValue(void *pvParameter){
    for(;;){
        ESP_LOGI("..", "ADC 1 running.");
        float adc_val = 0.0L;
        static bool calibrateComplete = false;
        adc_val = ADS1230_read();
        if(adc_val < 50000 && calibrateComplete == false){
            calibrateWeight();
            calibrateComplete = true;
        }
        if(adc_val > 50000){
            adc_val = 0.0L;
        }
        weightVal = getWeight(adc_val);
        sprintf(dispTxt, "%.3f", weightVal);
        vTaskDelay(pdMS_TO_TICKS(15));
    }
}

extern void screen_init(void);

void lvgl_task(void* arg) {
    for (;;) {
        int numInt = map((int32_t)weightVal);
        lv_label_set_text(ui_Label2, dispTxt);//lv_label_set_text(label1, myStr);
        lv_meter_set_indicator_end_value(ui_meter, indic1, numInt);
        lv_timer_handler();
        lv_task_handler();
        memset(dispTxt, 0, sizeof(dispTxt));
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void app_main(void) {
   ADS1230_init();
   screen_init();
   ui_init();
   xTaskCreatePinnedToCore(lvgl_task, "LCD", 8 * 1024, NULL, 3, NULL, 1);
   xTaskCreatePinnedToCore(getADCValue, "ADC", 4 * 1024, NULL, 5, NULL, 1);
}
