ProjectATP
Opens port 5000, IP 193.168.1.120 and waits for commands.
Also uses UART: UART7 (RX - PE7, TX - PE8)  to receive commands
See command structure at: ./ProjectATP/RTG/protocol.h

When command received, program sends a pattern via specified peripheral and then sends back result (to UART or ETH). Timeout - 30 millis.
All peripheral are working via interrupt.


ProjectDMA (same as ProjectATP, except all peripherals are working via DMA)
Opens port 5000, IP 193.168.1.120 and waits for commands.
Also uses UART: UART7 (RX - PE7, TX - PE8)  to receive commands
See command structure at: ./ProjectDMA/RTG/protocol.h

When command received, program sends a pattern via specified peripheral and then sends back result (to UART or ETH). Timeout - 30 millis.
All peripheral are working via DMA.



ProjectATP_GUI
build it using maven: mvn package

launch: java -jar ./target/ProjectATP_GUI-1.0-SNAPSHOT.jar
or	java -jar ./ProjectATP_GUI-1.0-SNAPSHOT.jar

REQUIRES JRE 17+
