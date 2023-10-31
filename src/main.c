#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>

static void cfg_changed(const struct bt_gatt_attr *attr, uint16_t value) {}
static ssize_t read_attr(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf, uint16_t len, uint16_t offset) { return 0; }

#define read_blvl read_attr
#define blvl_ccc_cfg_changed cfg_changed

static uint8_t battery_level = 100U;

BT_GATT_SERVICE_DEFINE(bas,
                       BT_GATT_PRIMARY_SERVICE(BT_UUID_BAS),
                       BT_GATT_CHARACTERISTIC(BT_UUID_BAS_BATTERY_LEVEL, BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_READ, read_blvl, NULL, &battery_level),
                       BT_GATT_CCC(blvl_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE));

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

BT_GATT_SERVICE_DEFINE(ess,
                       BT_GATT_PRIMARY_SERVICE(BT_UUID_ESS),

                       BT_GATT_CHARACTERISTIC(BT_UUID_TEMPERATURE, BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_READ, NULL, NULL, NULL),
                       BT_GATT_CUD("CPU", BT_GATT_PERM_READ),
                       BT_GATT_DESCRIPTOR(BT_UUID_ES_MEASUREMENT, BT_GATT_PERM_READ, NULL, NULL, NULL),
                       BT_GATT_DESCRIPTOR(BT_UUID_VALID_RANGE, BT_GATT_PERM_READ, NULL, NULL, NULL),
                       BT_GATT_DESCRIPTOR(BT_UUID_ES_TRIGGER_SETTING, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_AUTHEN, NULL, NULL, NULL),
                       BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

                       BT_GATT_CHARACTERISTIC(BT_UUID_TEMPERATURE, BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_READ, NULL, NULL, NULL),
                       BT_GATT_CUD("Ambient", BT_GATT_PERM_READ),
                       BT_GATT_DESCRIPTOR(BT_UUID_ES_MEASUREMENT, BT_GATT_PERM_READ, NULL, NULL, NULL),
                       BT_GATT_DESCRIPTOR(BT_UUID_VALID_RANGE, BT_GATT_PERM_READ, NULL, NULL, NULL),
                       BT_GATT_DESCRIPTOR(BT_UUID_ES_TRIGGER_SETTING, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_AUTHEN, NULL, NULL, NULL),
                       BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

                       BT_GATT_CHARACTERISTIC(BT_UUID_HUMIDITY, BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_READ, NULL, NULL, NULL),
                       BT_GATT_DESCRIPTOR(BT_UUID_VALID_RANGE, BT_GATT_PERM_READ, NULL, NULL, NULL),
                       BT_GATT_DESCRIPTOR(BT_UUID_ES_TRIGGER_SETTING, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_AUTHEN, NULL, NULL, NULL),
                       BT_GATT_DESCRIPTOR(BT_UUID_ES_TRIGGER_SETTING, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_AUTHEN, NULL, NULL, NULL),
                       BT_GATT_DESCRIPTOR(BT_UUID_ES_TRIGGER_SETTING, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_AUTHEN, NULL, NULL, NULL),
                       BT_GATT_DESCRIPTOR(BT_UUID_ES_CONFIGURATION, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_AUTHEN, NULL, NULL, NULL),
                       BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

                       BT_GATT_CHARACTERISTIC(BT_UUID_DESC_VALUE_CHANGED, BT_GATT_CHRC_INDICATE, BT_GATT_PERM_NONE, NULL, NULL, NULL),

);

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_LIMITED | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE, 0x00, 0x03),
    BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_ESS_VAL))};

int main(void)
{
        int err;

        err = bt_enable(NULL);
        if (err)
        {
                printk("Bluetooth init failed (err %d)\n", err);
                return 0;
        }

        err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
        if (err)
        {
                printk("Advertising failed to start (err %d)\n", err);
                return 0;
        }

        return 0;
}
