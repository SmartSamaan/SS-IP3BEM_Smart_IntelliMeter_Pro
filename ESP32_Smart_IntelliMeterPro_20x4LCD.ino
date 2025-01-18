#include <LiquidCrystal.h>
#include <splash.h>
//#include <TimerOne.h>
#include <SPI.h>
#include <ADE7758.h>
#include <EEPROM.h>
#include <WiFi.h>
#include <HTTPClient.h>


TaskHandle_t HTTPTaskHandle;
// WiFi credentials
const char* ssid = "Galaxy A52s 5G35D8";
const char* password = "sajjad7193";
// Server URL
const char* serverURL = "http://192.168.118.31/Energy_Meter/insert_data.php"; // Replace with your server's IP
uint8_t WiFi_Connect_Count = 0;

#define ADE_IRQ_PIN 39      // IRQ pin for interrupt handling
#define ADE_APCF_PIN 34
#define ADE_VARCF_PIN 35
/*
==============================================================
*/
#define SW1_PIN 0
#define SW2_PIN 36
#define DEBOUNCE_TIME 50 // Debounce time in milliseconds

unsigned long lastDebounceTimeSW1 = 0; // Last debounce time for SW1
unsigned long lastDebounceTimeSW2 = 0; // Last debounce time for SW2
bool lastSW1State = HIGH; // Last stable state of SW1
bool lastSW2State = HIGH; // Last stable state of SW2
/*
==============================================================
*/
// Define EEPROM size
#define EEPROM_SIZE 32
const int rs = 4, rw = 2, en = 32, d4 = 27, d5 = 26, d6 = 25, d7 = 33;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

ADE7758 EnergyMeter;
uint32_t V_Register[3], I_Register[3];
int16_t WattHR_Register[3], VAHR_Register[3], VARHR_Register[3];
bool EnergyFlag = false, DisplayFlag = false, READ_ADE_FLAG = false;

float Volt_A = 0, Volt_B = 0, Volt_C = 0;
float Curr_A = 0, Curr_B = 0, Curr_C = 0;
double Pow_A = 0, Pow_B = 0, Pow_C = 0;
double VAR_Pow_A = 0, VAR_Pow_B = 0, VAR_Pow_C = 0;
double VA_Pow_A = 0, VA_Pow_B = 0, VA_Pow_C = 0;
double Ener_A = 0, Ener_B = 0, Ener_C = 0;
uint8_t OneSec = 0, WinNumberOld = 0, FourSec = 0;
bool IsInitialized = true;
/*
==============================================================
*/
int8_t WinNum = 0;
/*
==============================================================
*/
hw_timer_t * timer = NULL;
void IRAM_ATTR timer_isr() 
{
    // This code will be executed every 1000 ticks, 1ms
  DisplayFlag = true;
  Ener_A += Pow_A/3600;
  Ener_B += Pow_B/3600;
  Ener_C += Pow_C/3600;
  OneSec++;
}

/*
==============================================================
*/
void IRAM_ATTR handleADEIRQ() {
  // Interrupt Service Routine (ISR) for handling ADE7758 IRQ
  //Serial.println("IRQ triggered! Reading data...");
  //Serial.println("IRQ Outer");
  //if(!digitalRead(AFECS))
  {
    
    READ_ADE_FLAG = true;
    //Serial.println("IRQ");
  }
  
}

