#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/device.h>
LOG_MODULE_REGISTER(app, 4);

int main(void)
{
    uint64_t timestamp = k_uptime_get();
    LOG_DBG("boot: %d", (uint32_t)timestamp);

    const struct device *const sht4x = DEVICE_DT_GET_ANY(sensirion_sht4x);
    if (!device_is_ready(sht4x))
    {
        LOG_ERR("Device %s is not ready.", sht4x->name);
        return 1;
    }

    while (true)
    {
        if (sensor_sample_fetch(sht4x))
        {
            LOG_ERR("Failed to fetch sample from SHT4X device");
            return 1;
        }

        struct sensor_value temp, hum;
        sensor_channel_get(sht4x, SENSOR_CHAN_AMBIENT_TEMP, &temp);
        sensor_channel_get(sht4x, SENSOR_CHAN_HUMIDITY, &hum);

        LOG_INF("SHT4X: %.2f Temp. [C]", sensor_value_to_double(&temp));
        LOG_INF("SHT4X: %.2f RH. [%%]", sensor_value_to_double(&hum));

        k_msleep(200);
    }

    return 0;
}
