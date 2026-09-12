import re
import matplotlib.pyplot as plt
import pandas as pd
import matplotlib.colors as mcolors
# 假设日志内容已经加载为字符串，或者可以从文件读取
# 读取日志文件
log_file = 'log.txt'
log_entries = []

# 正则表达式用于匹配 "Venus Lane Xbar Request logged" 行
pattern = re.compile(r"Venus Lane Xbar Request logged: Time = (\d+), Src_Port_ID = (\d+), Mem_Side_Port_ID = (\d+), Is_Collision = (\d)")

log_entries = []

# 读取文件并提取数据
with open(log_file, 'r') as file:
    for line in file:
        match = pattern.search(line)
        if match:
            time = int(match.group(1))  # 时间戳
            src_port_id = int(match.group(2))  # 源端口ID
            mem_side_port_id = int(match.group(3))  # 目标端口（银行）ID
            is_collision = int(match.group(4))  # 是否发生冲突
            log_entries.append([time, src_port_id, mem_side_port_id, is_collision])
# 将提取的数据转化为 Pandas DataFrame
df = pd.DataFrame(log_entries, columns=['Time', 'Src_Port_ID', 'Mem_Side_Port_ID', 'Is_Collision'])

# 查看提取的数据
print(df)

# 创建一个图形，设置大小
fig, ax = plt.subplots(figsize=(10, 6))
# 获取所有唯一的 Mem_Side_Port_ID
unique_banks = df['Mem_Side_Port_ID'].unique()

# 使用一个颜色字典将每个 Mem_Side_Port_ID 映射到一个颜色
colors = list(mcolors.TABLEAU_COLORS.values())  # 获取一些预设的颜色
color_dict = {bank: colors[i % len(colors)] for i, bank in enumerate(unique_banks)}
# 创建一个字典来记录同一时刻在同一银行上的条形数量
highlighted_bars = {}
for idx, row in df.iterrows():
    # 根据 Mem_Side_Port_ID 获取颜色
    bank_color = color_dict[row['Mem_Side_Port_ID']]
    
    # 检查是否有多个请求在同一时间发生在同一银行
    time_bank_key = (row['Time'], row['Mem_Side_Port_ID'])
    if time_bank_key not in highlighted_bars:
        highlighted_bars[time_bank_key] = 0
    highlighted_bars[time_bank_key] += 1
    
    # 如果同一时刻同一银行有多个请求，则高亮显示
    edgecolor = 'red' if highlighted_bars[time_bank_key] > 1 else 'none'
    # 用时间戳表示请求的开始时间，假设每个请求占用1个单位的时间
    ax.barh(row['Src_Port_ID'], 500, left=row['Time'], color=bank_color, edgecolor=edgecolor, linewidth=1)


# 设置时间轴格式
ax.set_xlabel('Time (Tick)')
ax.set_ylabel('Source Port ID')
ax.set_title('Request Timeline with Collision and Non-Collision Requests')

# 添加时间轴的格式
plt.xticks(rotation=45)

# 保存图表为 PNG 文件
plt.tight_layout()
plt.savefig('request_timeline.png')  # 保存为 PNG 文件


# 按时间汇总请求数和冲突数
request_count = df.groupby('Time').size()
collision_count = df[df['Is_Collision'] == 1].groupby('Time').size()

# 创建时间序列图
fig, ax = plt.subplots(figsize=(12, 6))

# 绘制请求数量
ax.plot(request_count.index, request_count.values, label='Total Requests', color='blue')

# 绘制冲突数量
ax.plot(collision_count.index, collision_count.values, label='Collisions', color='red')

ax.set_xlabel('Time (Tick)')
ax.set_ylabel('Request Count')
ax.set_title('Request and Collision Count Over Time')
ax.legend()
# 保存图表为 PNG 文件
plt.tight_layout()
plt.savefig('request_timeline_1.png')  # 保存为 PNG 文件