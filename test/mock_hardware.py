import socket
import threading
import json
import time

# ================= 配置 =================
BIND_IP = '127.0.0.1'
UDP_PORT = 9099  # 模拟诱骗逻辑单元

def run_udp_server():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((BIND_IP, UDP_PORT))
    print(f"✅ [Mock-UDP] 逻辑单元模拟器已启动，监听 {BIND_IP}:{UDP_PORT}")

    # 模拟定期发送 600 状态包 (心跳)
    # 模拟 PDF 3.1: 状态信息上报
    def send_status_heartbeat():
        while True:
            # iSysSta=3 (就绪), iOcxoSta=3 (晶振锁定)
            status_json = '{"iSysSta": 3, "iOcxoSta": 3, "iPASwitch": 0, "dbFixLat": 30.0, "dbFixLon": 104.0}'
            # 拼包: FF + Len(4) + 600 + JSON
            msg_len = str(len(status_json)).zfill(4)
            packet = f"FF{msg_len}600{status_json}"
            
            # 注意：实际中需要知道对方端口，这里简化处理，只有收到消息后才回
            time.sleep(2)

    # 启动心跳线程 (可选，这里暂时不主动发，依靠回复)
    
    while True:
        data, addr = sock.recvfrom(2048)
        try:
            raw_str = data.decode('utf-8', errors='ignore')
            if raw_str.startswith("FF"):
                code = raw_str[6:9]
                json_str = raw_str[9:]
                obj = json.loads(json_str)
                
                print(f"📩 [UDP] 指令 {code} 来自 {addr}")
                
                if code == "601":
                    print(f"   -> [位置] Lat:{obj.get('dbLat')}, Lon:{obj.get('dbLon')}")
                elif code == "602":
                    print(f"   -> [开关] {'ON' if obj.get('iSwitch')==1 else 'OFF'}")
                elif code == "603":
                    print(f"   -> [衰减] Type:{obj.get('iType')}, Value:{list(obj.values())[-1]}")
                elif code == "604":
                    print(f"   -> [时延] Type:{obj.get('iType')}, Delay:{list(obj.values())[-1]}")
                elif code == "608":
                    print(f"   -> [直线] Speed:{obj.get('fInitSpeedVal')}, Angle:{obj.get('fInitSpeedHead')}")
                elif code == "610":
                    print(f"   -> [圆周] Radius:{obj.get('fCirRadius')}, Cycle:{obj.get('fCirCycle')}")
                elif code == "613":
                    print(f"   -> [时间] {obj.get('iHour')}:{obj.get('iMin')}:{obj.get('iSec')}")
                elif code == "619":
                    print(f"   -> [登录] 上报IP:{obj.get('sIP')}:{obj.get('iPort')}")

                # 模拟回复通用成功包 (PDF 4.1 命令反馈: 6xx -> 65x)
                # 这里简单回复 600 状态包，模拟设备在线
                reply_content = '{"iSysSta": 3, "iOcxoSta": 3, "iPASwitch": 1}'
                # 简单拼包
                reply_len = str(len(reply_content)).zfill(4)
                reply_packet = f"FF{reply_len}600{reply_content}"
                sock.sendto(reply_packet.encode(), addr)

        except Exception as e:
            print(f"❌ 解析错误: {e}")

if __name__ == '__main__':
    run_udp_server()
