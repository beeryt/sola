#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/device.h>
#include <zephyr/smf.h>
LOG_MODULE_REGISTER(app, 4);

#define EVENT_SENSOR_TIMEOUT BIT(1)
#define EVENT_SENSOR_CONFIG BIT(0)
#define EVENT_SENSOR_ALL (EVENT_SENSOR_TIMEOUT | EVENT_SENSOR_CONFIG)

typedef struct
{
    k_timeout_t period;
} sensor_config_t;

#define DEFAULT_SENSOR_CONFIG   \
    {                           \
        .period = K_SECONDS(5), \
    }

typedef struct
{
    struct k_timer timer;
    struct k_event events;
    const struct device *const device;
    sensor_config_t config;
} sensor_context_t;

void sensor_timeout(struct k_timer *timer)
{
    sensor_context_t *ctx = k_timer_user_data_get(timer);
    k_event_post(&ctx->events, EVENT_SENSOR_TIMEOUT);
}

void sensor_thread(void *, void *, void *)
{
    LOG_INF("initializing sensor thread");

    sensor_context_t ctx = {
        .device = DEVICE_DT_GET_ANY(sensirion_sht4x),
        .config = DEFAULT_SENSOR_CONFIG,
    };

    if (!device_is_ready(ctx.device))
    {
        LOG_ERR("device %s is not ready", ctx.device->name);
        return;
    }

    k_timer_init(&ctx.timer, sensor_timeout, NULL);
    k_timer_user_data_set(&ctx.timer, &ctx);
    k_event_init(&ctx.events);

    k_event_post(&ctx.events, EVENT_SENSOR_CONFIG);

    while (true)
    {
        uint32_t events = k_event_wait(&ctx.events, EVENT_SENSOR_ALL, false, K_FOREVER);

        if (events & EVENT_SENSOR_CONFIG)
        {
            k_event_clear(&ctx.events, EVENT_SENSOR_CONFIG);
            LOG_DBG("configuring sensor thread");
            // when we handle reconfiguration, ignore any past/upcoming timeout events
            k_timer_stop(&ctx.timer);
            k_event_clear(&ctx.events, EVENT_SENSOR_TIMEOUT);
            events &= ~EVENT_SENSOR_TIMEOUT;

            k_timer_start(&ctx.timer, ctx.config.period, ctx.config.period);
        }

        if (events & EVENT_SENSOR_TIMEOUT)
        {
            k_event_clear(&ctx.events, EVENT_SENSOR_TIMEOUT);
            LOG_DBG("reading sensor values");

            if (sensor_sample_fetch(ctx.device))
            {
                LOG_ERR("failed to fetch sample from sensor device %s", ctx.device->name);
                continue;
            }

            struct sensor_value temp, hum;
            sensor_channel_get(ctx.device, SENSOR_CHAN_AMBIENT_TEMP, &temp);
            sensor_channel_get(ctx.device, SENSOR_CHAN_HUMIDITY, &hum);

            LOG_DBG("%s: %.2f Temp. [C]", ctx.device->name, sensor_value_to_double(&temp));
            LOG_DBG("%s: %.2f RH. [%%]", ctx.device->name, sensor_value_to_double(&hum));
        }
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
