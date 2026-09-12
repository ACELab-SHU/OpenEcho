def process_lines(lines):
    result = []
    for line in lines:
        # 找到冒号的位置
        colon_pos = line.find(':')
        if (colon_pos == -1):
            continue  # 如果没有找到冒号，跳过这行

        # 获取冒号后面的数据并去掉空格
        data = line[colon_pos + 1:].strip()

        # 将数据每8个字符拆分开来
        chunks = [data[i:i + 8] for i in range(0, len(data), 8)]

        # 在前面加上0x后面加上逗号
        formatted_chunks = [f'0x{chunk},' for chunk in chunks]

        # 合并结果
        result.append(' '.join(formatted_chunks))

    return result

# 从文件中读取文本
with open('input.txt', 'r') as file:
    lines = file.readlines()

# 处理每一行
processed_lines = process_lines(lines)

# 将结果写入到输出文件
with open('output.txt', 'w') as file:
    for line in processed_lines:
        file.write(line + '\n')