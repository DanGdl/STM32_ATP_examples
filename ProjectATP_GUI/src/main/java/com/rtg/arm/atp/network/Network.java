package com.rtg.arm.atp.network;

import com.rtg.arm.atp.dto.TestResult;
import com.rtg.arm.atp.dto.TestRequest;

public interface Network {
    int SERVER_PORT = 1044;

    void shutdown();

    void setupServerSocket(NetworkCallback<TestResult> networkCallback);

    void sendProtocolSettings(TestRequest request, NetworkCallback<Void> networkCallback);
}
