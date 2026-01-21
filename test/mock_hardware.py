import socket
import threading
import json
import time
import binascii

BIND_IP = '127.0.0.1'
TCP_PORT = 7001
UDP_PORT = 9099

def run_tcp_server():
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind((BIND_IP, TCP_PORT))
    server.listen(1)
    print(f"✅ [Mock-TCP] 监听 {BIND_IP}:{TCP_PORT}")

    while True:
        conn, addr = server.accept()
        print(f"🔗 [Mock-TCP] 连接: {addr}")
        
        while True:
            try:
                data = conn.recv(1024)
                if not data: break
                
                # HEX 打印
                hex_str = binascii.hexlify(data).decode('utf-8').upper()
                
                # 简单协议解析
                if hex_str.startswith("EB90"):
                    cmd = hex_str[8:10] # 获取指令码
                    if cmd == "01": print(f"📩 [TCP] 设置频率 (0x01)")
                    elif cmd == "05": print(f"📩 [TCP] 设置时隙 (0x05)")
                    elif cmd == "0C": print(f"📩 [TCP] 设置模组 (0x0C)")
                    elif cmd == "02": print(f"📩 [TCP] 设置衰减 (0x02)")
                    elif cmd == "09": print(f"📩 [TCP] 准备接收文件 (0x09)")
                    elif cmd == "06": print(f"📩 [TCP] 关闭发射 (0x06)")
                    else: print(f"📩 [TCP] 未知指令: {cmd}")
                else:
                    print(f"📩 [TCP] 收到文件流/数据: {len(data)} bytes")
                    
            except Exception as e:
                print(e)
                break
        conn.close()

def run_udp_server():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((BIND_IP, UDP_PORT))
    print(f"✅ [Mock-UDP] 监听 {BIND_IP}:{UDP_PORT}")

    while True:
        data, addr = sock.recvfrom(2048)
        try:
            raw_str = data.decode('utf-8', errors='ignore')
            if raw_str.startswith("FF"):
                code = raw_str[6:9]
                json_str = raw_str[9:]
                print(f"📩 [UDP] 指令 {code}: {json_str}")
                
                # 自动回复心跳
                reply = '{"iState": 1}'
                sock.sendto(reply.encode(), addr)
        except:
            pass

if __name__ == '__main__':
    threading.Thread(target=run_tcp_server, daemon=True).start()
    threading.Thread(target=run_udp_server, daemon=True).start()
    while True: time.sleep(1)
