#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ESP_OK
#define ESP_OK 0
#endif

#ifndef ESP_FAIL
#define ESP_FAIL -1
#endif

typedef int esp_err_t;

/* WiFi interface */
typedef enum {
    WIFI_IF_STA = 0,
    WIFI_IF_AP  = 1
} wifi_interface_t;

/* WiFi mode */
typedef enum {
    WIFI_MODE_NULL = 0,
    WIFI_MODE_STA,
    WIFI_MODE_AP,
    WIFI_MODE_APSTA
} wifi_mode_t;

/* WiFi power save */
typedef enum {
    WIFI_PS_NONE = 0,
    WIFI_PS_MIN_MODEM,
    WIFI_PS_MAX_MODEM
} wifi_ps_type_t;

/* WiFi bandwidth */
typedef enum {
    WIFI_BW_HT20 = 1,
    WIFI_BW_HT40 = 2
} wifi_bandwidth_t;

/* Auth modes */
typedef enum {
    WIFI_AUTH_OPEN = 0,
    WIFI_AUTH_WEP,
    WIFI_AUTH_WPA_PSK,
    WIFI_AUTH_WPA2_PSK,
    WIFI_AUTH_WPA_WPA2_PSK,
    WIFI_AUTH_WPA2_ENTERPRISE,
    WIFI_AUTH_WPA3_PSK,
    WIFI_AUTH_WPA2_WPA3_PSK
} wifi_auth_mode_t;

/* Country policy */
typedef enum {
    WIFI_COUNTRY_POLICY_AUTO = 0,
    WIFI_COUNTRY_POLICY_MANUAL
} wifi_country_policy_t;

typedef struct {
    char cc[3];
    uint8_t schan;
    uint8_t nchan;
    int8_t max_tx_power;
    wifi_country_policy_t policy;
} wifi_country_t;

/* Minimal config structs */
typedef struct {
    uint8_t ssid[32];
    uint8_t password[64];
    uint8_t channel;
    wifi_auth_mode_t authmode;
    uint8_t ssid_hidden;
    uint8_t max_connection;
    uint16_t beacon_interval;
} wifi_ap_config_t;

typedef struct {
    uint8_t ssid[32];
    uint8_t password[64];
} wifi_sta_config_t;

typedef union {
    wifi_ap_config_t ap;
    wifi_sta_config_t sta;
} wifi_config_t;

/* Stub functions */
static inline esp_err_t esp_wifi_init(void* cfg) {
    (void)cfg;
    return ESP_OK;
}

static inline esp_err_t esp_wifi_deinit(void) {
    return ESP_OK;
}

static inline esp_err_t esp_wifi_start(void) {
    return ESP_OK;
}

static inline esp_err_t esp_wifi_stop(void) {
    return ESP_OK;
}

static inline esp_err_t esp_wifi_restore(void) {
    return ESP_OK;
}

static inline esp_err_t esp_wifi_disconnect(void) {
    return ESP_OK;
}

static inline esp_err_t esp_wifi_set_mode(wifi_mode_t mode) {
    (void)mode;
    return ESP_OK;
}

static inline esp_err_t esp_wifi_get_mode(wifi_mode_t* mode) {
    if (mode) *mode = WIFI_MODE_STA;
    return ESP_OK;
}

static inline esp_err_t esp_wifi_set_ps(wifi_ps_type_t type) {
    (void)type;
    return ESP_OK;
}

static inline esp_err_t esp_wifi_get_ps(wifi_ps_type_t* type) {
    if (type) *type = WIFI_PS_NONE;
    return ESP_OK;
}

static inline esp_err_t esp_wifi_set_channel(uint8_t primary, uint8_t second) {
    (void)primary;
    (void)second;
    return ESP_OK;
}

static inline esp_err_t esp_wifi_get_channel(uint8_t* primary, uint8_t* second) {
    if (primary) *primary = 1;
    if (second) *second = 0;
    return ESP_OK;
}

static inline esp_err_t esp_wifi_set_max_tx_power(int8_t power) {
    (void)power;
    return ESP_OK;
}

static inline esp_err_t esp_wifi_get_max_tx_power(int8_t* power) {
    if (power) *power = 78;
    return ESP_OK;
}

static inline esp_err_t esp_wifi_set_bandwidth(wifi_interface_t ifx, wifi_bandwidth_t bw) {
    (void)ifx;
    (void)bw;
    return ESP_OK;
}

static inline esp_err_t esp_wifi_get_bandwidth(wifi_interface_t ifx, wifi_bandwidth_t* bw) {
    (void)ifx;
    if (bw) *bw = WIFI_BW_HT20;
    return ESP_OK;
}

static inline esp_err_t esp_wifi_get_mac(wifi_interface_t ifx, uint8_t mac[6]) {
    (void)ifx;

    if (mac) {
        mac[0] = 0x24;
        mac[1] = 0x6F;
        mac[2] = 0x28;
        mac[3] = 0x12;
        mac[4] = 0x34;
        mac[5] = 0x56;
    }

    return ESP_OK;
}

static inline esp_err_t esp_wifi_set_mac(wifi_interface_t ifx, const uint8_t mac[6]) {
    (void)ifx;
    (void)mac;
    return ESP_OK;
}

static inline esp_err_t esp_wifi_set_config(wifi_interface_t ifx, wifi_config_t* conf) {
    (void)ifx;
    (void)conf;
    return ESP_OK;
}

static inline esp_err_t esp_wifi_get_config(wifi_interface_t ifx, wifi_config_t* conf) {
    (void)ifx;
    (void)conf;
    return ESP_OK;
}

static inline esp_err_t esp_wifi_set_country(const wifi_country_t* country) {
    (void)country;
    return ESP_OK;
}

static inline esp_err_t esp_wifi_get_country(wifi_country_t* country) {
    if (country) {
        country->cc[0] = 'C';
        country->cc[1] = 'Z';
        country->cc[2] = 0;
        country->schan = 1;
        country->nchan = 13;
        country->max_tx_power = 78;
        country->policy = WIFI_COUNTRY_POLICY_AUTO;
    }

    return ESP_OK;
}

static inline esp_err_t esp_wifi_set_promiscuous(bool en) {
    (void)en;
    return ESP_OK;
}

static inline esp_err_t esp_wifi_get_promiscuous(bool* en) {
    if (en) *en = false;
    return ESP_OK;
}

#ifdef __cplusplus
}
#endif