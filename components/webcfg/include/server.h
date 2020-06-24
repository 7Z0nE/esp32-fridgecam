#ifndef INCLUDE_SERVER
#define INCLUDE_SERVER

#include "esp_http_server.h"
#include "esp_event.h"

// TYPES
typedef struct {
	esp_event_loop_handle_t* event_loop;
} server_config_t;

typedef struct {
	esp_event_loop_handle_t* event_loop;
	httpd_handle_t server;
	server_config_t config;
	httpd_config_t httpd_config;
} server_t;


// PUBLIC
server_t* server_create(server_config_t config);

void server_clean(server_t* this);
void server_start(server_t* this);
#endif
