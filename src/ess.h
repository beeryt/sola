#include <stdint.h>

struct es_measurement_desc
{
    uint16_t flags;
    uint8_t sampling_func;
    uint8_t period[3];
    uint8_t update_interval[3];
    uint8_t application;
    uint8_t uncertainty;
} __packed;

union Value
{
    uint16_t humidity;
    int16_t temperature;
};

struct es_trigger_setting_desc
{
    uint8_t condition;
    union
    {
        uint8_t operand[3];
        union Value value;
    };
} __packed;

struct es_configuration_desc
{
    uint8_t trigger_logic;
} __packed;

struct gatt_valid_range_desc
{
    union Value lower;
    union Value upper;
} __packed;