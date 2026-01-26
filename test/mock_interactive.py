import asyncio
import socketio
from aiohttp import web
import json
import random

# ==========================================
# 全局配置与状态
# ==========================================
# 初始距离 (米)
current_distance = 1000.0  
is_jamming = False         # 干扰状态 (Linux板卡)
is_spoofing = False        # 诱骗状态
uav_id = "DJI_Mavic_3_Pro"

# 基地坐标 (必须与 C++ Consts.h 保持一致)
BASE_LAT = 31.2304
BASE_LON = 121.4737

# ==========================================
# 1. Socket.IO Server (侦测数据下发)
# ==========================================
# ping_timeout=60: 防止频繁断开
sio = socketio.AsyncServer(
    async_mode='aiohttp', 
    cors_allowed_origins='*', 
    ping_timeout=60, 
    ping_interval=25
)
app = web.Application()
sio.attach(app)

@sio.event
async def connect(sid, environ):
    print(f"[SocketIO] Client connected (SID: {sid})")

@sio.event
async def disconnect(sid):
    print("[SocketIO] Client disconnected")

# ==========================================
# 2. HTTP Server (Linux 干扰板卡模拟)
# ==========================================

# 接口: 设置频率参数
async def handle_set_write_freq(request):
    try:
        data = await request.json()
        freq_list = data.get('writeFreq', [])
        
        print("-" * 40)
        print(f"[HTTP] Config Received (/setWriteFreq):")
        if not freq_list:
            print("   [WARN] Empty configuration list")
        
        for item in freq_list:
            board_id = item.get('freqType', '?')
            start_f = item.get('startFreq', 0)
            end_f = item.get('endFreq', 0)
            enable = item.get('isSelect', 0)
            status_str = "ENABLED" if enable == 1 else "DISABLED"
            print(f"   > Board {board_id}: {start_f}MHz - {end_f}MHz [{status_str}]")
        print("-" * 40)
        
        return web.json_response({'status': 'ok', 'msg': 'Params updated'})
    except Exception as e:
        print(f"[ERROR] /setWriteFreq failed: {e}")
        return web.json_response({'status': 'error'}, status=500)

# 接口: 干扰开关控制
async def handle_interference_control(request):
    global is_jamming
    try:
        data = await request.json()
        cmd = data.get('switch', 0)
        
        if cmd == 1:
            is_jamming = True
            print(f"[HTTP] Jammer Control -> ON")
            print("   [INFO] Drone halted (Jamming Active)")
        else:
            is_jamming = False
            print(f"[HTTP] Jammer Control -> OFF")
            
        return web.json_response({'status': 'ok'})
    except Exception as e:
        print(f"[ERROR] /interferenceControl failed: {e}")
        return web.json_response({'status': 'error'}, status=500)

app.router.add_post('/setWriteFreq', handle_set_write_freq)
app.router.add_post('/interferenceControl', handle_interference_control)

# ==========================================
# 3. UDP Server (诱骗设备模拟)
# ==========================================
class UdpProtocol:
    def connection_made(self, transport):
        self.transport = transport

    def datagram_received(self, data, addr):
        global is_spoofing
        try:
            message = data.decode()
            # 简单解析 JSON 字符串
            if '"iSwitch":1' in message:
                if not is_spoofing:
                    is_spoofing = True
                    print("[UDP] Spoof Command -> ON (Drone Repelling)")
            elif '"iSwitch":0' in message:
                if is_spoofing:
                    is_spoofing = False
                    print("[UDP] Spoof Command -> OFF")
        except Exception as e:
            print(f"[ERROR] UDP Decode failed: {e}")

