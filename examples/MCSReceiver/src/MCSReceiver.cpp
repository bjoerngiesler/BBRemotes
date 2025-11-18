/*
  BBRemoteTransmitter example
  2025 by Björn Giesler
  This example realizes a simple 3-axis remote control using the Bavarian Builders' 
  BBRemotes framework to communicate over various communication mechanisms.
  HOW TO USE:
    1. Choose what communication protocol you want to use, and uncomment the respective
       lines in the protocol section.
    2. Modify the pin mappings in the pins section to reflect the pins you have connected
       the hardware (joysticks, pots, buttons) to.
*/

#include <LibBBRemotes.h>
#include <BBRTypes.h>
#include <wiring_private.h>

using namespace bb::rmt;

MXBProtocol protocol;
Receiver *rx = nullptr;

// These variables hold the values for the inputs the droid defines.
float speed=0, turn=0;
// These hold the input IDs.
uint8_t speedInput, turnInput;

Uart *dfplayerSerial, *serialTXSerial;
static const uint8_t P_SERIALTX_TX     = 0;
static const uint8_t P_SERIALTX_RX     = 1;

bool setupBoardComm() {
  serialTXSerial = new Uart(&sercom3, P_SERIALTX_RX, P_SERIALTX_TX, SERCOM_RX_PAD_1, UART_TX_PAD_0);
  pinPeripheral(P_SERIALTX_RX, PIO_SERCOM);
  pinPeripheral(P_SERIALTX_TX, PIO_SERCOM);
  return true;
}

void SERCOM3_Handler() {
  serialTXSerial->IrqHandler();
}

// Called whenever a data packet was received.
void dataFinishedCB(const NodeAddr& addr, uint8_t seqnum) {
  bb::rmt::printf("Speed: %f Turn: %f Seqnum: %d\n", speed, turn, seqnum);
}

// Called whenever nothing was received for a certain time (default: 0.5s).
void timeoutCB(const NodeAddr& addr) {
  bb::rmt::printf("Timeout!\n");
  // Would stop the drive motors here
}

void setup() {
  Serial.begin(2000000);

  Serial.print("Hello world\n");
  bb::rmt::printf("MCS Receiver example\n");

  // Initialize the protocol, giving this station a name
  setupBoardComm();
  protocol.setUart(serialTXSerial);
  protocol.init("Receiver");
  // Ask the protocol to create a receiver. Some protocols cannot do this and will return nullptr.
  rx = protocol.createReceiver();
  if(rx == nullptr) {
    bb::rmt::printf("Error: Could not create receiver!\n");
    while(true);
  }
  Serial.print("Sizes:\n");
  Serial.print("Control packet:"); Serial.println(sizeof(MControlPacket));
  Serial.print("State packet:"); Serial.println(sizeof(MStatePacket));
  Serial.print("Config packet:"); Serial.println(sizeof(MConfigPacket));
  Serial.print("Pairing packet:"); Serial.println(sizeof(MPairingPacket));
  Serial.print("Pairing Discovery:"); Serial.println(sizeof(MPairingPacket::PairingDiscovery));
  Serial.print("Pairing Request:"); Serial.println(sizeof(MPairingPacket::PairingRequest));
  Serial.print("Pairing Reply:"); Serial.println(sizeof(MPairingPacket::PairingReplyResult));
  Serial.print("MPacket:"); Serial.println(sizeof(MPacket));

  // Tell the receiver about the two inputs our droid knows
  speedInput = rx->addInput(INPUT_SPEED, speed);
  turnInput = rx->addInput(INPUT_TURN_RATE, turn);

  // Tell the receiver which axis to map to which input, and how to interpolate
  rx->setMix(speedInput, AxisMix(0, INTERP_LIN_CENTERED));
  rx->setMix(turnInput, AxisMix(1, INTERP_LIN_CENTERED));

  // Tell the receiver to call dataFinishedCB() whenever a packet was handled
  rx->setDataFinishedCallback(dataFinishedCB);

  // Tell the receiver to call timeoutCB() whenever no data is received for a certain time
  rx->setTimeoutCallback(timeoutCB);
}

void loop() {
  // Let the protocol do its work
  protocol.step();
  delay(5);
}