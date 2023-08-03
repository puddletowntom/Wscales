#include "driver/gpio.h"
#include "ADS1230.h"
#include "esp_log.h"
//#include <rom/ets_sys.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_timer.h"
#include "math.h"
#include "stdio.h"

#define GPIO_ADS_DATA   45
#define GPIO_ADS_CLK    44
#define GPIO_ADS_PDN    43
#define DELAY_CLOCK     20
#define PWR_ON_DELAY    10
#define CLK_DELAY       1

#define HIGH 1
#define LOW 0
#define PRECISION_20_BIT 1048576.0L //(2^20)
#define VOLT_REF 3.3L; //Full scale input voltage (AINP - AINN) This is 
#define GAIN 1 
#define VOLTAGE_IN 0.5L*VOLT_REF/GAIN
#define PGA_1 64 //Power Gain Amplifier
#define PGA_2 128 //Power Gain Amplifier
#define KNOWN_WEIGHT 1000.0L

float calibrateVal = 0.0L;

static void microSecondDelay(uint64_t usTime){
    uint64_t start_time = esp_timer_get_time();
    while((esp_timer_get_time() - start_time) < usTime); 
}

void ADS1230_init(){
    gpio_config_t io_conf;
    io_conf.intr_type = 0;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << GPIO_ADS_DATA);
    gpio_config(&io_conf);

    io_conf.intr_type = 0;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << GPIO_ADS_CLK);
    gpio_config(&io_conf);

    io_conf.intr_type = 0;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << GPIO_ADS_PDN);
    gpio_config(&io_conf);

    microSecondDelay(PWR_ON_DELAY);//Allow power to device for at least 10us before power up.
    pwrADS1230_up(); //Power on the chip with PDWN line
    calibrateADC(); //Offset calibration for ADC error
}

bool hasData(){ //Wait until data line is pulled low for data to be available
    while(gpio_get_level(GPIO_ADS_DATA)){
        //printf("pending...\n");
    }
    return true;
}

void pwrADS1230_up(){
    gpio_set_level(GPIO_ADS_PDN, HIGH);
}

void pwrADS1230_down(){
    gpio_set_level(GPIO_ADS_PDN, LOW);
}

uint8_t myArray[20];

int32_t binaryToDecimal(uint8_t binaryArray[], int length) {
    int32_t decimalValue = 0;
    int powerOfTwo = 1;
    for(int i = length - 1; i >= 0; i--) {
        if(binaryArray[i] == 1) {
            decimalValue += powerOfTwo;
        }
        powerOfTwo *= 2;
    }
    return decimalValue;
}

void calibrateADC(){
    hasData();
    for(int i = 0; i < 26; i++){ //Will start with first 20 bits and increase to 24 bits.
        gpio_set_level(GPIO_ADS_CLK, HIGH); //Pulse the clock high and low
        microSecondDelay(CLK_DELAY);
        gpio_set_level(GPIO_ADS_CLK, LOW);
        microSecondDelay(CLK_DELAY);
    }
    //Was going to put a delay in here to allow tcal to finish but you are checking for data line to be low before reading anyway.
}

void calibrateWeight(){
    ESP_LOGI("..", "Calibrating.");
    float samples = 0.0L;
    ADS1230_read(); //skip first val
    vTaskDelay(500/portTICK_PERIOD_MS);
    for(int i = 0; i < 5; i++){
        samples += ADS1230_read();
        vTaskDelay(250/portTICK_PERIOD_MS);
        ESP_LOGI("..", "Calibrating.");
    }
    calibrateVal = samples/5.0L;
    char myStr[20];
    sprintf(myStr, "%f", calibrateVal);
    ESP_LOGI("..", "Values calibration for 1kg: %s \n", myStr);
   // printf("Valus calibration for 1kg: %s \n", myStr);
}

float ADS1230_read(){
    hasData();
    gpio_set_level(GPIO_ADS_CLK, LOW); //Set the clock low to ready it for pulse.
    
    for(int i = 0; i < 20; i++){ //Will start with first 20 bits and increase to 24 bits.
        gpio_set_level(GPIO_ADS_CLK, HIGH); //Pulse the clock high and low
        microSecondDelay(CLK_DELAY);
        //value = value << 1;
        gpio_set_level(GPIO_ADS_CLK, LOW);
        microSecondDelay(CLK_DELAY);
        myArray[i] = gpio_get_level(GPIO_ADS_DATA); //Get value from data line nad add bit to 20 bit array.
        //printf("%u", myArray[i]);
    }
        gpio_set_level(GPIO_ADS_CLK, HIGH); //Pulse the clock high and low
        gpio_set_level(GPIO_ADS_CLK, LOW);
    //Binary to decimal conversion for 20 bit number.
    int32_t decimalValue = binaryToDecimal(myArray, 20); //Convert 20 bit number to decimal

   // float testVal = (float)decimalValue*VOLT_RANGE/PRECISION_20_BIT;
    //printf("\n");
    //pwrADS1230_down(); //Power off the chip with PDWN line
    //value =value^0x800000;
    return decimalValue;
}

float getWeight(float adc_val){
    float weight_val = 0.0L;
    if(adc_val > 0.0L && calibrateVal > 0.0L){
        weight_val = (adc_val/calibrateVal)*KNOWN_WEIGHT;
    }
    return weight_val;
}