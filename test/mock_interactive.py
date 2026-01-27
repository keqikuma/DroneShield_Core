import asyncio
import socketio
from aiohttp import web
import json
import random
import time
import math

# ==========================================
# 全局配置
# ==========================================
# 初始距离 (米)
current_distance = 2000.0  
is_jamming = False         
is_spoofing = False        
uav_id = "1636J1400AAXML"  # 模拟 Mavic 2 ID

# 基地坐标 (西安大华 - 对应你提供的代码)
BASE_LAT = 34.218146
BASE_LON = 108.834316

# ==========================================
# 1. Socket.IO Server Setup
# ==========================================
sio = socketio.AsyncServer(async_mode='aiohttp', cors_allowed_origins='*')
app = web.Application()
sio.attach(app)

@sio.event
async def connect(sid, environ):
    print(f"[SocketIO] Client connected (SID: {sid})")

# ==========================================
# 2. HTTP Server (干扰模拟)
# ==========================================
async def handle_set_write_freq(request):
    print(f"[HTTP] Received Frequency Config")
    return web.json_response({'status': 'ok'})

async def handle_interference_control(request):
    global is_jamming
    data = await request.json()
    if data.get('switch', 0) == 1:
        is_jamming = True
        print(f"[HTTP] Jammer -> ON (Drone Hovering)")
    else:
        is_jamming = False
        print(f"[HTTP] Jammer -> OFF")
    return web.json_response({'status': 'ok'})

app.router.add_post('/setWriteFreq', handle_set_write_freq)
app.router.add_post('/interferenceControl', handle_interference_control)

# ==========================================
# 3. UDP Server (诱骗模拟)
# ==========================================
class UdpProtocol:
    def connection_made(self, transport):
        self.transport = transport
    def datagram_received(self, data, addr):
        global is_spoofing
        msg = data.decode(errors='ignore')
        if '"iSwitch":1' in msg:
            is_spoofing = True
            print("[UDP] Spoof -> ON (Drone Repelling)")
        elif '"iSwitch":0' in msg:
            is_spoofing = False
            print("[UDP] Spoof -> OFF")

# ==========================================
# 4. 物理引擎与数据生成 (核心)
# ==========================================
async def simulation_loop():
    global current_distance, is_jamming, is_spoofing
    
    print("[SIM] 物理引擎启动，等待连接...")
    
    while True:
        # 1. 物理运动计算
        if is_spoofing:
            current_distance += 40.0 # 驱离：快速远离
        elif is_jamming:
            current_distance += random.uniform(-1, 1) # 干扰：悬停抖动
        else:
            current_distance -= 10.0 # 正常：靠近
            
        # 边界处理
        if current_distance < 0: current_distance = 0
        if current_distance > 5000: current_distance = 5000

        # 2. 坐标计算 (假设从正北方向 0度 飞来)
        # 纬度每度 ≈ 111km -> 1m ≈ 1/111000 度
        lat_offset = current_distance / 111000.0
        
        real_uav_lat = BASE_LAT + lat_offset
        real_uav_lon = BASE_LON + (random.uniform(-0.0001, 0.0001)) # 加点抖动
        
        # 3. 构建全量无人机数据 (droneStatus)
        drone_data = []
        # 在 3000米范围内才显示
        if 0 < current_distance < 3000:
            drone_info = {
                "uav_info": {
                    "uav_id": uav_id,
                    "model_name": "Mavic 2",
                    "distance": round(current_distance, 1),
                    "azimuth": 0, # 正北
                    "uav_lat": real_uav_lat,
                    "uav_lng": real_uav_lon,
                    "height": 120.5,
                    "freq": 2400.0, # MHz
                    "velocity": "South 10.0 m/s",
                    "pilot_lat": BASE_LAT + 0.001, # 假设飞手在基地附近
                    "pilot_lng": BASE_LON + 0.001,
                    "pilot_distance": 150.0,
                    "whiteList": False,
                    "uuid": "uuid-sim-123456",
                    "img": 1,
                    "type": "drone"
                },
                # 轨迹点 (可选)
                "uav_points": [[real_uav_lat, real_uav_lon]], 
                "pilot_points": [[BASE_LAT + 0.001, BASE_LON + 0.001]]
            }
            drone_data.append(drone_info)

        # 4. 构建图传/频谱数据 (imageStatus)
        # 模拟一个常驻的 FPV 信号
        image_data = []
        image_info = {
            "id": "5800_fpv",
            "freq": 5800.0,
            "amplitude": random.uniform(80, 100), # 信号强度跳变
            "type": 1, # 1=FPV
            "mes": int(time.time() * 1000),
            "first": 0
        }
        image_data.append(image_info)

        # 5. 推送所有协议
        # A. 无人机列表
        await sio.emit('droneStatus', drone_data)
        
        # B. 图传列表
        await sio.emit('imageStatus', image_data)
        
        # C. 告警数量 (无人机数 + 图传数)
        count = len(drone_data) + len(image_data)
        await sio.emit('detect_batch', [count])
        
        # D. 设备自身定位 (每5秒发一次即可，这里简化为每次都发)
        await sio.emit('info', {'lat': BASE_LAT, 'lng': BASE_LON})

        await asyncio.sleep(1) # 1秒刷新一次

# ==========================================
# 5. 启动入口
# ==========================================
async def start_background_tasks(app):
    # UDP 监听 9099 (诱骗)
    loop = asyncio.get_running_loop()
    await loop.create_datagram_endpoint(lambda: UdpProtocol(), local_addr=('0.0.0.0', 9099))
    print("[UDP] Listening on 9099")
    
    # 物理引擎
    asyncio.create_task(simulation_loop())

if __name__ == '__main__':
    app.on_startup.append(start_background_tasks)
    # 注意：模拟真实环境端口 3000 (虽然你之前是8090，但C++里我们要对齐真实环境)
    web.run_app(app, port=3000)
