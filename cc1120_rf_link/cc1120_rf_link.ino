#include <SPI.h>

/************************************************************
 * CHANGE THIS ONE LINE
 *
 * TRANSMITTER: ROLE_TX = 1
 * RECEIVER:    ROLE_TX = 0
 ************************************************************/
#define ROLE_TX 1


/************************************************************
 * XIAO ESP32-S3 <-> CC1120
 ************************************************************/
constexpr uint8_t RF_SCLK  = D8;    // GPIO7
constexpr uint8_t RF_MISO  = D9;    // GPIO8
constexpr uint8_t RF_MOSI  = D10;   // GPIO9
constexpr uint8_t RF_CS    = D3;    // GPIO4
constexpr uint8_t RF_RESET = D2;    // GPIO3

SPISettings ccSPI(1000000, MSBFIRST, SPI_MODE0);


/************************************************************
 * CC1120 COMMAND STROBES
 ************************************************************/
constexpr uint8_t SRES  = 0x30;
constexpr uint8_t SCAL  = 0x33;
constexpr uint8_t SRX   = 0x34;
constexpr uint8_t STX   = 0x35;
constexpr uint8_t SIDLE = 0x36;
constexpr uint8_t SFRX  = 0x3A;
constexpr uint8_t SFTX  = 0x3B;


/************************************************************
 * EXTENDED STATUS REGISTER ADDRESSES
 ************************************************************/
constexpr uint8_t MARCSTATE     = 0x73;
constexpr uint8_t PARTNUMBER    = 0x8F;
constexpr uint8_t PARTVERSION   = 0x90;
constexpr uint8_t NUM_TXBYTES   = 0xD6;
constexpr uint8_t NUM_RXBYTES   = 0xD7;

constexpr uint8_t MARC_IDLE     = 0x41;
constexpr uint8_t MARC_RX       = 0x6D;


/************************************************************
 * LOW LEVEL SPI
 ************************************************************/
bool waitReady(uint32_t timeout_us = 100000)
{
  uint32_t start = micros();

  while (digitalRead(RF_MISO) == HIGH)
  {
    if ((micros() - start) > timeout_us)
      return false;
  }

  return true;
}


uint8_t strobe(uint8_t cmd)
{
  SPI.beginTransaction(ccSPI);

  digitalWrite(RF_CS, LOW);

  if (!waitReady())
  {
    digitalWrite(RF_CS, HIGH);
    SPI.endTransaction();
    return 0xFF;
  }

  uint8_t status = SPI.transfer(cmd);

  digitalWrite(RF_CS, HIGH);
  SPI.endTransaction();

  return status;
}


/************************************************************
 * NORMAL REGISTER WRITE / READ
 ************************************************************/
void writeReg(uint8_t address, uint8_t value)
{
  SPI.beginTransaction(ccSPI);

  digitalWrite(RF_CS, LOW);

  if (!waitReady())
  {
    digitalWrite(RF_CS, HIGH);
    SPI.endTransaction();
    return;
  }

  SPI.transfer(address);
  SPI.transfer(value);

  digitalWrite(RF_CS, HIGH);
  SPI.endTransaction();
}


uint8_t readReg(uint8_t address)
{
  SPI.beginTransaction(ccSPI);

  digitalWrite(RF_CS, LOW);

  if (!waitReady())
  {
    digitalWrite(RF_CS, HIGH);
    SPI.endTransaction();
    return 0xFF;
  }

  SPI.transfer(address | 0x80);
  uint8_t data = SPI.transfer(0x00);

  digitalWrite(RF_CS, HIGH);
  SPI.endTransaction();

  return data;
}


/************************************************************
 * EXTENDED REGISTER WRITE / READ
 ************************************************************/
