package com.rtg.arm.atp.network;

public interface NetworkCallback<T> {
    void onResult(Result<T> result);
}
