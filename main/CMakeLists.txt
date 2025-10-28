idf_component_register(SRCS "main.c"
        INCLUDE_DIRS "."
        REQUIRES nvs access_point webserver sd_card tft_display
)
