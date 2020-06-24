#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "lwip/inet.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "ping/ping_sock.h"
#include "webcfg.h"

static const char* TAG = "ping";

static void on_ping_success(esp_ping_handle_t hdl, void *args)
{
	esp_event_loop_handle_t* event_loop = (esp_event_loop_handle_t*) args;

    uint8_t ttl;
    uint16_t seqno;
    uint32_t elapsed_time, recv_len;
    ip_addr_t target_addr;
 
    esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno));
    esp_ping_get_profile(hdl, ESP_PING_PROF_TTL, &ttl, sizeof(ttl));
    esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr));
    esp_ping_get_profile(hdl, ESP_PING_PROF_SIZE, &recv_len, sizeof(recv_len));
    esp_ping_get_profile(hdl, ESP_PING_PROF_TIMEGAP, &elapsed_time, sizeof(elapsed_time));

    ESP_LOGI(TAG, "%d bytes from %s icmp_seq=%d ttl=%d time=%d ms",
           recv_len, inet_ntoa(target_addr.u_addr.ip4), seqno, ttl, elapsed_time);

	esp_event_post_to(
		*event_loop,
		WEBCFG_EVENT, WEBCFG_EVENT_PING_SUCCESSFUL,
		NULL, 0,
		100);
}

static void on_ping_timeout(esp_ping_handle_t hdl, void *args)
{
	esp_event_loop_handle_t* event_loop = (esp_event_loop_handle_t*) args;

    uint16_t seqno;
    ip_addr_t target_addr;

    esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno));
    esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr));
    
	ESP_LOGI(TAG, "From %s icmp_seq=%d timeout", inet_ntoa(target_addr.u_addr.ip4), seqno);
	
	esp_event_post_to(
		*event_loop,
		WEBCFG_EVENT, WEBCFG_EVENT_PING_FAILURE,
		NULL, 0,
		100);
}

static void on_ping_end(esp_ping_handle_t hdl, void *args)
{
	esp_event_loop_handle_t* event_loop = (esp_event_loop_handle_t*) args;
    
	uint32_t transmitted;
    uint32_t received;
    uint32_t total_time_ms;

    esp_ping_get_profile(hdl, ESP_PING_PROF_REQUEST, &transmitted, sizeof(transmitted));
    esp_ping_get_profile(hdl, ESP_PING_PROF_REPLY, &received, sizeof(received));
    esp_ping_get_profile(hdl, ESP_PING_PROF_DURATION, &total_time_ms, sizeof(total_time_ms));

    ESP_LOGI(TAG, "%d packets transmitted, %d received, time %dms", transmitted, received, total_time_ms);
}

void ping_start(esp_event_loop_handle_t* event_loop)
{
    /* convert URL to IP address */
    ip_addr_t target_addr;
    struct addrinfo hint;
    struct addrinfo *res = NULL;
    memset(&hint, 0, sizeof(hint));
    memset(&target_addr, 0, sizeof(target_addr));
    getaddrinfo("www.espressif.com", NULL, &hint, &res);
    struct in_addr addr4 = ((struct sockaddr_in *) (res->ai_addr))->sin_addr;
    inet_addr_to_ip4addr(ip_2_ip4(&target_addr), &addr4);
    freeaddrinfo(res);

    esp_ping_config_t ping_config = ESP_PING_DEFAULT_CONFIG();
    ping_config.target_addr = target_addr;          // target IP address
    ping_config.count = 5;

    /* set callback functions */
    esp_ping_callbacks_t cbs;
    cbs.on_ping_success = on_ping_success;
    cbs.on_ping_timeout = on_ping_timeout;
    cbs.on_ping_end = on_ping_end;
    cbs.cb_args = event_loop;  // arguments that will feed to all callback functions, can be NULL

    esp_ping_handle_t ping;
    esp_ping_new_session(&ping_config, &cbs, &ping);
    ESP_LOGI(TAG, "Ping started");
}