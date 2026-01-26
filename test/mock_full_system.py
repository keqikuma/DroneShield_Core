import asyncio
import socket
import threading
import json
from aiohttp import web
import socketio

# ================= 配置 =================
HTTP_PORT = 8090  
UDP_PORT = 9099

sio = socketio.AsyncServer(async_mode='aiohttp', cors_allowed_origins='*')
app = web.Application()
sio.attach(app)

# 全局同步锁：用于等待 Qt 连接
qt_connected_event = asyncio.Event()

# ================= 1. UDP 诱骗模拟 =================
def run_udp_server():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        sock.bind(('0.0.0.0', UDP_PORT))
        print(f"[UDP] 诱骗监听端口: {UDP_PORT}")
        while True:
            data, addr = sock.recvfrom(4096)
            raw = data.decode('utf-8', errors='ignore')
            if raw.startswith('FF'):
                code = raw[6:9]
                print(f"[UDP指令] Code: {code}")
                # 简单解析一下关键指令
                if code == "602":
                    obj = json.loads(raw[9:])
                    sw = "开 (ON)" if obj.get('iSwitch') == 1 else "关 (OFF)"
                    print(f"      >>> 诱骗开关状态: {sw} <<<")
    except Exception as e:
        print(f"UDP Error: {e}")

# ================= 2. HTTP 干扰模拟 =================
async def handle_write_freq(request):
    print(f"[HTTP] 收到写频参数")
    return web.json_response({"status": "ok"})

async def handle_interference(request):
    data = await request.json()
    state = "开 (ON)" if data.get('switch') == 1 else "关 (OFF)"
    print(f"[HTTP] >>> 干扰压制状态: {state} <<<")
    return web.json_response({"status": "ok"})

app.router.add_post('/setWriteFreq', handle_write_freq)
app.router.add_post('/interferenceControl', handle_interference)

# ================= 3. Socket.IO 导演逻辑 =================

@sio.event
async def connect(sid, environ):
    print(f"[SocketIO] Qt客户端已连接! (SID: {sid})")
    # 解锁导演线程
    qt_connected_event.set()

@sio.event
async def disconnect(sid):
    print(f"[SocketIO] Qt客户端断开")
    qt_connected_event.clear()

async def simulation_director():
    print("[Director] 等待 Qt 程序连接...")
    
    # 【关键修改】阻塞等待，直到 Qt 连上
    await qt_connected_event.wait()
    
    print("[Director] Qt 已上线，3秒后开始播放剧本...")
    await asyncio.sleep(3) 

    # --- 场景 1: 发现目标 ---
    print("\n[Director] === Action! 发现 Mini 4 Pro (距离 800m) ===")
    print("   (预期: Qt 应触发【红色】干扰和诱骗)")
    
    drone_data = [{
        "uav_info": {
            "uav_id": "TEST_DRONE_001",
            "model_name": "Mini 4 Pro",
            "uav_lat": 30.0, "uav_lng": 120.0,
            "freq": 2400
        }
    }]
    
    # 持续推送 8 秒 (给 Qt 足够的反应时间)
    for i in range(8):
        await sio.emit('droneStatus', drone_data)
        await asyncio.sleep(1)
    
    # --- 场景 2: 目标消失 ---
    print("\n[Director] === Cut! 目标消失 ===")
    print("   (预期: Qt 应停止所有动作)")
    await sio.emit('droneStatus', []) 
    
    print("[Director] 演示结束")

async def start_server():
    runner = web.AppRunner(app)
    await runner.setup()
    site = web.TCPSite(runner, '0.0.0.0', HTTP_PORT)
    print(f"[HTTP/SocketIO] 服务启动: http://localhost:{HTTP_PORT}")
    await site.start()
    await simulation_director()
    while True: await asyncio.sleep(3600)

if __name__ == '__main__':
    t = threading.Thread(target=run_udp_server, daemon=True)
    t.start()
    try:
        asyncio.run(start_server())
    except KeyboardInterrupt: pass
