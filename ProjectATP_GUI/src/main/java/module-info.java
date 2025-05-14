module com.rtg.arm.atp {
    requires javafx.controls;
    requires javafx.fxml;

    opens com.rtg.arm.atp to javafx.fxml;
    exports com.rtg.arm.atp;
}
