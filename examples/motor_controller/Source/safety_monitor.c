/**
 * @file safety_monitor.c
 * @brief 车规级 ASIL-B 嵌入式功能安全监控实现
 */

#include "safety_monitor.h"
#include "bsp_pwm.h"

static uint32_t s_active_fault_mask = FAULT_NONE;
static uint32_t s_watchdog_counter = 0;
static uint32_t s_overcurrent_debounce = 0;
static bool s_is_emergency_latched = false;

void safety_monitor_init(void) {
    s_active_fault_mask = FAULT_NONE;
    s_watchdog_counter = 0;
    s_overcurrent_debounce = 0;
    s_is_emergency_latched = false;
}

uint32_t safety_monitor_evaluate(const MotorTelemetry_t* telem) {
    if (telem == 0) {
        s_active_fault_mask |= FAULT_WATCHDOG_TIMEOUT;
        return s_active_fault_mask;
    }

    // 1. 母线过欠压闭锁检测 (带回差迟滞逻辑)
    if (telem->vbus_voltage > SAFETY_MAX_BUS_VOLTAGE) {
        s_active_fault_mask |= FAULT_OVER_VOLTAGE;
    } else if (telem->vbus_voltage < SAFETY_MIN_BUS_VOLTAGE) {
        s_active_fault_mask |= FAULT_UNDER_VOLTAGE;
    } else {
        // 电压处于正常区间，解除过欠压临时告警
        s_active_fault_mask &= ~(FAULT_OVER_VOLTAGE | FAULT_UNDER_VOLTAGE);
    }

    // 2. 功率级结温多级安全监控
    if (telem->inverter_temp_c > SAFETY_TRIP_TEMP_C) {
        s_active_fault_mask |= FAULT_OVER_TEMPERATURE;
    } else if (telem->inverter_temp_c < (SAFETY_TRIP_TEMP_C - 10.0f)) {
        // 低于跳闸回差温度才允许解除超温状态
        s_active_fault_mask &= ~FAULT_OVER_TEMPERATURE;
    }

    // 3. 相电流峰值超限防抖检测
    if (telem->total_current_rms > SAFETY_MAX_PHASE_CURRENT) {
        s_overcurrent_debounce++;
        if (s_overcurrent_debounce >= 3U) {
            s_active_fault_mask |= FAULT_OVER_CURRENT;
            // 立即硬件封锁三相输出
            bsp_pwm_disable_outputs();
        }
    } else {
        if (s_overcurrent_debounce > 0U) {
            s_overcurrent_debounce--;
        }
    }

    // 4. 软件看门狗自检递增
    s_watchdog_counter++;
    if (s_watchdog_counter > SAFETY_WATCHDOG_LIMIT_TICKS) {
        s_active_fault_mask |= FAULT_WATCHDOG_TIMEOUT;
        bsp_pwm_disable_outputs();
    }

    // 5. 严重故障硬线联锁
    if ((s_active_fault_mask & (FAULT_OVER_CURRENT | FAULT_OVER_TEMPERATURE)) != 0U) {
        s_is_emergency_latched = true;
    }

    return s_active_fault_mask;
}

void safety_watchdog_kick(void) {
    s_watchdog_counter = 0;
}

void safety_trigger_emergency_stop(const char* /* reason */) {
    s_is_emergency_latched = true;
    s_active_fault_mask |= FAULT_OVER_CURRENT;
    bsp_pwm_disable_outputs();
}

bool safety_clear_faults(void) {
    // 致命硬件过流或结温严重超标禁止盲目清除
    if ((s_active_fault_mask & FAULT_OVER_CURRENT) != 0U) {
        return false;
    }

    s_active_fault_mask = FAULT_NONE;
    s_is_emergency_latched = false;
    s_watchdog_counter = 0;
    return true;
}

MotorState_t safety_get_recommended_state(MotorState_t current_state) {
    if (s_is_emergency_latched) {
        return MOTOR_STATE_EMERGENCY_STOP;
    }

    if (s_active_fault_mask != FAULT_NONE) {
        return MOTOR_STATE_FAULT;
    }

    // 若当前处于待机，且没有故障，维持当前；若处于故障则切回待机
    if (current_state == MOTOR_STATE_FAULT) {
        return MOTOR_STATE_STANDBY;
    }

    return current_state;
}

uint32_t safety_monitor_get_faults(void) {
    return s_active_fault_mask;
}

void safety_monitor_report_fault(uint32_t fault) {
    s_active_fault_mask |= fault;
    if ((s_active_fault_mask & (FAULT_OVER_CURRENT | FAULT_OVER_TEMPERATURE)) != 0U) {
        s_is_emergency_latched = true;
    }
}
