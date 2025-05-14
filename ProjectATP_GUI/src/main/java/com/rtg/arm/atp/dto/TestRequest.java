package com.rtg.arm.atp.dto;

public class TestRequest {
    public final String ip;
    public final int port;
    public final String message;
    public final Protocols protocol;

    public final int testId;
    public final int iterations;

    public TestRequest(String ip, int port, String message, Protocols protocol, int testId, int iterations) {
        this.ip = ip;
        this.port = port;
        this.testId = testId;
        this.message = message;
        this.protocol = protocol;
        this.iterations = iterations;
    }
}
