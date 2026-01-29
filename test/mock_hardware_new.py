import socket
import time
import json
import struct
import random
import datetime

# ================= 配置 =================
# Qt 程序所在的 IP (如果是本机测试就是 127.0.0.1)
TARGET_IP = '127.0.0.1' 
TARGET_PORT = 8089

# ================= 协议封装 =================
def build_packet(json_data):
    """
    封装协议包: Head(4) + Len(4) + Type(4) + JSON(N) + Tail(4)
    Head: 55 55 55 55
    Tail: AA AA AA AA
    """
    json_bytes = json.dumps(json_data).encode('utf-8')
    
    # 构造包头
    header = bytes.fromhex('55 55 55 55')
    tail = bytes.fromhex('AA AA AA AA')
    
    # 长度字段 (虽然Qt目前是靠花括号解析的，但为了仿真，我们填真实的长度)
    # struct.pack('<I', val) 表示小端序 Unsigned Int
    length_field = struct.pack('<I', len(json_bytes)) 
    type_field = struct.pack('<I', 1) # 随便填个类型占位
    
    return header + length_field + type_field + json_bytes + tail

# ================= 数据生成器 =================

def get_drone_packet(lat, lng, azimuth):
    """ 生成无人机定位包 (station_droneInfo) """
    return {
        "station_droneInfo": {
            "trace": {
                "model_name": "Mavic 3 Pro",
                "uav_id": "1581F45D000000",
                "uuid": "uuid-sim-8888",
                "uav_lat": lat,
                "uav_lng": lng,
                "Height": 120.5,      # 注意大小写，日志里有时是 Height
                "pilot_lat": 34.220000,
                "pilot_lng": 108.835000,
                "pilot_distance": 150.0,
                "distance": 500.0,
                "freq": 2400.0,
                "velocity": "North 5.0 m/s",
                "azimuth": azimuth
            }
        }
    }

def get_spectrum_packet():
    """ 生成图传/频谱包 (imageInfo) """
    freq = random.choice([2405.0, 2410.0, 2420.0, 5765.0, 5780.0])
    return {
        "imageInfo": {
            "id": f"Spectrum_{freq}", # 模拟唯一ID
            "freq": freq,
            "amplitude": random.randint(200, 500),
            "pro": "DJIO2", # 协议类型
            "type": 0,
            "mes": int(time.time() * 1000)
        },
        "station_id": "sim_station_01"
    }

def get_fpv_packet():
    """ 生成 FPV 包 (fpvInfo) """
    freq = random.choice([5550.0, 5600.0])
    return {
        "fpvInfo": {
            "id": f"FPV_{freq}",
            "freq": freq,
            "amplitude": random.randint(600, 900), # FPV信号通常强一点
            "type": 1,
            "mes": int(time.time() * 1000)
        },
        "station_id": "sim_station_01"
    }

def get_heartbeat_packet():
    """ 生成基站心跳 (device_status) """
    return {
        "device_status": {
            "type": 0,
            "drone": 1,
            "gps": 1
        },
        "station_pos": {
            "lat": 34.219146,
            "lng": 108.835316
        }
    }

# ================= 主循环 =================
def run_simulation():
    client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    
    try:
        print(f"[*] 正在连接 Qt 服务: {TARGET_IP}:{TARGET_PORT} ...")
        client.connect((TARGET_IP, TARGET_PORT))
        print("[+] 连接成功！开始发送模拟数据...")
        
        # 初始无人机位置 (西安附近)
        lat = 34.225533
        lng = 108.834254
        azimuth = 0.0
        
        while True:
            # 1. 模拟无人机移动 (简单的圆周运动或者直线偏移)
            lat += random.uniform(-0.0001, 0.0001)
            lng += random.uniform(-0.0001, 0.0001)
            azimuth = (azimuth + 5) % 360
            
            # 2. 发送无人机数据 (频率低一点，每秒2次)
            drone_pkt = build_packet(get_drone_packet(lat, lng, azimuth))
            client.sendall(drone_pkt)
            print(f"[Send] Drone Pos: {lat:.6f}, {lng:.6f}")
            
            # 3. 发送图传数据 (频率高一点)
            for _ in range(2):
                spec_pkt = build_packet(get_spectrum_packet())
                client.sendall(spec_pkt)
                time.sleep(0.1) # 稍微间隔
                
            # 4. 发送 FPV 数据
            fpv_pkt = build_packet(get_fpv_packet())
            client.sendall(fpv_pkt)
            
            # 5. 偶尔发个心跳 (每10次循环发一次)
            if random.randint(0, 10) > 8:
                hb_pkt = build_packet(get_heartbeat_packet())
                client.sendall(hb_pkt)
                print("[Send] Heartbeat")

            # 这里的 sleep 决定了数据刷新的快慢
            time.sleep(0.5) 

    except ConnectionRefusedError:
        print("[!] 连接失败：请先启动 Qt 程序，并确保它正在监听 8089 端口")
    except Exception as e:
        print(f"[!] 发生错误: {e}")
    finally:
        client.close()

if __name__ == "__main__":
    run_simulation()