import socketio
import json
import datetime

# ================= 配置 =================
TARGET_IP = '192.178.1.12'
TARGET_PORT = 8090

# 初始化 (保持兼容性配置)
sio = socketio.Client(logger=False, engineio_logger=False)

@sio.event
def connect():
    print(f"[LINK] {TARGET_IP}:{TARGET_PORT}")
    print("正在等待数据推送...")

@sio.event
def disconnect():
    print("[LINK] 连接断开")

# ================= 1. 数传数据 (Drone Protocol) =================
@sio.on('droneStatus')
def on_drone_status(data):
    print(f"\n 数传报文 {datetime.datetime.now().time()} ================")
    # 直接打印原始数据，不做任何解析
    # json.dumps 用于格式化打印，ensure_ascii=False 保证中文正常显示
    print(json.dumps(data, indent=4, ensure_ascii=False))

# ================= 2. 图传/频谱信号 (Image/Spectrum) =================
@sio.on('imageStatus')
def on_image_status(data):
    print(f"\n 图传信号 {datetime.datetime.now().time()} ================")
    print(json.dumps(data, indent=4, ensure_ascii=False))

# ================= 3. 频谱扫描图 (Spectrum View) =================
# 注意：这个数据量极大，可能会刷屏，想看细节可以解开注释
@sio.on('spectrumViewData')
def on_spectrum_view(data):
    # 只打印摘要，防止卡死终端
    start = data.get('bfreq', '未知')
    end = data.get('efreq', '未知')
    points_count = len(data.get('points', []))
    print(f"\n[频谱波形] 范围: {start}-{end}MHz | 数据点数: {points_count}")
    # print(json.dumps(data, indent=4)) # <--- 如果你不怕刷屏，可以把这行解开

# ================= 4. 设备/系统状态 (System Status) =================
@sio.on('deviceStatus')  # 或者是 systemStatus，根据不同版本可能不同
def on_device_status(data):
    print(f"\n[设备状态] {json.dumps(data, ensure_ascii=False)}")

if __name__ == '__main__':
    url = f'http://{TARGET_IP}:{TARGET_PORT}'
    try:
        sio.connect(url)
        sio.wait()
    except KeyboardInterrupt:
        print("\n停止监控")
    except Exception as e:
        print(f"发生错误: {e}")