cmake_minimum_required(VERSION 3.19)
project(DroneShield_Core LANGUAGES CXX)

# 1. 【关键修正】必须包含 Network 模块，否则 QTcpSocket/QUdpSocket 报错
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Network)

qt_standard_project_setup()

# 2. 【模拟模式开关】
# 没有硬件时保留此行；连接真实硬件时，请在行首加 # 注释掉
add_compile_definitions(SIMULATION_MODE)

qt_add_executable(DroneShield_Core
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui

    # --- 后端常量 ---
    # 请确认你的实际文件名是 Consts.h 还是 consts.h？这里暂时写 Consts.h
    src/Backend/Consts.h

    # --- 核心管理类 ---
    # 请确认实际文件名是 DeviceManager.h 还是 devicemanager.h？
    # 根据你之前的习惯，这里改回小写，如果不对应请手动修改！
    src/Backend/devicemanager.h          src/Backend/devicemanager.cpp

    # --- HAL 层 (硬件通信) ---
    # 全部改为小写文件名，匹配你的描述
    src/Backend/HAL/tcpclient.h          src/Backend/HAL/tcpclient.cpp
    src/Backend/HAL/udpsender.h          src/Backend/HAL/udpsender.cpp

    # --- Drivers 层 (协议驱动) ---
    src/Backend/Drivers/relaydriver.h    src/Backend/Drivers/relaydriver.cpp
    src/Backend/Drivers/spectrumdriver.h src/Backend/Drivers/spectrumdriver.cpp
    src/Backend/Drivers/jammerdriver.h   src/Backend/Drivers/jammerdriver.cpp
    src/Backend/Drivers/spoofdriver.h    src/Backend/Drivers/spoofdriver.cpp

    # --- Utils 工具 ---
    src/Utils/crcutils.h                 src/Utils/crcutils.cpp
)

target_link_libraries(DroneShield_Core
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Network  # 3. 【关键修正】链接网络库
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
