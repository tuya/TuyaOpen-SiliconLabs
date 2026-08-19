#!/usr/bin/env bash
# SiWx917 串口 ISP / Kermit 链路验证工具
#
#   ./siwx917_kermit.sh ports                     列出串口并标出是哪块板
#   ./siwx917_kermit.sh raw   [口] [波特率]        纯监听：只收不发
#   ./siwx917_kermit.sh probe [口] [波特率]        只读探测：握手 + 打印 bootloader 菜单
#   ./siwx917_kermit.sh menu  [口] [波特率]        菜单对话：走到"等待传输"，打印设备每句话
#   ./siwx917_kermit.sh menu-ta [口] [波特率]      同 menu，但选 'B'(烧 NWP/TA)，只观察不传文件
#   ./siwx917_kermit.sh check [口] [波特率]        只读：逐槽(0-f)校验无线固件完整性('K')
#   ./siwx917_kermit.sh select-ta <槽0-f> [口] [波特率]  把某槽设为默认无线固件(会写一点 flash)
#   ./siwx917_kermit.sh send  <file> [口] [波特率] 完整烧录：握手 + 选菜单 + Kermit 传输
#
# 口可以只写 ttyUSB0，不用带 /dev/。波特率默认 115200。
#   ./siwx917_kermit.sh probe ttyUSB0 921600
# 也支持环境变量（必须放命令前面）：PORT=/dev/ttyUSB0 BAUD=921600 ./siwx917_kermit.sh probe
#
# 官方文档两种说法，都要试：
#   AN1431      : 115200，走 GPIO_8/GPIO_9 专用 ISP UART
#   Matter 文档 : 921600，走 JLink CDC 口
#
# 注意：BRD2605A（DK2605A）的 VCOM（ttyACM*）是控制台口，不是 ISP 口，
#       对着它握手一定是 0 字节。要测它得跟 AI 板一样接 GPIO_8/9 + 拉低 GPIO_34。
#
# 所有交互记到 ./siwx917-logs/ 下，跑完把日志发我。
set -u

LOGDIR="$(cd "$(dirname "$0")" && pwd)/siwx917-logs"
STAMP="$(date +%Y%m%d-%H%M%S)"

usage() { sed -n '2,24p' "$0" | sed 's/^# \{0,1\}//'; exit 1; }

