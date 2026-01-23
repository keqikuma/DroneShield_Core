import socketio

# 1. 创建客户端实例
# logger=True 会打印底层的握手日志，方便咱们排查问题
sio = socketio.Client(logger=True, engineio_logger=True)

# 2. 定义连接成功时的回调
@sio.event
def connect():
    print("[连接成功] 已经连上 Linux !!!!!")
    print(f"我的 Session ID 是: {sio.sid}")

# 3. 定义连接断开时的回调
@sio.event
def disconnect():
    print("[断开] 连接断开了")

# 4. 主程序
if __name__ == '__main__':
    # 替换成你 Linux 开发板的真实 IP
    # 注意：千万不要写 127.0.0.1，那是你自己的电脑
    target_ip = '192.168.1.12' 
    target_port = 8090 # 根据 config.ini 确定的端口
    
    url = f'http://{target_ip}:{target_port}'
    
    print(f" 正在尝试连接: {url} ...")
    
    try:
        sio.connect(url)
        sio.wait() # 让程序一直运行，不要退出
    except Exception as e:
        print(f" 连接失败: {e}")
        print("提示：请检查 1. IP是否正确 2. 开发板是否已启动 3. 电脑能不能ping通开发板")
