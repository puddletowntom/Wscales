idf_component_get_property(LVGL_DIR lvgl COMPONENT_DIR)
file(GLOB LVGL_EXAMPLES ${LVGL_DIR}/examples/widgets/label)
file(GLOB_RECURSE SRC_UI ${CMAKE_SOURCE_DIR} "ui/*.c")
file(GLOB_RECURSE SRC_ADS1230 ${CMAKE_SOURCE_DIR} "ADS1230/*.c")

idf_component_register(
    SRC_DIRS ${DIRS} ${LVGL_EXAMPLES} "."
	SRCS "main.c" "screen.c" ${SRC_UI} ${SRC_ADS1230}
    INCLUDE_DIRS "." ${LVGL_EXAMPLES} ${DIRS} "ui" "ADS1230"
    REQUIRES esp_lcd driver lvgl esp_timer nvs_flash
)

target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")

