#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
激励场景 4: 紧急急停按钮按下 (Hardware E-Stop) 测试向量
测试目标:
  - 在电机全速 2500 RPM 运转时注入紧急硬停信号 (estop = 1.0)
  - 触发 motor_controller_step 中的 E-Stop 快速熔断与动态能耗制动分支
  - 触发 fault_recorder 记录最高严重等级 DTC 0x008000 (FAULT_SEV_FATAL) 并使能系统安全闭锁
"""
import json

def generate_stimulus():
    steps = []
    total_steps = 30
    
    for i in range(total_steps):
        t_ms = i * 20.0
        speed_target = 2500.0
        temp_val = 50.0
        bus_volt = 3.3
        # 在第 10 步突然触发急停按键
        estop_flag = 1.0 if i >= 10 else 0.0
        
        steps.append({
            "timestampMs": t_ms,
            "signalValues": {
                "speed_cmd": speed_target,
                "temp": temp_val,
                "voltage": bus_volt,
                "estop": estop_flag
            },
            "rawPayload": f"ESTOP_TEST: step={i+1}, estop={int(estop_flag)} (Active=Emergency Halt)"
        })
        
    return {
        "name": "硬件急停信号注入激励 (Emergency E-Stop)",
        "targetModule": "motor_controller.c",
        "description": "注入最高优先级紧急硬停信号，验证安全制动与急停熔断分支",
        "durationMs": total_steps * 20.0,
        "signalNames": ["speed_cmd", "temp", "voltage", "estop"],
        "steps": steps
    }

if __name__ == "__main__":
    plan = generate_stimulus()
    print("[STIMULUS_START]")
    print(json.dumps(plan, indent=2))
    print("[STIMULUS_END]")
