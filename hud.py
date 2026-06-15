import os
import sys
import time
import json
from datetime import datetime

# ============================================================================
# Helper Functions
# ============================================================================

def parse_simple_yaml(filepath):
    """
    极简 YAML 解析器，专为解析本项目的 state.yaml 设计，避免第三方库依赖。
    """
    if not os.path.exists(filepath):
        return None
    
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()
        
    data = {"concepts": {}}
    current_concept = None
    in_concepts = False
    
    for line in content.splitlines():
        stripped = line.strip()
        if not stripped or stripped.startswith('#'):
            continue
            
        # 检测是否进入 concepts 块
        if stripped.startswith('concepts:'):
            in_concepts = True
            continue
            
        if in_concepts:
            # 检测是否为新概念声明 (形如 "什么是RTOS": 或 '什么是RTOS':)
            if stripped.endswith(':'):
                current_concept = stripped[:-1].strip().strip('"').strip("'")
                data["concepts"][current_concept] = {}
            # 检测属性键值对 (形如 status: in_progress)
            elif current_concept and ':' in stripped:
                key, val = stripped.split(':', 1)
                key = key.strip()
                val = val.strip().strip('"').strip("'")
                # 类型转换
                if val == 'null':
                    val = None
                elif val == 'true':
                    val = True
                elif val == 'false':
                    val = False
                else:
                    try:
                        if '.' in val:
                            val = float(val)
                        else:
                            val = int(val)
                    except ValueError:
                        pass
                data["concepts"][current_concept][key] = val
                
    return data

def get_latest_conversation_logs():
    """
    获取 antigravity-cli 最近一次会话的日志路径和 ID。
    """
    appdata_dir = os.path.join(os.path.expanduser('~'), '.gemini', 'antigravity-cli', 'brain')
    if not os.path.exists(appdata_dir):
        return None, None
        
    # 获取修改时间最晚的文件夹（代表当前会话）
    subdirs = [os.path.join(appdata_dir, d) for d in os.listdir(appdata_dir)]
    subdirs = [d for d in subdirs if os.path.isdir(d)]
    
    if not subdirs:
        return None, None
        
    latest_dir = max(subdirs, key=os.path.getmtime)
    conv_id = os.path.basename(latest_dir)
    log_file = os.path.join(latest_dir, '.system_generated', 'logs', 'transcript.jsonl')
    
    return conv_id, log_file

def parse_conversation_stats(log_file):
    """
    解析 transcript.jsonl 获取当前会话统计。
    """
    if not log_file or not os.path.exists(log_file):
        return {"turns": 0, "tool_calls": 0, "size_kb": 0.0}
        
    turns = 0
    tool_calls = 0
    size_bytes = os.path.getsize(log_file)
    
    with open(log_file, 'r', encoding='utf-8') as f:
        for line in f:
            try:
                data = json.loads(line)
                # 统计用户输入次数（代表对话轮数）
                if data.get("type") == "USER_INPUT":
                    turns += 1
                # 统计模型工具调用次数
                if data.get("type") == "PLANNER_RESPONSE" and "tool_calls" in data:
                    tool_calls += len(data["tool_calls"])
            except json.JSONDecodeError:
                continue
                
    return {
        "turns": turns,
        "tool_calls": tool_calls,
        "size_kb": size_bytes / 1024.0
    }

# ============================================================================
# Main HUD Renderer
# ============================================================================

def clear_screen():
    os.system('cls' if os.name == 'nt' else 'clear')