# ==========================================
# 4. TCP Server (继电器压制模拟)
# ==========================================
async def handle_relay_client(reader, writer):
    addr = writer.get_extra_info('peername')
    print(f"[RELAY] TCP Connection from {addr}")
    
    try:
        while True:
            data = await reader.read(1024)
            if not data:
                break
            
            hex_str = data.hex().upper()
            
            # 解析协议
            if hex_str.startswith("FE0F"):
                # 全开/全关指令
                status = "ALL ON (FIRE)" if "FF" in hex_str else "ALL OFF"
                print(f"[RELAY] Command: {status} | Raw: {hex_str}")
            elif hex_str.startswith("FE05"):
                # 单路控制指令 FE 05 00 XX ...
                try:
                    channel_hex = hex_str[6:8]
                    channel_id = int(channel_hex, 16) + 1
                    state = "ON" if "FF00" in hex_str else "OFF"
                    print(f"[RELAY] Command: Channel {channel_id} -> {state} | Raw: {hex_str}")
                except:
                    print(f"[RELAY] Parse Error: {hex_str}")
            else:
                print(f"[RELAY] Unknown Data: {hex_str}")
                
    except Exception as e:
        print(f"[RELAY] Connection Error: {e}")
    finally:
        print("[RELAY] Client disconnected")
        writer.close()

# ==========================================
# 5. 核心物理引擎 (模拟循环)
# ==========================================
async def drone_simulation_loop():
    global current_distance, is_jamming, is_spoofing
    
    print("[SIM] Simulation Engine Started. Drone approaching...")
    
    while True:
        # --- 物理状态计算 ---
        status_text = "APPROACHING"
        
        if is_spoofing:
            # 诱骗开启：驱离 (距离增加)
            current_distance += 30.0 
            status_text = "REPELLING (SPOOF)"
        elif is_jamming:
            # 干扰开启：悬停 (距离波动)
            current_distance += random.uniform(-2, 2) 
            status_text = "HOVERING (JAMMED)"
        else:
            # 正常：逼近 (距离减少)
            current_distance -= 15.0 
            status_text = "APPROACHING"

        # 边界限制
        if current_distance < 0:
            current_distance = 0
            status_text = "IMPACT"
        if current_distance > 5000:
            current_distance = 5000 

        # --- 坐标转换 (距离 -> 真实经纬度) ---
        # 假设从正北方向飞来 (纬度变化，经度不变)
        # 1度纬度 ≈ 111,000 米
        lat_offset = current_distance / 111000.0
        real_lat = BASE_LAT + lat_offset
        real_lon = BASE_LON 

        # --- 发送数据包 ---
        drone_list = []
        # 雷达量程假设 3000m
        if 0 < current_distance < 3000:
            drone_data = {
                "uav_info": {
                    "uav_id": uav_id,
                    "model_name": "Mavic 3",
                    "uav_lat": real_lat, 
                    "uav_lng": real_lon,
                    "freq": 2400.0
                }
            }
            drone_list.append(drone_data)
        
        await sio.emit('droneStatus', drone_list)
        
        # 可选：打印状态心跳
        # print(f"[SIM] Dist: {current_distance:.0f}m | {status_text}")

        await asyncio.sleep(1)

# ==========================================
# 6. 程序入口
# ==========================================
async def start_background_tasks(app):
    # 启动 UDP 监听 (端口 9099)
    app['udp_listener'] = asyncio.create_task(start_udp_server())
    # 启动 TCP 监听 (端口 2000)
    server = await asyncio.start_server(handle_relay_client, '0.0.0.0', 2000)
    app['relay_server'] = server
    print("[RELAY] TCP Listener Ready on port 2000")
    # 启动物理引擎
    app['simulation'] = asyncio.create_task(drone_simulation_loop())

async def start_udp_server():
    loop = asyncio.get_running_loop()
    transport, protocol = await loop.create_datagram_endpoint(
        lambda: UdpProtocol(),
        local_addr=('127.0.0.1', 9099)
    )
    print("[UDP] Listener Ready on port 9099")
    return transport

if __name__ == '__main__':
    # 注册后台任务
    app.on_startup.append(start_background_tasks)
    # 启动 HTTP/SocketIO 服务器 (端口 8090)
    web.run_app(app, port=8090)
