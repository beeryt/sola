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
#define LED1_GPIOTE_CH 0
#define LED3_GPIOTE_CH 1
#define BUT4_GPIOTE_CH 2

#define SET(value, ident) ((value & ident##_Msk) << ident##_Pos)
#define GPIOTE_CONFIG(mode, psel, port, polarity, outint) \
    SET(mode, GPIOTE_CONFIG_MODE) |                       \
        SET(psel, GPIOTE_CONFIG_PSEL) |                   \
        SET(port, GPIOTE_CONFIG_PORT) |                   \
        SET(polarity, GPIOTE_CONFIG_POLARITY) |           \
        SET(outint, GPIOTE_CONFIG_OUTINIT)

#define BIT_READ(Val, Msk, Pos) (Val >> Pos) & Msk
#define BIT_WRITE(Val, Msk, Pos) (Val & Msk) << Pos

void toggle(void *)
{
    printk("toggle!\n");
    nrf_egu_event_clear(NRF_EGU4, nrf_egu_triggered_event_get(4));
    nrf_gpiote_event_clear(NRF_GPIOTE, nrf_gpiote_in_event_get(BUT4_GPIOTE_CH));
    nrf_gpiote_task_trigger(NRF_GPIOTE, nrf_gpiote_out_task_get(LED1_GPIOTE_CH));
}

/**                                            \
 * @brief GPIOTE_CONFIG [CONFIG] (Unspecified) \
 */
