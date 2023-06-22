

cmake_minimum_required(VERSION 3.20.0)

if (CONFIG_APP_RPC)

zephyr_library()
zephyr_include_directories(include)
zephyr_library_sources_ifdef(CONFIG_NRF_RPC src/app_rpc.c)

endif()