void writeExt(uint8_t address, uint8_t value)
{
  SPI.beginTransaction(ccSPI);

  digitalWrite(RF_CS, LOW);

  if (!waitReady())
  {
    digitalWrite(RF_CS, HIGH);
    SPI.endTransaction();
    return;
  }

  SPI.transfer(0x2F);
  SPI.transfer(address);
  SPI.transfer(value);

  digitalWrite(RF_CS, HIGH);
  SPI.endTransaction();
}


uint8_t readExt(uint8_t address)
{
  SPI.beginTransaction(ccSPI);

  digitalWrite(RF_CS, LOW);

  if (!waitReady())
  {
    digitalWrite(RF_CS, HIGH);
    SPI.endTransaction();
    return 0xFF;
  }

  SPI.transfer(0xAF);
  SPI.transfer(address);

  uint8_t data = SPI.transfer(0x00);

  digitalWrite(RF_CS, HIGH);
  SPI.endTransaction();

  return data;
}


/************************************************************
 * FIFO ACCESS
 ************************************************************/
void writeTxFIFO(const uint8_t *data, uint8_t len)
{
  SPI.beginTransaction(ccSPI);

  digitalWrite(RF_CS, LOW);

  if (!waitReady())
  {
    digitalWrite(RF_CS, HIGH);
    SPI.endTransaction();
    return;
  }

  // 0x3F FIFO + burst-write bit
  SPI.transfer(0x7F);

  for (uint8_t i = 0; i < len; i++)
    SPI.transfer(data[i]);

  digitalWrite(RF_CS, HIGH);
  SPI.endTransaction();
}


void readRxFIFO(uint8_t *data, uint8_t len)
{
  SPI.beginTransaction(ccSPI);

  digitalWrite(RF_CS, LOW);

  if (!waitReady())
  {
    digitalWrite(RF_CS, HIGH);
    SPI.endTransaction();
    return;
  }

  // FIFO + burst + READ
  SPI.transfer(0xFF);

  for (uint8_t i = 0; i < len; i++)
    data[i] = SPI.transfer(0x00);

  digitalWrite(RF_CS, HIGH);
  SPI.endTransaction();
}


/************************************************************
 * HARDWARE RESET
 ************************************************************/
void hardwareReset()
{
  digitalWrite(RF_CS, HIGH);

  digitalWrite(RF_RESET, LOW);
  delay(5);

  digitalWrite(RF_RESET, HIGH);
  delay(20);
}


/************************************************************
 * WAIT FOR MARCSTATE
 ************************************************************/
bool waitMarc(uint8_t expected, uint32_t timeout_ms)
{
  uint32_t start = millis();

  while ((millis() - start) < timeout_ms)
  {
    if (readExt(MARCSTATE) == expected)
      return true;

    delay(1);
  }

  return false;
}


/************************************************************
 * LOAD YOUR SMARTRF STUDIO CONFIGURATION
 *
 * Based directly on your exported 434 MHz configuration.
 ************************************************************/
