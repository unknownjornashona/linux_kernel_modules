import requests  
import bluetooth  
  
# 扫描附近的蓝牙设备  
nearby_devices = bluetooth.discover_devices(duration=8, lookup_names=True)  
  
# 将元组列表转换为字典列表  
devices_info = [{'name': name, 'address': addr} for addr, name in nearby_devices]  
  
# 打印设备列表  
for device in devices_info:  
    print(f"设备名称: {device['name']}, 物理地址: {device['address']}")  
  
# 发送信息到 VPS  
vps_url = "http://your-vps-url/api/bluetooth_data"  
data = {"devices": devices_info}  
response = requests.post(vps_url, json=data)  
  
print("数据发送状态:", response.status_code)