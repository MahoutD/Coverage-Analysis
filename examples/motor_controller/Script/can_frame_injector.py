#!/usr/bin/env python3
"""
CAN Bus Frame & Command Injector for Embedded C Verification
Simulates vehicle / industrial CAN bus telemetry frames:
Speed commands, heartbeat packets, motor status, and fault injection.
"""

import json
import sys

def generate_can_stimulus(steps=40):
    vectors = []
    
    can_ids = [0x120, 0x121, 0x122, 0x200]
    
    for i in range(steps):
        t_ms = i * 25.0
        
        # Command speed ramp
        target_rpm = min(3000, i * 80)
        can_id = 0x120
        e_stop = 0
        
        if i >= 30 and i <= 35:
            # Inject emergency stop frame
            can_id = 0x121
            e_stop = 1
            target_rpm = 0
        elif i % 5 == 0:
            # Heartbeat frame
            can_id = 0x200

        step_data = {
            "timestampMs": float(t_ms),
            "signals": {
                "can_id": float(can_id),
                "target_rpm": float(target_rpm),
                "e_stop": float(e_stop),
                "packet_seq": float(i % 16)
            },
            "rawPayload": f"CAN_ID=0x{can_id:03X} DLC=8 DATA=[{target_rpm & 0xFF:02X}, {(target_rpm >> 8) & 0xFF:02X}, {e_stop:02X}]"
        }
        vectors.append(step_data)

    plan = {
        "name": "CAN Bus Telemetry & E-Stop Injector",
        "scriptPath": "can_frame_injector.py",
        "targetModule": "motor_controller.c",
        "description": "Simulates periodic CAN bus speed commands and emergency braking frames.",
        "durationMs": steps * 25.0,
        "generatedAt": "2026-09-30T16:00:00Z",
        "signalNames": ["can_id", "target_rpm", "e_stop", "packet_seq"],
        "steps": vectors
    }
    return plan

if __name__ == "__main__":
    plan = generate_can_stimulus()
    print("[STIMULUS_START]")
    print(json.dumps(plan, indent=2))
    print("[STIMULUS_END]")
