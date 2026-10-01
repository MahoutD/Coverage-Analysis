#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
激励场景 2: 电机过温故障保护跳闸测试向量
测试目标:
  - 模拟散热风道堵塞，电机绕组温度急剧上升超过 105.0℃ 保护阈值
  - 触发 safety_monitor / motor_check_safety 中的过温闭锁判定分支 (temperature_celsius > CRITICAL_TEMP_THRESHOLD)
  - 触发 fault_recorder 记录 DTC 0x002100 并触发 motor_emergency_shutdown 切断输出
"""
import json

def generate_stimulus():
    steps = []
    total_steps = 30
    
    for i in range(total_steps):
        t_ms = i * 20.0
        speed_target = 1800.0
        # 温度在第 15 步突破 105℃，最高升至 125℃
        temp_val = 30.0 + (i * 3.5)
        bus_volt = 3.30
        
        steps.append({
            "timestampMs": t_ms,
            "signalValues": {
                "speed_cmd": speed_target,
                "temp": temp_val,
                "voltage": bus_volt,
                "estop": 0.0
            },
            "rawPayload": f"OVERTEMP_TEST: step={i+1}, temp={temp_val:.1f}C (Trip if > 105C)"
        })
        
    return {
        "name": "电机过温故障切断激励 (Overtemp Fault Trip)",
        "targetModule": "motor_controller.c",
        "description": "温度突破 105℃ 触发硬件级过温故障保护与冻结帧记录分支",
        "durationMs": total_steps * 20.0,
        "signalNames": ["speed_cmd", "temp", "voltage", "estop"],
        "steps": steps
    }

if __name__ == "__main__":
    plan = generate_stimulus()
    print("[STIMULUS_START]")
    print(json.dumps(plan, indent=2))
    print("[STIMULUS_END]")
