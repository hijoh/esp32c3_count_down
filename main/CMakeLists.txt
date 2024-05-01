file(GLOB_RECURSE SOURCES_C *.c components/*.c fonts/*.c screens/*.c  esp_now/*.c)
file(GLOB_RECURSE SOURCES_CPP *.cpp)

idf_component_register(SRCS ${SOURCES_C} ${SOURCES_CPP} 
                            "main.c"
                            "blufi_security.c"
                            "blufi_init.c"
                            "mqtt.c"
                            "./components/bsp_gpio/bsp_gpio.c"
                             "./components/hello_task/hello_task.c"
                            

                    INCLUDE_DIRS "." "components" "components/bsp_gpio/include" "components/hello_task/include")

                    set(COMPONENT_REQUIRES blufi)
                    set(COMPONENT_REQUIRES esp_timer)

