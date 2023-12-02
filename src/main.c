#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <nrfx_gpiote.h>
#include <nrfx_ppi.h>
#include <nrfx_power.h>
LOG_MODULE_REGISTER(app, CONFIG_APP_LOG_LEVEL);

extern char const *nrfx_err_string_get(nrfx_err_t);
#define HELPER_NRFX_FORMAT_ERR(err) "%s", nrfx_error_string_get(err)

typedef uint8_t nrf_gpiote_channel_t;

typedef struct
{
    nrf_gpiote_channel_t led_gpiote_ch;
    nrf_ppi_channel_t sleep_ppi_ch;
    nrf_ppi_channel_t wake_ppi_ch;
    // nrf_ppi_channel_group_t ppi_gp;
} power_context_t;

typedef struct
{
    nrf_gpiote_channel_t led_gpiote_ch;
    nrf_gpiote_channel_t swi_gpiote_ch;
    nrf_ppi_channel_t button_ppi_ch;
} button_context_t;

typedef struct
{
    power_context_t power;
    button_context_t button;
} app_context_t;

nrfx_err_t gpiote_channel_alloc(nrf_gpiote_channel_t *ch)
{
    if (ch == NULL)
        return NRFX_ERROR_INVALID_ADDR;
    nrfx_err_t err = nrfx_gpiote_channel_alloc(ch);
    if (err != NRFX_SUCCESS)
    {
        LOG_ERR("failed to allocate gpiote channel: " HELPER_NRFX_FORMAT_ERR(err));
    }
#ifdef DEBUG
    LOG_DBG("0x%02x", *ch);
#endif
    return err;
}

nrfx_err_t ppi_channel_alloc(nrf_ppi_channel_t *ch)
{
    if (ch == NULL)
        return NRFX_ERROR_INVALID_ADDR;
    nrfx_err_t err = nrfx_ppi_channel_alloc(ch);
    if (err != NRFX_SUCCESS)
    {
        LOG_ERR("failed to allocate ppi channel: " HELPER_NRFX_FORMAT_ERR(err));
    }
#ifdef DEBUG
    LOG_DBG("0x%02x", *ch);
#endif
    return err;
}

nrfx_err_t ppi_channel_group_alloc(nrf_ppi_channel_group_t *gp)
{
    if (gp == NULL)
        return NRFX_ERROR_INVALID_ADDR;
    nrfx_err_t err = nrfx_ppi_group_alloc(gp);
    if (err != NRFX_SUCCESS)
    {
        LOG_ERR("failed to allocate ppi channel group: " HELPER_NRFX_FORMAT_ERR(err));
    }
#ifdef DEBUG
    LOG_DBG("0x%02x", *gp);
#endif
    return err;
}

nrfx_err_t create_button_context(button_context_t *ctx)
{
    nrfx_err_t err = NRFX_SUCCESS;

    if (ctx == NULL)
        return NRFX_ERROR_INVALID_ADDR;

    err = gpiote_channel_alloc(&ctx->led_gpiote_ch);
    if (err != NRFX_SUCCESS)
        return err;

    err = gpiote_channel_alloc(&ctx->swi_gpiote_ch);
    if (err != NRFX_SUCCESS)
        return err;

    err = ppi_channel_alloc(&ctx->button_ppi_ch);
    if (err != NRFX_SUCCESS)
        return err;

    return err;
}

nrfx_err_t create_power_context(power_context_t *ctx)
{
    nrfx_err_t err = NRFX_SUCCESS;

    if (ctx == NULL)
    {
        return NRFX_ERROR_INVALID_ADDR;
    }

    err = gpiote_channel_alloc(&ctx->led_gpiote_ch);
    if (err != NRFX_SUCCESS)
        return err;

    err = ppi_channel_alloc(&ctx->sleep_ppi_ch);
    if (err != NRFX_SUCCESS)
        return err;

    err = ppi_channel_alloc(&ctx->wake_ppi_ch);
    if (err != NRFX_SUCCESS)
        return err;

    // err = ppi_channel_group_alloc(&ctx->ppi_gp);
    // if (err != NRFX_SUCCESS)
    //     return err;

    return err;
}

nrfx_err_t create_app_context(app_context_t *ctx)
{
    nrfx_err_t err = NRFX_SUCCESS;

    if (ctx == NULL)
    {
        return NRFX_ERROR_INVALID_PARAM;
    }

    err = create_power_context(&ctx->power);
    if (err != NRFX_SUCCESS)
    {
        LOG_ERR("failed to create power context: " HELPER_NRFX_FORMAT_ERR(err));
        return err;
    }

    err = create_button_context(&ctx->button);
    if (err != NRFX_SUCCESS)
    {
        LOG_ERR("failed to create button context: " HELPER_NRFX_FORMAT_ERR(err));
        return err;
    }

    return err;
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
        LOG_DBG("failed to configure gpiote task: 0x%08x", err);
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
        LOG_DBG("failed to configure gpiote event: 0x%08x", err);
    }
    return ret;
}

static inline bool assign_ppi_channel(nrf_ppi_channel_t ch, uint32_t eep, uint32_t tep)
{
#ifdef DEBUG
    LOG_DBG("assigning ppi channel %x (%08x->%08x)", ch, eep, tep);
#endif
    nrfx_err_t err = nrfx_ppi_channel_assign(ch, eep, tep);
    bool ret = err == NRFX_SUCCESS;
    if (!ret)
    {
        LOG_DBG("failed to assign ppi channel %x: 0x%08x", ch, err);
    }
    return ret;
}

