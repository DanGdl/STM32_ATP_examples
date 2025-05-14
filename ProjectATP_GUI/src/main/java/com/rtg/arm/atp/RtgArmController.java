package com.rtg.arm.atp;

import com.rtg.arm.atp.dto.TestResult;
import com.rtg.arm.atp.dto.Protocols;
import com.rtg.arm.atp.dto.TestRequest;
import com.rtg.arm.atp.network.Network;
import com.rtg.arm.atp.network.UdpNetworkImpl;
import javafx.event.ActionEvent;
import javafx.event.EventHandler;
import javafx.fxml.FXML;
import javafx.scene.control.*;
import javafx.scene.input.KeyEvent;

import java.net.URL;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.ResourceBundle;
import java.util.regex.Pattern;

public class RtgArmController {
    // 192.168.1.120:5000
    private final Pattern ipPortPattern = Pattern.compile("\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}:\\d+");
    private final Pattern ipPattern = Pattern.compile("\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}");
    private final Pattern digitPattern = Pattern.compile("\\d+");
    private final SimpleDateFormat sdf = new SimpleDateFormat("yyyy.MM.dd HH:mm:ss");
    private final Network network = new UdpNetworkImpl();

    @FXML
    private ResourceBundle resources;

    @FXML
    private TextField input_ip;

    @FXML
    private TextField input_message;

    @FXML
    private TextField input_test_id;

    @FXML
    private TextField input_iterations;

    @FXML
    private Label lbl_status;

    @FXML
    private Button btn_send;

    @FXML
    private Label progress_message;

    @FXML
    private TextArea output;

    @FXML
    private URL location;

    @FXML
    private ToggleGroup protocols;

    @FXML
    void initialize() {
        btn_send.setDisable(true);
        final EventHandler<KeyEvent> inputChangeListener = event -> setupButtonEnable(
                isMessageValid(input_message.getCharacters().toString()),
                isIpValid(input_ip.getCharacters().toString())
        );

        input_ip.setOnKeyTyped(inputChangeListener);
        input_message.setOnKeyTyped(inputChangeListener);

        network.setupServerSocket(result -> {
            final TestResult data = result.data;
            if (data != null) {
                final String line = String.format("%s TestID %s %s\n", sdf.format(new Date()), data.testId, data.message);
                output.setText(line + output.getText());
            }
        });
    }

    @FXML
    void onClearClick(ActionEvent event) {
        output.setText("");
    }

    @FXML
    void onSendClick(ActionEvent event) {
        lbl_status.setText("");
        final String conn = input_ip.getCharacters().toString().replace(" ", "");
        final String iter = input_iterations.getCharacters().toString().trim();
        final String testId = input_test_id.getCharacters().toString().trim();
        final String msg = input_message.getCharacters().toString();
        String[] connections = null;
        if (!conn.contains(":")) {
            lbl_status.setText("Enter Ip and port to send command. ");
        } else {
            connections = conn.split(":");
            if (!ipPattern.matcher(connections[0]).matches() || !digitPattern.matcher(connections[1]).matches()) {
                lbl_status.setText("Doesn't looks like IP and port. ");
            }
        }
        if (msg.isEmpty()) {
            lbl_status.setText(lbl_status.getText() + "Enter a message. ");
        }
        if (testId.isEmpty() || testId.length() > 5 || !digitPattern.matcher(testId).matches()) {
            lbl_status.setText(lbl_status.getText() + "Enter a Test ID (digits only). ");
        }
        if (iter.isEmpty() || testId.length() > 3 || !digitPattern.matcher(testId).matches()) {
            lbl_status.setText(lbl_status.getText() + "Enter an amount of iterations (digits only). ");
        }
        final String id = ((RadioButton) protocols.getSelectedToggle()).getId();
        final Protocols protocol;
        if ("radio_uart".equals(id)) {
            protocol = Protocols.UART;
        } else if ("radio_i2c".equals(id)) {
            protocol = Protocols.I2C;
        } else if ("radio_spi".equals(id)) {
            protocol = Protocols.SPI;
        } else {
            protocol = Protocols.UNKNOWN;
            lbl_status.setText(lbl_status.getText() + "Select a protocol. ");
        }

        if (!lbl_status.getText().isEmpty()) {
            return;
        }

        btn_send.setDisable(true);
        progress_message.setOpacity(1D);
        network.sendProtocolSettings(
                new TestRequest(
                        connections[0], Integer.parseInt(connections[1]), msg, protocol,
                        Integer.parseInt(testId), Integer.parseInt(iter)
                ), result -> {
                    progress_message.setOpacity(0D);
                    btn_send.setDisable(false);
                });
    }

    private void setupButtonEnable(boolean messageValid, boolean ipValid) {
        btn_send.setDisable(!messageValid || !ipValid);
    }

    private boolean isIpValid(String ip) {
        return ipPortPattern.matcher(ip).matches();
    }

    private boolean isMessageValid(String message) {
        return !message.isEmpty() && message.length() <= 10;
    }

    public void onDestroy() {
        network.shutdown();
    }
}
