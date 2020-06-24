#include "esp_event.h"
#include "wifi.h"
#include "cbtimer.h"
#include "server.h"
#include "webcfg.h"
#include "sdkconfig.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "ping.h"

ESP_EVENT_DEFINE_BASE(WEBCFG_EVENT);

// PRIVATE VARIABLS
static webcfg_t* instance; //singleton
static char* TAG = "webcfg";

// PRIVATE METHOD DECLARATIONS 
static void on_ap_started(void* arg, esp_event_base_t base, int32_t id, void* data);
static void on_config_receive(void* arg, esp_event_base_t base, int32_t id, void* data);
static void on_sta_connected(void* arg, esp_event_base_t base, int32_t id, void* data);
static void on_sta_started(void* arg, esp_event_base_t base, int32_t id, void* data);
static void on_sta_connecting(void* arg, esp_event_base_t base, int32_t id, void* data);
static void on_sta_disconnected(void* arg, esp_event_base_t base, int32_t id, void* data);
static void on_sta_connect_timeout(void* arg);
static void on_ping_failure(void* arg, esp_event_base_t base, int32_t id, void* data);
static void on_ping_success(void* arg, esp_event_base_t base, int32_t id, void* data);


static void init(webcfg_t* this, webcfg_config_t config);
static void clean(webcfg_t* this);

// PRIVATE METHOD DEFINITIONS
static void on_sta_started(void* arg, esp_event_base_t base, int32_t id, void* data)
{
	ESP_LOGI(TAG, "event STA_STARTED");
	ESP_ERROR_CHECK(esp_wifi_connect());
}

static void on_sta_connecting(void* arg, esp_event_base_t base, int32_t id, void* data)
{
	ESP_LOGI(TAG, "connecting but no IP yet");
}

static void on_sta_disconnected(void* arg, esp_event_base_t base, int32_t id, void* data)
{
	wifi_event_sta_disconnected_t* event_data = (wifi_event_sta_disconnected_t*) data;	
	ESP_LOGI(TAG, "disconnected from %s, reason: %d", (char*)(event_data->ssid), event_data->reason);
	ESP_ERROR_CHECK(esp_wifi_connect());
}

static void on_ap_started(void* arg, esp_event_base_t base, int32_t id, void* data)
{
	ESP_LOGI(TAG, "event ap_started");
	webcfg_t* this = (webcfg_t*) arg;

	server_config_t server_config = {
		.event_loop = &(this->event_loop),
	};
	this->server = server_create(server_config);
	server_start(this->server);
}

static void on_config_receive(void* arg, esp_event_base_t base, int32_t id, void* data)
{
	ESP_LOGI(TAG, "event config_receive");
	webcfg_t* this = (webcfg_t*) arg;
	webcfg_wifi_creds_t* creds = (webcfg_wifi_creds_t*) data;

	if (creds != NULL) {
		wifi_sta_config_t config = {
			.channel=2,
		};
		memcpy(config.ssid, creds->ssid, sizeof(config.ssid));
		memcpy(config.password, creds->password, sizeof(config.password));
		config.scan_method = WIFI_FAST_SCAN;

		ESP_LOGI(TAG, "Received sta config ssid: %s, password: %s", config.ssid, config.password);

		server_clean(this->server);
		ESP_ERROR_CHECK(esp_wifi_stop());
		start_sta(this->sta, config);
	}
	this->sta_timeout = cbtimer_create(120, &on_sta_connect_timeout, this);
}

static void on_sta_connect_timeout(void* args)
{
	ESP_LOGI(TAG, "event sta_connect_timeout");
	webcfg_t* this = (webcfg_t*) args;
	// assume wifi connection failed
	// restart ap and webserver
	ESP_ERROR_CHECK(esp_wifi_stop());
	start_ap(this->ap, this->config.ap_config, this->config.hostname, this->config.instance_name);
}

