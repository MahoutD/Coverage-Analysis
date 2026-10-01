#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
激励场景 3: 供电母线电压异常波动与欠压/过压保护测试向量
测试目标:
  - 模拟母线电压大幅波动：跌落至 1.8V (欠压 < 2.2V)，随后激增至 4.8V (过压 > 4.2V)
  - 触发 motor_check_safety 中的欠压闭锁与过压闭锁判定分支
  - 触发 fault_recorder 记录 DTC 0x001100
"""
import json

def generate_stimulus():
    steps = []
    total_steps = 30
    
    for i in range(total_steps):
        t_ms = i * 20.0
        speed_target = 1000.0
        temp_val = 35.0
        
        # 0~9 正常 3.3V，10~19 欠压跌落 1.8V，20~29 浪涌过压 4.6V
        if i < 10:
            bus_volt = 3.3
        elif i < 20:
            bus_volt = 1.8  # 触发欠压保护分支
        else:
            bus_volt = 4.6  # 触发过压保护分支
            
        steps.append({
            "timestampMs": t_ms,
            "signalValues": {
                "speed_cmd": speed_target,
                "temp": temp_val,
                "voltage": bus_volt,
                "estop": 0.0
            },
            "rawPayload": f"VOLT_TEST: step={i+1}, bus_volt={bus_volt:.2f}V (Normal: 2.2V ~ 4.2V)"
        })
        
    return {
        "name": "母线电压波动与欠压过压激励 (Voltage Fluctuation)",
        "targetModule": "motor_controller.c",
        "description": "电压低于 2.2V 或高于 4.2V 时触发电压越限安全保护分支",
        "durationMs": total_steps * 20.0,
        "signalNames": ["speed_cmd", "temp", "voltage", "estop"],
        "steps": steps
    }

if __name__ == "__main__":
    plan = generate_stimulus()
    print("[STIMULUS_START]")
    print(json.dumps(plan, indent=2))
    print("[STIMULUS_END]")
