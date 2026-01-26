import asyncio
import socketio
from aiohttp import web
import json
import random
import time

# === 全局状态 ===
current_distance = 800.0  # 初始距离 2000米
is_jamming = False         # 干扰状态
is_spoofing = False        # 诱骗状态
uav_id = "DJI_Mavic_3_Pro"

# === 1. Socket.IO (用于发送侦测数据给 Qt) ===
sio = socketio.AsyncServer(async_mode='aiohttp', cors_allowed_origins='*')
app = web.Application()
sio.attach(app)

@sio.event
async def connect(sid, environ):
    print(f"[SocketIO] Qt客户端已连接! (SID: {sid})")

@sio.event
async def disconnect(sid):
    print("[SocketIO] Qt客户端断开")

# === 2. HTTP Server (用于接收 Qt 的干扰指令) ===
async def handle_jammer_cmd(request):
    global is_jamming
    data = await request.json()
    # Qt 发过来的是 {"switch": 1} 或 0
    cmd = data.get('switch', 0)
    
    if cmd == 1:
        is_jamming = True
        print("⚡ [收到指令] 干扰已开启！无人机将停止前进！")
    else:
        is_jamming = False
        print("🛑 [收到指令] 干扰已停止。")
        
    return web.json_response({'status': 'ok'})

app.router.add_post('/api/jammer/switch', handle_jammer_cmd)

# === 3. UDP Server (用于接收 Qt 的诱骗指令) ===
class UdpProtocol:
    def connection_made(self, transport):
        self.transport = transport

    def datagram_received(self, data, addr):
        global is_spoofing
        message = data.decode()
        # Qt 发过来的格式类似: FF0039602{"iSwitch":1,...}
        # 我们简单判断一下 iSwitch":1
        if '"iSwitch":1' in message:
            if not is_spoofing:
                is_spoofing = True
                print("🌀 [收到指令] 诱骗已开启！无人机将被驱离！")
        elif '"iSwitch":0' in message:
            if is_spoofing:
                is_spoofing = False
                print("🛑 [收到指令] 诱骗已停止。")

# === 4. 核心物理引擎 (模拟无人机运动) ===
async def drone_simulation_loop():
    global current_distance, is_jamming, is_spoofing
    
    print("🎮 [模拟器] 游戏开始！无人机从 2000m 处向你飞来...")
    
    while True:
        # --- 物理计算 ---
        if is_spoofing:
            # 如果开了诱骗，无人机被驱离，距离变远 (速度快)
            current_distance += 30.0 
            status_text = "被驱离 🔙"
        elif is_jamming:
            # 如果开了干扰，无人机悬停 (模拟链路丢失，悬停或漂移)
            current_distance += random.uniform(-2, 2) 
            status_text = "受干扰悬停 ⚡"
        else:
            # 正常情况，无人机接近基地
            current_distance -= 15.0 
            status_text = "逼近中 🚨"

        # 边界限制
        if current_distance < 0:
            current_distance = 0
            status_text = "💥 已撞击基地! 💥"
        if current_distance > 5000:
            current_distance = 5000 # 飞太远就不管了

        # --- 构造数据包发给 Qt ---
        # 只有距离在 3000m 以内才显示在雷达上
        drone_list = []
        if 0 < current_distance < 3000:
            drone_data = {
                "uav_info": {
                    "uav_id": uav_id,
                    "model_name": "Mavic 3",
                            
                    # 【修改这里】把 0 改为 current_distance
                    # 我们暂时用 "纬度" 字段来传递 "距离"
                    "uav_lat": current_distance, 
                            
                    "uav_lng": 0,
                    "freq": 2400.0
                }
            }
            drone_list.append(drone_data)

        # 这里有个小问题：你的 Qt 代码目前是写死 "800m" 的。
        # 为了看到效果，你需要改一下 Qt 代码里的 slotUpdateTargets 
        # 把模拟的 800m 改成动态计算，或者我们在 Python 端打印出来看自嗨一下
        
        # 发送数据
        await sio.emit('droneStatus', drone_list)

        # 控制台打印状态
        print(f"无人机距离: {current_distance:.1f}m [{status_text}] | 干扰:{is_jamming} 诱骗:{is_spoofing}")

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
    print("[UDP] 诱骗监听端口: 9099 Ready")
    return transport

if __name__ == '__main__':
    app.on_startup.append(start_background_tasks)
    web.run_app(app, port=8090)
