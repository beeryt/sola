#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/smf.h>
#include <zephyr/logging/log.h>

#include "ess.h"

#define DEFINE_CB(name, ...) \
    void name(void *, ...) { LOG_INF(#name); }

LOG_MODULE_REGISTER(main);

#define DEFINE_GATT_READ(name, data)                                                                                     \
    static ssize_t name(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf, uint16_t len, uint16_t offset) \
    {                                                                                                                    \
        LOG_INF(#name);                                                                                                  \
        return bt_gatt_attr_read(conn, attr, buf, len, offset, &data, sizeof(data));                                     \
    }

// #undef DEFINE_GATT_READ
// #define DEFINE_GATT_READ DEFINE_CB

uint8_t battery_level = 100U;
int16_t tmp1_value = 4365;
int16_t tmp2_value = 1820;
uint16_t hum_value = 4665;

DEFINE_GATT_READ(bas_lvl_read, battery_level);
DEFINE_CB(bas_ccc_changed);
DEFINE_GATT_READ(ess_tmp1_read, tmp1_value);
DEFINE_CB(ess_tmp1_meas_read);
DEFINE_CB(ess_tmp1_range_read);
DEFINE_CB(ess_tmp1_trig_read);
DEFINE_CB(ess_tmp1_trig_write);
DEFINE_CB(ess_tmp1_ccc_changed);
DEFINE_GATT_READ(ess_tmp2_read, tmp2_value);
DEFINE_CB(ess_tmp2_meas_read);
DEFINE_CB(ess_tmp2_range_read);
DEFINE_CB(ess_tmp2_trig_read);
DEFINE_CB(ess_tmp2_trig_write);
DEFINE_CB(ess_tmp2_ccc_changed);
DEFINE_GATT_READ(ess_hum_read, hum_value);
DEFINE_CB(ess_hum_meas_read);
DEFINE_CB(ess_hum_range_read);
DEFINE_CB(ess_hum_trig1_read);
DEFINE_CB(ess_hum_trig1_write);
DEFINE_CB(ess_hum_trig2_read);
DEFINE_CB(ess_hum_trig2_write);
DEFINE_CB(ess_hum_trig3_read);
DEFINE_CB(ess_hum_trig3_write);
DEFINE_CB(ess_hum_cfg_read);
DEFINE_CB(ess_hum_cfg_write);
DEFINE_CB(ess_hum_ccc_changed);

BT_GATT_SERVICE_DEFINE(bas,
                       BT_GATT_PRIMARY_SERVICE(BT_UUID_BAS),
                       BT_GATT_CHARACTERISTIC(BT_UUID_BAS_BATTERY_LEVEL, BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_READ, bas_lvl_read, NULL, NULL),
                       BT_GATT_CCC(bas_ccc_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE));

BT_GATT_SERVICE_DEFINE(ess,
                       BT_GATT_PRIMARY_SERVICE(BT_UUID_ESS),

                       BT_GATT_CHARACTERISTIC(BT_UUID_TEMPERATURE, BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_READ | BT_GATT_PERM_READ_LESC, ess_tmp1_read, NULL, NULL),
                       BT_GATT_CUD("CPU", BT_GATT_PERM_READ),
                       BT_GATT_DESCRIPTOR(BT_UUID_ES_MEASUREMENT, BT_GATT_PERM_READ, ess_tmp1_meas_read, NULL, NULL),
                       BT_GATT_DESCRIPTOR(BT_UUID_VALID_RANGE, BT_GATT_PERM_READ, ess_tmp1_range_read, NULL, NULL),
                       BT_GATT_DESCRIPTOR(BT_UUID_ES_TRIGGER_SETTING, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_AUTHEN, ess_tmp1_trig_read, ess_tmp1_trig_write, NULL),
                       BT_GATT_CCC(ess_tmp1_ccc_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

                       BT_GATT_CHARACTERISTIC(BT_UUID_TEMPERATURE, BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_READ, ess_tmp2_read, NULL, NULL),
                       BT_GATT_CUD("Ambient", BT_GATT_PERM_READ),
                       BT_GATT_DESCRIPTOR(BT_UUID_ES_MEASUREMENT, BT_GATT_PERM_READ, ess_tmp2_meas_read, NULL, NULL),
                       BT_GATT_DESCRIPTOR(BT_UUID_VALID_RANGE, BT_GATT_PERM_READ, ess_tmp2_range_read, NULL, NULL),
                       BT_GATT_DESCRIPTOR(BT_UUID_ES_TRIGGER_SETTING, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_AUTHEN, ess_tmp2_trig_read, ess_tmp2_trig_write, NULL),
                       BT_GATT_CCC(ess_tmp2_ccc_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

                       BT_GATT_CHARACTERISTIC(BT_UUID_HUMIDITY, BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_READ, ess_hum_read, NULL, NULL),
                       BT_GATT_DESCRIPTOR(BT_UUID_VALID_RANGE, BT_GATT_PERM_READ, ess_hum_range_read, NULL, NULL),
                       BT_GATT_DESCRIPTOR(BT_UUID_ES_TRIGGER_SETTING, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_AUTHEN, ess_hum_trig1_read, ess_hum_trig1_write, NULL),
                       BT_GATT_DESCRIPTOR(BT_UUID_ES_TRIGGER_SETTING, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_AUTHEN, ess_hum_trig2_read, ess_hum_trig2_write, NULL),
                       BT_GATT_DESCRIPTOR(BT_UUID_ES_TRIGGER_SETTING, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_AUTHEN, ess_hum_trig3_read, ess_hum_trig3_write, NULL),
                       BT_GATT_DESCRIPTOR(BT_UUID_ES_CONFIGURATION, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_AUTHEN, ess_hum_cfg_read, ess_hum_cfg_write, NULL),
                       BT_GATT_CCC(ess_hum_ccc_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

                       BT_GATT_CHARACTERISTIC(BT_UUID_DESC_VALUE_CHANGED, BT_GATT_CHRC_INDICATE, BT_GATT_PERM_NONE, NULL, NULL, NULL),

);

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_LIMITED | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE, 0x00, 0x03),
    BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_ESS_VAL))};

DEFINE_CB(cb_connected);
DEFINE_CB(cb_idenity_resolved);

void cb_disconnected(struct bt_conn *conn, uint8_t reason)
{
    LOG_INF("cb_disconnected (reason:0x%x)", reason);
}

#define CASE(value, desc) \
    case value:           \
        return #value ": " desc
static const char *get_security_err(enum bt_security_err err)
{
    switch (err)
    {
        CASE(BT_SECURITY_ERR_SUCCESS, "Security procedure successful");
        CASE(BT_SECURITY_ERR_AUTH_FAIL, "Authentication failed");
        CASE(BT_SECURITY_ERR_PIN_OR_KEY_MISSING, "PIN or encryption key is missing");
        CASE(BT_SECURITY_ERR_OOB_NOT_AVAILABLE, "OOB data is not available");
        CASE(BT_SECURITY_ERR_AUTH_REQUIREMENT, "The requested security level could not be reached");
        CASE(BT_SECURITY_ERR_PAIR_NOT_SUPPORTED, "Pairing is not supported");
        CASE(BT_SECURITY_ERR_PAIR_NOT_ALLOWED, "Pairing is not allowed");
        CASE(BT_SECURITY_ERR_INVALID_PARAM, "Invalid parameters");
        CASE(BT_SECURITY_ERR_KEY_REJECTED, "Distributed Key Rejected");
        CASE(BT_SECURITY_ERR_UNSPECIFIED, "Pairing failed but the exact reason could not be specified");
    default:
        return "Invalid security error";
    }
}

static const char *get_oob_data_flag(uint8_t oob_flag)
{
    switch (oob_flag)
    {
        CASE(0x00, "OOB Authentication data not present");
        CASE(0x01, "OOB Authentication data from remote device present");
    default:
        return "Reserved for future use";
    }
}

const char *get_bonding_flags(uint8_t bonding_flags)
{
    switch (bonding_flags)
    {
        CASE(0b00, "No Bonding");
        CASE(0b01, "Bonding");
    default:
        return "Reserved for future use";
    }
}

const char *get_mitm(uint8_t mitm)
{
    return (mitm) ? "device is requesting MITM protection" : "";
}

const char *get_sc(uint8_t sc)
{
    return (sc) ? "LE Secure Connections is supported by the device" : "";
}

void parse_auth_req(uint8_t auth_req)
{
    uint8_t bonding_flags = auth_req & 0x03;
    uint8_t mitm = (auth_req >> 2) & 0x01;
    uint8_t SC = (auth_req >> 3) & 0x01;
    uint8_t keypress = (auth_req >> 4) & 0x01;
    uint8_t CT2 = (auth_req >> 5) & 0x01;
    uint8_t RFU = (auth_req >> 6) & 0x03;
    LOG_DBG("auth_req: %x", auth_req);
    LOG_DBG("\tbonding_flags: %s", get_bonding_flags(bonding_flags));
    LOG_DBG("\tmitm: %d", mitm);
    LOG_DBG("\tSC: %d", SC);
    LOG_DBG("\tkeypress: %d", keypress);
    LOG_DBG("\tCT2: %d", CT2);
    LOG_DBG("\tRFU: %d", RFU);
}

static const char *get_io_capability(uint8_t io_capability)
{
    switch (io_capability)
    {
        CASE(0x00, "DisplayOnly");
        CASE(0x01, "DisplayYesNo");
        CASE(0x02, "KeyboardOnly");
        CASE(0x03, "NoInputNoOutput");
        CASE(0x04, "KeyboardDisplay");
    default:
        return "Reserved for future use";
    }
}

void cb_security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
    LOG_INF("security_changed (level:%d): %s", level, get_security_err(err));
}

struct bt_conn_cb bt_conn_callbacks = {
    .connected = cb_connected,
    .disconnected = cb_disconnected,
    .security_changed = cb_security_changed,
};

void cb_pairing_complete(struct bt_conn *conn, bool bonded)
{
    LOG_INF("cb_pairing_complete (bonded: %d)", bonded);
}

DEFINE_CB(cb_pairing_failed);
DEFINE_CB(cb_bond_deleted);

struct bt_conn_auth_info_cb bt_auth_info_callbacks = {
    .pairing_complete = cb_pairing_complete,
    .pairing_failed = cb_pairing_failed,
    .bond_deleted = cb_bond_deleted,
};

void cb_oob_data_request(struct bt_conn *conn, struct bt_conn_oob_info *info)
{
    LOG_INF("cb_oob_data_request");
    switch (info->type)
    {
    case BT_CONN_OOB_LE_LEGACY:
    {
        LOG_WRN("Legacy OOB Request");
        break;
    }
    case BT_CONN_OOB_LE_SC:
    {
        LOG_INF("LESC OOB Request");
        LOG_ERR("Not implemented");
        break;
    }
    default:
    {
        LOG_ERR("Unknown OOB Request type: %d", info->type);
        break;
    }
    }
}

void cb_cancel_request(struct bt_conn *conn)
{
    LOG_INF("cb_cancel_request");
}

enum bt_security_err cb_pairing_accept(struct bt_conn *conn, const struct bt_conn_pairing_feat *const feat)
{
    LOG_INF("cb_pairing_accept");
    LOG_DBG("io_cap: %s", get_io_capability(feat->io_capability));
    LOG_DBG("oob_flag: %s", get_oob_data_flag(feat->oob_data_flag));
    parse_auth_req(feat->auth_req);

    return BT_SECURITY_ERR_SUCCESS;
}

struct bt_conn_auth_cb bt_auth_callbacks = {
    .oob_data_request = cb_oob_data_request,
    .cancel = cb_cancel_request,
    .pairing_accept = cb_pairing_accept,
};

int main(void)
{
    k_sleep(K_SECONDS(2));
    int err;

    bt_conn_cb_register(&bt_conn_callbacks);
    bt_conn_auth_cb_register(&bt_auth_callbacks);
    bt_conn_auth_info_cb_register(&bt_auth_info_callbacks);

    LOG_INF("Enabling Bluetooth");
    err = bt_enable(NULL);
    if (err)
    {
        LOG_ERR("Bluetooth init failed (err %d)", err);
        return 0;
    }

    LOG_INF("Starting advertising");
    err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
    if (err)
    {
        LOG_ERR("Advertising failed to start (err %d)", err);
        return 0;
    }

    return 0;
}