void configureRadio()
{
  Serial.println("Loading SmartRF configuration...");

  /********************
   * NORMAL REGISTERS
   ********************/

  writeReg(0x00, 0xB0);   // IOCFG3
  writeReg(0x01, 0x06);   // IOCFG2
  writeReg(0x02, 0xB0);   // IOCFG1
  writeReg(0x03, 0x40);   // IOCFG0

  writeReg(0x04, 0x55);   // SYNC3
  writeReg(0x05, 0x55);   // SYNC2
  writeReg(0x06, 0x7A);   // SYNC1
  writeReg(0x07, 0x0E);   // SYNC0

  writeReg(0x08, 0x08);   // SYNC_CFG1
  writeReg(0x09, 0x0B);   // SYNC_CFG0

  writeReg(0x0A, 0x99);   // DEVIATION_M
  writeReg(0x0B, 0x0D);   // MODCFG_DEV_E
  writeReg(0x0C, 0x15);   // DCFILT_CFG

  writeReg(0x0D, 0x18);   // PREAMBLE_CFG1

  writeReg(0x0F, 0x3A);   // FREQ_IF_CFG
  writeReg(0x10, 0x00);   // IQIC
  writeReg(0x11, 0x02);   // CHAN_BW

  writeReg(0x13, 0x05);   // MDMCFG0

  writeReg(0x14, 0x99);   // SYMBOL_RATE2
  writeReg(0x15, 0x99);   // SYMBOL_RATE1
  writeReg(0x16, 0x99);   // SYMBOL_RATE0

  writeReg(0x17, 0x3C);   // AGC_REF
  writeReg(0x18, 0xEF);   // AGC_CS_THR

  writeReg(0x1C, 0xA9);   // AGC_CFG1
  writeReg(0x1D, 0xC0);   // AGC_CFG0

  writeReg(0x1E, 0x00);   // FIFO_CFG

  writeReg(0x21, 0x14);   // FS_CFG: 410-480 MHz band


  /**********************************************************
   * Packet settings
   *
   * These are written explicitly so our test behavior is
   * deterministic.
   **********************************************************/

  writeReg(0x26, 0x04);   // PKT_CFG2: FIFO mode
  writeReg(0x27, 0x05);   // PKT_CFG1:
                           // CRC enabled + append status

  writeReg(0x28, 0x20);   // PKT_CFG0:
                           // variable packet length

  writeReg(0x29, 0x0F);   // RFEND_CFG1:
                           // good RX packet -> IDLE

  writeReg(0x2A, 0x00);   // RFEND_CFG0:
                           // TX complete -> IDLE

  writeReg(0x2D, 0x79);   // PA_CFG0
  writeReg(0x2E, 0xFF);   // PKT_LEN max


  /********************
   * EXTENDED REGISTERS
   ********************/

  writeExt(0x00, 0x00);   // IF_MIX_CFG
  writeExt(0x02, 0x0A);   // TOC_CFG

  // 434 MHz frequency configuration
  writeExt(0x0C, 0x6C);   // FREQ2
  writeExt(0x0D, 0x80);   // FREQ1
  writeExt(0x0E, 0x00);   // FREQ0

  writeExt(0x12, 0x00);   // FS_DIG1
  writeExt(0x13, 0x5F);   // FS_DIG0

  writeExt(0x16, 0x40);   // FS_CAL1
  writeExt(0x17, 0x0E);   // FS_CAL0

  writeExt(0x19, 0x03);   // FS_DIVTWO
  writeExt(0x1B, 0x33);   // FS_DSM0
  writeExt(0x1D, 0x17);   // FS_DVC0

  writeExt(0x1F, 0x50);   // FS_PFD
  writeExt(0x20, 0x6E);   // FS_PRE
  writeExt(0x21, 0x14);   // FS_REG_DIV_CML
  writeExt(0x22, 0xAC);   // FS_SPARE

  writeExt(0x27, 0xB4);   // FS_VCO0

  writeExt(0x32, 0x0E);   // XOSC5
  writeExt(0x36, 0x03);   // XOSC1

  Serial.println("Configuration loaded.");
}


/************************************************************
 * CALIBRATE
 ************************************************************/
bool calibrateRadio()
{
  Serial.println("Calibrating frequency synthesizer...");

  strobe(SIDLE);
  waitMarc(MARC_IDLE, 100);

  strobe(SCAL);

  if (!waitMarc(MARC_IDLE, 500))
  {
    Serial.printf(
      "CAL FAILED. MARCSTATE = 0x%02X\n",
      readExt(MARCSTATE)
    );

    return false;
  }

  Serial.println("Calibration complete.");
  return true;
}


/************************************************************
 * RADIO INITIALIZATION
 ************************************************************/
