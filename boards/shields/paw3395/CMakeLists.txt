if(CONFIG_SHIELD_PAW3395)
  zephyr_library()
  zephyr_library_sources(neopixel_power_delay.c)
endif()
