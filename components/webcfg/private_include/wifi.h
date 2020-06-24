#ifndef INCLUDE_WIFI
#define INCLUDE_WIFI

#include "esp_netif.h"

void start_sta(esp_netif_t* netif, wifi_sta_config_t config);
void start_ap(esp_netif_t* netif, wifi_ap_config_t config, char* hostname, char* instance_name);

#endif INCLUDE_WIFI