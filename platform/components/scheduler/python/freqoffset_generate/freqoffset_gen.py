import math

# 定义频率偏移数组，单位为kHz
freq_offset_khz = [0, 7.5, -7.5, 15, -15, 22.5, -22.5, 30, -30, 37.5, -37.5, 45, -45]
# freq_offset_khz = [7.5]

# 基准频率
base_freq = 245.76e6

# 定义计算NCO步长的函数
def calculate_nco_step(freq_khz, base_freq):
    # 将频率偏移从 kHz 转换为 Hz
    freq_hz = freq_khz * 1e3
    # freq = freq_hz / 2.0
    normalized_freq = freq_hz / base_freq
    scaled_freq = normalized_freq * pow(2.0, 48.0)
    nco_step = math.floor(scaled_freq + 0.5)
    return nco_step


nco_steps = [calculate_nco_step(offset, base_freq) for offset in freq_offset_khz]
output_str = "long long freq_offset[] = {" + ", ".join(map(str, nco_steps)) + "};"
with open("nco_steps.txt", "w") as f:
    f.write(output_str)


# # 计算并打印每个频率偏移的结果
# for offset_khz in freq_offset_khz:
#     nco_step = calculate_nco_step(offset_khz, base_freq)
#     print(f"Frequency offset: {offset_khz} kHz, NCO Step: {nco_step}")
