cmake_minimum_required(VERSION 3.19)
project(DroneShield_Core LANGUAGES CXX)

# 1. 核心模块依赖
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Network WebSockets)

qt_standard_project_setup()

# 2. 模拟模式宏定义 (连接真实硬件时请注释掉此行)
add_compile_definitions(SIMULATION_MODE)

qt_add_executable(DroneShield_Core
    WIN32 MACOSX_BUNDLE

    # --- 主程序入口 & 主窗口 ---
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    src/AppStyle.h            # 样式表头文件

    # --- 自定义 UI 控件 ---
    src/UI/radarview.h
    src/UI/radarview.cpp

    # --- 后端核心 ---
    src/Backend/Consts.h
    src/Backend/devicemanager.h
    src/Backend/devicemanager.cpp

    # --- HAL 层 (硬件通信) ---
    src/Backend/HAL/udpsender.h
    src/Backend/HAL/udpsender.cpp
    src/Backend/HAL/httpclient.h
    src/Backend/HAL/httpclient.cpp
    src/Backend/HAL/socketioclient.h
    src/Backend/HAL/socketioclient.cpp
    src/Backend/HAL/tcpclient.h   # 暂时未用到的可以注释
    src/Backend/HAL/tcpclient.cpp

    # --- Drivers 层 (业务驱动) ---
    src/Backend/Drivers/jammerdriver.h
    src/Backend/Drivers/jammerdriver.cpp
    src/Backend/Drivers/spoofdriver.h
    src/Backend/Drivers/spoofdriver.cpp
    src/Backend/Drivers/detectiondriver.h
    src/Backend/Drivers/detectiondriver.cpp
    src/Backend/Drivers/relaydriver.h      # 确保你目录下有这些文件，没有就注释掉
    src/Backend/Drivers/relaydriver.cpp
    src/Backend/Drivers/spectrumdriver.h   # 确保你目录下有这些文件，没有就注释掉
    src/Backend/Drivers/spectrumdriver.cpp

    # --- Utils 工具 ---
    src/Utils/crcutils.h
    src/Utils/crcutils.cpp
    src/Utils/configloader.h
    src/Utils/configloader.cpp
    src/UI/jammerconfdialog.h src/UI/jammerconfdialog.cpp
    src/UI/relaydialog.h src/UI/relaydialog.cpp
    src/UI/toggleswitch.h src/UI/toggleswitch.cpp
    src/Backend/DataStructs.h
)

target_link_libraries(DroneShield_Core
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Network
        Qt::WebSockets
)

include(GNUInstallDirs)

install(TARGETS DroneShield_Core
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET DroneShield_Core
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
