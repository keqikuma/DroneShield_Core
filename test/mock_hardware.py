import socket
import threading
import json

# ================= 配置 =================
# 必须与 Consts.h 中的 SIMULATION_MODE 配置一致
TCP_IP = '127.0.0.1'
TCP_PORT = 7001  # 模拟板卡15

UDP_IP = '127.0.0.1'
UDP_PORT = 9099  # 模拟诱骗逻辑单元

# ================= TCP 模拟 (板卡15) =================
def run_tcp_server():
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.bind((TCP_IP, TCP_PORT))
    server.listen(1)
    print(f"✅ [Mock-TCP] 板卡15模拟器已启动，监听 {TCP_IP}:{TCP_PORT}")

    while True:
        conn, addr = server.accept()
        print(f"🔗 [Mock-TCP] 收到连接: {addr}")
        while True:
            try:
                data = conn.recv(1024)
                if not data: break
                # 这里可以打印收到的 16 进制指令，验证 EB 90 协议
                print(f"📩 [Mock-TCP] 收到指令(Hex): {data.hex().upper()}")
            except:
                break
        conn.close()

# ================= UDP 模拟 (诱骗逻辑) =================
def run_udp_server():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((UDP_IP, UDP_PORT))
    print(f"✅ [Mock-UDP] 诱骗逻辑模拟器已启动，监听 {UDP_IP}:{UDP_PORT}")

    while True:
        data, addr = sock.recvfrom(1024)
        try:
            # 解析 Node.js 风格协议: FF + Length(4) + Encode(3) + JSON
            raw_str = data.decode('utf-8', errors='ignore')
            token = raw_str[0:2]   # FF
            length = raw_str[2:6]  # 长度
            encode = raw_str[6:9]  # 指令码 (601/602)
            json_str = raw_str[9:] # JSON数据
            
            print(f"\n📩 [Mock-UDP] 收到数据包:")
            print(f"   Token: {token} | Code: {encode} | Length: {length}")
            print(f"   Payload: {json_str}")
            
            # 验证 JSON 是否合法
            json_obj = json.loads(json_str)
            if encode == "601":
                print(f"   -> 动作: 设置坐标 ({json_obj.get('dbLat')}, {json_obj.get('dbLon')})")
            elif encode == "602":
                state = "开" if json_obj.get('iSwitch') == 1 else "关"
                print(f"   -> 动作: 诱骗开关 [{state}]")
                
        except Exception as e:
            print(f"❌ [Mock-UDP] 解析失败: {e}")

# ================= 启动线程 =================
if __name__ == '__main__':
    t1 = threading.Thread(target=run_tcp_server)
    t2 = threading.Thread(target=run_udp_server)
    t1.start()
    t2.start()
    t1.join()
    t2.join()
