package com.rtg.arm.atp.network;

public class Result<T> {
    public final T data;
    public final Throwable error;


    public Result(T data) {
        this.data = data;
        error = null;
    }

    public Result(Throwable error) {
        this.error = error;
        data = null;
    }
}
