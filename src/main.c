#include <zephyr/kernel.h>
#include <hal/nrf_power.h>
#include <hal/nrf_ppi.h>
#include <hal/nrf_gpiote.h>
#include <hal/nrf_gpio.h>
#include <hal/nrf_egu.h>

#include <nrfx_gpiote.h>
#include <nrfx_ppi.h>

#define BIT_READ(Val, Msk, Pos) (Val >> Pos) & Msk
#define BIT_WRITE(Val, Msk, Pos) (Val & Msk) << Pos

static inline bool allocate_ppi(nrf_ppi_channel_t *ch)
{
    nrfx_err_t err = nrfx_ppi_channel_alloc(ch);
    bool ret = err == NRFX_SUCCESS;
    if (!ret)
    {
        printk("failed to allocate ppi channel: %d\n", err);
    }
    return ret;
}

static inline bool allocate_gpiote(uint8_t *ch)
{
    nrfx_err_t err = nrfx_gpiote_channel_alloc(ch);
    bool ret = err == NRFX_SUCCESS;
    if (!ret)
    {
        printk("failed to allocate gpiote channel: %d\n", err);
        return false;
    }
    return ret;
}

/**                                            \
 * @brief GPIOTE_CONFIG [CONFIG] (Unspecified) \
 */
int main(void)
{
    uint64_t timestamp = k_uptime_get();
    printk("boot: %d\n", (uint32_t)timestamp);
    const uint32_t led1_pin = 13;
    const uint32_t led2_pin = 14;
    const uint32_t led3_pin = 15;
    const uint32_t but4_pin = 25;

    NRFX_PPI_CHANNELS_USED;
    NRFX_GPIOTE_CHANNELS_USED;
    NRFX_EGUS_USED;

    printk("MAINREGSTATUS: 0x%x\n", BIT_READ(NRF_POWER->MAINREGSTATUS, POWER_MAINREGSTATUS_MAINREGSTATUS_Msk, POWER_MAINREGSTATUS_MAINREGSTATUS_Pos));
    printk("REGOUT: 0x%x\n", BIT_READ(NRF_UICR->REGOUT0, UICR_REGOUT0_VOUT_Msk, UICR_REGOUT0_VOUT_Pos));

    nrfx_err_t err;

    uint8_t led1_gpiote_ch, led3_gpiote_ch, but4_gpiote_ch;
    if (!allocate_gpiote(&led1_gpiote_ch))
        return 1;
    if (!allocate_gpiote(&led3_gpiote_ch))
        return 1;
    if (!allocate_gpiote(&but4_gpiote_ch))
        return 1;

    nrf_ppi_channel_t button_ppi_ch, sleep_ppi_ch, wake_ppi_ch;
    if (!allocate_ppi(&button_ppi_ch))
        return 1;
    if (!allocate_ppi(&sleep_ppi_ch))
        return 1;
    if (!allocate_ppi(&but4_gpiote_ch))
        return 1;

    // errata-155
    *(volatile uint32_t *)(NRF_GPIOTE_BASE + 0x600 + (4 * but4_gpiote_ch)) = 1;

    // configure led2
    nrf_gpio_cfg_output(led2_pin);
    nrf_gpio_pin_clear(led2_pin);

    // configure button->led behavior (GPIO)
    nrf_gpio_cfg_output(led1_pin);
    nrf_gpio_cfg_input(but4_pin, NRF_GPIO_PIN_PULLUP);
    // configure button->led behavior (GPIOTE)
    nrf_gpiote_task_configure(NRF_GPIOTE, led1_gpiote_ch, led1_pin, NRF_GPIOTE_POLARITY_TOGGLE, NRF_GPIOTE_INITIAL_VALUE_HIGH);
    nrf_gpiote_event_configure(NRF_GPIOTE, but4_gpiote_ch, but4_pin, NRF_GPIOTE_POLARITY_HITOLO);
    nrf_gpiote_task_enable(NRF_GPIOTE, led1_gpiote_ch);
    nrf_gpiote_event_enable(NRF_GPIOTE, but4_gpiote_ch);
    // configure button->led behavior (PPI)
    nrf_ppi_channel_endpoint_setup(NRF_PPI, button_ppi_ch, &NRF_GPIOTE->EVENTS_IN[but4_gpiote_ch], &NRF_GPIOTE->TASKS_OUT[led1_gpiote_ch]);
    nrf_ppi_channel_enable(NRF_PPI, button_ppi_ch);

    // configure power->led behavior (GPIO)
    nrf_gpio_cfg_output(led3_pin);
    // configure power->led behavior (GPIOTE)
    nrf_gpiote_task_configure(NRF_GPIOTE, led3_gpiote_ch, led3_pin, NRF_GPIOTE_POLARITY_TOGGLE, NRF_GPIOTE_INITIAL_VALUE_LOW);
    nrf_gpiote_task_enable(NRF_GPIOTE, led3_gpiote_ch);
    // configure power->led behavior (PPI)
    nrf_ppi_channel_endpoint_setup(NRF_PPI, sleep_ppi_ch, &NRF_POWER->EVENTS_SLEEPENTER, &NRF_GPIOTE->TASKS_SET[led3_gpiote_ch]);
    nrf_ppi_channel_endpoint_setup(NRF_PPI, wake_ppi_ch, &NRF_POWER->EVENTS_SLEEPEXIT, &NRF_GPIOTE->TASKS_CLR[led3_gpiote_ch]);
    nrf_ppi_channel_enable(NRF_PPI, sleep_ppi_ch);
    nrf_ppi_channel_enable(NRF_PPI, wake_ppi_ch);

    timestamp = k_uptime_delta(&timestamp);
    printk("program success: %d\n", (uint32_t)timestamp);
    return 0;
}
