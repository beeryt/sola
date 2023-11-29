#include <zephyr/kernel.h>
#include <hal/nrf_power.h>
#include <hal/nrf_ppi.h>
#include <hal/nrf_gpiote.h>
#include <hal/nrf_gpio.h>
#include <hal/nrf_egu.h>

#define PPI_SLEEP_CH 0
#define PPI_WAKE_CH 1
#define PPI_EGU_CH 2
#define PPI_BUTTON_CH 3
#define LED0_GPIOTE_CH 0
#define LED3_GPIOTE_CH 1
#define BUT4_GPIOTE_CH 2

#define SET(value, ident) ((value & ident##_Msk) << ident##_Pos)
#define GPIOTE_CONFIG(mode, psel, port, polarity, outint) \
    SET(mode, GPIOTE_CONFIG_MODE) |                       \
        SET(psel, GPIOTE_CONFIG_PSEL) |                   \
        SET(port, GPIOTE_CONFIG_PORT) |                   \
        SET(polarity, GPIOTE_CONFIG_POLARITY) |           \
        SET(outint, GPIOTE_CONFIG_OUTINIT)

/**                                            \
 * @brief GPIOTE_CONFIG [CONFIG] (Unspecified) \
 */
int main(void)
{
    const uint32_t LED0_Pin = 13;
    const uint32_t LED0_Port = 0;
    const uint32_t LED3_Pin = 15;
    const uint32_t LED3_Port = 0;
    const uint32_t BUT4_Pin = 25;
    const uint32_t BUT4_Port = 0;

    NRFX_PPI_CHANNELS_USED;
    NRFX_GPIOTE_CHANNELS_USED;
    NRFX_EGUS_USED;

    // configure LED0_GPIOTE_CH
    nrf_gpio_cfg_output(LED0_Pin);
    nrf_gpio_cfg_output(LED3_Pin);
    nrf_gpio_cfg_input(BUT4_Pin, NRF_GPIO_PIN_PULLUP);

    nrf_gpiote_task_configure(NRF_GPIOTE, LED0_GPIOTE_CH, LED0_Pin, NRF_GPIOTE_POLARITY_TOGGLE, NRF_GPIOTE_INITIAL_VALUE_HIGH);
    nrf_gpiote_task_configure(NRF_GPIOTE, LED3_GPIOTE_CH, LED3_Pin, NRF_GPIOTE_POLARITY_TOGGLE, NRF_GPIOTE_INITIAL_VALUE_LOW);
    nrf_gpiote_event_configure(NRF_GPIOTE, BUT4_GPIOTE_CH, BUT4_Pin, NRF_GPIOTE_POLARITY_HITOLO);
    nrf_gpiote_task_enable(NRF_GPIOTE, LED0_GPIOTE_CH);
    nrf_gpiote_task_enable(NRF_GPIOTE, LED3_GPIOTE_CH);
    nrf_gpiote_event_enable(NRF_GPIOTE, BUT4_GPIOTE_CH);
    __ISB();
    __DSB();
    nrf_barrier_w();
    __DMB();
    // nrf_gpiote_task_trigger(NRF_GPIOTE, NRF_GPIOTE_TASK_CLR_0);
    // nrf_gpiote_task_trigger(NRF_GPIOTE, NRF_GPIOTE_TASK_SET_0);
    nrf_gpiote_task_trigger(NRF_GPIOTE, NRF_GPIOTE_TASK_OUT_0);

    nrf_ppi_channel_endpoint_setup(NRF_PPI, PPI_SLEEP_CH, NRF_POWER->EVENTS_SLEEPENTER, NRF_GPIOTE->TASKS_CLR[LED0_GPIOTE_CH]);
    nrf_ppi_channel_endpoint_setup(NRF_PPI, PPI_WAKE_CH, NRF_POWER->EVENTS_SLEEPEXIT, NRF_GPIOTE->TASKS_SET[LED0_GPIOTE_CH]);
    nrf_ppi_channel_endpoint_setup(NRF_PPI, PPI_BUTTON_CH, NRF_GPIOTE->EVENTS_IN[BUT4_GPIOTE_CH], NRF_GPIOTE->TASKS_OUT[LED3_GPIOTE_CH]);
    // nrf_ppi_channel_endpoint_setup(NRF_PPI, PPI_EGU_CH, NRF_EGU4->EVENTS_TRIGGERED[4], NRF_GPIOTE->TASKS_OUT[LED3_GPIOTE_CH]);
    nrf_ppi_channel_enable(NRF_PPI, PPI_SLEEP_CH);
    nrf_ppi_channel_enable(NRF_PPI, PPI_WAKE_CH);
    nrf_ppi_channel_enable(NRF_PPI, PPI_BUTTON_CH);
    // nrf_ppi_channel_enable(NRF_PPI, PPI_EGU_CH);

    nrf_egu_int_enable(NRF_EGU4, NRF_EGU_INT_TRIGGERED4);

    volatile uint32_t count = 0x20000;
    printk("uptime: %d\n", k_uptime_get_32());
    while (count--)
    {
    }
    printk("uptime: %d\n", k_uptime_get_32());
    nrf_gpiote_task_trigger(NRF_GPIOTE, NRF_GPIOTE_TASK_OUT_0);

    // trigger test EGU
    // nrf_egu_task_trigger(NRF_EGU4, NRF_EGU_TASK_TRIGGER4);

    __disable_irq();
    __WFI();
    __WFE();
    while (true)
    {
        k_msleep(200);
    }
    return 0;
}
