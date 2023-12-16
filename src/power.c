#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(power, 4);

#include <hal/nrf_gpiote.h>
#include <nrfx_gpiote.h>
#include <nrfx_ppi.h>

#define CONFIG_POWER_LED_GPIO_PIN 13
#define CONFIG_POWER_LED_PRIORITY CONFIG_KERNEL_INIT_PRIORITY_DEVICE

typedef uint8_t nrf_gpiote_channel_t;
nrf_gpiote_channel_t led_gpiote_ch;
nrf_ppi_channel_t sleep_ppi_ch;
nrf_ppi_channel_t wake_ppi_ch;
nrf_ppi_channel_group_t ppi_gp;

#define FATAL_ERR(msg)       \
    if (err != NRFX_SUCCESS) \
    {                        \
        LOG_ERR(msg);        \
        return;              \
    }

void init_power_led()
{
    uint32_t pin = CONFIG_POWER_LED_GPIO_PIN;
    nrfx_gpiote_output_config_t config = {
        .drive = NRF_GPIO_PIN_S0S1,
        .input_connect = NRF_GPIO_PIN_INPUT_DISCONNECT,
        .pull = NRF_GPIO_PIN_NOPULL,
    };
    nrfx_gpiote_task_config_t task = {
        .task_ch = led_gpiote_ch,
        .polarity = NRF_GPIOTE_POLARITY_TOGGLE,
        .init_val = NRF_GPIOTE_INITIAL_VALUE_LOW,
    };
    nrfx_err_t err;

    err = nrfx_gpiote_channel_alloc(&led_gpiote_ch);
    FATAL_ERR("failed to allocate gpiote channel");

    err = nrfx_ppi_channel_alloc(&sleep_ppi_ch);
    FATAL_ERR("failed to allocate sleep ppi channel");

    err = nrfx_ppi_channel_alloc(&wake_ppi_ch);
    FATAL_ERR("failed to allocate wake ppi channel");

    err = nrfx_ppi_group_alloc(&ppi_gp);
    FATAL_ERR("failed to allocate ppi group");

    err = nrfx_gpiote_output_configure(pin, &config, &task);
    FATAL_ERR("failed to configure gpiote task");

    nrfx_gpiote_out_task_enable(pin);

    err = nrfx_ppi_channel_assign(sleep_ppi_ch, &NRF_POWER->EVENTS_SLEEPENTER, nrfx_gpiote_set_task_address_get(pin));
    FATAL_ERR("failed to assign sleep ppi channel");

    err = nrfx_ppi_channel_assign(wake_ppi_ch, &NRF_POWER->EVENTS_SLEEPEXIT, nrfx_gpiote_clr_task_address_get(pin));
    FATAL_ERR("failed to assign wake ppi channel");

    err = nrfx_ppi_channels_include_in_group(BIT(sleep_ppi_ch) | BIT(wake_ppi_ch), ppi_gp);
    FATAL_ERR("failed to assign ppi channels to group");

    err = nrfx_ppi_group_enable(ppi_gp);
    FATAL_ERR("failed to enable ppi group");
}

K_THREAD_DEFINE(power_led, 1024, init_power_led, NULL, NULL, NULL, CONFIG_POWER_LED_PRIORITY, 0, 0);