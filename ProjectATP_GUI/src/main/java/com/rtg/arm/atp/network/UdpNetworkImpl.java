package com.rtg.arm.atp.network;

import com.rtg.arm.atp.dto.TestResult;
import com.rtg.arm.atp.dto.Protocols;
import com.rtg.arm.atp.dto.TestRequest;
import javafx.application.Platform;

import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;
import java.util.Date;
import java.util.Enumeration;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.regex.Pattern;

public class UdpNetworkImpl implements Network {
    private final String IP = "169.254.22.245";
    private final ExecutorService pool = Executors.newCachedThreadPool();
    private final String ip;
    private DatagramSocket serverSocket = null;

    public UdpNetworkImpl() {
        String ip_ = null;
        try {
            final Pattern ipPattern = Pattern.compile("\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}");

            boolean ipFound = false;
            Enumeration<NetworkInterface> e = NetworkInterface.getNetworkInterfaces();
            while (e.hasMoreElements()) {
                Enumeration<InetAddress> ee = e.nextElement().getInetAddresses();
                while (ee.hasMoreElements()) {
                    InetAddress i = ee.nextElement();
                    if (ipPattern.matcher(i.getHostAddress()).matches()) {
                        System.out.println("Matched " + i.getHostAddress());
                        // TODO: select ip
                        if (!ipFound) {
                            ipFound = true;
                            ip_ = i.getHostAddress();
                        }
                    }
                }
            }
        } catch (Throwable e) {
            e.printStackTrace();
            ip_ = null;
        }
        ip = IP; // ip_;
    }

    @Override
    public void shutdown() {
        pool.shutdown();
        if (serverSocket != null) {
            try {
                serverSocket.close();
            } catch (Throwable e) {
                e.printStackTrace();
            }
        }
    }

    @Override
    public void setupServerSocket(NetworkCallback<TestResult> callback) {
        pool.submit(() -> {
            try {
                final DatagramSocket sSocket = new DatagramSocket(SERVER_PORT);
                serverSocket = sSocket;
                while (true) {
                    final byte[] buf = new byte[1024];
                    final DatagramPacket packet = new DatagramPacket(buf, buf.length);
                    sSocket.receive(packet);
                    pool.submit(() -> handleConnection(packet, callback));
                }
            } catch (Throwable e) {
                e.printStackTrace();
            }
        });
    }

    private void handleConnection(DatagramPacket packet, NetworkCallback<TestResult> callback) {
        Throwable error = null;
        TestResult m = null;
        try {
//            final InetAddress address = packet.getAddress();
//            final int port = packet.getPort();
            final String received = new String(packet.getData(), 0, packet.getLength());
            final String[] split = received.trim().split(":");
            m = new TestResult(split[0], split[1]);
        } catch (Throwable e) {
            e.printStackTrace();
            error = e;
        }
        if (error == null) {
            final TestResult data = m;
            Platform.runLater(() -> callback.onResult(new Result<>(data)));
        } else {
            final Throwable e = error;
            Platform.runLater(() -> callback.onResult(new Result<>(e)));
        }
    }

    @Override
    public void sendProtocolSettings(TestRequest request, NetworkCallback<Void> callback) {
        pool.submit(() -> {
            Throwable error = null;
            try  {
                final InetAddress address = InetAddress.getByName(request.ip);
/*
typedef struct TestCmdHeader {
	uint32_t test_id;
	Peripheral_t peripheral_id;
	uint8_t iterations;
	uint8_t pattern_len;
} TestCmdHeader_t;

typedef struct TestCommand {
	TestCmdHeader_t header;
	uint8_t pattern[256];
} TestCommand_t;
*/
                ByteBuffer buff = ByteBuffer.allocate(263 /*256 + 7*/);
                buff.order(ByteOrder.LITTLE_ENDIAN);
                buff.putInt(0, request.testId);
                buff.put(4, (byte) request.protocol.type);
                buff.put(5, (byte) request.iterations);
                buff.put(6, (byte) request.message.length());
                buff.put(7, request.message.getBytes(StandardCharsets.UTF_8));
                final byte[] array = buff.array();
                System.out.println("Ip: " + request.ip + " port " + request.port);
                DatagramPacket packet = new DatagramPacket(array, array.length, address, request.port);
                serverSocket.send(packet);
            } catch (Throwable e) {
                e.printStackTrace();
                error = e;
            }
            if (error == null) {
                Platform.runLater(() -> callback.onResult(new Result<>((Void) null)));
            } else {
                final Throwable e = error;
                Platform.runLater(() -> callback.onResult(new Result<>(e)));
            }
        });
    }
}
