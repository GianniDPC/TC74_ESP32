#include <stdio.h>
#include "sdkconfig.h"
#include "tc74.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void)
{
    TC74_init(0, TC74_A0);

    while(1){
        printf("Temperature in Celsius: %d\r\n", (int)read_TC74(Celsius));
        printf("Temperature in Fahrenheit: %.2f\r\n", read_TC74(Fahrenheit));
        vTaskDelay(1000 / portTICK_PERIOD_MS);      
    }
}


