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

int32_t map(int32_t x) {
    return (int32_t) ((x*100)/3000); 
}

// void getADCValue(void *pvParameter){
//     for(;;){
//         float adc_val = 0.0L;
//         static bool calibrateComplete = false;
//         adc_val = ADS1230_read();
//         if(adc_val < 50000 && calibrateComplete == false){
//             calibrateWeight();
//             calibrateComplete = true;
//             printf("Calibration complete \n");
//         }
//         if(adc_val > 50000){
//             adc_val = 0.0L;
//         }
//         float weightVal = getWeight(adc_val);
//         sprintf(dispTxt, "%.3f", weightVal);
//         ESP_LOGI("..", "ADC 1 running.");
//         vTaskDelay(1000 / portTICK_PERIOD_MS);
//     }
// }

extern void screen_init(void);

// void lvgl_task(void* arg) {
//     screen_init();
//     ESP_LOGI("..", "LCD running.");
//     ui_init();
//     for (;;) {
//         lv_task_handler();
//         vTaskDelay(pdMS_TO_TICKS(10));
//     }
// }

void app_main(void) {
   // xTaskCreatePinnedToCore(lvgl_task, "LCD", 8 * 1024, NULL, 5, NULL, 1);
   // xTaskCreatePinnedToCore(getADCValue, "ADC", 4 * 1024, NULL, 4, NULL, 1);
   ADS1230_init();
   screen_init();
   ui_init();
   while(1){
        float adc_val = 0.0L;
        static bool calibrateComplete = false;
        adc_val = ADS1230_read();
        if(adc_val < 50000 && calibrateComplete == false){
            calibrateWeight();
            calibrateComplete = true;
            printf("Calibration complete \n");
        }
        if(adc_val > 50000){
            adc_val = 0.0L;
        }
        float weightVal = getWeight(adc_val);
        sprintf(dispTxt, "%.3f", weightVal);
        int numInt = map((int32_t)weightVal);
        lv_label_set_text(ui_Label2, dispTxt);//lv_label_set_text(label1, myStr);
        lv_meter_set_indicator_end_value(ui_meter, indic1, numInt);
        lv_timer_handler();
        memset(dispTxt, 0, sizeof(dispTxt));
        //ESP_LOGI("..", "Main running.");
        //vTaskDelay(1000 / portTICK_PERIOD_MS);
        lv_task_handler();
        vTaskDelay(pdMS_TO_TICKS(10));
       // vTaskDelay(1000 / portTICK_PERIOD_MS);
   }
}