bool initializeRadio()
{
  hardwareReset();

  uint8_t part = readExt(PARTNUMBER);
  uint8_t ver  = readExt(PARTVERSION);

  Serial.printf("PARTNUMBER  = 0x%02X\n", part);
  Serial.printf("PARTVERSION = 0x%02X\n", ver);

  if (part != 0x48)
  {
    Serial.println("ERROR: CC1120 not detected!");
    return false;
  }

  configureRadio();

  /**********************************************************
   * Quick sanity check that frequency/band settings stuck
   **********************************************************/
  Serial.printf(
    "FS_CFG = 0x%02X\n",
    readReg(0x21)
  );

  Serial.printf(
    "FREQ = %02X %02X %02X\n",
    readExt(0x0C),
    readExt(0x0D),
    readExt(0x0E)
  );

  if (!calibrateRadio())
    return false;

  return true;
}


/************************************************************
 * TRANSMITTER
 ************************************************************/
uint32_t packetCounter = 0;

void transmitPacket()
{
  char message[50];

  snprintf(
    message,
    sizeof(message),
    "HELLO kuku",
    (unsigned long)packetCounter
  );

  uint8_t payloadLen = strlen(message);


  /**********************************************************
   * Variable length mode requires:
   *
   * BYTE 0 = payload length
   * BYTE 1...N = payload
   **********************************************************/

  uint8_t txBuffer[64];

  txBuffer[0] = payloadLen;

  memcpy(
    &txBuffer[1],
    message,
    payloadLen
  );

  uint8_t totalBytes = payloadLen + 1;


  /**********************************************************
   * Prepare transmitter
   **********************************************************/

  strobe(SIDLE);
  waitMarc(MARC_IDLE, 100);

  strobe(SFTX);

  delay(1);


  /**********************************************************
   * Write packet into FIFO
   **********************************************************/

  writeTxFIFO(
    txBuffer,
    totalBytes
  );

  uint8_t fifoBefore =
    readExt(NUM_TXBYTES) & 0x7F;

  Serial.println();
  Serial.printf(
    "TX #%lu: \"%s\"\n",
    (unsigned long)packetCounter,
    message
  );

  Serial.printf(
    "TX FIFO before STX = %u bytes\n",
    fifoBefore
  );


  /**********************************************************
   * TRANSMIT!
   **********************************************************/

  strobe(STX);


  /**********************************************************
   * Wait until FIFO begins draining
   **********************************************************/

  uint32_t start = millis();

  while (
    ((readExt(NUM_TXBYTES) & 0x7F) == fifoBefore)
    &&
    ((millis() - start) < 500)
  )
  {
    delay(1);
  }


  /**********************************************************
   * Wait until radio returns to IDLE
   **********************************************************/

  if (waitMarc(MARC_IDLE, 1000))
  {
    Serial.println("TX COMPLETE");
  }
  else
  {
    Serial.printf(
      "TX TIMEOUT - MARCSTATE = 0x%02X\n",
      readExt(MARCSTATE)
    );

    strobe(SIDLE);
    strobe(SFTX);
  }

  packetCounter++;
}


/************************************************************
 * RECEIVER
 ************************************************************/
void enterReceive()
{
  strobe(SIDLE);
  waitMarc(MARC_IDLE, 100);

  strobe(SFRX);

  delay(1);

  strobe(SRX);

  if (waitMarc(MARC_RX, 100))
  {
    Serial.println("RX listening...");
  }
  else
  {
    Serial.printf(
      "Could not enter RX. MARCSTATE = 0x%02X\n",
      readExt(MARCSTATE)
    );
  }
}


