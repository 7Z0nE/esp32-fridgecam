idf_component_register(SRCS "src/server.c"
							"src/wifi.c"
							"src/webcfg.c"
							"src/ping.c"
							"src/cbtimer.c"
					   INCLUDE_DIRS "include"
					   PRIV_INCLUDE_DIRS "private_include"
					   REQUIRES esp_http_server esp_netif esp_event
					   PRIV_REQUIRES freertos mdns nvs_flash)