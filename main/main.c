#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "mdns.h"
#include "esp_sleep.h"

#include "webcfg.h"
#include "foto.h"

// set AP CONFIG values
#ifdef CONFIG_AP_HIDE_SSID
	#define CONFIG_AP_SSID_HIDDEN 1
#else
	#define CONFIG_AP_SSID_HIDDEN 0
#endif	
#ifdef CONFIG_WIFI_AUTH_OPEN
	#define CONFIG_AP_AUTHMODE WIFI_AUTH_OPEN
#endif
#ifdef CONFIG_WIFI_AUTH_WEP
	#define CONFIG_AP_AUTHMODE WIFI_AUTH_WEP
#endif
#ifdef CONFIG_WIFI_AUTH_WPA_PSK
	#define CONFIG_AP_AUTHMODE WIFI_AUTH_WPA_PSK
#endif
#ifdef CONFIG_WIFI_AUTH_WPA2_PSK
	#define CONFIG_AP_AUTHMODE WIFI_AUTH_WPA2_PSK
#endif
#ifdef CONFIG_WIFI_AUTH_WPA_WPA2_PSK
	#define CONFIG_AP_AUTHMODE WIFI_AUTH_WPA_WPA2_PSK
#endif
#ifdef CONFIG_WIFI_AUTH_WPA2_ENTERPRISE
	#define CONFIG_AP_AUTHMODE WIFI_AUTH_WPA2_ENTERPRISE
#endif

static const char* TAG = "main";

void on_config_complete(void* args, esp_event_base_t base, int32_t id, void* data)
{
	printf("WUUUHUUUUUUUUU!!!!");
}

void start_station(esp_netif_t* netif, wifi_sta_config_t sta_config)
{
	wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
	
	wifi_config_t config = {.sta=sta_config};	
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &config));
	ESP_LOGI(TAG, "configured station ssid: %s, pw: %s", config.sta.ssid, config.sta.password);

	// start the wifi interface
	ESP_ERROR_CHECK(esp_wifi_start());
	ESP_LOGI(TAG, "- Wifi adapter starting...");
}

// Main application
void run_webcfg()
{
	esp_log_level_set("wifi", ESP_LOG_WARN);
	 webcfg_config_t config = {
		.hostname = CONFIG_DNS_HOSTNAME,
		.instance_name = CONFIG_DNS_INSTANCE,
		.ap_config = (wifi_ap_config_t){
			.ssid = CONFIG_AP_SSID,
			.password = CONFIG_AP_PASSWORD,
			.ssid_len = 0,
			.channel = CONFIG_AP_CHANNEL,
			.authmode = CONFIG_AP_AUTHMODE,
			.ssid_hidden = CONFIG_AP_SSID_HIDDEN,
			.max_connection = CONFIG_AP_MAX_CONNECTIONS,
			.beacon_interval = CONFIG_AP_BEACON_INTERVAL,
		},
	};

	printf("ESP32 SoftAP HTTP Wifi Configuration Server\n");
	
	// initialize NVS
	ESP_ERROR_CHECK(nvs_flash_init());
	printf("- NVS initialized\n");
	
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	webcfg_t* webcfg = webcfg_create(config);
	webcfg_start(webcfg);
	printf("Webconfig started...");

	esp_event_handler_instance_register_with(
		webcfg->event_loop,
		WEBCFG_EVENT, WEBCFG_EVENT_COMPLETE,
		&on_config_complete,
		NULL, NULL);
}

void on_sta_connected(void* arg, esp_event_base_t base, int32_t id, void* data)
{
	ESP_LOGI(TAG, "event sta_connected");
	start_server();
}

static void on_sta_started(void* arg, esp_event_base_t base, int32_t id, void* data)
{
	ESP_LOGI(TAG, "event STA_STARTED");
	ESP_ERROR_CHECK(esp_wifi_connect());
}

static void on_sta_disconnected(void* arg, esp_event_base_t base, int32_t id, void* data)
{
	ESP_LOGI(TAG, "wifi disconnected");
	ESP_ERROR_CHECK(esp_wifi_connect());
}

void log_event(void* arg, esp_event_base_t base, int32_t id, void* data) {
	char* msg = (char*) arg;
	ESP_LOGI(TAG, "%s", msg);
}

void run_foto()
{
	esp_log_level_set("camera", ESP_LOG_VERBOSE);
	// initialize NVS
	ESP_ERROR_CHECK(nvs_flash_init());
	printf("- NVS initialized\n");
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	wifi_sta_config_t wifi_config = {
		.ssid=CONFIG_STA_SSID,
		.password=CONFIG_STA_PASSWORD,
	};
	esp_netif_t* netif = esp_netif_create_default_wifi_sta();
	esp_netif_set_hostname(netif, "esp32");
	
	ESP_ERROR_CHECK(esp_event_handler_instance_register(
		IP_EVENT, IP_EVENT_STA_GOT_IP,
		&on_sta_connected, NULL, NULL));
	ESP_ERROR_CHECK(esp_event_handler_instance_register(
		WIFI_EVENT, WIFI_EVENT_STA_START,
		&on_sta_started, NULL, NULL));
	ESP_ERROR_CHECK(esp_event_handler_instance_register(
		WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED,
		&on_sta_disconnected, NULL, NULL));
	start_station(netif, wifi_config);
}

void run_sleep()
{
	switch(esp_sleep_get_wakeup_cause()) {
		case ESP_SLEEP_WAKEUP_TIMER: {
			ESP_LOGI(TAG, "cause: Timer");
			break;
		}
		case ESP_SLEEP_WAKEUP_EXT0: {
			ESP_LOGI(TAG, "cause: EXT0");
			break;
		}
		case ESP_SLEEP_WAKEUP_UNDEFINED: 
		default:
			ESP_LOGI(TAG, "not a deepsleep reset");
	}

	if(esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_UNDEFINED) {
		esp_sleep_enable_ext0_wakeup(GPIO_NUM_14, 1);
		uint64_t sleep_time = 5 * 1000000;
		ESP_LOGI(TAG, "Sleeping for %lld", sleep_time);
		//esp_sleep_enable_timer_wakeup(sleep_time);
		esp_deep_sleep_start();
	}
}

void app_main() {
	run_sleep();
}