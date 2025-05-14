package com.rtg.arm.atp.dto;

public enum Protocols {
    I2C(2), SPI(3), UART(1), UNKNOWN(0);

    public final int type;

    Protocols(int type) {
        this.type = type;
    }
}