static void on_sta_connected(void* arg, esp_event_base_t base, int32_t id, void* data)
{
	ESP_LOGI(TAG, "event sta_connectedj");
	webcfg_t* this = (webcfg_t*) arg;
	// stop sta_timeout
	cbtimer_clean(this->sta_timeout);
	ping_start(this->event_loop);
}

static void on_ping_success(void* arg, esp_event_base_t base, int32_t id, void* data)
{
	ESP_LOGI(TAG, "event ping_success");
	webcfg_t* this = (webcfg_t*) arg;
	// tear down webserver
	server_clean(this->server);

	// post configuration successful event
	esp_event_post_to(
		this->event_loop,
		WEBCFG_EVENT, WEBCFG_EVENT_COMPLETE,
		NULL, 0,
		100);
}

static void on_ping_failure(void* arg, esp_event_base_t base, int32_t id, void* data)
{
	ESP_LOGI(TAG, "event ping_failure");
	webcfg_t* this = (webcfg_t*) arg;
	ESP_ERROR_CHECK(esp_wifi_stop());
	start_ap(this->ap, this->config.ap_config, this->config.hostname, this->config.instance_name);
}

static void init(webcfg_t* this, webcfg_config_t config)
{
	wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
	
	this->config = config;
	this->ap = esp_netif_create_default_wifi_ap();
	this->sta = esp_netif_create_default_wifi_sta();
	this->server = NULL;
	this->sta_timeout = NULL;
	
	// setup event loop
	esp_event_loop_args_t loop_args = {
		.queue_size=      4,
		.task_name=       "webcfg_event",
		.task_priority=   uxTaskPriorityGet(NULL),
		.task_stack_size= 8192,
		.task_core_id=    tskNO_AFFINITY,
	};
	ESP_ERROR_CHECK(esp_event_loop_create(&loop_args, &(this->event_loop)));

	// bind to system event loop
	ESP_ERROR_CHECK(esp_event_handler_instance_register(
		WIFI_EVENT, WIFI_EVENT_AP_START,
		&on_ap_started,
		this, NULL));
	ESP_ERROR_CHECK(esp_event_handler_instance_register(
		IP_EVENT, IP_EVENT_STA_GOT_IP,
		&on_sta_connected,
		this, NULL));
	ESP_ERROR_CHECK(esp_event_handler_instance_register(
		WIFI_EVENT, WIFI_EVENT_STA_CONNECTED,
		&on_sta_connecting,
		this, NULL));
	ESP_ERROR_CHECK(esp_event_handler_instance_register(
		WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED,
		&on_sta_disconnected,
		this, NULL));
	ESP_ERROR_CHECK(esp_event_handler_instance_register(
		WIFI_EVENT, WIFI_EVENT_STA_START,
		&on_sta_started,
		this, NULL));
	ESP_ERROR_CHECK(esp_event_handler_instance_register_with(
		this->event_loop,
		WEBCFG_EVENT, WEBCFG_EVENT_CONFIG_RECEIVE,
		&on_config_receive,
		this, NULL));
	ESP_ERROR_CHECK(esp_event_handler_instance_register_with(
		this->event_loop,
		WEBCFG_EVENT, WEBCFG_EVENT_PING_SUCCESSFUL,
		&on_ping_success,
		this, NULL));
	ESP_ERROR_CHECK(esp_event_handler_instance_register_with(
		this->event_loop,
		WEBCFG_EVENT, WEBCFG_EVENT_PING_FAILURE,
		&on_ping_failure,
		this, NULL));
}

void webcfg_start(webcfg_t* this)
{
	start_ap(this->ap, this->config.ap_config, this->config.hostname, this->config.instance_name);
}

void webcfg_stop(webcfg_t* this)
{
	clean(this);
}

webcfg_t* webcfg_create(webcfg_config_t config)
{
	//is singleton
	if (instance != NULL) {
		return instance;
	}

	instance = (webcfg_t*) malloc(sizeof(webcfg_t));
	
	init(instance, config);	

	return instance;
}

static void clean(webcfg_t* this)
{
	free(this->event_loop);
	free(this->ap);
	free(this->sta);
	free(this->server);
	free(this);
}