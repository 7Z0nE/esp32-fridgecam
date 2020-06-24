#ifndef INCLUDE_WEBCONFIG
#define INCLUDE_WEBCONFIG
#include "esp_http_server.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "server.h"
#include "cbtimer.h"


// EVENT
enum {
	WEBCFG_EVENT_COMPLETE,
	WEBCFG_EVENT_CONFIG_RECEIVE,
	WEBCFG_EVENT_PING_SUCCESSFUL,
	WEBCFG_EVENT_PING_FAILURE,
};

ESP_EVENT_DECLARE_BASE(WEBCFG_EVENT);

// TYPES
typedef struct {
	wifi_ap_config_t ap_config;
	char hostname[32];
	char instance_name[128];
} webcfg_config_t;

typedef struct {
	esp_event_loop_handle_t event_loop;
	esp_netif_t* ap;
	esp_netif_t* sta;
	server_t* server;
	cbtimer_t* sta_timeout;
	webcfg_config_t config;
} webcfg_t;

typedef struct {
	char ssid[32];
	char password[64];
} webcfg_wifi_creds_t;

// PUBLIC METHODS
webcfg_t* webcfg_create(webcfg_config_t config);
void webcfg_start(webcfg_t* this);
void webcfg_stop(webcfg_t* this);

#endif
