#!/bin/bash

set -e  # 启用错误退出

# 显示帮助信息的函数
print_help() {
    echo "使用之前，先将自己的公钥复制到两个远程地址上面"
    echo "ssh-keygen -t rsa -b 4096 -C "your personal information""
    echo "ssh-copy-id -i ~/.ssh/id_rsa.pub REMOTE_USER1@REMOTE_HOST1"
    echo "ssh-copy-id -i ~/.ssh/id_rsa.pub REMOTE_USER2@REMOTE_HOST2"
    echo "Usage: $0 <filecount>"
    exit 0
}
# 解析传入的命令行参数
while getopts ":h" opt; do
    case $opt in
        h)
            print_help  # 打印帮助信息并退出
            ;;
        \?)
            echo "Invalid option: -$OPTARG"  # 错误选项
            print_help
            ;;
    esac
done

# 检查是否传入了参数
if [ $# -lt 1 ]; then
    echo "Usage: $0 <filecount>"
    exit 1
fi

# 获取传入的第一个参数
FILE_COUNT="$1"  # 获取传入的文件名参数

# rb服务器生成hex
# 定义变量
REMOTE_USER="ruanbo"  # 远程主机的用户名
REMOTE_HOST="10.0.0.17"   # 远程主机的IP地址或主机名
REMOTE_PATH="/home/ruanbo/venus_soc/0704/venus_soc/software/sram"   # 远程主机的路径名
REMOTE_PORT=22               # SSH端口，默认是22
SSH_KEY="~/.ssh/id_rsa"      # 私钥路径，如果使用密码登录，可以忽略这行
COMMANDS="
cd $REMOTE_PATH
make gen_hex hex_name=flash_l1_$FILE_COUNT
echo "make_hex_done"
" # 要在远程主机上执行的命令

make -B venus
scp l1.bin "$REMOTE_USER@$REMOTE_HOST:$REMOTE_PATH"
# 使用 SSH 连接到远程主机并执行命令
ssh -i "$SSH_KEY" -p "$REMOTE_PORT" "$REMOTE_USER@$REMOTE_HOST" <<EOF
$COMMANDS
EOF


# 笔记本烧录hex
# 定义变量
REMOTE_USER2="rb"  # 远程主机的用户名
REMOTE_HOST2="10.0.0.172"   # 远程主机的IP地址或主机名
COMMANDS2="
sudo su
cd /home/$REMOTE_USER2/ace/venus/flash
python3 main.py $FILE_COUNT
" # 要在远程主机上执行的命令

ssh -i "$SSH_KEY" -p "$REMOTE_PORT" "$REMOTE_USER2@$REMOTE_HOST2" <<EOF
$COMMANDS2
EOF