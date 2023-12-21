#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include <zephyr/smf.h>
LOG_MODULE_REGISTER(app, 4);

#define SENSOR_EVENT_CONFIG BIT(0)
#define SENSOR_EVENT_READ BIT(1)
#define SENSOR_EVENT_MSK (SENSOR_EVENT_CONFIG | SENSOR_EVENT_READ)

typedef struct
{
    uint16_t read_period_ms;
} sensor_config_t;

typedef struct
{
    struct smf_ctx ctx;
    struct k_timer read_timer;
    struct k_event events;
    uint32_t triggered_events;
    const struct device *const device;

    sensor_config_t config;
} sensor_smf_ctx_t;

static const struct smf_state sensor_states[];

enum sensor_state
{
    SENSOR_STATE_INIT,
    SENSOR_STATE_IDLE,
    SENSOR_STATE_CONFIG,
    SENSOR_STATE_READ,
};

static void sensor_init_entry(void *o)
{
    sensor_smf_ctx_t *ctx = o;
    LOG_DBG("ctx: %p", ctx);

    if (!ctx->device)
    {
        LOG_ERR("sensirion_sht4x device doesn't exist");
        return;
    }

    if (!device_is_ready(ctx->device))
    {
        LOG_ERR("device %s is not ready", ctx->device->name);
        return;
    }

    smf_set_state(o, &sensor_states[SENSOR_STATE_IDLE]);
}

static void sensor_idle_entry(void *o) { LOG_DBG("ctx: %p", o); }

static void sensor_idle_run(void *o)
{
    sensor_smf_ctx_t *ctx = o;
    LOG_DBG("ctx: %p", ctx);

    if (ctx->triggered_events & SENSOR_EVENT_CONFIG)
    {
        smf_set_state(o, &sensor_states[SENSOR_STATE_CONFIG]);
    }

    if (ctx->triggered_events & SENSOR_EVENT_READ)
    {
        smf_set_state(o, &sensor_states[SENSOR_STATE_READ]);
    }
}

static void sensor_config_entry(void *o)
{
    sensor_smf_ctx_t *ctx = o;
    LOG_DBG("ctx: %p", ctx);

    k_timer_stop(&ctx->read_timer);
    k_event_clear(&ctx->events, SENSOR_EVENT_READ);

    if (ctx->config.read_period_ms == 0)
    {
        LOG_ERR("Sensor configuration must specify non-zero read period");
        k_panic();
    }
    LOG_DBG("Starting timer for %d ms", ctx->config.read_period_ms);

    k_timeout_t timeout = K_MSEC(ctx->config.read_period_ms);
    k_timer_start(&ctx->read_timer, timeout, timeout);
    smf_set_state(o, &sensor_states[SENSOR_STATE_IDLE]);
}

static void sensor_config_run(void *o) { LOG_DBG("ctx: %p", o); }

static void sensor_read_entry(void *o)
{
    sensor_smf_ctx_t *ctx = o;
    LOG_DBG("ctx: %p", ctx);

    if (sensor_sample_fetch(ctx->device))
    {
        LOG_ERR("failed to fetch sample from sensor device %s", ctx->device->name);
        return;
    }

    struct sensor_value temp, hum;
    sensor_channel_get(ctx->device, SENSOR_CHAN_AMBIENT_TEMP, &temp);
    sensor_channel_get(ctx->device, SENSOR_CHAN_HUMIDITY, &hum);

    LOG_DBG("%s: %.2f Temp. [C]", ctx->device->name, sensor_value_to_double(&temp));
    LOG_DBG("%s: %.2f RH. [%%]", ctx->device->name, sensor_value_to_double(&hum));

    smf_set_state(o, &sensor_states[SENSOR_STATE_IDLE]);
}

static void sensor_read_run(void *o) { LOG_DBG("ctx: %p", o); }

static const struct smf_state sensor_states[] = {
    [SENSOR_STATE_INIT] = SMF_CREATE_STATE(sensor_init_entry, NULL, NULL, NULL),
    [SENSOR_STATE_IDLE] = SMF_CREATE_STATE(sensor_idle_entry, sensor_idle_run, NULL, NULL),
    [SENSOR_STATE_CONFIG] = SMF_CREATE_STATE(sensor_config_entry, sensor_config_run, NULL, NULL),
    [SENSOR_STATE_READ] = SMF_CREATE_STATE(sensor_read_entry, sensor_read_run, NULL, NULL),
};

void sensor_read_timeout(struct k_timer *timer)
{
    sensor_smf_ctx_t *ctx = (sensor_smf_ctx_t *)k_timer_user_data_get(timer);
    LOG_DBG("ctx: %p", ctx);
    k_event_post(&ctx->events, SENSOR_EVENT_READ);
}

void sensor_config_received()
{
}

void sensor_thread(void *, void *, void *)
{
    LOG_DBG("Initializing sensor thread");

    sensor_smf_ctx_t ctx = {
        .device = DEVICE_DT_GET_ANY(sensirion_sht4x),
    };
    ctx.config.read_period_ms = 5000;

    LOG_DBG("ctx: %p", &ctx);

    k_event_init(&ctx.events);
    k_timer_init(&ctx.read_timer, sensor_read_timeout, NULL);
    k_timer_user_data_set(&ctx.read_timer, &ctx);

    k_event_post(&ctx.events, SENSOR_EVENT_CONFIG);

    smf_set_initial(SMF_CTX(&ctx), &sensor_states[SENSOR_STATE_INIT]);
    while (true)
    {
        ctx.triggered_events = k_event_wait(&ctx.events, SENSOR_EVENT_MSK, false, K_FOREVER);
        ctx.triggered_events |= k_event_clear(&ctx.events, ctx.triggered_events);

        int ret = smf_run_state(SMF_CTX(&ctx));
        if (ret)
        {
            LOG_ERR("SMF framework error: %d", ret);
            break;
        }
        k_yield();
    }
}

K_THREAD_DEFINE(sensor_tid, 1024, sensor_thread, NULL, NULL, NULL, 1, 0, 0);

int main(void)
{
    LOG_DBG("HEllo");
    k_sleep(K_FOREVER);
}