static inline bool enable_ppi_channel(nrf_ppi_channel_t ch)
{
    nrfx_err_t err = nrfx_ppi_channel_enable(ch);
    bool ret = err == NRFX_SUCCESS;
    if (!ret)
    {
        LOG_DBG("failed to enable ppi channel: 0x%08x", err);
    }
#ifdef DEBUG
    LOG_DBG("enabling ppi channel %x", ch);
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
            LOG_DBG("failed to init gpiote: 0x%08x", err);
            return false;
        }
#ifdef DEBUG
        LOG_DBG("initialized gpiote");
#endif
    }
#ifdef DEBUG
    else
    {
        LOG_DBG("gpoite initialized by environment");
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
    LOG_DBG("boot: %d", (uint32_t)timestamp);
    const uint32_t led1_pin = DT_GPIO_PIN(DT_ALIAS(led0), gpios);
    const uint32_t led2_pin = DT_GPIO_PIN(DT_ALIAS(led1), gpios);
    const uint32_t led3_pin = DT_GPIO_PIN(DT_ALIAS(led2), gpios);
    const uint32_t but4_pin = DT_GPIO_PIN(DT_ALIAS(sw3), gpios);

    if (!init_gpiote())
        return 1;

    app_context_t ctx;
    nrfx_err_t err = create_app_context(&ctx);
    if (err != NRFX_SUCCESS)
    {
        LOG_ERR("failed to create app context: " HELPER_NRFX_FORMAT_ERR(err));
        return 1;
    }

    NRFX_PPI_CHANNELS_USED;
    NRFX_PPI_GROUPS_USED;
    NRFX_GPIOTE_CHANNELS_USED;
    NRFX_EGUS_USED;

    // configure button->led behavior (GPIOTE)
    if (!configure_gpiote_task(led1_pin, ctx.button.led_gpiote_ch, NRF_GPIO_PIN_NOPULL, NRF_GPIOTE_POLARITY_TOGGLE, NRF_GPIOTE_INITIAL_VALUE_HIGH))
        return 1;
    if (!configure_gpiote_event(but4_pin, &ctx.button.swi_gpiote_ch, NRF_GPIO_PIN_PULLUP, NRFX_GPIOTE_TRIGGER_HITOLO))
        return 1;
    nrfx_gpiote_out_task_enable(led1_pin);
    nrfx_gpiote_trigger_enable(but4_pin, false);
    // configure button->led behavior (PPI)
    if (!assign_ppi_channel(ctx.button.button_ppi_ch, nrfx_gpiote_in_event_address_get(but4_pin), nrfx_gpiote_out_task_address_get(led1_pin)))
        return 1;
    if (!enable_ppi_channel(ctx.button.button_ppi_ch))
        return 1;

    // configure power->led behavior (GPIOTE)
    if (!configure_gpiote_task(led3_pin, ctx.power.led_gpiote_ch, NRF_GPIO_PIN_NOPULL, NRF_GPIOTE_POLARITY_TOGGLE, NRF_GPIOTE_INITIAL_VALUE_LOW))
        return 1;
    nrfx_gpiote_out_task_enable(led3_pin);
    // configure power->led behavior (PPI)
    if (!assign_ppi_channel(ctx.power.sleep_ppi_ch, &NRF_POWER->EVENTS_SLEEPENTER, nrfx_gpiote_set_task_address_get(led3_pin)))
        return 1;
    if (!assign_ppi_channel(ctx.power.wake_ppi_ch, &NRF_POWER->EVENTS_SLEEPEXIT, nrfx_gpiote_clr_task_address_get(led3_pin)))
        return 1;
    if (!enable_ppi_channel(ctx.power.sleep_ppi_ch))
        return 1;
    if (!enable_ppi_channel(ctx.power.wake_ppi_ch))
        return 1;

#ifdef DEBUG
#define _DEBUG_GPIOTE_CHANNEL(pin, endpoint) nrfx_gpiote_##endpoint##_address_get(pin)
#define DEBUG_GPIOTE_CHANNEL(pin) \
    LOG_DBG("%s\t0x%08x 0x%08x 0x%08x 0x%08x", #pin, _DEBUG_GPIOTE_CHANNEL(pin, out_task), _DEBUG_GPIOTE_CHANNEL(pin, set_task), _DEBUG_GPIOTE_CHANNEL(pin, clr_task), _DEBUG_GPIOTE_CHANNEL(pin, in_event));
    LOG_DBG("channel\t\tout_task   set_task   clr_task   in_event");
    DEBUG_GPIOTE_CHANNEL(led1_pin);
    DEBUG_GPIOTE_CHANNEL(led3_pin);
    DEBUG_GPIOTE_CHANNEL(but4_pin);
#endif DEBUG

    nrfx_gpiote_out_task_trigger(led1_pin);
    nrfx_gpiote_out_task_trigger(led1_pin);
    nrfx_gpiote_out_task_trigger(led1_pin);
    nrfx_gpiote_out_task_trigger(led1_pin);
    uint64_t alt = k_uptime_get();
    k_msleep(10);
    alt = k_uptime_delta(&alt);
    nrfx_gpiote_out_task_trigger(led1_pin);
    nrfx_gpiote_out_task_trigger(led1_pin);
    nrfx_gpiote_out_task_trigger(led1_pin);
    nrfx_gpiote_out_task_trigger(led1_pin);
    LOG_DBG("known delay: %d", (uint32_t)alt);

    timestamp = k_uptime_delta(&timestamp);
    LOG_DBG("program success: %d", (uint32_t)timestamp);
    return 0;
}
