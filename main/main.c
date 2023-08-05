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
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_system.h"

#define NVS_KEY_BOOL "calBool"
#define NVS_KEY_FLOAT "caliVal"

#define TAG "MAIN"
//#define ADS1230_HOST    SPI1_HOST
#define ADS1230_HOST    HSPI_HOST
char dispTxt[20];
float weightVal = 0.0L;
esp_err_t ret;
nvs_handle_t nvsHandle;
bool calibrateComplete = false;

int32_t map(int32_t x) {
    return (int32_t) ((x*100)/3000); 
}

void nvsCalInit(){
    // Initialize NVS
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ret = nvs_open("storage", NVS_READWRITE, &nvsHandle);
    if (ret != ESP_OK) {
        printf("Error (%s) opening NVS!\n", esp_err_to_name(ret));
        return;
    }
}

void nvsCalibrationRecord(){
    nvsCalInit();

    printf("Writing value: %d\n", (int8_t)calibrateComplete);
    esp_err_t ret = nvs_set_i8(nvsHandle, NVS_KEY_BOOL, (int8_t)calibrateComplete);
    if (ret != ESP_OK) {
        printf("Failed to save boolean value. Error code: %d\n", ret);
    }
    // Save the value to NVS
    ret = nvs_set_blob(nvsHandle, NVS_KEY_FLOAT, &calibrateVal, sizeof(float)); //calibrateVal is the value used to calibrate
    if (ret != ESP_OK) {
        printf("Error (%s) saving value to NVS!\n", esp_err_to_name(ret));
    } else {
        printf("Value saved to NVS.\n");
    }
    // Close NVS
    ret = nvs_commit(nvsHandle);
    if (ret != ESP_OK) {
        printf("Failed to commit changes. Error code: %d\n", ret);
    }
    nvs_close(nvsHandle);
}

void nvsCalibrationLoad(){
    nvsCalInit();
    // Read the boolean value with the key "my_bool_key"
    ret = nvs_get_i8(nvsHandle, NVS_KEY_BOOL, (int8_t*)&calibrateComplete);
    if (ret == ESP_OK) {
        printf("Boolean value: %d\n", calibrateComplete);
    } else {
        printf("Failed to read boolean value. Error code: %d\n", ret);
    }

    // Read the float value as a blob with the key "my_float_key"
    size_t blob_size = sizeof(float);
    ret = nvs_get_blob(nvsHandle, NVS_KEY_FLOAT, &calibrateVal, &blob_size);
    if (ret == ESP_OK) {
        printf("Float value: %f\n", calibrateVal);
    } else {
        printf("Failed to read float value as blob. Error code: %d\n", ret);
    }
    // Close the NVS handle when done
    nvs_close(nvsHandle);
}

//If you want to remove persistent storage, use this function.
void nvsClearValues(){
    nvsCalInit();
    // Clear the boolean value with the key "my_bool_key"
    ret = nvs_erase_key(nvsHandle, NVS_KEY_BOOL);
    if (ret == ESP_OK) {
        printf("Boolean value cleared from NVS\n");
    } else {
        printf("Failed to clear boolean value from NVS. Error code: %d\n", ret);
    }

    // Clear the float value with the key "my_float_key"
    ret = nvs_erase_key(nvsHandle, NVS_KEY_FLOAT);
    if (ret == ESP_OK) {
        printf("Float value cleared from NVS\n");
    } else {
        printf("Failed to clear float value from NVS. Error code: %d\n", ret);
    }

    // Close the NVS handle when done
    nvs_close(nvsHandle);
}


void getADCValue(void *pvParameter){
    for(;;){
        ESP_LOGI("..", "ADC 1 running.");
        float adc_val = 0.0L;
        adc_val = ADS1230_read();
        if(adc_val < 50000 && calibrateComplete == false){
            calibrateWeight();
            calibrateComplete = true;
            nvsCalibrationRecord();
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
        if(calibrateComplete){
            lv_label_set_text(ui_Label2, dispTxt);//lv_label_set_text(label1, myStr);
            lv_meter_set_indicator_end_value(ui_meter, indic1, numInt);
        }else{
            lv_label_set_text(ui_Label2, "Not Calibrated");
        }
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
   nvsCalibrationLoad();
   //nvsClearValues();
   xTaskCreatePinnedToCore(lvgl_task, "LCD", 8 * 1024, NULL, 3, NULL, 1);
   xTaskCreatePinnedToCore(getADCValue, "ADC", 4 * 1024, NULL, 5, NULL, 1);
}
