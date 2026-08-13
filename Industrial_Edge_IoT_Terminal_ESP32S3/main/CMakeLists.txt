# 方法二：使用 GLOB 自动收集（新增文件后需重新配置，或执行 idf.py fullclean）
file(GLOB HAL_SRCS     "../HAL/Src/*.c")
file(GLOB BSP_SRCS     "../BSP/Src/*.c")
file(GLOB DRIVER_SRCS  "../Driver/Src/*.c")
file(GLOB SERVICE_SRCS "../Service/Src/*.c")
file(GLOB APP_SRCS     "../APP/Src/*.c")
file(GLOB SYSTEM_SRCS  "../System/Src/*.c")

idf_component_register(
    SRCS "main.c" ${HAL_SRCS} ${BSP_SRCS} ${DRIVER_SRCS} ${SERVICE_SRCS} ${APP_SRCS} ${SYSTEM_SRCS}
    INCLUDE_DIRS "."
                  "../HAL/Inc"
                  "../BSP/Inc"
                  "../Driver/Inc"
                  "../Service/Inc"
                  "../APP/Inc"
                  "../System/Inc"
    PRIV_REQUIRES
                  nvs_flash
                  esp_wifi
                  esp_event
                  esp_netif
                  mqtt
                  esp_adc
                  driver
                  esp_timer
                  app_update
                  esp_system
                  esp_http_client
)