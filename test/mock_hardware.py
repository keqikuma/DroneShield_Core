import socket
import json
import time
import sys

# ================= 配置 =================
BIND_IP = '127.0.0.1'
UDP_PORT = 9099 

def get_direction_name(angle):
    # ... (保持原样) ...
    try:
        a = float(angle)
        if abs(a - 0.0) < 0.1: return "北 (North)"
        if abs(a - 90.0) < 0.1: return "东 (East)"
        if abs(a - 180.0) < 0.1: return "南 (South)"
        if abs(a - 270.0) < 0.1: return "西 (West)"
        return f"{a}°"
    except:
        return str(angle)

def run_udp_server():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    
    # 【关键修改 1】设置超时时间为 1.0 秒
    # 这样它每秒钟会“醒”一次，看看有没有 Ctrl+C
    sock.settimeout(1.0)
    
    try:
        sock.bind((BIND_IP, UDP_PORT))
        print(f"✅ [Mock-UDP] 逻辑单元模拟器已启动")
        print(f"📡 监听地址: {BIND_IP}:{UDP_PORT}")
        print(f"⌨️  按 Ctrl+C 可优雅停止...")
        print("-" * 40)

        while True:
            try:
                # 这里的 recvfrom 现在最多只等 1 秒
                data, addr = sock.recvfrom(2048)
                
                # ... (下面的解析逻辑完全不用动) ...
                raw_str = data.decode('utf-8', errors='ignore')
                if raw_str.startswith("FF"):
                    code = raw_str[6:9]
                    json_str = raw_str[9:]
                    obj = json.loads(json_str)
                    current_time = time.strftime("%H:%M:%S", time.localtime())
                    print(f"[{current_time}] 📩 指令 {code} | 来自 {addr}")
                    
                    if code == "601":
                        print(f"   📍 [位置] Lat: {obj.get('dbLat')}, Lon: {obj.get('dbLon')}, Alt: {obj.get('dbAlt')}")
                    elif code == "602":
                        state = "🟢 开启 (ON)" if obj.get('iSwitch') == 1 else "🔴 关闭 (OFF)"
                        print(f"   ⚡️ [开关] {state}")
                    elif code == "603":
                        val = list(obj.values())[-1]
                        print(f"   📉 [衰减] Type: {obj.get('iType')}, Val: {val} dB")
                    elif code == "604":
                        val = list(obj.values())[-1]
                        print(f"   ⏱️ [时延] Type: {obj.get('iType')}, Val: {val} ns")
                    elif code == "608":
                        speed = obj.get('fInitSpeedVal')
                        angle = obj.get('fInitSpeedHead')
                        dir_name = get_direction_name(angle)
                        print(f"   🚀 [直线] 速度: {speed} m/s, 方向: {dir_name}")
                    elif code == "610":
                        radius = obj.get('fCirRadius')
                        cycle = obj.get('fCirCycle')
                        print(f"   🔄 [圆周] 半径: {radius} m, 周期: {cycle} s")
                    elif code == "619":
                        print(f"   👋 [登录] 上报本机: {obj.get('sIP')}:{obj.get('iPort')}")

                    reply_content = '{"iSysSta": 3, "iOcxoSta": 3, "iPASwitch": 1}'
                    reply_len = str(len(reply_content)).zfill(4)
                    reply_packet = f"FF{reply_len}600{reply_content}"
                    sock.sendto(reply_packet.encode(), addr)

            # 【关键修改 2】捕获超时异常
            except socket.timeout:
                # 超时了没关系，说明这1秒内没人发数据
                # 直接 continue 进入下一次循环
                # 在这个间隙，Python 会自动检查 Ctrl+C
                continue
                
            except json.JSONDecodeError:
                print(f"⚠️  [警告] 收到非 JSON 数据: {raw_str}")
            except Exception as e:
                print(f"❌ [错误] 处理数据时异常: {e}")

    except KeyboardInterrupt:
        print("\n\n🛑 [停止] 用户终止了模拟器。再见！")
    finally:
        sock.close()

if __name__ == '__main__':
    run_udp_server()