# 允许 ttyUSB0 / /dev/ttyUSB0 两种写法
norm_port() {
    case "$1" in
        /dev/*) echo "$1" ;;
        *)      echo "/dev/$1" ;;
    esac
}

list_ports() {
    local found=0
    for p in /dev/ttyUSB* /dev/ttyACM*; do
        [ -c "$p" ] || continue
        found=1
        local vid mdl ifn
        vid=$(udevadm info -q property -n "$p" 2>/dev/null | sed -n 's/^ID_VENDOR_ID=//p')
        mdl=$(udevadm info -q property -n "$p" 2>/dev/null | sed -n 's/^ID_MODEL=//p')
        ifn=$(udevadm info -q property -n "$p" 2>/dev/null | sed -n 's/^ID_USB_INTERFACE_NUM=//p')
        local tag=""
        case "$vid" in
            1366) tag="  <- SEGGER J-Link（BRD2605A 板载调试器的 VCOM，是控制台口不是 ISP 口）" ;;
            1a86) tag="  <- CH34x 串口芯片" ;;
            10c4) tag="  <- CP210x 串口芯片" ;;
            0403) tag="  <- FTDI 串口芯片" ;;
        esac
        printf "  %-16s %s (%s) 接口%s%s\n" "$p" "${mdl:-?}" "${vid:-?}" "${ifn:-?}" "$tag"
    done
    [ "$found" = 0 ] && echo "  （没有串口设备，板子插了吗？）"
}

pick_port() {
    # $1 = 用户给的口（可能为空）
    local given="${1:-${PORT:-}}"
    if [ -n "$given" ]; then
        norm_port "$given"
        return
    fi
    local cands=()
    for p in /dev/ttyUSB* /dev/ttyACM*; do
        [ -c "$p" ] && cands+=("$p")
    done
    if [ "${#cands[@]}" = 1 ]; then
        echo "${cands[0]}"
    else
        echo ""
    fi
}

check_port() {
    [ -c "$1" ] || { echo "!! 串口 $1 不存在"; echo ""; echo "当前可用："; list_ports; exit 1; }
    if command -v lsof >/dev/null && lsof "$1" >/dev/null 2>&1; then
        echo "!! $1 被占用，先停掉占用它的程序："
        lsof "$1"
        exit 1
    fi
}

need_port() {
    [ -n "$1" ] || {
        echo "!! 没指定串口，而且检测到多个（或没有）。当前："
        echo ""
        list_ports
        echo ""
        echo "指定一个：  $0 $2 ttyUSB0"
        exit 1
    }
}

need_kermit() {
    command -v kermit >/dev/null || {
        echo "!! 没装 C-Kermit：  sudo apt install ckermit"
        exit 1
    }
}

isp_hint() {
    cat <<'EOF'
==============================================================
 让板子进 ISP 模式（二选一）：

   有 ISP 键：  按住 ISP 键 → 点 Reset → 松开 ISP 键
   没有 ISP 键：把 GPIO_34 (BOOT_MODE) 拉到 GND，再点 Reset

 依据 AN1516：GPIO_34 在复位期间拉低即进 ISP，
 bootloader 在 GPIO_8(RX)/GPIO_9(TX) @115200 上应答。

 进好之后按回车继续。
==============================================================
EOF
    read -r _
}

mkdir -p "$LOGDIR"
mode="${1:-}"
[ -z "$mode" ] && usage

# ---------- ports ----------
if [ "$mode" = "ports" ]; then
    echo "当前串口设备："
    list_ports
    exit 0
fi

# ---------- 参数：send 的文件、select-ta 的槽号在前，口和波特率在后 ----------
if [ "$mode" = "send" ]; then
    file="${2:-}"
    [ -z "$file" ] && { echo "!! 要给文件：$0 send xxx.rps [口] [波特率]"; exit 1; }
    [ -f "$file" ] || { echo "!! 找不到 $file"; exit 1; }
    PORT_ARG="${3:-}"
    BAUD="${4:-${BAUD:-115200}}"
elif [ "$mode" = "select-ta" ]; then
    slot="${2:-}"
    case "$slot" in
        [0-9a-f]) ;;
        *) echo "!! 槽号要是 0-f 里的一个：$0 select-ta 0 [口] [波特率]"; exit 1 ;;
    esac
    PORT_ARG="${3:-}"
    BAUD="${4:-${BAUD:-115200}}"
else
    PORT_ARG="${2:-}"
    BAUD="${3:-${BAUD:-115200}}"
fi

PORT="$(pick_port "$PORT_ARG")"
need_port "$PORT" "$mode"
check_port "$PORT"

# ---------- raw：纯监听 ----------
if [ "$mode" = "raw" ]; then
    log="$LOGDIR/raw-$STAMP.log"
    echo ">> 监听 $PORT @ $BAUD，15 秒。现在点一下 Reset。"
    echo "   日志: $log"
    python3 - "$PORT" "$BAUD" "$log" <<'PY'
import sys, time, serial
port, baud, logpath = sys.argv[1], int(sys.argv[2]), sys.argv[3]
s = serial.Serial(port, baud, timeout=0.3)
buf = b""
end = time.time() + 15
while time.time() < end:
    d = s.read(4096)
    if d:
        buf += d
        sys.stdout.buffer.write(d); sys.stdout.buffer.flush()
s.close()
open(logpath, "wb").write(buf)
print(f"\n{'='*62}\n共收到 {len(buf)} 字节，已存 {logpath}")
PY
    exit 0
fi

# ---------- probe：只读探测握手 ----------
if [ "$mode" = "probe" ]; then
    log="$LOGDIR/probe-$STAMP.log"
    isp_hint
    echo ">> 探测 $PORT @ $BAUD    日志: $log"
    python3 - "$PORT" "$BAUD" "$log" <<'PY'
import sys, time, serial

port, baud, logpath = sys.argv[1], int(sys.argv[2]), sys.argv[3]
out = open(logpath, "w", encoding="utf-8")

def rec(msg=""):
    print(msg); out.write(msg + "\n"); out.flush()

def dump(tag, data):
    """文本和 hex 都打，菜单里可能有不可见控制字符。"""
    rec(f"--- {tag}: {len(data)} 字节")
    if not data:
        rec("    (空)")
        return
    rec("    TEXT: " + repr(data.decode("utf-8", errors="replace")))
    for i in range(0, min(len(data), 512), 16):
        chunk = data[i:i+16]
        hexs = " ".join(f"{b:02x}" for b in chunk)
        txt = "".join(chr(b) if 32 <= b < 127 else "." for b in chunk)
        rec(f"    {i:04x}  {hexs:<47}  {txt}")

def read_for(s, secs):
    buf = b""
    end = time.time() + secs
    while time.time() < end:
        d = s.read(4096)
        if d:
            buf += d
            end = time.time() + 0.5     # 还在出数据就顺延
    return buf

# 一次进 ISP 只有一次机会，所以把候选波特率全扫一遍再退出
bauds = [baud] + [b for b in (115200, 921600, 460800, 9600) if b != baud]
got = b""

for bd in bauds:
    rec("\n" + "=" * 62)
    rec(f"@@ 波特率 {bd}")
    try:
        s = serial.Serial(port, bd, timeout=0.2)
    except Exception as e:
        rec(f"    打开失败: {e}")
        continue

    s.reset_input_buffer()
    dump("开场白（不发任何东西，先听 1 秒）", read_for(s, 1.0))

    # 官方文档：Ctrl+| —— '|'=0x7C，取低 5 位得 0x1C
    rec(">> 发 0x1C (Ctrl+|)")
    s.write(b"\x1c"); s.flush()
    r1 = read_for(s, 2.0)
    dump("0x1C 的回应（文档说应该收到 'U'）", r1)

    rec(">> 发 'U'")
    s.write(b"U"); s.flush()
    r2 = read_for(s, 3.0)
    dump("'U' 的回应（应该是 bootloader 菜单）", r2)

    # 这个波特率全无反应，换几种引导字符兜底
    if not r1 and not r2:
        for name, b in [("0x1B ESC", b"\x1b"), ("回车", b"\r\n"), ("'U' 单独", b"U")]:
            rec(f">> 兜底：发 {name}")
            s.reset_input_buffer()
            s.write(b); s.flush()
            extra = read_for(s, 1.5)
            dump(f"{name} 的回应", extra)
            r2 += extra

    s.close()
    got += r1 + r2
    # 全 0x00 / 全 0xff 是波特率不匹配或悬空线的噪声，不算应答，继续扫下一个波特率
    real = bytes(b for b in (r1 + r2) if b not in (0x00, 0xff))
    if real:
        rec(f"\n>> {bd} 有实质应答，停止扫描")
        break
    if r1 or r2:
        rec(f">> {bd} 只收到 {len(r1+r2)} 字节噪声（全 00/ff），继续扫")

rec("\n" + "=" * 62)
menu = got.decode("utf-8", errors="replace")
if not got:
    rec("判断: 所有波特率都零字节 —— 没进 ISP / TX-RX 接反 / GPIO_8-9 接错")
elif "tuyaos" in menu or "ty D]" in menu or "ty I]" in menu:
    rec("判断: 收到的是 TuyaOpen 应用日志 —— 还接在日志口上，不是 ISP 口")
elif "U" in menu or "oot" in menu:
    rec("判断: 像 bootloader 应答，把日志发我")
else:
    rec("判断: 收到数据但认不出来，把日志发我")
rec(f"日志: {logpath}")
out.close()
PY
    exit 0
fi

# ---------- menu：走到"等待 Kermit 传输"那一步，把设备的每句话打出来 ----------
if [ "$mode" = "menu" ]; then
    log="$LOGDIR/menu-$STAMP.log"
    isp_hint
    echo ">> 菜单交互 $PORT @ $BAUD    日志: $log"
    python3 - "$PORT" "$BAUD" "$log" <<'PY'
import sys, time, serial

port, baud, logpath = sys.argv[1], int(sys.argv[2]), sys.argv[3]
out = open(logpath, "w", encoding="utf-8")

def rec(m=""):
    print(m); out.write(m + "\n"); out.flush()

def dump(tag, d):
    rec(f"--- {tag}: {len(d)} 字节")
    if not d:
        rec("    (空)")
        return
    rec("    TEXT: " + repr(d.decode("utf-8", errors="replace")))
    for i in range(0, min(len(d), 256), 16):
        c = d[i:i+16]
        rec(f"    {i:04x}  {' '.join(f'{b:02x}' for b in c):<47}  "
            + "".join(chr(b) if 32 <= b < 127 else '.' for b in c))

def rd(s, t=2.0):
    buf = b""
    end = time.time() + t
    while time.time() < end:
        x = s.read(4096)
        if x:
            buf += x
            end = time.time() + 0.4
    return buf

s = serial.Serial(port, baud, timeout=0.2)
s.reset_input_buffer()

rec(">> 0x1C")
s.write(b"\x1c"); s.flush()
dump("回应", rd(s, 2.0))

rec("\n>> 'U'")
s.write(b"U"); s.flush()
dump("菜单", rd(s, 3.0))

rec("\n>> '4'  (Burn M4 Firmware)")
s.write(b"4"); s.flush()
dump("选 4 之后设备说什么", rd(s, 3.0))

rec("\n>> '1'  (Image No 1)")
s.write(b"1"); s.flush()
dump("选 1 之后设备说什么（这里应该开始等 Kermit）", rd(s, 5.0))

rec("\n>> 再静听 5 秒，看它是否周期性发 NAK 催促")
dump("静听", rd(s, 5.0))

s.close()
rec("\n" + "=" * 62)
rec("设备现在停在'等传输'状态。要接着烧就直接跑 send（不用再进一次 ISP）。")
rec(f"日志: {logpath}")
out.close()
PY
    exit 0
fi

# ---------- menu-ta：探 'B'(烧 NWP/TA) 之后设备要什么 ----------
# 目的：menu 模式只驱动过 '4'->'1'（烧 M4），'B' 之后设备是直接等 Kermit 传输、
#       还是也要先选一个 image number，从来没人测过。这里只发 'B' 然后纯观察，
#       不再发任何字节、更不传文件——没有数据传过去就不会写 flash。
if [ "$mode" = "menu-ta" ]; then
    log="$LOGDIR/menu-ta-$STAMP.log"
    isp_hint
    echo ">> TA/NWP 菜单探测 $PORT @ $BAUD    日志: $log"
    python3 - "$PORT" "$BAUD" "$log" <<'PY'
import sys, time, serial

port, baud, logpath = sys.argv[1], int(sys.argv[2]), sys.argv[3]
out = open(logpath, "w", encoding="utf-8")

def rec(m=""):
    print(m); out.write(m + "\n"); out.flush()

def dump(tag, d):
    rec(f"--- {tag}: {len(d)} 字节")
    if not d:
        rec("    (空)")
        return
    rec("    TEXT: " + repr(d.decode("utf-8", errors="replace")))
    for i in range(0, min(len(d), 512), 16):
        c = d[i:i+16]
        rec(f"    {i:04x}  {' '.join(f'{b:02x}' for b in c):<47}  "
            + "".join(chr(b) if 32 <= b < 127 else '.' for b in c))

def rd(s, t=2.0):
    buf = b""
    end = time.time() + t
    while time.time() < end:
        x = s.read(4096)
        if x:
            buf += x
            end = time.time() + 0.4
    return buf

s = serial.Serial(port, baud, timeout=0.2)
s.reset_input_buffer()

rec(">> 0x1C")
s.write(b"\x1c"); s.flush()
dump("回应", rd(s, 2.0))

rec("\n>> 'U'  (打印菜单——这段最关键，要看清 'B' 那一项到底叫什么)")
s.write(b"U"); s.flush()
menu = rd(s, 4.0)
dump("完整菜单", menu)

rec("\n>> 'B'  (SDK 里 BURN_NWP_FW = 'B'，烧 NWP/TA 无线固件)")
s.write(b"B"); s.flush()
after_b = rd(s, 5.0)
dump("选 B 之后设备说什么", after_b)

rec("\n>> 再静听 8 秒（看它是否周期性发 NAK 催你传文件）")
tail = rd(s, 8.0)
dump("静听", tail)

s.close()

# 判读：Kermit 接收方在等文件时会周期性发 NAK 包(0x01 开头、类型 'N')或裸 'N'/'C'
blob = after_b + tail
rec("\n" + "=" * 62)
if not after_b and not tail:
    rec("判断: 'B' 之后零字节 —— 可能这版 bootloader 菜单里没有 'B' 这一项")
elif b"\x01" in blob and b"N" in blob:
    rec("判断: 像是已经在等 Kermit 传输了（收到疑似 NAK 包）")
    rec("      => 'B' 后面不需要再选 image number，直接可以传文件")
elif any(w in blob.lower() for w in (b"image", b"no", b"enter", b"select")):
    rec("判断: 设备像是在追问参数（文本里有 image/no/enter/select 字样）")
    rec("      => 'B' 后面还要再答一个值，看上面 TEXT 里它具体问的是什么")
else:
    rec("判断: 收到数据但认不出来，把日志发我")
rec("\n设备可能停在'等传输'状态；没传任何文件 => flash 没被改写。")
rec("断电重上电即可恢复正常启动。")
rec(f"日志: {logpath}")
out.close()
PY
    exit 0
fi

# ---------- check：只读，逐槽校验无线固件完整性 ----------
# menu-ta 的日志抓到了完整菜单，里面有
#   K Check Wireless Firmware Integrity (Image No : 0-f)
# 这是让 bootloader 自己去校验已存固件，不写 flash。值得单独做一个模式，因为
# mfg917 info 报的 nwp_firmware_version 只是存着的元数据 —— 实测遇到过它报得出
# 版本、而应用起来仍然 VALID_FIRMWARE_NOT_PRESENT(0x16056) 的板子。'K' 是目前
# 唯一能区分"存着"和"存着且完好"的手段。
#
# 无线固件有 16 个槽(0-f)，所以全跑一遍：万一 0 号槽坏了而别的槽完好，用菜单里
# 的 '5 Select Default Wireless Firmware' 改个指针就行，不必重传 1.6 MB。
if [ "$mode" = "check" ]; then
    log="$LOGDIR/check-$STAMP.log"
    isp_hint
    echo ">> 无线固件完整性校验（只读）$PORT @ $BAUD    日志: $log"
    python3 - "$PORT" "$BAUD" "$log" <<'PY'
import sys, time, serial

port, baud, logpath = sys.argv[1], int(sys.argv[2]), sys.argv[3]
out = open(logpath, "w", encoding="utf-8")

def rec(m=""):
    print(m); out.write(m + "\n"); out.flush()

def rd(s, t=2.0, quiet=0.4):
    buf = b""
    end = time.time() + t
    while time.time() < end:
        x = s.read(4096)
        if x:
            buf += x
            end = time.time() + quiet
    return buf

s = serial.Serial(port, baud, timeout=0.2)
s.reset_input_buffer()

# 唤醒 + 取菜单，确认真的在 bootloader 里，否则后面发的字节没有意义
s.write(b"\x1c"); s.flush(); rd(s, 2.0)
s.write(b"U"); s.flush()
menu = rd(s, 4.0)
if b"BootLoader" not in menu:
    rec("!! 没拿到 bootloader 菜单，收到: "
        + repr(menu.decode("utf-8", "replace")[:200]))
    rec("   板子进 ISP 模式了吗？（GPIO_34 拉低后点 Reset）接的是 GPIO_8/9 吗？")
    out.close(); s.close(); sys.exit(1)
rec("菜单已拿到，BootLoader 在线。开始逐槽校验。\n")

verdict = {}
for slot in "0123456789abcdef":
    s.write(b"U"); s.flush(); rd(s, 1.5)      # 回到已知状态
    s.write(b"K"); s.flush()
    prompt = rd(s, 3.0)
    s.write(slot.encode()); s.flush()
    # 校验 1.6 MB 可能要好几秒，给足时间再判空
    body = rd(s, 15.0, quiet=1.5)
    text = (prompt + body).decode("utf-8", "replace")
    rec(f"--- 槽 {slot} ---")
    rec("    " + repr(text))
    low = text.lower()
    if not body:
        v = "无回应"
    elif any(w in low for w in ("success", "valid", "pass", "ok", "good")):
        v = "完好"
    elif any(w in low for w in ("fail", "invalid", "corrupt", "error", "not present")):
        v = "损坏/为空"
    else:
        v = "无法判读"
    verdict[slot] = v
    rec(f"    => {v}\n")

s.close()
rec("=" * 62)
rec("汇总: " + "  ".join(f"{k}:{v}" for k, v in verdict.items()))
good = [k for k, v in verdict.items() if v == "完好"]
if good:
    rec(f"有完好的槽: {good} —— 可用菜单 '5 Select Default Wireless Firmware' 指过去，")
    rec("            比重传固件代价小得多。")
else:
    rec("没有任何槽报完好 —— 需要 'B' + 槽号 + Kermit 传固件。")
rec("\n本模式只发 U/K 和槽号，没有传输任何文件 => flash 未被改写。")
rec(f"日志: {logpath}")
out.close()
PY
    exit 0
fi

# ---------- select-ta：把某个槽设为默认无线固件 ----------
# 实测过一块板子：check 报槽 0 'Integrity Passed'，mfg917 info 也报得出
# nwp_firmware_version，可应用启动仍然 VALID_FIRMWARE_NOT_PRESENT(0x16056)。镜像
# 在、且完好，却没有被加载 —— 指向"默认无线固件"的那个选择记录坏了。菜单里
#   5 Select Default Wireless Firmware (Image No : 0-f)
# 就是重设它。写入量是一个选择器而不是 1.6 MB，而且随时可以再选别的槽，是这一族
# 问题里代价最小的一步，值得在传固件之前先试。
#
# 这个模式会写 flash（虽然只有一点），不是只读的。
if [ "$mode" = "select-ta" ]; then
    log="$LOGDIR/select-ta-$STAMP.log"
    isp_hint
    echo ">> 把槽 $slot 设为默认无线固件  $PORT @ $BAUD    日志: $log"
    python3 - "$PORT" "$BAUD" "$log" "$slot" <<'PY'
import sys, time, serial

port, baud, logpath, slot = sys.argv[1], int(sys.argv[2]), sys.argv[3], sys.argv[4]
out = open(logpath, "w", encoding="utf-8")

def rec(m=""):
    print(m); out.write(m + "\n"); out.flush()

def rd(s, t=3.0, quiet=0.5):
    buf = b""
    end = time.time() + t
    while time.time() < end:
        x = s.read(4096)
        if x:
            buf += x
            end = time.time() + quiet
    return buf

s = serial.Serial(port, baud, timeout=0.2)
s.reset_input_buffer()

s.write(b"\x1c"); s.flush(); rd(s, 2.0)
s.write(b"U"); s.flush()
menu = rd(s, 4.0)
if b"BootLoader" not in menu:
    rec("!! 没拿到 bootloader 菜单，收到: "
        + repr(menu.decode("utf-8", "replace")[:200]))
    rec("   板子进 ISP 模式了吗？接的是 GPIO_8/9 吗？")
    out.close(); s.close(); sys.exit(1)

# 先确认要指过去的槽真的完好，别把默认指到一个坏槽上
s.write(b"K"); s.flush(); rd(s, 3.0)
s.write(slot.encode()); s.flush()
chk = rd(s, 15.0, quiet=1.5).decode("utf-8", "replace")
rec(f"槽 {slot} 完整性: {chk!r}")
if "Passed" not in chk:
    rec(f"!! 槽 {slot} 完整性没通过，不把默认指过去。先用 send 把固件传进这个槽。")
    out.close(); s.close(); sys.exit(1)

rec(f"\n>> '5' 然后 '{slot}'")
s.write(b"U"); s.flush(); rd(s, 1.5)
s.write(b"5"); s.flush()
prompt = rd(s, 3.0)
rec("    提示: " + repr(prompt.decode("utf-8", "replace")))
s.write(slot.encode()); s.flush()
resp = rd(s, 10.0, quiet=1.0)
rec("    应答: " + repr(resp.decode("utf-8", "replace")))

s.close()
rec("\n" + "=" * 62)
rec("做完了。断电重上电（松开 GPIO_34，让它正常启动），然后看应用日志：")
rec("  WiFi initialization success + Running TA fw:  => 修好了")
rec("  还是 16056 / 16059                            => 指针不是病因，回来传固件")
rec(f"日志: {logpath}")
out.close()
PY
    exit 0
fi

# ---------- send：完整烧录 ----------
if [ "$mode" = "send" ]; then
    need_kermit

    ksc="$LOGDIR/send-$STAMP.ksc"
    session="$LOGDIR/session-$STAMP.log"
    packets="$LOGDIR/packets-$STAMP.log"
    trans="$LOGDIR/trans-$STAMP.log"

    # packets 日志最关键：里面有 Send-Init 协商出的 MAXL / CAPAS / CHKT，
    # 直接决定 Rust 那边要不要实现长包，以及烧录要多久。
    cat > "$ksc" <<EOF
set line $PORT
set speed $BAUD
set carrier-watch off
set flow-control none
set handshake none
set parity none
set file type binary

; 设备 Send-Init 应答里 CAPAS=2（不支持长包/滑动窗口）、CHKT=1、MAXL=94。
; C-Kermit 默认会发扩展长度包，设备一律 NAK，所以锁死成短包停等。
set send packet-length 94
set receive packet-length 94
set window 1
set block-check 1
set retry 20

; 关键：C-Kermit 默认 PREFIXING CAUTIOUS，会把 0x0c/0x80 这类控制字符裸发，
; SiWx917 的 ROM Kermit 是最小实现，收到裸控制字符一律 NAK。强制全部转义。
set prefixing all

; 传输中键盘上任何杂散输入都会被当成中断字符（X 取消文件 / Z 取消批次），
; 上一次就是这么被打断的（零 NAK 却发了 Z-D 丢弃）。关掉。
set transfer cancellation off
set transfer display serial
set exit warning off

log session $session
log packets $packets
log transactions $trans

echo {>> 握手：发 Ctrl+| (0x1C)}
output \\{28}
pause 1

echo {>> 发 U 进菜单}
output U
pause 2

echo {>> 选 "4" -> "1"（烧 M4 应用）}
output 4
pause 1
output 1
pause 2

echo {>> 开始 Kermit 传输}
send $file

echo {>> 传完，静观设备打印什么（不再乱发字节）}
pause 8

close
exit
EOF

    isp_hint
    echo ">> 烧录 $file"
    echo "   串口 $PORT @ $BAUD"
    echo "   脚本 $ksc"
    echo " 传输中请勿按键。94 字节短包 + 全转义 + 停等，115200 下要跑十几分钟。"
    echo ""
    # stdin 接 /dev/null：彻底杜绝杂散按键被 C-Kermit 当成传输中断字符
    kermit "$ksc" < /dev/null
    rc=$?
    echo ""
    echo "=============================================================="
    echo " kermit 退出码: $rc"
    echo " 关键日志（协商参数在这里）: $packets"
    echo "=============================================================="
    exit $rc
fi

echo "未知模式: $mode"
usage
