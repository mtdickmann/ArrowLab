#pragma once

#include <driver/gpio.h>
#include <driver/uart.h>
#include <esp_rom_gpio.h>
#include <hal/gpio_hal.h>
#include <soc/io_mux_reg.h>
#include <soc/uart_periph.h>

namespace OneWireUart
{
    /**
     * Route one UART's receiver and transmitter to the same open-drain pad.
     *
     * Arduino-ESP32 3.1.1 does not preserve both peripheral-manager routes
     * when HardwareSerial is started with identical RX and TX pins. The
     * direction call establishes the electrical open-drain mode; the two
     * matrix calls deliberately come afterwards so neither route is removed.
     */
    inline bool attach(uart_port_t uartNumber, gpio_num_t signalPin)
    {
        if (
            gpio_set_direction(signalPin, GPIO_MODE_INPUT_OUTPUT_OD)
            != ESP_OK
        ) {
            return false;
        }

        gpio_hal_iomux_func_sel(
            GPIO_PIN_MUX_REG[static_cast<uint32_t>(signalPin)],
            PIN_FUNC_GPIO);
        esp_rom_gpio_connect_out_signal(
            static_cast<uint32_t>(signalPin),
            UART_PERIPH_SIGNAL(uartNumber, SOC_UART_TX_PIN_IDX),
            false,
            false);
        esp_rom_gpio_connect_in_signal(
            static_cast<uint32_t>(signalPin),
            UART_PERIPH_SIGNAL(uartNumber, SOC_UART_RX_PIN_IDX),
            false);
        return true;
    }
}
