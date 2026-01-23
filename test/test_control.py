import requests
import json
import time

# 配置
TARGET_IP = '192.178.1.12'
TARGET_PORT = 8090
BASE_URL = f'http://{TARGET_IP}:{TARGET_PORT}'

def set_params():
    url = f"{BASE_URL}/setWriteFreq"
    # 模拟设置：开启 900M 和 5.8G
    payload = {
        "writeFreq": [
            {
                "freqType": 1, 
                "startFreq": 900, 
                "endFreq": 920, 
                "isSelect": 1
            },
            {
                "freqType": 2, 
                "startFreq": 5700, 
                "endFreq": 5800, 
                "isSelect": 1
            }
        ]
    }
    try:
        print(f"📡 正在下发参数到: {url}")
        resp = requests.post(url, json=payload, timeout=3)
        print(f"✅ 参数设置响应: {resp.text}")
    except Exception as e:
        print(f"❌ 参数设置失败: {e}")

def start_jamming():
    url = f"{BASE_URL}/interferenceControl"
    payload = {"switch": 1} # 1=开
    try:
        print(f"🔥 正在开启干扰...")
        resp = requests.post(url, json=payload, timeout=3)
        print(f"✅ 开启响应: {resp.text}")
    except Exception as e:
        print(f"❌ 开启失败: {e}")

def stop_jamming():
    url = f"{BASE_URL}/interferenceControl"
    payload = {"switch": 0} # 0=关
    try:
        print(f"🛑 正在停止干扰...")
        resp = requests.post(url, json=payload, timeout=3)
        print(f"✅ 停止响应: {resp.text}")
    except Exception as e:
        print(f"❌ 停止失败: {e}")

if __name__ == '__main__':
    # 1. 先设置参数
    set_params()
    time.sleep(1)
    
    # 2. 开启干扰 (你会听到板子风扇或者看到灯亮)
    start_jamming()
    
    print("⏳ 干扰持续 5秒...")
    time.sleep(5)
    
    # 3. 停止干扰
    stop_jamming()
