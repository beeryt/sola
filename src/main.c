#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/smf.h>

#include "ess.h"

BT_GATT_SERVICE_DEFINE(bas,
                       BT_GATT_PRIMARY_SERVICE(BT_UUID_BAS),
                       BT_GATT_CHARACTERISTIC(BT_UUID_BAS_BATTERY_LEVEL, BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_READ, NULL, NULL, NULL),
                       BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE));

BT_GATT_SERVICE_DEFINE(ess,
                       BT_GATT_PRIMARY_SERVICE(BT_UUID_ESS),

                       BT_GATT_CHARACTERISTIC(BT_UUID_TEMPERATURE, BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_AUTH, BT_GATT_PERM_READ, NULL, NULL, NULL),
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

void cb_connected(struct bt_conn *conn, uint8_t err) {}
void cb_disconnected(struct cb *conn, uint8_t reason) {}
void cb_identity_resolved(struct cb *conn, const bt_addr_le_t *rpa, const bt_addr_le_t *identity) {}
void cb_security_changed(struct cb *conn, bt_security_t level, enum bt_security_err err) {}

struct bt_conn_cb bt_conn_callbacks = {
    .connected = cb_connected,
    .disconnected = cb_disconnected,
    .security_changed = cb_security_changed,
};

void cb_pairing_complete(struct bt_conn *conn, bool bonded) {}
void cb_pairing_failed(struct bt_conn *conn, enum bt_security_err reason) {}
void cb_bond_deleted(uint8_t id, const bt_addr_le_t *peer) {}

struct bt_conn_auth_info_cb bt_auth_info_callbacks = {
    .pairing_complete = cb_pairing_complete,
    .pairing_failed = cb_pairing_failed,
    .bond_deleted = cb_bond_deleted,
};

struct bt_conn_auth_cb bt_auth_callbacks = {
    .oob_data_request = NULL,
};

int main(void)
{
        int err;

        bt_conn_cb_register(&bt_conn_callbacks);
        bt_conn_auth_cb_register(&bt_auth_callbacks);
        bt_conn_auth_info_cb_register(&bt_auth_info_callbacks);

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
