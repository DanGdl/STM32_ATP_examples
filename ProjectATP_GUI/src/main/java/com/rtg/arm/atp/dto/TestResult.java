package com.rtg.arm.atp.dto;

import java.util.Date;

public class TestResult {
    public final String testId;
    public final String message;

    public TestResult(String testId, String message) {
        this.testId = testId;
        this.message = message;
    }
}
