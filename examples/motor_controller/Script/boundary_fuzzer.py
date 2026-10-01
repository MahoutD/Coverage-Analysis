#!/usr/bin/env python3
"""
Embedded Boundary & Fault Fuzzer
Generates edge cases (0, boundary thresholds, negative values, maximum limits)
specifically crafted to trigger hard-to-reach branches in embedded C software.
"""

import json
import sys

def generate_fuzz_vectors():
    boundary_cases = [
        {"desc": "Normal Baseline", "speed": 1000.0, "temp": 30.0, "volt": 3.3, "fault": 0.0},
        {"desc": "Zero Speed", "speed": 0.0, "temp": 25.0, "volt": 3.3, "fault": 0.0},
        {"desc": "Overspeed Trip Point", "speed": 3500.0, "temp": 45.0, "volt": 3.3, "fault": 0.0},
        {"desc": "Critical Temperature", "speed": 1500.0, "temp": 115.0, "volt": 3.3, "fault": 1.0},
        {"desc": "Undervoltage Trip Point", "speed": 800.0, "temp": 40.0, "volt": 1.8, "fault": 2.0},
        {"desc": "Overvoltage Surge", "speed": 1200.0, "temp": 50.0, "volt": 4.5, "fault": 2.0},
        {"desc": "Negative Current Spike", "speed": -50.0, "temp": 30.0, "volt": 3.3, "fault": 4.0},
        {"desc": "Watchdog Timeout Trigger", "speed": 2000.0, "temp": 60.0, "volt": 3.3, "fault": 8.0},
        {"desc": "Extreme Multi-Fault Condition", "speed": 4000.0, "temp": 130.0, "volt": 5.0, "fault": 15.0},
        {"desc": "System Safe Recovery", "speed": 0.0, "temp": 20.0, "volt": 3.3, "fault": 0.0}
    ]

    vectors = []
    for idx, case in enumerate(boundary_cases):
        vectors.append({
            "timestampMs": idx * 50.0,
            "signals": {
                "speed_rpm": case["speed"],
                "temp_celsius": case["temp"],
                "bus_voltage": case["volt"],
                "fault_flags": case["fault"]
            },
            "rawPayload": f"FUZZ[{idx}]: {case['desc']} -> RPM={case['speed']}, T={case['temp']}, V={case['volt']}"
        })

    plan = {
        "name": "Branch Coverage Boundary Fuzzer",
        "scriptPath": "boundary_fuzzer.py",
        "targetModule": "motor_controller.c",
        "description": "Exhaustive edge-case stimulus designed to achieve >90% branch and condition coverage.",
        "durationMs": len(vectors) * 50.0,
        "generatedAt": "2026-09-30T16:00:00Z",
        "signalNames": ["speed_rpm", "temp_celsius", "bus_voltage", "fault_flags"],
        "steps": vectors
    }
    return plan

if __name__ == "__main__":
    plan = generate_fuzz_vectors()
    print("[STIMULUS_START]")
    print(json.dumps(plan, indent=2))
    print("[STIMULUS_END]")
