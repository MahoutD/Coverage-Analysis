#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
激励场景 1: 正常平稳巡航工况测试向量
测试目标:
  - 驱动电机从 0 RPM 加速至 1500 RPM 稳态
  - 温度维持在 35~45℃ 正常区间
  - 母线电压保持在 3.3V 稳压状态
  - 触发正常加速分支 (ACCELERATING) 与稳态闭环调节分支 (STEADY)
"""
import json

def generate_stimulus():
    steps = []
    total_steps = 30
    
    for i in range(total_steps):
        t_ms = i * 20.0
        # 目标转速在加速后维持 1500 RPM
        speed_target = min(1500.0, i * 100.0)
        temp_val = 25.0 + (i * 0.5)  # 25 ~ 40℃ 正常升温
        bus_volt = 3.30 + ((i % 3) * 0.02) # 3.30 ~ 3.34V
        
        steps.append({
            "timestampMs": t_ms,
            "signalValues": {
                "speed_cmd": speed_target,
                "temp": temp_val,
                "voltage": bus_volt,
                "estop": 0.0
            },
            "rawPayload": f"NORMAL_CRUISE: step={i+1}, speed={speed_target:.1f}rpm, temp={temp_val:.1f}C"
        })
        
    return {
        "name": "正常巡航控制激励 (Normal Cruise)",
        "targetModule": "motor_controller.c",
        "description": "模拟正常加速与稳态巡航闭环调节分支",
        "durationMs": total_steps * 20.0,
        "signalNames": ["speed_cmd", "temp", "voltage", "estop"],
        "steps": steps
    }

if __name__ == "__main__":
    plan = generate_stimulus()
    print("[STIMULUS_START]")
    print(json.dumps(plan, indent=2))
    print("[STIMULUS_END]")