void checkForPacket()
{
  uint8_t rxCount =
    readExt(NUM_RXBYTES) & 0x7F;

  if (rxCount == 0)
    return;


  /**********************************************************
   * Some bytes are present. Wait for complete packet.
   *
   * Default RX end behavior returns CC1120 to IDLE.
   **********************************************************/

  uint32_t start = millis();

  while (
    readExt(MARCSTATE) == MARC_RX &&
    (millis() - start) < 500
  )
  {
    delay(1);
  }

  delay(2);

  rxCount =
    readExt(NUM_RXBYTES) & 0x7F;


  if (rxCount == 0)
  {
    enterReceive();
    return;
  }


  /**********************************************************
   * Protect our local array
   **********************************************************/

  if (rxCount > 127)
  {
    Serial.println("RX FIFO invalid/overflow.");

    strobe(SIDLE);
    strobe(SFRX);
    enterReceive();

    return;
  }


  uint8_t rxBuffer[128];

  readRxFIFO(
    rxBuffer,
    rxCount
  );


  /**********************************************************
   * VARIABLE LENGTH PACKET:
   *
   * rxBuffer[0] = payload length
   **********************************************************/

  uint8_t payloadLen =
    rxBuffer[0];


  Serial.println();
  Serial.println("==============================");
  Serial.println("PACKET RECEIVED");
  Serial.println("==============================");

  Serial.printf(
    "FIFO bytes: %u\n",
    rxCount
  );

  Serial.printf(
    "Payload length: %u\n",
    payloadLen
  );


  /**********************************************************
   * Expected FIFO:
   *
   * length
   * payload bytes
   * RSSI status
   * LQI/CRC status
   **********************************************************/

  if (
    payloadLen > 0 &&
    payloadLen <= 60 &&
    rxCount >= payloadLen + 3
  )
  {
    Serial.print("Message: ");

    for (
      uint8_t i = 0;
      i < payloadLen;
      i++
    )
    {
      char c =
        (char)rxBuffer[i + 1];

      Serial.print(c);
    }

    Serial.println();


    uint8_t rssiStatus =
      rxBuffer[payloadLen + 1];

    uint8_t lqiStatus =
      rxBuffer[payloadLen + 2];

    bool crcOK =
      (lqiStatus & 0x80) != 0;


    Serial.printf(
      "RSSI status raw: 0x%02X\n",
      rssiStatus
    );

    Serial.printf(
      "LQI status raw:  0x%02X\n",
      lqiStatus
    );

    Serial.print("CRC: ");

    if (crcOK)
      Serial.println("PASS");
    else
      Serial.println("FAIL");
  }
  else
  {
    Serial.println(
      "Unexpected packet structure."
    );

    Serial.print("Raw FIFO: ");

    for (uint8_t i = 0; i < rxCount; i++)
    {
      Serial.printf(
        "%02X ",
        rxBuffer[i]
      );
    }

    Serial.println();
  }


  /**********************************************************
   * Get ready for next packet
   **********************************************************/

  enterReceive();
}


/************************************************************
 * SETUP
 ************************************************************/
void setup()
{
  Serial.begin(115200);

  delay(2000);

  Serial.println();
  Serial.println();
  Serial.println("================================");
  Serial.println(" CC1120 434 MHz LINK TEST");
  Serial.println(" XIAO ESP32-S3");
  Serial.println("================================");


#if ROLE_TX

  Serial.println("ROLE: TRANSMITTER");

#else

  Serial.println("ROLE: RECEIVER");

#endif


  pinMode(RF_CS, OUTPUT);
  pinMode(RF_RESET, OUTPUT);
  pinMode(RF_MISO, INPUT);

  digitalWrite(RF_CS, HIGH);
  digitalWrite(RF_RESET, HIGH);


  SPI.begin(
    RF_SCLK,
    RF_MISO,
    RF_MOSI,
    RF_CS
  );


  if (!initializeRadio())
  {
    Serial.println(
      "RADIO INITIALIZATION FAILED."
    );

    while (true)
      delay(1000);
  }


  Serial.println();
  Serial.println("RADIO INITIALIZATION PASS");


#if !ROLE_TX

  enterReceive();

#endif
}


/************************************************************
 * LOOP
 ************************************************************/
void loop()
{

#if ROLE_TX

  transmitPacket();

  // One short packet every second
  delay(1000);

#else

  checkForPacket();

  delay(2);

#endif
}
