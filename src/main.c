#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/device.h>
#include <zephyr/smf.h>
LOG_MODULE_REGISTER(app, 4);

#define EVENT_SENSOR_TIMER BIT(1)

enum sensor_state
{
    SENSOR_INIT,
    SENSOR_IDLE,
    SENSOR_TRIGGER,
};

void sensor_init_entry(void *);
void sensor_idle_entry(void *);
void sensor_idle_run(void *);
void sensor_trigger_entry(void *);

const struct smf_state sensor_states[] = {
    [SENSOR_INIT] = SMF_CREATE_STATE(sensor_init_entry, NULL, NULL, NULL),
    [SENSOR_IDLE] = SMF_CREATE_STATE(sensor_idle_entry, sensor_idle_run, NULL, NULL),
    [SENSOR_TRIGGER] = SMF_CREATE_STATE(sensor_trigger_entry, NULL, NULL, NULL),
};

typedef struct
{
} sensor_config_t;

typedef struct
{
    struct smf_ctx ctx;
    const struct device *const device;
    struct k_timer timer;
    struct k_event events;
    sensor_config_t config;
} sensor_context_t;

#define DECLARE_SENSOR_CTX(name, o) sensor_context_t *name = (sensor_context_t *)o

void sensor_thread(void *, void *, void *)
{
    LOG_INF("initializing sensor thread");
    sensor_context_t ctx = {
        .device = DEVICE_DT_GET_ANY(sensirion_sht4x)};

    smf_set_initial(SMF_CTX(&ctx), &sensor_states[SENSOR_INIT]);

    while (true)
    {
        int32_t ret = smf_run_state(SMF_CTX(&ctx));
        if (ret)
        {
            break;
        }
        k_yield();
    }
}

K_THREAD_DEFINE(sensor_tid, 1024, sensor_thread, NULL, NULL, NULL, 50, 0, 0);

int main(void)
{
    uint64_t timestamp = k_uptime_get();
    LOG_DBG("boot: %d", (uint32_t)timestamp);
    while (true)
    {
        k_msleep(100000);
    }
}

void sensor_timer_expiry(struct k_timer *t)
{
    LOG_DBG("trigger");
    DECLARE_SENSOR_CTX(ctx, k_timer_user_data_get(t));
    k_event_post(&ctx->events, EVENT_SENSOR_TIMER);
}

// one-time initialization of sensor context
void sensor_init_entry(void *o)
{
    LOG_DBG("entry");
    DECLARE_SENSOR_CTX(ctx, o);

    if (!device_is_ready(ctx->device))
    {
        LOG_ERR("Device %s is not ready", ctx->device->name);
        LOG_DBG("We are stuck here");
        return;
    }

    k_event_init(&ctx->events);
    k_timer_init(&ctx->timer, sensor_timer_expiry, NULL);
    k_timer_user_data_set(&ctx->timer, ctx);

    // TODO: this should be set by config
    k_timer_start(&ctx->timer, K_SECONDS(5), K_SECONDS(5));

    smf_set_state(SMF_CTX(o), &sensor_states[SENSOR_IDLE]);
}

void sensor_idle_entry(void *)
{
    LOG_DBG("entry");
}

void sensor_idle_run(void *o)
{
    DECLARE_SENSOR_CTX(ctx, o);
    if (k_event_wait(&ctx->events, EVENT_SENSOR_TIMER, false, K_NO_WAIT))
    {
        k_event_clear(&ctx->events, EVENT_SENSOR_TIMER);
        smf_set_state(SMF_CTX(o), &sensor_states[SENSOR_TRIGGER]);
    }
}

void sensor_trigger_entry(void *o)
{
    LOG_DBG("entry");
    DECLARE_SENSOR_CTX(ctx, o);

    if (sensor_sample_fetch(ctx->device))
    {
        LOG_ERR("Failed to fetch sample from sensor device %s", ctx->device->name);
        return;
    }

    struct sensor_value temp, hum;
    sensor_channel_get(ctx->device, SENSOR_CHAN_AMBIENT_TEMP, &temp);
    sensor_channel_get(ctx->device, SENSOR_CHAN_HUMIDITY, &hum);

    LOG_INF("%s: %.2f Temp. [C]", ctx->device->name, sensor_value_to_double(&temp));
    LOG_INF("%s: %.2f RH. [%%]", ctx->device->name, sensor_value_to_double(&hum));

    smf_set_state(SMF_CTX(o), &sensor_states[SENSOR_IDLE]);
}