void setup() {
  //===========SERIAL INITIALIZATION==========================
  Serial.begin(115200);
  //===========20x4 LCD INITIALIZATION==========================
  pinMode(rw, OUTPUT);
  digitalWrite(rw, LOW);
  lcd.begin(20, 4);
  lcd.setCursor(3, 1);
  lcd.print("  Device is");
  lcd.setCursor(3, 2);
  lcd.print("Initializing...");
  delay(2000);
  //===========WIFI INITIALIZATION==========================
  WiFi.mode(WIFI_OFF);
  delay(1000);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  lcd.setCursor(3, 1);
  lcd.print("  Connecting");
  lcd.setCursor(3, 2);
  lcd.print("  to WiFi");
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) 
  {
    WiFi_Connect_Count++;
    if(WiFi_Connect_Count == 1)
    {
      lcd.setCursor(12, 2);
      lcd.print(".      ");
    }
    else if(WiFi_Connect_Count == 2)
    {
      lcd.setCursor(12, 2);
      lcd.print("..     ");
    }
    else if(WiFi_Connect_Count == 3)
    {
      WiFi_Connect_Count = 0;
      lcd.setCursor(12, 2);
      lcd.print("...    ");
    }
    Serial.print(".");
    delay(1000); 
  }
  lcd.clear();
  lcd.setCursor(3, 1);
  lcd.print("WiFi Connected");
  Serial.println("WiFi Connected");
  //===========ADE7758 INITIALIZATION==========================
  pinMode(ADE_IRQ_PIN, INPUT_PULLUP); // Configure IRQ_PIN as input with pull-up resistor
  attachInterrupt(digitalPinToInterrupt(ADE_IRQ_PIN), handleADEIRQ, FALLING); // Attach interrupt to the IRQ pin
  if(EnergyMeter.Init())
    Serial.println("ADE7758 is Initialized Successfully!");
  Calibrate_ADE7758();
  //pinMode(ADE_APCF_PIN, PULLDOWN);
  //pinMode(ADE_VARCF_PIN, PULLDOWN);
  //===========EEPROM INITIALIZATION==========================
  if (!EEPROM.begin(EEPROM_SIZE))
  {
    Serial.println("Failed to initialize EEPROM");
  }
  EEPROM_ReadEnergyData(); // Read Energy Data from EEPROM
  //Ener_A = 0; Ener_B = 0; Ener_C = 0;
  //===========TIMER INTERRUPT INITIALIZATION==========================
  timer = timerBegin(1000000);
  timerAttachInterrupt(timer, &timer_isr);
  timerAlarm(timer, 1000000, true, 0);
  //===========TASK INITIALIZATION==========================
  // Create a separate task for HTTP requests
  xTaskCreatePinnedToCore(
    HTTPTask,         // Function to be executed
    "HTTPTask",       // Task name
    10000,            // Stack size (bytes)
    NULL,             // Parameter
    1,                // Task priority
    &HTTPTaskHandle,  // Task handle
    1                 // Core
  );
  //
  // Initialize switches
  pinMode(SW1_PIN, INPUT);
  pinMode(SW2_PIN, INPUT);
}
void EEPROM_ReadEnergyData()
{
  int32_t temp = 0;
  temp = EEPROM.read(0);
  temp |= (int16_t)(EEPROM.read(1) << 8);
  temp |= (int32_t)(EEPROM.read(2) << 16);
  temp |= (int32_t)(EEPROM.read(3) << 24);
  Ener_A = (double)temp/1000;
  
  temp = EEPROM.read(4);
  temp |= (int16_t)(EEPROM.read(5) << 8);
  temp |= (int32_t)(EEPROM.read(6) << 16);
  temp |= (int32_t)(EEPROM.read(7) << 24);
  Ener_B = (double)temp/1000;
  temp = EEPROM.read(8);
  temp |= (int16_t)(EEPROM.read(9) << 8);
  temp |= (int32_t)(EEPROM.read(10) << 16);
  temp |= (int32_t)(EEPROM.read(11) << 24);
  Ener_C = (double)temp/1000;
}
void EEPROM_WriteEnergyData()
{
  int32_t temp = Ener_A * 1000;
  EEPROM.write(0, (temp & 0xFF));
  EEPROM.write(1, ((temp >> 8) & 0xFF));
  EEPROM.write(2, ((temp >> 16) & 0xFF));
  EEPROM.write(3, ((temp >> 24) & 0xFF));
  temp = Ener_B * 1000;
  EEPROM.write(4, (temp & 0xFF));
  EEPROM.write(5, ((temp >> 8) & 0xFF));
  EEPROM.write(6, ((temp >> 16) & 0xFF));
  EEPROM.write(7, ((temp >> 24) & 0xFF));
  temp = Ener_C * 1000;
  EEPROM.write(8, (temp & 0xFF));
  EEPROM.write(9, ((temp >> 8) & 0xFF));
  EEPROM.write(10, ((temp >> 16) & 0xFF));
  EEPROM.write(11, ((temp >> 24) & 0xFF));
  EEPROM.commit();
}
void loop() {
  handleSwitches();
  Read_ADE7758(); // Read data from ADE7758 registers
  if(DisplayFlag)
  {
    if(IsInitialized)
    {
      lcd.clear();
      IsInitialized = false;
    }
    Display_Data(WinNum);
  }
  if(OneSec >= 4)
  {
    //Serial.println(Ener_A);
    //Serial.print(EnergyMeter.getWatt(PHASE_A), DEC);
    //Serial.print("\t");
    //Serial.println(EnergyMeter.getVa(PHASE_A), DEC);
    OneSec = 0;
    FourSec++;
    WinNum++;
    if(WinNum >= 6)
    {
      WinNum = 0;
    }
  }
  if(FourSec > 1)
  {
    FourSec = 0;
    EEPROM_WriteEnergyData();
    // Signal HTTP task to send data
    xTaskNotify(HTTPTaskHandle, 0, eNoAction);
  }

}
void handleSwitches()
{
  // Read switches
    bool currentSW1State = digitalRead(SW1_PIN);
    bool currentSW2State = digitalRead(SW2_PIN);

    // Handle SW1 (next window)
    if (currentSW1State != lastSW1State) { // State changed
        if (millis() - lastDebounceTimeSW1 > DEBOUNCE_TIME) { // Debounce check
            lastDebounceTimeSW1 = millis();
            if (currentSW1State == LOW) { // Button pressed
                OneSec = 0;
                WinNum--;
                if (WinNum < 0) WinNum = 5; // Wrap around
                //Serial.println("SW1 Pressed: Next Window");
            }
        }
    }
    lastSW1State = currentSW1State;

    // Handle SW2 (previous window)
    if (currentSW2State != lastSW2State) { // State changed
        if (millis() - lastDebounceTimeSW2 > DEBOUNCE_TIME) { // Debounce check
            lastDebounceTimeSW2 = millis();
            if (currentSW2State == LOW) { // Button pressed
                OneSec = 0;
                WinNum++;
                if (WinNum >= 6) WinNum = 0; // Wrap around
                //Serial.println("SW2 Pressed: Previous Window");
            }
        }
    }
    lastSW2State = currentSW2State;
}
// Task for sending HTTP requests
void HTTPTask(void *pvParameters) {
  while (true) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // Wait for notification

    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(serverURL);

      String postData = String("phase1_voltage=") + Volt_A + "&phase1_current=" + Curr_A +
                        "&phase1_active_power=" + Pow_A + "&phase1_reactive_power=" + VAR_Pow_A +
                        "&phase1_apparent_power=" + VA_Pow_A + "&phase1_energy=" + Ener_A +
                        "&phase2_voltage=" + Volt_B + "&phase2_current=" + Curr_B +
                        "&phase2_active_power=" + Pow_B + "&phase2_reactive_power=" + VAR_Pow_B +
                        "&phase2_apparent_power=" + VA_Pow_B + "&phase2_energy=" + Ener_B +
                        "&phase3_voltage=" + Volt_C + "&phase3_current=" + Curr_C +
                        "&phase3_active_power=" + Pow_C + "&phase3_reactive_power=" + VAR_Pow_C +
                        "&phase3_apparent_power=" + VA_Pow_C + "&phase3_energy=" + Ener_C;

      http.addHeader("Content-Type", "application/x-www-form-urlencoded");

      int httpResponseCode = http.POST(postData);
      if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.println("Server Response: " + response);
      } else {
        Serial.println("Error in HTTP request");
      }

      http.end();
    }
  }
}
void printWithNewline(LiquidCrystal &lcd, const String &text) {
  int column = 0, row = 0; // Start at the first column and row
  for (unsigned int i = 0; i < text.length(); i++) {
    if (text[i] == '\n') {
      row++;               // Move to the next row
      column = 0;          // Reset column
      if (row < 4) lcd.setCursor(column, row); // Move cursor if within bounds
    } else {
      lcd.print(text[i]);
      column++;
      if (column >= 20) {  // Automatically move to the next row if text overflows
        row++;
        column = 0;
        if (row < 4) lcd.setCursor(column, row); // Move cursor if within bounds
      }
    }
    if (row >= 4) break; // Prevent overflow if the LCD has only 4 rows
  }
}
void Display_Data(uint8_t WindowNum)
{
  DisplayFlag = false;
  String dispText[] = {"   VOLTAGES (V)\n", "   CURRENTS (A)\n", "   A-POWER (kW)\n", "  R-POWER (kVAR)\n", "  AP-POWER (kVA)\n", "   ENERGY (kWh)\n"};
  switch(WindowNum)
  {
    case 0:
      dispText[WindowNum] += "Ph-A:  " + String(Volt_A, 1) + "\nPh-B:  " + String(Volt_B, 1) + "\nPh-C:  " + String(Volt_C, 1);
      break;
    case 1:
      dispText[WindowNum] += "Ph-A:  " + String(Curr_A, 3) + "\nPh-B:  " + String(Curr_B, 3) + "\nPh-C:  " + String(Curr_C, 3);
      break;
    case 2:
      dispText[WindowNum] += "Ph-A:  " + String(Pow_A, 3) + "\nPh-B:  " + String(Pow_B, 3) + "\nPh-C:  " + String(Pow_C, 3);
      break;
    case 3:
      dispText[WindowNum] += "Ph-A:  " + String(VAR_Pow_A, 3) + "\nPh-B:  " + String(VAR_Pow_B, 3) + "\nPh-C:  " + String(VAR_Pow_C, 3);
      break;
    case 4:
      dispText[WindowNum] += "Ph-A:  " + String(VA_Pow_A, 3) + "\nPh-B:  " + String(VA_Pow_B, 3) + "\nPh-C:  " + String(VA_Pow_C, 3);
      break;
    case 5:
      Ener_A = (fabs(Ener_A) < 0.001) ? 0.0: Ener_A;
      Ener_B = (fabs(Ener_B) < 0.001) ? 0.0: Ener_B;
      Ener_C = (fabs(Ener_C) < 0.001) ? 0.0: Ener_C;
      dispText[WindowNum] += "Ph-A:  " + String(Ener_A, 3) + "\nPh-B:  " + String(Ener_B, 3) + "\nPh-C:  " + String(Ener_C, 3);
      break;
  }
  // Clear the buffer
  if(WinNumberOld != WindowNum)
  {
    lcd.clear();
    WinNumberOld = WindowNum;
  }
  lcd.setCursor(0,0);
  printWithNewline(lcd, dispText[WindowNum]);
}
void Calibrate_ADE7758(void)
{
  EnergyMeter.setOpMode(0x00); // All HPF in Current Channels are Disabled, LPF2's are disabled, Frequency output is enabled
  EnergyMeter.gainSetup(INTEGRATOR_OFF,FULLSCALESELECT_0_5V,GAIN_1,GAIN_4); // Voltage gain 1, Current gain 4
  EnergyMeter.setupDivs(0x800,0x800,0x800); //Divider of the values ??that are stored in the power registers
  EnergyMeter.setLcycMode(0x0F);    // Phase A zero-crossing is used
  EnergyMeter.setLineCyc(20);  // Get data of 100 cycles
  EnergyMeter.setMaskInterrupts(ZXTOA | ZXTOB | ZXTOC | LENERGY | ZXA | ZXB | ZXC);
  EnergyMeter.setCompMode(0x24);    // Phase A contributes to APCF
  EnergyMeter.setAPCFNUM(0);
  EnergyMeter.setVARCFNUM(0);
  EnergyMeter.setAPCFDEN(3235);
  EnergyMeter.setVARCFDEN(3235);
  //
  EnergyMeter.setAVoltageOffset(-210);
  EnergyMeter.setBVoltageOffset(-247);
  EnergyMeter.setCVoltageOffset(-385);
  //
  EnergyMeter.setACurrentOffset(-26);
  EnergyMeter.setBCurrentOffset(-26);
  EnergyMeter.setCCurrentOffset(-26);
  //
  EnergyMeter.setAWattOffset(-10);
  //setBWattOffset(0);
  //setCWattOffset(0);
  EnergyMeter.setAWG(0);
  EnergyMeter.setBWG(0);
  EnergyMeter.setCWG(0);
  EnergyMeter.setAVARG(0);
  EnergyMeter.setBVARG(0);
  EnergyMeter.setCVARG(0);
  EnergyMeter.setAVAG(0);
  EnergyMeter.setBVAG(0);
  EnergyMeter.setCVAG(0);
  /*(setAVoltageOffset(1000);
  setBVoltageOffset(0);
  setCVoltageOffset(0);
  setACurrentOffset(0);
  setBCurrentOffset(0);
  setCCurrentOffset(0);*/
  
  //setPotLine(1,50);
  EnergyMeter.resetStatus();
}
void Read_ADE7758(void)
{
  uint32_t ADE7758_Status;
  if(READ_ADE_FLAG)
  {
    noInterrupts();
    ADE7758_Status = EnergyMeter.resetStatus();
    //EnergyMeter.setMaskInterrupts(0);
    READ_ADE_FLAG = false;
    
    if(ADE7758_Status & ZXTOA)
    {
      V_Register[0] = 0;
      I_Register[0] = 0;
      WattHR_Register[0] = 0;
    }
    if(ADE7758_Status & ZXTOB)
    {
      V_Register[1] = 0;
      I_Register[1] = 0;
      WattHR_Register[1] = 0;
    }
    if(ADE7758_Status & ZXTOC)
    {
      V_Register[2] = 0;
      I_Register[2] = 0;
      WattHR_Register[2] = 0;
    }
    if(ADE7758_Status & LENERGY)
    {
      WattHR_Register[0] = EnergyMeter.getWatt(PHASE_A);
      WattHR_Register[1] = EnergyMeter.getWatt(PHASE_B);
      WattHR_Register[2] = EnergyMeter.getWatt(PHASE_C);
      VARHR_Register[0] = EnergyMeter.getVar(PHASE_A);
      VARHR_Register[1] = EnergyMeter.getVar(PHASE_B);
      VARHR_Register[2] = EnergyMeter.getVar(PHASE_C);
      VAHR_Register[0] = EnergyMeter.getVa(PHASE_A);
      VAHR_Register[1] = EnergyMeter.getVa(PHASE_B);
      VAHR_Register[2] = EnergyMeter.getVa(PHASE_C);
      EnergyFlag = true;
    }
    if(ADE7758_Status & ZXA)
    {
      V_Register[0] = EnergyMeter.getAVRMS();
      I_Register[0] = EnergyMeter.getAIRMS();
    }
    if(ADE7758_Status & ZXB)
    {
      V_Register[1] = EnergyMeter.getBVRMS();
      I_Register[1] = EnergyMeter.getBIRMS();
    }
    if(ADE7758_Status & ZXC)
    {
      V_Register[2] =EnergyMeter.getCVRMS();
      I_Register[2] = EnergyMeter.getCIRMS();
    }
  }
  interrupts();
  //EnergyMeter.setMaskInterrupts(ZXTOA | LENERGY | ZXA | ZXB | ZXC);
  Volt_A = (float)(V_Register[0]/3923.19);
  Volt_A = (Volt_A < 20) ? 0.0: (Volt_A>400)?0:Volt_A;
  Volt_B = (float)(V_Register[1]/3923.19);
  Volt_B = (Volt_B < 20) ? 0.0: (Volt_B>400)?0:Volt_B;
  Volt_C = (float)(V_Register[2]/3923.19);
  Volt_C = (Volt_C < 20) ? 0.0: (Volt_C>400)?0:Volt_C;
  Curr_A = (float)(I_Register[0]/142600.0);
  Curr_A = (Curr_A < 0.04) ? 0.0: Curr_A;
  Curr_B = (float)(I_Register[1]/142600.0);
  Curr_B = (Curr_B < 0.04) ? 0.0: Curr_B;
  Curr_C = (float)(I_Register[2]/142600.0);
  Curr_C = (Curr_C < 0.04) ? 0.0: Curr_C;
  Pow_A = (double)WattHR_Register[0]/2273.5;
  Pow_A = (fabs(Pow_A) < 0.005) ? 0.0: Pow_A;
  Pow_B = (double)WattHR_Register[1]/2273.5;
  Pow_B = (fabs(Pow_B) < 0.005) ? 0.0: Pow_B;
  Pow_C = (double)WattHR_Register[2]/2273.5;
  Pow_C = (fabs(Pow_C) < 0.005) ? 0.0: Pow_C;
  VAR_Pow_A = (double)VARHR_Register[0]/2273.5;
  VAR_Pow_A = (fabs(VAR_Pow_A) < 0.005) ? 0.0: VAR_Pow_A;
  VAR_Pow_B = (double)VARHR_Register[1]/2273.5;
  VAR_Pow_B = (fabs(VAR_Pow_B) < 0.005) ? 0.0: VAR_Pow_B;
  VAR_Pow_C = (double)VARHR_Register[2]/2273.5;
  VAR_Pow_C = (fabs(VAR_Pow_C) < 0.005) ? 0.0: VAR_Pow_C;
  //
  VA_Pow_A = (double)VAHR_Register[0]/2273.5;
  VA_Pow_A = (fabs(VA_Pow_A) < 0.005) ? 0.0: VA_Pow_A;
  VA_Pow_B = (double)VAHR_Register[1]/2273.5;
  VA_Pow_B = (fabs(VA_Pow_B) < 0.005) ? 0.0: VA_Pow_B;
  VA_Pow_C = (double)VAHR_Register[2]/2273.5;
  VA_Pow_C = (fabs(VA_Pow_C) < 0.005) ? 0.0: VA_Pow_C;
}
