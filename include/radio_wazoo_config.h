#ifndef CONFIG_H
#define CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

// WiFi AP Configuration
#define WIFI_AP_SSID      "RadioWazooAP"
#define WIFI_AP_PASSWORD  "12345678"
#define WIFI_AP_CHANNEL   1
#define WIFI_AP_MAX_CONN  4

// IP address (OCTETS)
#define AP_IP_1             192
#define AP_IP_2             168
#define AP_IP_3             4
#define AP_IP_4             1

// Macro
#define SET_AP_IP(ip_info) do { \
IP4_ADDR(&(ip_info).ip,       AP_IP_1, AP_IP_2, AP_IP_3, AP_IP_4); \
IP4_ADDR(&(ip_info).gw,       AP_IP_1, AP_IP_2, AP_IP_3, AP_IP_4); \
IP4_ADDR(&(ip_info).netmask,  255,     255,     255,     0); \
} while(0)

// Filesystem Configuration
#define LITTLEFS_BASE_PATH "/littlefs"
#define LITTLEFS_PARTITION_LABEL "storage"


#ifdef __cplusplus
}
#endif

#endif // CONFIG_H
