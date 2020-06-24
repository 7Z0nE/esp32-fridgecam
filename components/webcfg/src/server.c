#include <string.h>
#include <stdio.h>
#include "sdkconfig.h"
#include "esp_system.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "webcfg.h"

#include "server.h"


// PRIVATE
static const char *TAG = "server";

static server_t* instance;
//////////

esp_err_t get_root_handle(httpd_req_t *req);

esp_err_t post_root_handle(httpd_req_t *req);

esp_err_t get_status_handle(httpd_req_t *req);

static void init();


esp_err_t get_root_handle(httpd_req_t *req)
{
	printf("Received a get on /auth\n");
	const char resp[] = "<form method=\"post\"><label for=\"essid\">ESSID</label><input type=\"text\" name=\"ssid\" required><label for=\"password\">Password</label><input type=\"password\" name=\"password\" required><button type=\"submit\">Submit</button></form>";

	ESP_ERROR_CHECK(httpd_resp_send(req, resp, strlen(resp)));
	return ESP_OK;
}

esp_err_t post_root_handle(httpd_req_t *req)
{
	server_t* this = (server_t*) req->user_ctx;
	
	ESP_LOGI(TAG, "received POST /root");

	esp_err_t err;
	char query[128];
	webcfg_wifi_creds_t* creds = malloc(sizeof(webcfg_wifi_creds_t));

	err = httpd_req_get_url_query_str(req, query, 128);
	if (err != ESP_OK) {
		return httpd_resp_send_err(req, 400, "could not parse query");
	}
	
	err = httpd_query_key_value(query, "ssid", creds->ssid, 32);
	if (err != ESP_OK) {
		return httpd_resp_send_err(req, 400, "query missing field 'ssid'");
	}
	
	err = httpd_query_key_value(query, "password", creds->password, 64);
	if (err != ESP_OK) {
		return httpd_resp_send_err(req, 400, "query missing field 'password'");
	}

	ESP_LOGI(TAG, "ssid: %s, password: %s", creds->ssid, creds->password);

	esp_event_post_to(
		*(this->event_loop),
		WEBCFG_EVENT, WEBCFG_EVENT_CONFIG_RECEIVE,
		creds, sizeof(webcfg_wifi_creds_t),
		1000000);

	ESP_LOGI(TAG, "config receive event emitted");

	return httpd_resp_send(req, "", 0);
}

esp_err_t get_status_handle(httpd_req_t *req)
{
	server_t* this = (server_t*) req->user_ctx;

	ESP_LOGI(TAG, "received GET /status");
	return ESP_OK;
}

void server_start(server_t* this) {
	if (httpd_start(&(this->server), &(this->httpd_config)) == ESP_OK) {
		httpd_uri_t get_root = {
			.uri      = "/",
			.method   = HTTP_GET,
			.handler  = get_root_handle,
			.user_ctx = this,
		};
		httpd_register_uri_handler(this->server, &get_root);

		httpd_uri_t post_root = {
			.uri      = "/",
			.method   = HTTP_POST,
			.handler  = post_root_handle,
			.user_ctx = this,
		};
		httpd_register_uri_handler(this->server, &post_root); 
	
		httpd_uri_t get_status = {
			.uri      = "/status",
			.method   = HTTP_GET,
			.handler  = get_status_handle,
			.user_ctx = this,
		};
		httpd_register_uri_handler(this->server, &get_status);
	}
	else {
		printf("Failed to start webserver\n");
	}
}

void init(server_t* this, server_config_t config)
{
	httpd_config_t defconfig = HTTPD_DEFAULT_CONFIG();
	this->httpd_config = defconfig;
	this->config = config;
	this->event_loop = config.event_loop;
}

server_t* server_create(server_config_t config)
{
	//singleton
	if (instance != NULL) {
		return instance;
	}

	instance = (server_t*) malloc(sizeof(server_t));

	instance->config = config;

	init(instance, config);

	return instance;
}

void server_clean(server_t* this)
{
	ESP_ERROR_CHECK(httpd_stop(this->server));
	free(this);
}