int main(void)
{
    printk("boot: %d\n", k_uptime_get_32());
    const uint32_t led1_pin = 13;
    const uint32_t LED1_Port = 0;
    const uint32_t LED3_Pin = 15;
    const uint32_t LED3_Port = 0;
    const uint32_t but4_pin = 25;
    const uint32_t BUT4_Port = 0;

    NRFX_PPI_CHANNELS_USED;
    NRFX_GPIOTE_CHANNELS_USED;
    NRFX_EGUS_USED;

#if 0
    printk("MAINREGSTATUS: 0x%x\n", BIT_READ(NRF_POWER->MAINREGSTATUS, POWER_MAINREGSTATUS_MAINREGSTATUS_Msk, POWER_MAINREGSTATUS_MAINREGSTATUS_Pos));
    printk("REGOUT: 0x%x\n", BIT_READ(NRF_UICR->REGOUT0, UICR_REGOUT0_VOUT_Msk, UICR_REGOUT0_VOUT_Pos));

    // errata-155
    *(volatile uint32_t *)(NRF_GPIOTE_BASE + 0x600 + (4 * BUT4_GPIOTE_CH)) = 1;

    nrf_gpio_cfg_output(led1_pin);
    nrf_gpio_cfg_output(LED3_Pin);
    nrf_gpio_cfg_input(but4_pin, NRF_GPIO_PIN_PULLUP);
    k_msleep(20);

    nrf_gpio_pin_clear(led1_pin);
    nrf_gpio_pin_set(LED3_Pin);
    k_msleep(20);

    nrf_gpiote_task_configure(NRF_GPIOTE, LED1_GPIOTE_CH, led1_pin, NRF_GPIOTE_POLARITY_TOGGLE, NRF_GPIOTE_INITIAL_VALUE_HIGH);
    nrf_gpiote_task_configure(NRF_GPIOTE, LED3_GPIOTE_CH, LED3_Pin, NRF_GPIOTE_POLARITY_TOGGLE, NRF_GPIOTE_INITIAL_VALUE_LOW);
    nrf_gpiote_event_configure(NRF_GPIOTE, BUT4_GPIOTE_CH, but4_pin, NRF_GPIOTE_POLARITY_TOGGLE);
    nrf_gpiote_task_enable(NRF_GPIOTE, LED1_GPIOTE_CH);
    nrf_gpiote_task_enable(NRF_GPIOTE, LED3_GPIOTE_CH);
    nrf_gpiote_event_enable(NRF_GPIOTE, BUT4_GPIOTE_CH);
    k_msleep(20);

    nrf_gpiote_task_trigger(NRF_GPIOTE, NRF_GPIOTE_TASK_CLR_0);
    nrf_gpiote_task_trigger(NRF_GPIOTE, NRF_GPIOTE_TASK_OUT_1);
    k_msleep(20);
    // nrf_gpiote_task_trigger(NRF_GPIOTE, NRF_GPIOTE_TASK_OUT_0);

    nrf_ppi_channel_endpoint_setup(NRF_PPI, PPI_SLEEP_CH, &NRF_POWER->EVENTS_SLEEPENTER, &NRF_GPIOTE->TASKS_CLR[LED1_GPIOTE_CH]);
    nrf_ppi_channel_endpoint_setup(NRF_PPI, PPI_WAKE_CH, &NRF_POWER->EVENTS_SLEEPEXIT, &NRF_GPIOTE->TASKS_SET[LED1_GPIOTE_CH]);
    nrf_ppi_channel_and_fork_endpoint_setup(NRF_PPI, PPI_BUTTON_CH, &NRF_GPIOTE->EVENTS_IN[BUT4_GPIOTE_CH], &NRF_GPIOTE->TASKS_OUT[LED3_GPIOTE_CH], &NRF_EGU4->TASKS_TRIGGER[4]);
    // nrf_ppi_channel_endpoint_setup(NRF_PPI, PPI_EGU_CH, &NRF_EGU4->EVENTS_TRIGGERED[4], &NRF_GPIOTE->TASKS_OUT[LED3_GPIOTE_CH]);
    nrf_ppi_channel_enable(NRF_PPI, PPI_SLEEP_CH);
    nrf_ppi_channel_enable(NRF_PPI, PPI_WAKE_CH);
    nrf_ppi_channel_enable(NRF_PPI, PPI_BUTTON_CH);
    // nrf_ppi_channel_enable(NRF_PPI, PPI_EGU_CH);

    IRQ_CONNECT(SWI4_EGU4_IRQn, IRQ_PRIO_LOWEST, toggle, NULL, 0);
    irq_enable(SWI4_EGU4_IRQn);
    nrf_egu_int_enable(NRF_EGU4, NRF_EGU_INT_TRIGGERED4);

    volatile uint32_t count = 0x20000;
    printk("uptime: %d\n", k_uptime_get_32());
    while (count--)
    {
    }
    printk("uptime: %d\n", k_uptime_get_32());
    nrf_gpiote_task_trigger(NRF_GPIOTE, NRF_GPIOTE_TASK_OUT_1);

    // trigger test EGU
    // nrf_egu_task_trigger(NRF_EGU4, NRF_EGU_TASK_TRIGGER4);

    printk("going to sleep...\n");
    printk("am I asleep?\n");
    while (true)
    {
        k_msleep(200);
    }
#else
    enum
    {
        led1_gpiote_ch = 0,
        but4_gpiote_ch
    };
    enum
    {
        button_ppi_ch = 0
    };

    nrf_gpio_cfg_output(led1_pin);
    nrf_gpio_cfg_input(but4_pin, NRF_GPIO_PIN_PULLUP);

    nrf_gpiote_task_configure(NRF_GPIOTE, led1_gpiote_ch, led1_pin, NRF_GPIOTE_POLARITY_TOGGLE, NRF_GPIOTE_INITIAL_VALUE_HIGH);
    nrf_gpiote_event_configure(NRF_GPIOTE, but4_gpiote_ch, but4_pin, NRF_GPIOTE_POLARITY_TOGGLE);
    nrf_gpiote_task_enable(NRF_GPIOTE, led1_gpiote_ch);
    nrf_gpiote_event_enable(NRF_GPIOTE, but4_gpiote_ch);

    nrf_ppi_channel_endpoint_setup(NRF_PPI, button_ppi_ch, &NRF_GPIOTE->EVENTS_IN[but4_gpiote_ch], &NRF_GPIOTE->TASKS_OUT[led1_gpiote_ch]);
    nrf_ppi_channel_enable(NRF_PPI, button_ppi_ch);
#endif
    return 0;
}
