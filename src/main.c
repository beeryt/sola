#include <zephyr/kernel.h>
#include <nrfx_gpiote.h>
#include <nrfx_ppi.h>
#include <nrfx_power.h>

#define BIT_READ(Val, Msk, Pos) (Val >> Pos) & Msk
#define BIT_WRITE(Val, Msk, Pos) (Val & Msk) << Pos
#define DEBUG

static inline bool allocate_ppi(nrf_ppi_channel_t *ch)
{
    nrfx_err_t err = nrfx_ppi_channel_alloc(ch);
    bool ret = err == NRFX_SUCCESS;
    if (!ret)
    {
        printk("failed to allocate ppi channel: 0x%08x\n", err);
    }
    return ret;
}

static inline bool allocate_gpiote(uint8_t *ch)
{
    nrfx_err_t err = nrfx_gpiote_channel_alloc(ch);
    bool ret = err == NRFX_SUCCESS;
    if (!ret)
    {
        printk("failed to allocate gpiote channel: 0x%08x\n", err);
        return false;
    }
    return ret;
}

static inline bool configure_gpiote_task(nrfx_gpiote_pin_t pin, uint8_t ch, nrf_gpio_pin_pull_t pull, nrf_gpiote_polarity_t polarity, nrf_gpiote_outinit_t init)
{
    nrfx_gpiote_output_config_t config = {
        .drive = NRF_GPIO_PIN_S0S1,
        .input_connect = NRF_GPIO_PIN_INPUT_DISCONNECT,
        .pull = pull};
    nrfx_gpiote_task_config_t task = {
        .task_ch = ch,
        .polarity = polarity,
        .init_val = init};
    nrfx_err_t err = nrfx_gpiote_output_configure(pin, &config, &task);
    bool ret = err == NRFX_SUCCESS;
    if (!ret)
    {
        printk("failed to configure gpiote task: 0x%08x\n", err);
    }
    return ret;
}

static inline bool configure_gpiote_event(nrfx_gpiote_pin_t pin, uint8_t *ch, nrf_gpio_pin_pull_t pull, nrfx_gpiote_trigger_t trigger_type)
{
    nrfx_gpiote_input_config_t config = {
        .pull = pull};
    nrfx_gpiote_trigger_config_t trigger = {
        .p_in_channel = ch,
        .trigger = trigger_type};
    nrfx_err_t err = nrfx_gpiote_input_configure(pin, &config, &trigger, NULL);
    bool ret = err == NRFX_SUCCESS;
    if (!ret)
    {
        printk("failed to configure gpiote event: 0x%08x\n", err);
    }
    return ret;
}

static inline bool assign_ppi_channel(nrf_ppi_channel_t ch, uint32_t eep, uint32_t tep)
{
#ifdef DEBUG
    printk("assigning ppi channel %x (%08x->%08x)\n", ch, eep, tep);
#endif
    nrfx_err_t err = nrfx_ppi_channel_assign(ch, eep, tep);
    bool ret = err == NRFX_SUCCESS;
    if (!ret)
    {
        printk("failed to assign ppi channel %x: 0x%08x\n", ch, err);
    }
    return ret;
}

static inline bool enable_ppi_channel(nrf_ppi_channel_t ch)
{
    nrfx_err_t err = nrfx_ppi_channel_enable(ch);
    bool ret = err == NRFX_SUCCESS;
    if (!ret)
    {
        printk("failed to enable ppi channel: 0x%08x\n", err);
    }
#ifdef DEBUG
    printk("enabling ppi channel %x\n", ch);
#endif
    return ret;
}

