import requests
import json
import time

# ================= 配置区域 =================
#这是 Linux 主控板的 IP，不是射频板卡的 IP！
# 也就是运行 nodejs 的那个板子
TARGET_IP = '192.178.1.12'  
TARGET_PORT = 8090
BASE_URL = f'http://{TARGET_IP}:{TARGET_PORT}'

def set_params():
    url = f"{BASE_URL}/setWriteFreq"
    
    # 构造数据包：同时控制两块板卡
    # freqType 1 对应第一块板 (通常是 0.3G-3G 低频板) -> 对应内部 IP 例如 .85
    # freqType 2 对应第二块板 (通常是 2G-6G 高频板) -> 对应内部 IP 例如 .86
    payload = {
        "writeFreq": [
            {
                "freqType": 1,       # 控制板卡 1
                "startFreq": 900,    # 起始 900 MHz
                "endFreq": 920,      # 终止 920 MHz
                "isSelect": 1        # 1=开启
            },
            {
                "freqType": 2,       # 控制板卡 2
                "startFreq": 900,    # 起始 900 MHz
                "endFreq": 920,      # 终止 920 MHz
                "isSelect": 1        # 1=开启
            }
        ]
    }
    
    try:
        print(f"正在下发参数到 Linux 主控: {url}")
        # 打印一下我们要发什么，方便调试
        print(f"数据包: {json.dumps(payload, indent=2)}")
        
        resp = requests.post(url, json=payload, timeout=3)
        print(f"参数设置响应: {resp.text}")
    except Exception as e:
        print(f"参数设置失败: {e}")

def start_jamming():
    url = f"{BASE_URL}/interferenceControl"
    payload = {"switch": 1} # 1=开
    try:
        print(f"正在发送开启指令...")
        resp = requests.post(url, json=payload, timeout=3)
        print(f"开启响应: {resp.text}")
    except Exception as e:
        print(f"开启失败: {e}")

def stop_jamming():
    url = f"{BASE_URL}/interferenceControl"
    payload = {"switch": 0} # 0=关
    try:
        print(f"正在发送停止指令...")
        resp = requests.post(url, json=payload, timeout=3)
        print(f"停止响应: {resp.text}")
    except Exception as e:
        print(f"停止失败: {e}")

if __name__ == '__main__':
    # 1. 确保在虚拟环境中运行 (命令行前面要有 (venv))
    
    # 2. 先设置频率参数
    set_params()
    time.sleep(1)
    
    # 3. 开启干扰 (Linux 会根据上面的参数去控制 .86 和 .85)
    start_jamming()
    
    print("干扰持续 20秒 (方便你观察板卡状态)...")
    time.sleep(20)
    
    # 4. 停止干扰
    stop_jamming()
