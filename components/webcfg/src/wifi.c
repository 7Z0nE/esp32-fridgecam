#include <stdlib.h>
#include <string.h>
#include "esp_system.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "wifi.h"
#include "esp_log.h"
#include "mdns.h"
#include "sdkconfig.h"

// PRIVATE

static void setup_mdns();
static void static_ip(esp_netif_t* netif);
const char* TAG="wifi";

//////////


static void static_ip(esp_netif_t* netif)
{
	// stop DHCP server
	ESP_ERROR_CHECK(esp_netif_dhcps_stop(netif));
	ESP_LOGI(TAG, "static_ip: DHCP server stopped");
	
	// assign a static IP to the network interface
	// tcpip_adapter_ip_info_t info;
	esp_netif_ip_info_t info;
	memset(&info, 0, sizeof(info));
	IP4_ADDR(&info.ip, 192, 168, 1, 1);
	IP4_ADDR(&info.gw, 192, 168, 1, 1);
	IP4_ADDR(&info.netmask, 255, 255, 255, 0);
	ESP_ERROR_CHECK(esp_netif_set_ip_info(netif, &info));
	ESP_LOGI(TAG, "- TCP adapter configured with IP" IPSTR, IP2STR(&info.ip));
	
	// start the DHCP server   
	ESP_ERROR_CHECK(esp_netif_dhcps_start(netif));
	ESP_LOGI(TAG, "- DHCP server started");
}

static void setup_mdns(char* hostname, char* instance_name)
{
	ESP_ERROR_CHECK(mdns_init());
	ESP_ERROR_CHECK(mdns_hostname_set(hostname));
	ESP_ERROR_CHECK(mdns_instance_name_set(instance_name));
}

void start_ap(esp_netif_t* netif, wifi_ap_config_t ap_config, char* hostname, char* instance_name)
{
	esp_netif_set_hostname(netif, hostname);

	static_ip(netif);
	setup_mdns(hostname, instance_name);

	wifi_config_t config = {.ap=ap_config};	
	
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &config));
	ESP_LOGI(TAG, "configured ap ssid: %s, pw: %s", config.ap.ssid, config.ap.password);

	// start the wifi interface
	ESP_ERROR_CHECK(esp_wifi_start());
	ESP_LOGI(TAG, "- Wifi adapter starting...");
}

void start_sta(esp_netif_t* netif, wifi_sta_config_t sta_config)
{
	wifi_config_t config = {.sta=sta_config};	
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &config));
	ESP_LOGI(TAG, "configured station ssid: %s, pw: %s", config.sta.ssid, config.sta.password);

	// start the wifi interface
	ESP_ERROR_CHECK(esp_wifi_start());
	ESP_LOGI(TAG, "- Wifi adapter starting...");
}