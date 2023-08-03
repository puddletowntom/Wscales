
#ifndef ADS1230_H
#define ADS1230_H

#include <stdbool.h>

void ADS1230_init();
float ADS1230_read();
bool hasData();
void calibrateADC();
void calibrateWeight();
void pwrADS1230_up();
void pwrADS1230_down();
float getWeight(float adc_val);

#endif
