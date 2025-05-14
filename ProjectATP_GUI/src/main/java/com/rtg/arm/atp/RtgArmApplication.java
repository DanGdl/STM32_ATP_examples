package com.rtg.arm.atp;

import javafx.application.Application;
import javafx.fxml.FXMLLoader;
import javafx.scene.Parent;
import javafx.scene.Scene;
import javafx.stage.Stage;

import java.io.IOException;

public class RtgArmApplication extends Application {
    RtgArmController controller;

    @Override
    public void start(Stage stage) throws IOException {
        final FXMLLoader fxmlLoader = new FXMLLoader(RtgArmApplication.class.getResource("view_atp.fxml"));
        final Scene scene = new Scene(fxmlLoader.load(), 700, 450);
        controller = fxmlLoader.getController();
        stage.setTitle("RTG ATP");
        stage.setScene(scene);
        stage.setResizable(true);
        stage.show();
    }

    @Override
    public void stop() throws Exception {
        super.stop();
        controller.onDestroy();
    }

    public static void main(String[] args) {
        launch();
    }
}
