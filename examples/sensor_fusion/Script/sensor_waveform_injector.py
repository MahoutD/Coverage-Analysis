#!/usr/bin/env python3
"""
Sensor Waveform & Noise Injector for Embedded C Verification
Generates time-series sensor inputs (ADC voltage, temperature, current)
including boundary conditions and fault injections to trigger coverage paths.
"""

import json
import math
import sys

def generate_sensor_stimulus(steps=50, dt_ms=20.0):
    vectors = []
    
    for i in range(steps):
        t_ms = i * dt_ms
        t_sec = t_ms / 1000.0
        
        # Base sinusoidal sensor waveform (0 - 3.3V)
        base_voltage = 1.65 + 1.2 * math.sin(2.0 * math.pi * 0.5 * t_sec)
        
        # Temperature gradually rising with load
        temp_celsius = 25.0 + 3.5 * t_sec
        
        # Current draw (Amperes)
        current_amp = 2.0 + 1.5 * math.cos(2.0 * math.pi * 0.2 * t_sec)
        
        # Inject boundary faults at specific steps to exercise coverage branches
        fault_flags = 0
        if 20 <= i <= 25:
            # High temperature fault condition (> 100 C)
            temp_celsius = 108.5
            fault_flags |= 0x01
        elif 35 <= i <= 38:
            # Overvoltage spike (> 3.0V threshold)
            base_voltage = 3.25
            fault_flags |= 0x02
        elif i == 45:
            # Emergency shutoff flag
            fault_flags |= 0x08

        step_data = {
            "timestampMs": round(t_ms, 2),
            "signals": {
                "adc_voltage": round(base_voltage, 3),
                "temp_celsius": round(temp_celsius, 2),
                "current_amp": round(current_amp, 2),
                "fault_flags": float(fault_flags)
            },
            "rawPayload": f"ADC={base_voltage:.2f}V, T={temp_celsius:.1f}C, I={current_amp:.2f}A, FAULT={fault_flags}"
        }
        vectors.append(step_data)

    plan = {
        "name": "Dynamic Sensor & Fault Waveform",
        "scriptPath": "sensor_waveform_injector.py",
        "targetModule": "sensor_fusion.c",
        "description": "Multi-channel analog sensor simulation with overtemperature and overvoltage fault injections.",
        "durationMs": steps * dt_ms,
        "generatedAt": "2026-09-30T16:00:00Z",
        "signalNames": ["adc_voltage", "temp_celsius", "current_amp", "fault_flags"],
        "steps": vectors
    }
    return plan

if __name__ == "__main__":
    plan = generate_sensor_stimulus()
    print("[STIMULUS_START]")
    print(json.dumps(plan, indent=2))
    print("[STIMULUS_END]")
