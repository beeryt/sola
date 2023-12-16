#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/device.h>
#include <zephyr/smf.h>
LOG_MODULE_REGISTER(app, 4);

typedef struct
{
} sensor_config_t;

void sensor_thread(void *, void *, void *)
{
    LOG_INF("initializing sensor thread");

    struct k_timer timer;
    struct device *device = DEVICE_DT_GET_ANY(sensirion_sht4x);

    if (!device_is_ready(device))
    {
        LOG_ERR("Device %s is not ready", device->name);
        return;
    }

    k_timer_init(&timer, NULL, NULL);
    k_timer_start(&timer, K_SECONDS(5), K_SECONDS(5));

    while (true)
    {
        k_timer_status_sync(&timer);
        if (sensor_sample_fetch(device))
        {
            LOG_ERR("Failed to fetch sample from sensor device %s", device->name);
            return;
        }

        struct sensor_value temp, hum;
        sensor_channel_get(device, SENSOR_CHAN_AMBIENT_TEMP, &temp);
        sensor_channel_get(device, SENSOR_CHAN_HUMIDITY, &hum);

        LOG_INF("%s: %.2f Temp. [C]", device->name, sensor_value_to_double(&temp));
        LOG_INF("%s: %.2f RH. [%%]", device->name, sensor_value_to_double(&hum));
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
