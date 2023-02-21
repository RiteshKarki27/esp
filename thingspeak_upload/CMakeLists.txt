# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES )

set(COMPONENT_SRCS "thingspeak_main.c")
set(COMPONENT_ADD_INCLUDEDIRS "../ESP8266_RTOS_SDK/components")

register_component()