def main():
    # 强制设置控制台输出为 UTF-8 编码以支持 Emoji 和中文
    if hasattr(sys.stdout, 'reconfigure'):
        sys.stdout.reconfigure(encoding='utf-8', errors='replace')
        
    state_file = os.path.join('.learn', 'topics', 'freertos', 'state.yaml')
    
    # ANSI 颜色转义字符
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    CYAN = '\033[96m'
    RED = '\033[91m'
    BOLD = '\033[1m'
    RESET = '\033[0m'
    
    # 在某些终端需要初始化以防乱码
    if os.name == 'nt':
        os.system('') # 启用 Windows 虚拟终端 ANSI 颜色转义

    try:
        while True:
            clear_screen()
            conv_id, log_file = get_latest_conversation_logs()
            stats = parse_conversation_stats(log_file)
            state_data = parse_simple_yaml(state_file)
            
            now_str = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
            
            print(f"{BOLD}{BLUE}======================================================================{RESET}")
            print(f"               🛸  {BOLD}{CYAN}ANTIGRAVITY CLI - ACTIVE HUD{RESET}  🛸")
            print(f"{BOLD}{BLUE}======================================================================{RESET}")
            
            # 1. 会话状态面板
            if conv_id:
                print(f"{BOLD}Active Session:{RESET} {GREEN}{conv_id}{RESET}")
                print(f" ├─ {BOLD}Chat Turns{RESET}   : {stats['turns']} turns")
                print(f" ├─ {BOLD}Tool Execs{RESET}   : {stats['tool_calls']} calls")
                print(f" └─ {BOLD}Session Size{RESET} : {stats['size_kb']:.2f} KB (Context Approx)")
            else:
                print(f"{RED}No active antigravity session found.{RESET}")
                
            print(f"{BOLD}{BLUE}----------------------------------------------------------------------{RESET}")
            
            # 2. 学习进度面板
            if state_data:
                concepts = state_data.get("concepts", {})
                total = len(concepts)
                
                mastered = [c for c, detail in concepts.items() if detail.get("status") == "mastered"]
                in_progress = [c for c, detail in concepts.items() if detail.get("status") == "in_progress"]
                unexplored = [c for c, detail in concepts.items() if detail.get("status") == "unexplored"]
                needs_practice = [c for c, detail in concepts.items() if detail.get("status") == "needs_practice"]
                
                mastered_count = len(mastered)
                active_count = len(in_progress)
                unexplored_count = len(unexplored)
                needs_practice_count = len(needs_practice)
                
                percent = (mastered_count / total * 100) if total > 0 else 0
                bar_size = 20
                filled = int(percent / 100 * bar_size)
                bar = '█' * filled + '░' * (bar_size - filled)
                
                print(f"{BOLD}Learning Topic:{RESET} {YELLOW}FreeRTOS{RESET}")
                print(f" ├─ {BOLD}Progress{RESET}   : [{GREEN}{bar}{RESET}] {BOLD}{percent:.1f}%{RESET} ({mastered_count}/{total} mastered)")
                print(f" ├─ {BOLD}Mastered{RESET}   : {GREEN}{mastered_count}{RESET} concepts")
                print(f" ├─ {BOLD}Active{RESET}     : {YELLOW}{active_count}{RESET} concepts")
                print(f" └─ {BOLD}Unexplored{RESET} : {unexplored_count} concepts")
                
                # 3. 间隔重复和待练提示
                reminders = []
                today = datetime.now().date()
                
                # 收录需要练习或正在攻克但时间久远的内容
                for name, detail in concepts.items():
                    status = detail.get("status")
                    last_practice_str = detail.get("last_practice")
                    confidence = detail.get("confidence", 0.0)
                    
                    if status in ["in_progress", "needs_practice"]:
                        days_ago = ""
                        if last_practice_str:
                            try:
                                lp_date = datetime.strptime(last_practice_str, '%Y-%m-%d').date()
                                diff = (today - lp_date).days
                                days_ago = f"({diff} days ago)"
                            except ValueError:
                                pass
                        reminders.append((name, status, confidence, days_ago))
                
                if reminders:
                    print(f"{BOLD}{BLUE}----------------------------------------------------------------------{RESET}")
                    print(f"{BOLD}{YELLOW}[SPACED REPETITION REMINDERS]{RESET}")
                    # 按掌握置信度由低到高排序，提醒先复习弱项
                    reminders.sort(key=lambda x: x[2])
                    for r_name, r_status, r_conf, r_days in reminders[:4]: # 最多显示 4 条
                        status_tag = f"{RED}[Need Practice]{RESET}" if r_status == "needs_practice" else f"{YELLOW}[In Progress]{RESET}"
                        print(f" ⚠️  {BOLD}{r_name}{RESET} {status_tag} - Conf: {r_conf*100:.0f}% {r_days}")
            else:
                print(f"{RED}No state.yaml learning record found in .learn/topics/freertos/.{RESET}")
                
            print(f"{BOLD}{BLUE}======================================================================{RESET}")
            print(f"Refreshed at: {now_str} (Press {BOLD}Ctrl+C{RESET} to exit)")
            
            time.sleep(2)
    except KeyboardInterrupt:
        print("\nHUD monitoring stopped. Bye!")
        sys.exit(0)

if __name__ == '__main__':
    main()
