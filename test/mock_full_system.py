import asyncio
import socketio
from aiohttp import web
import json
import random

# === 全局状态 ===
# 初始距离 (米)
current_distance = 2000.0  
is_jamming = False         # 干扰状态
is_spoofing = False        # 诱骗状态
uav_id = "DJI_Mavic_3_Pro"

# === 基地坐标 (必须与 C++ Consts.h 保持一致) ===
BASE_LAT = 31.2304
BASE_LON = 121.4737

# === 1. Socket.IO (用于发送侦测数据给 Qt) ===
# ping_timeout=60: 允许客户端 60秒 不响应心跳而不由服务端主动断开
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

# === 2. HTTP Server (用于接收 Qt 的干扰指令) ===
async def handle_jammer_cmd(request):
    global is_jamming
    try:
        data = await request.json()
        # Qt 发过来的是 {"switch": 1} 或 0
        cmd = data.get('switch', 0)
        
        if cmd == 1:
            is_jamming = True
            print("[JAMMER] Command Received: ON - Drone stopped")
        else:
            is_jamming = False
            print("[JAMMER] Command Received: OFF")
            
        return web.json_response({'status': 'ok'})
    except Exception as e:
        print(f"[ERROR] HTTP Request failed: {e}")
        return web.json_response({'status': 'error'})

app.router.add_post('/api/jammer/switch', handle_jammer_cmd)

# === 3. UDP Server (用于接收 Qt 的诱骗指令) ===
class UdpProtocol:
    def connection_made(self, transport):
        self.transport = transport

    def datagram_received(self, data, addr):
        global is_spoofing
        try:
            message = data.decode()
            # Qt 发过来的格式类似: FF0039602{"iSwitch":1,...}
            if '"iSwitch":1' in message:
                if not is_spoofing:
                    is_spoofing = True
                    print("[SPOOF] Command Received: ON - Drone repelled")
            elif '"iSwitch":0' in message:
                if is_spoofing:
                    is_spoofing = False
                    print("[SPOOF] Command Received: OFF")
        except Exception as e:
            print(f"[ERROR] UDP Decode failed: {e}")

# === 4. 核心物理引擎 (模拟无人机运动) ===
async def drone_simulation_loop():
    global current_distance, is_jamming, is_spoofing
    
    print("[SIM] Simulation started. Drone approaching from 2000m...")
    
    while True:
        # --- 物理计算 ---
        status_text = "APPROACHING"
        
        if is_spoofing:
            # 如果开了诱骗，无人机被驱离，距离变远 (速度快)
            current_distance += 30.0 
            status_text = "REPELLING"
        elif is_jamming:
            # 如果开了干扰，无人机悬停 (模拟链路丢失，悬停或漂移)
            current_distance += random.uniform(-2, 2) 
            status_text = "HOVERING (JAMMED)"
        else:
            # 正常情况，无人机接近基地
            current_distance -= 15.0 
            status_text = "APPROACHING"

        # 边界限制
        if current_distance < 0:
            current_distance = 0
            status_text = "IMPACT"
        if current_distance > 5000:
            current_distance = 5000 # 飞太远就不管了

        # --- 坐标转换 (距离 -> 经纬度) ---
        # 假设无人机从正北方向飞来 (经度不变，纬度变化)
        # 1度纬度 ≈ 111,000 米
        lat_offset = current_distance / 111000.0
        real_lat = BASE_LAT + lat_offset
        real_lon = BASE_LON # 保持在正北

        # --- 构造数据包发给 Qt ---
        drone_list = []
        # 只有在 3000m 以内才被雷达发现
        if 0 < current_distance < 3000:
            drone_data = {
                "uav_info": {
                    "uav_id": uav_id,
                    "model_name": "Mavic 3",
                    # 发送计算好的真实经纬度
                    "uav_lat": real_lat, 
                    "uav_lng": real_lon,
                    "freq": 2400.0
                }
            }
            drone_list.append(drone_data)
        
        # 发送数据
        await sio.emit('droneStatus', drone_list)

        # 控制台打印状态 (纯文本)
        print(f"[SIM] Distance: {current_distance:.1f}m | Status: {status_text} | Jam: {is_jamming} | Spoof: {is_spoofing}")

        await asyncio.sleep(1) # 1秒刷新一次

# === 启动入口 ===
async def start_background_tasks(app):
    app['udp_listener'] = asyncio.create_task(start_udp_server())
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
    # 启动 Web 服务器
    web.run_app(app, port=8090)
