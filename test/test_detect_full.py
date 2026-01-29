import socket
import threading
import json
import time

# ================= 配置区域 =================
# Linux 服务器 IP (发送指令的目标)
LINUX_IP = '192.168.1.12' 
LINUX_CONTROL_PORT = 5001 # 根据文档  推测是 5001，如果不行试一下 8090 UDP

# 本机监听设置 (接收数据)
LOCAL_IP = '0.0.0.0' # 监听所有网卡
LOCAL_PORT = 8089    # 根据文档  Linux 会往这个端口推数据

# 指令集 (Hex 字符串)
# 报文模式 (Drone/Telemetry)
CMD_MODE_DRONE = "D5 D5 D5 D5 00 00 00 01 00 00 17 1C 00 00 00 28 00 00 00 00 00 00 00 0B 00 00 00 04 00 00 01 4C 00 00 00 01 D1 D1 D1 D1"
# 图传/频谱模式 (FPV/Spectrum)
CMD_MODE_FPV   = "D5 D5 D5 D5 00 00 00 01 00 00 1E 13 00 00 00 30 00 00 00 00 00 00 00 0C 00 00 00 07 00 00 02 58 00 00 17 07 00 00 01 4C 00 00 00 01 D1 D1 D1 D1"

# ================= 发送功能 (UDP) =================
def send_udp_command(hex_str):
    try:
        # 创建 UDP Socket
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        # 将 Hex 字符串转换为 bytes
        msg_bytes = bytes.fromhex(hex_str)
        
        print(f"\n[发送指令] -> {LINUX_IP}:{LINUX_CONTROL_PORT}")
        print(f"Content: {hex_str[:20]}...")
        
        sock.sendto(msg_bytes, (LINUX_IP, LINUX_CONTROL_PORT))
        sock.close()
        print("[发送成功]")
    except Exception as e:
        print(f"[发送失败] {e}")

# ================= 数据解析功能 =================
def parse_mixed_data(raw_data):
    """
    尝试从混合数据中提取 JSON
    数据格式参考文档[cite: 7]: 55 55 55 55 ... {JSON} ... AA AA AA AA
    """
    try:
        # 将 bytes 转为字符串，忽略解码错误（因为包含二进制头尾）
        text_data = raw_data.decode('utf-8', errors='ignore')
        
        # 寻找 JSON 的特征：大括号
        start_idx = text_data.find('{')
        end_idx = text_data.rfind('}')
        
        if start_idx != -1 and end_idx != -1:
            json_str = text_data[start_idx : end_idx+1]
            
            # 尝试解析 JSON
            data_obj = json.loads(json_str)
            
            # 打印关键信息
            print("\n" + "="*30)
            print("[解析到 JSON 数据]")
            
            # 1. 尝试解析 uav_info (无人机信息) [cite: 2, 8]
            if "station_droneInfo" in data_obj:
                trace = data_obj["station_droneInfo"].get("trace", {})
                uav_id = trace.get("uav_id", "Unknown")
                model = trace.get("model_name", "Unknown")
                lat = trace.get("uav_lat", 0)
                lng = trace.get("uav_lng", 0)
                print(f"👉 无人机: {model} (ID: {uav_id})")
                print(f"📍 坐标: {lat}, {lng}")
            
            # 2. 尝试解析 imageInfo (图传/频谱) [cite: 13, 20]
            elif "imageInfo" in data_obj:
                freq = data_obj["imageInfo"].get("freq", 0)
                amp = data_obj["imageInfo"].get("amplitude", 0)
                print(f"👉 图传信号: {freq} MHz (强度: {amp})")
                
            # 3. 尝试解析 fpvInfo [cite: 18]
            elif "fpvInfo" in data_obj:
                freq = data_obj["fpvInfo"].get("freq", 0)
                print(f"👉 FPV 信号: {freq} MHz")
                
            print("="*30 + "\n")
        else:
            # print("[收到非 JSON 数据包]") # 调试时可开启
            pass
            
    except Exception as e:
        print(f"[解析错误] {e}")

# ================= 接收服务 (TCP Server) =================
def start_tcp_server():
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    
    try:
        server.bind((LOCAL_IP, LOCAL_PORT))
        server.listen(1)
        print(f"[*] TCP 监听启动: {LOCAL_IP}:{LOCAL_PORT}")
        
        while True:
            client, addr = server.accept()
            print(f"[*] 链接建立: {addr}")
            
            while True:
                try:
                    data = client.recv(4096)
                    if not data: break
                    
                    # 1. 打印原始 Hex (调试用)
                    # print(f"[Raw]: {data.hex().upper()[:30]}...")
                    
                    # 2. 尝试解析内容
                    parse_mixed_data(data)
                    
                except Exception as e:
                    print(e)
                    break
            client.close()
            print("[*] 连接断开")
            
    except Exception as e:
        print(f"[TCP Server Error] {e}")

# ================= 主控制台 =================
if __name__ == "__main__":
    # 1. 启动接收线程
    t = threading.Thread(target=start_tcp_server, daemon=True)
    t.start()
    
    # 2. 命令行交互发送指令
    print("\n=== 无人机侦测控制终端 ===")
    print("输入 '1': 切换到 报文模式")
    print("输入 '2': 切换到 图传/频谱模式")
    print("输入 'q': 退出")
    print("==========================\n")
    
    while True:
        cmd = input()
        if cmd == '1':
            send_udp_command(CMD_MODE_DRONE)
        elif cmd == '2':
            send_udp_command(CMD_MODE_FPV)
        elif cmd == 'q':
            break
        else:
            print("无效指令")