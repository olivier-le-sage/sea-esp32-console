idf_component_register(SRCS "cmd_scan.c"
                    INCLUDE_DIRS .
                    REQUIRES console nvs_flash esp_wifi)