static inline bool init_gpiote()
{
    if (!nrfx_gpiote_is_init())
    {
        nrfx_err_t err = nrfx_gpiote_init(0);
        if (NRFX_SUCCESS != err)
        {
            printk("failed to init gpiote: 0x%08x\n", err);
            return false;
        }
#ifdef DEBUG
        printk("initialized gpiote\n");
#endif
    }
#ifdef DEBUG
    else
    {
        printk("gpoite initialized by environment\n");
    }
#endif
    return true;
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
    NRFX_PPI_GROUPS_USED;
    NRFX_GPIOTE_CHANNELS_USED;
    NRFX_EGUS_USED;

    printk("MAINREGSTATUS: 0x%x\n", BIT_READ(NRF_POWER->MAINREGSTATUS, POWER_MAINREGSTATUS_MAINREGSTATUS_Msk, POWER_MAINREGSTATUS_MAINREGSTATUS_Pos));
    printk("REGOUT: 0x%x\n", BIT_READ(NRF_UICR->REGOUT0, UICR_REGOUT0_VOUT_Msk, UICR_REGOUT0_VOUT_Pos));

    nrfx_err_t err;
    if (!init_gpiote())
        return 1;

#define ALLOC_GPIOTE_CHANNEL(ident)                                  \
    uint8_t ident;                                                   \
    {                                                                \
        if (!allocate_gpiote(&ident))                                \
            return 1;                                                \
        printk("allocated gpiote channel %x (%s)\n", ident, #ident); \
    }
    ALLOC_GPIOTE_CHANNEL(led1_gpiote_ch);
    ALLOC_GPIOTE_CHANNEL(led3_gpiote_ch);
    ALLOC_GPIOTE_CHANNEL(but4_gpiote_ch);

#define ALLOC_PPI_CHANNEL(ident)                                  \
    nrf_ppi_channel_t ident;                                      \
    {                                                             \
        if (!allocate_ppi(&ident))                                \
            return 1;                                             \
        printk("allocated ppi channel %x (%s)\n", ident, #ident); \
    }
    ALLOC_PPI_CHANNEL(button_ppi_ch);
    ALLOC_PPI_CHANNEL(sleep_ppi_ch);
    ALLOC_PPI_CHANNEL(wake_ppi_ch);

    // configure button->led behavior (GPIOTE)
    if (!configure_gpiote_task(led1_pin, led1_gpiote_ch, NRF_GPIO_PIN_NOPULL, NRF_GPIOTE_POLARITY_TOGGLE, NRF_GPIOTE_INITIAL_VALUE_HIGH))
        return 1;
    if (!configure_gpiote_event(but4_pin, &but4_gpiote_ch, NRF_GPIO_PIN_PULLUP, NRFX_GPIOTE_TRIGGER_HITOLO))
        return 1;
    nrfx_gpiote_out_task_enable(led1_pin);
    nrfx_gpiote_trigger_enable(but4_pin, false);
    // configure button->led behavior (PPI)
    if (!assign_ppi_channel(button_ppi_ch, nrfx_gpiote_in_event_address_get(but4_pin), nrfx_gpiote_out_task_address_get(led1_pin)))
        return 1;
    if (!enable_ppi_channel(button_ppi_ch))
        return 1;

    // configure power->led behavior (GPIOTE)
    if (!configure_gpiote_task(led3_pin, led3_gpiote_ch, NRF_GPIO_PIN_NOPULL, NRF_GPIOTE_POLARITY_TOGGLE, NRF_GPIOTE_INITIAL_VALUE_LOW))
        return 1;
    nrfx_gpiote_out_task_enable(led3_pin);
    // configure power->led behavior (PPI)
    if (!assign_ppi_channel(sleep_ppi_ch, &NRF_POWER->EVENTS_SLEEPENTER, nrfx_gpiote_set_task_address_get(led3_pin)))
        return 1;
    if (!assign_ppi_channel(wake_ppi_ch, &NRF_POWER->EVENTS_SLEEPEXIT, nrfx_gpiote_clr_task_address_get(led3_pin)))
        return 1;
    if (!enable_ppi_channel(sleep_ppi_ch))
        return 1;
    if (!enable_ppi_channel(wake_ppi_ch))
        return 1;

#define _DEBUG_GPIOTE_CHANNEL(pin, endpoint) nrfx_gpiote_##endpoint##_address_get(pin)
#define DEBUG_GPIOTE_CHANNEL(pin) \
    printk("%s\t0x%08x 0x%08x 0x%08x 0x%08x\n", #pin, _DEBUG_GPIOTE_CHANNEL(pin, out_task), _DEBUG_GPIOTE_CHANNEL(pin, set_task), _DEBUG_GPIOTE_CHANNEL(pin, clr_task), _DEBUG_GPIOTE_CHANNEL(pin, in_event));
    printk("channel\t\tout_task   set_task   clr_task   in_event\n");
    DEBUG_GPIOTE_CHANNEL(led1_pin);
    DEBUG_GPIOTE_CHANNEL(led3_pin);
    DEBUG_GPIOTE_CHANNEL(but4_pin);

    timestamp = k_uptime_delta(&timestamp);
    printk("program success: %d\n", (uint32_t)timestamp);
    return 0;
}
