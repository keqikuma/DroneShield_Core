import asyncio
import socketio
from aiohttp import web
import json
import random
import time
import datetime

# ==========================================
# 全局配置
# ==========================================
# 初始距离 (米)
current_distance = 1000.0  
is_jamming = False         
is_spoofing = False        
uav_id = "1636J1400AAXML"  # 模拟 Mavic 2 ID

# 基地坐标 (西安大华)
BASE_LAT = 34.218146
BASE_LON = 108.834316

# 辅助函数：获取当前时间字符串
def get_time():
    return datetime.datetime.now().strftime("%H:%M:%S.%f")[:-3]

# ==========================================
# 1. Socket.IO Server Setup
# ==========================================
sio = socketio.AsyncServer(async_mode='aiohttp', cors_allowed_origins='*')
app = web.Application()
sio.attach(app)

@sio.event
async def connect(sid, environ):
    print(f"[{get_time()}] [SocketIO] Client connected (SID: {sid})")

# ==========================================
# 2. HTTP Server (干扰模拟 - 接收 HTTP POST)
# ==========================================
async def handle_set_write_freq(request):
    try:
        data = await request.json()
        print(f"[{get_time()}] [HTTP] 收到写频参数配置 (/setWriteFreq):")
        print(f"   >>> Payload: {json.dumps(data, indent=2, ensure_ascii=False)}")
        return web.json_response({'status': 'ok'})
    except Exception as e:
        print(f"[HTTP Error] {e}")
        return web.json_response({'status': 'error'})

async def handle_interference_control(request):
    global is_jamming
    try:
        data = await request.json()
        print(f"[{get_time()}] [HTTP] 收到干扰控制指令 (/interferenceControl):")
        print(f"   >>> Payload: {json.dumps(data, ensure_ascii=False)}")
        
        if data.get('switch', 0) == 1:
            is_jamming = True
            print(f"   >>> 状态变更: 干扰开启 (Jamming ON)")
        else:
            is_jamming = False
            print(f"   >>> 状态变更: 干扰关闭 (Jamming OFF)")
            
        return web.json_response({'status': 'ok'})
    except Exception as e:
        print(f"[HTTP Error] {e}")
        return web.json_response({'status': 'error'})

app.router.add_post('/setWriteFreq', handle_set_write_freq)
app.router.add_post('/interferenceControl', handle_interference_control)

# ==========================================
# 3. UDP Server (诱骗模拟 - 接收 UDP 数据包)
# ==========================================
class UdpProtocol:
    def connection_made(self, transport):
        self.transport = transport

    def datagram_received(self, data, addr):
        global is_spoofing
        
        # 1. 打印原始 Hex
        hex_str = data.hex()
        # 2. 尝试解码字符串 (有些指令是 JSON 字符串，有些是二进制)
        try:
            str_content = data.decode(errors='ignore')
        except:
            str_content = "<binary>"

        print(f"[{get_time()}] [UDP] 收到诱骗指令 (来自 {addr}):")
        print(f"   >>> HEX : {hex_str}")
        print(f"   >>> STR : {str_content}")

        # 简单的状态模拟逻辑
        if '"iSwitch":1' in str_content:
            is_spoofing = True
            print("   >>> 状态变更: 诱骗开启 (Spoofing ON)")
        elif '"iSwitch":0' in str_content:
            is_spoofing = False
            print("   >>> 状态变更: 诱骗关闭 (Spoofing OFF)")

# ==========================================
# 4. TCP Server (继电器压制模拟 - 接收 TCP Hex)
# ==========================================
async def handle_relay_client(reader, writer):
    addr = writer.get_extra_info('peername')
    print(f"[{get_time()}] [RELAY] TCP 连接建立: {addr}")
    
    try:
        while True:
            data = await reader.read(1024)
            if not data:
                break
            
            hex_str = data.hex().upper()
            formatted_hex = ' '.join(hex_str[i:i+2] for i in range(0, len(hex_str), 2))
            
            print(f"[{get_time()}] [RELAY] 收到压制指令:")
            print(f"   >>> Raw Hex: {formatted_hex}")
            
            # 简单解析
            if hex_str.startswith("FE0F"):
                if "FF" in hex_str:
                    print("   >>> 解析: 全开 (ALL ON)")
                else:
                    print("   >>> 解析: 全关 (ALL OFF)")
            elif hex_str.startswith("FE05"):
                print("   >>> 解析: 单通道控制")

    except Exception as e:
        print(f"[RELAY Error] {e}")
    finally:
        print(f"[{get_time()}] [RELAY] 客户端断开")
        writer.close()

# ==========================================
# 5. 物理引擎 (模拟无人机运动)
# ==========================================
async def simulation_loop():
    global current_distance, is_jamming, is_spoofing
    print("[SIM] 物理引擎就绪...")
    
    while True:
        # 运动逻辑
        if is_spoofing:
            current_distance += 40.0 
        elif is_jamming:
            current_distance += random.uniform(-2, 2) 
        else:
            current_distance -= 10.0 
            
        if current_distance < 0: current_distance = 0
        if current_distance > 5000: current_distance = 5000

        # 坐标计算
        lat_offset = current_distance / 111000.0
        real_uav_lat = BASE_LAT + lat_offset
        real_uav_lon = BASE_LON + (random.uniform(-0.0001, 0.0001))
        
        # 数据推送
        if 0 < current_distance < 3000:
            drone_data = [{
                "uav_info": {
                    "uav_id": uav_id,
                    "model_name": "Mavic 2",
                    "distance": round(current_distance, 1),
                    "azimuth": 0,
                    "uav_lat": real_uav_lat,
                    "uav_lng": real_uav_lon,
                    "height": 120.5,
                    "freq": 2400.0,
                    "velocity": "South 10.0 m/s",
                    "pilot_lat": BASE_LAT + 0.001,
                    "pilot_lng": BASE_LON + 0.001,
                    "pilot_distance": 150.0,
                    "whiteList": False,
                    "uuid": "uuid-sim-123456",
                    "img": 1,
                    "type": "drone"
                }
            }]
            await sio.emit('droneStatus', drone_data)
            
            image_data = [{
                "id": "5800_fpv",
                "freq": 5800.0,
                "amplitude": random.uniform(80, 100),
                "type": 1,
                "mes": int(time.time() * 1000),
                "first": 0
            }]
            await sio.emit('imageStatus', image_data)
            await sio.emit('detect_batch', [len(drone_data) + len(image_data)])
            await sio.emit('info', {'lat': BASE_LAT, 'lng': BASE_LON})

        await asyncio.sleep(1)

# ==========================================
# 6. 启动入口
# ==========================================
async def start_background_tasks(app):
    loop = asyncio.get_running_loop()
    # 诱骗 UDP 端口 9099
    await loop.create_datagram_endpoint(lambda: UdpProtocol(), local_addr=('0.0.0.0', 9099))
    print("[UDP] Listening on 9099 (Spoofing)")
    
    # 压制 TCP 端口 2000
    server = await asyncio.start_server(handle_relay_client, '0.0.0.0', 2000)
    app['relay_server'] = server
    print("[TCP] Listening on 2000 (Relay)")
    
    asyncio.create_task(simulation_loop())

if __name__ == '__main__':
    app.on_startup.append(start_background_tasks)
    # HTTP/SocketIO 端口 3000
    web.run_app(app, port=8090)
