#include <stdio.h>
#include <driver/gpio.h>
#include <esp_adc/adc_monitor.h>
#include <driver/ledc.h>    //  controle de LED por onda PWM
#include <esp_log.h>    //  ESP_LOGI()
#include <esp_rom_sys.h>
#include <esp_adc/adc_cali.h>
#include <esp_adc/adc_oneshot.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define ADC_UNIT_ID ADC_UNIT_2  // ADC2 usa GPIO 4,0,2,15,13,12,14,27,25,26 como ADC_CHANNEL_[0:9]
#define ADC_CHANNEL ADC_CHANNEL_0   // GPIO4

#define LED_PWM GPIO_NUM_15

static const char* TAG_ADC = "ADC_READING";
static const char* TAG_DUTY = "DUTY_CYCLE";

void app_main(void)
{   
    int adc_raw=0;
    //  Inicializa o ADC
    adc_oneshot_unit_handle_t adc_handle;
    adc_oneshot_unit_init_cfg_t init_config1={
        .unit_id=ADC_UNIT_ID,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc_handle));

    //  Configura o ADC criado
    adc_oneshot_chan_cfg_t config={
        .atten=ADC_ATTEN_DB_12, // maior Vref disponível
        .bitwidth=ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &config));

    //======//======//======//======//======//======//======//======//======//======

    int duty=0;
    //  Configuração do TIMER usado
    ledc_timer_config_t timer_conf = {
        .speed_mode=LEDC_LOW_SPEED_MODE,
        .timer_num=LEDC_TIMER_0,
        .freq_hz=5000,
        .duty_resolution=LEDC_TIMER_10_BIT,
        .clk_cfg=LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_conf));

    //  Configuração do CANAL usado
    ledc_channel_config_t channel_conf = {
        .channel=LEDC_CHANNEL_0,
        .speed_mode=LEDC_LOW_SPEED_MODE,
        .timer_sel=LEDC_TIMER_0,
        .intr_type=LEDC_INTR_DISABLE,
        .gpio_num=LED_PWM,
        .duty=511,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel_conf));

    while(1){
        adc_oneshot_read(adc_handle, ADC_CHANNEL, &adc_raw);
        ESP_LOGI(TAG_ADC, "%d", adc_raw);
        duty=(adc_raw*1023)/4095;   // <== ADC SAR de 12 bits
        if(duty>1023) duty=1023;    // adc_raw tem 12 bits enquanto duty tem 10 bits

        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
        ESP_LOGI(TAG_DUTY, "%d", duty);
        vTaskDelay(100/portTICK_PERIOD_MS); // <- somente para fins de monitoramento
    }
}
