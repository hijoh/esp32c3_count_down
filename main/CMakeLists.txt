file(GLOB_RECURSE SOURCES_C *.c components/*.c fonts/*.c screens/*.c  esp_now/*.c)
file(GLOB_RECURSE SOURCES_CPP *.cpp)

idf_component_register(SRCS ${SOURCES_C} ${SOURCES_CPP} 
                            "main.c"
                            "blufi_security.c"
                            "blufi_init.c"
                            "mqtt.c"
                            "bsp_gpio.c"
                             "hello_task.c"
                             "components/t_h/aht20.c"
                    INCLUDE_DIRS "." "components" "components/t_h/include" "fonts"  "screens" "esp_gui" )

target_compile_definitions(${COMPONENT_LIB} PUBLIC "-DLV_HOR_RES_MAX=320")
target_compile_definitions(${COMPONENT_LIB} PUBLIC "-DLV_VER_RES_MAX=240")