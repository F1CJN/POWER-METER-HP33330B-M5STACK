/*
 * M5Stack Wattmetre RF utilisant une sonde Hewlett Packard HP33330B
 * M5stack RF Wattmeter using an HP33330B RF negative detector
 * By Ham radio F1CJN     HP33330B from F1GE f1ge.mg at gmail.com
 * alain.fort.f1cjn at gmail.com
 * Version V 6.0 avec possibilité s'étalonnage de la sonde
 * Version V7 avec voltmètre à aiguille
 * Version V8 avec choix attenuateur d'entrée
 * Version V9 avec choix des couleurs de texte
 * Voltage = voltage read by the ADS1115 from the output of the OP192 == (HP33330B voltage x -2)
*/

#include <M5Stack.h>
#include<ADS1115_WE.h> 
#include<Wire.h>
#include<EEPROM.h>

ADS1115_WE adc = ADS1115_WE(0x48);

#define M_SIZE 1.3333 // Define meter size at 1.3333
#define RGB(r, g, b) (((r&0xF8)<<8)|((g&0xFC)<<3)|(b>>3))
#define TFT_GREY 0x5AEB
#define TFT_BLUE 0x001F
#define TFT_AZURE RGB(0, 128, 255)

uint16_t osx = M_SIZE*120, osy = M_SIZE*120; // Saved x & y coords
uint32_t updateTime = 0;       // time for next update
float ltx = 0;    // Saved x coord of bottom of needle
float voltage = 0;
float voltage_m = 0;

float puissance_uW = 0;
float puissance_dBm = 0;
float v_m;

int interval = 10;  // Update interval 10 milliseconds
int old_analog =  -999; // Value last displayed
int value = 2,att=0;
int d = 0;
int flag=0;
int flag_BtnB=1;
int gamme = 1, oldgamme=3;
int moins = true ;
int Ctext = TFT_WHITE; // couleur texte;
int Ctext1 = TFT_AZURE; // couleur texte1;
bool toggle =true ;
int MeterLabel[11] = {-20,-19,-18,-17,-16,-15,-14,-13,-12,-11,-10};  //dBm
String MeterLabeldB10[11] = {"0", "1", "2", "3", "4","5","6","7","8","9","10"}; //dBm
String MeterLabeldB00[11] = {"-10", "-9", "-8", "-7", "-6","-5","-4","-3","-2","-1","0"};//dBm
String MeterLabeldB01[11] = {"-20","-19", "-18", "-17", "-16","-15","-14","-13","-12","-11","-10"};//dBm

//************         Pour un étallonnage précis de votre sonde HP33330B         **************
//********** Entrez ci-dessous les valeurs mesurées avec un générateur de précision*************
//***************               valeurs  par défaut avec ma sonde              *******************
float v_m10 = 43;  // Tension mesurée en mV par le M5stack avec -10dBm au géné
float v_m5 = 101;  // Tension mesurée en mV par le M5stack avec -5dBm au géné
float v_0 =  242;  // Tension mesurée en mV par le M5stack avec  0dBm au géné
float v_5 =  504;  // Tension mesurée en mV par le M5stack avec +5dBm au géné
float v_10 = 960;  // Tension mesurée en mV par le M5stack avec +10dBm au géné
float v_15 = 1900; // Tension mesurée en mV par le M5stack avec +15dBm au géné
float v_20 = 3600; // Tension mesurée en mV par le M5stack avec +20dBm au géné
//**********************************************************************************************

void setup(void) {
  M5.begin();
  Wire.begin();
  Serial.begin(115200);EEPROM.begin(16);
  flag_BtnB=EEPROM.read(0); 
  if( flag_BtnB==1 ||flag_BtnB==2 ||flag_BtnB==3 ){text();}  // 
  else {flag_BtnB=1; text();}
  adc.setCompareChannels(ADS1115_COMP_1_GND);

  if(!adc.init()){Serial.println("ADS1115 not connected");
  M5.Lcd.setTextColor(BLUE,BLACK);
  M5.Lcd.drawString(" ASD1115 not connected",5,40,4); }

  adc.setVoltageRange_mV(ADS1115_RANGE_2048); //ADC range = 2,047V  avec 32768 valeurs
  M5.Lcd.setTextColor(YELLOW, BLACK);
  M5.lcd.fillScreen(BLACK);
  M5.Lcd.drawCentreString("HP33330B",160,60,4);
  M5.Lcd.drawCentreString("RF POWER METER",160,100,4);
  M5.Lcd.drawCentreString("       F1CJN  F1GE  F1BHY       ",160,100,4);
  delay(2000);

  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setTextColor(Ctext1, TFT_BLACK); 
  M5.Lcd.drawCentreString("Att", 68, 220, 2);
  M5.Lcd.drawCentreString("Color", 160, 220, 2);
  M5.Lcd.drawCentreString("Volt/dBm",252,220,2); 
  updateTime = millis(); // Save update time
  dacWrite (25,0);// to stop the M5stack core screen white noise !
 
}

void loop() {
M5.Lcd.setTextColor(Ctext1, TFT_BLACK); 
  M5.Lcd.drawCentreString("Att", 68, 220, 2);
  M5.Lcd.drawCentreString("Color", 160, 220, 2);
  M5.Lcd.drawCentreString("Volt/dBm",252,220,2); 


  dacWrite (25,0);// to stop the M5stack core screen white noise !
  if (updateTime <= millis()) {
    updateTime = millis() + interval; // Update interval
    
     v_m = 0;voltage=0;
    for (int i = 1; i <= 10; i++) {
     voltage = readChannel(ADS1115_COMP_0_GND); // mesure ADC
     delay(1);
    v_m = v_m + voltage; }// tension en mv
 
    voltage = v_m/10 ; // moyenne de 10 mesures  // Vmax = 4V avec ampli G=2
    Serial.print("Voltage mV: ");Serial.println(voltage);
    voltage_m=voltage;
   if (voltage<=0) {(voltage=0.02);voltage_m=0.02; }  // 
   if (voltage<=v_m10) {puissance_dBm= (log10(voltage)+0.41)*10-30;}    //OK                      // -30 à -10dBm
   if ((voltage>v_m10)&&(voltage<=v_m5)) {puissance_dBm=((5/(v_m5-v_m10))*(voltage-v_m10))-10;}   // -10 à -5dBm 
   if ((voltage>v_m5)&&(voltage<=v_0))  {puissance_dBm=((5/(v_0-v_m5))*(voltage-v_m5))-5;}        // -5 à 0dBm
   if ((voltage>v_0)&&(voltage<=v_5))   {puissance_dBm=((5/(v_5-v_0))*(voltage-v_0));}            // 0 à +5dBm
   if ((voltage>v_5)&&(voltage<=v_10))  {puissance_dBm=((5/(v_10-v_5))*(voltage-v_5))+5;}         // +5 à +10dBm
   if ((voltage>v_10)&&(voltage<=v_15)) {puissance_dBm=((5/(v_15-v_10))*(voltage-v_10))+10;}      // +10 à +15dBm
   if ((voltage>v_15)&&(voltage<=v_20)) {puissance_dBm=((5/(v_20-v_15))*(voltage-v_15))+15;}      // +15 à +20dBm
   if (voltage>v_20) {puissance_dBm=(v_20);}  //  Butée si >+20dBm 
  
   //Serial.print("Puissance dBm: "); Serial.println(puissance_dBm,DEC);

  //M5.Lcd.drawCentreString("      "+String(voltage)+ "  mV        ", 160,140, 4);  
  //M5.Lcd.drawCentreString("      "+String(puissance_dBm)+ "  dBm        ", 160, 180, 4);  
 }
 float z= pow(10,((puissance_dBm+30)/10));Serial.print("z=");Serial.println(z); // power with microwatt
 puissance_uW =z;

    if ((puissance_dBm>=10) && (puissance_dBm <=20))          // gamme 10 à 20dBm
   {value=(puissance_dBm-10)*10;gamme=att +30;}    
   else if ((puissance_dBm>=0) && (puissance_dBm <=10))       // gamme 0 à 10dBm
   {value=puissance_dBm * 10;gamme=att +20;} 
   else if ((puissance_dBm <=0) && (puissance_dBm >=-10))     // gamme -10 à 0dBm
   {value=(puissance_dBm +10)* 10;gamme=att+10;} 
    else if ((puissance_dBm <=-10) && (puissance_dBm >=-20))  //gamme -20 à -10dBm
   {value=(puissance_dBm +20)* 10;gamme=att;}
    else if ((puissance_dBm <=-20) && (puissance_dBm >=-30))  //gamme -30 à -20dBm
   {value=(puissance_dBm +30)* 10;gamme=att-10;}
   else if (puissance_dBm <=-30) {value =-2;gamme=att-10;}   //gamme -30 à -20dBm{value=-2;}  

    //if (value <=-2) {value =-2;}   // Value negative limit for needle
    
    if (flag==0){gamme=0; analogMeter();plotNeedle(1,10);flag=1;}  // mise sous tension
    if (gamme <= oldgamme){moins=true;}else {moins=false;} 
    if ((gamme != oldgamme) || (flag==0)){oldgamme=gamme; analogMeter() ;flag=1;}    // changement de gamme  
   
     Serial.print("gamme=");Serial.println(gamme);
   
    M5.update();
    if (M5.BtnA.wasPressed()) { 
    att=att+10; if (att==50)att=-10;  
    //plotNeedle(value,10);
    delay(10);
    }
    
    else if (M5.BtnB.wasPressed()) {  //Couleurs
    flag_BtnB = flag_BtnB + 1;
    if (flag_BtnB==4) {flag_BtnB=1;}
    text(); 
    }
  
    else if (M5.BtnC.isPressed())  {     //Affichage Volt/dBm
      toggle = !toggle;
      delay(100);   }

     if (toggle==false) {
     M5.Lcd.drawCentreString("                                                 ",160, 172, 4); //Efface
     M5.Lcd.drawCentreString("                                                 ",160, 197, 4); //Efface
     M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
     M5.Lcd.drawCentreString("           Voltage         ", 160, 172, 4);
     M5.Lcd.drawCentreString("      "+String(voltage_m)+ "  mV        ", 160, 197, 4);  
     } 

     else if (toggle ==true){
     M5.Lcd.setTextColor(TFT_BLACK, TFT_BLACK);
     M5.Lcd.drawCentreString("                                                 ",160, 172, 4); //Efface
     M5.Lcd.drawCentreString("                                                 ",160, 197, 4); //Efface
     M5.Lcd.setTextColor(Ctext, TFT_BLACK);
     M5.Lcd.drawCentreString("       "+String(puissance_dBm+att)+" dBm     ", 160, 172, 4);

      if ((puissance_uW <=1000)&& (att==0))   // affichage puissance avec attenuateur = 0 dB
      {M5.Lcd.drawCentreString("       "+String(puissance_uW)+ " uW     ", 160, 197, 4);}
      if ((puissance_uW >1000)&& (att==0))      
      {M5.Lcd.drawCentreString(("       "+String(puissance_uW/1000))+ " mW     ", 160, 197, 4);}

      if (att==10)                            // affichage puissance avec attenuateur = 10 dB
      {M5.Lcd.drawCentreString("       "+String(puissance_uW/100)+ " mW     ", 160, 197, 4);}
     
      if ((puissance_uW <=1000)&& (att==20))  // affichage puissance avec attenuateur = 20 dB
       { M5.Lcd.drawCentreString("       "+String(puissance_uW/10)+ " mW     ", 160, 197, 4);}
      if ((puissance_uW >1000)&& (att==20)) 
       { M5.Lcd.drawCentreString("       "+String(puissance_uW/10000)+ " W     ", 160, 197, 4);}

      if  (att==30)                           // affichage puissance avec attenuateur = 30 dB
       {M5.Lcd.drawCentreString("       "+String(puissance_uW/1000)+ " W     ", 160, 197, 4);}
     
      if  (att==40)                           // affichage puissance avec attenuateur = 40 dB
       { M5.Lcd.drawCentreString("       "+String(puissance_uW/100)+ " W     ", 160, 197, 4);}
     }
     
     //Serial.print("att=");Serial.println(att);   
     plotNeedle(value,0); // It takes between 2 and 12ms to replot the needle with zero delay
  }
 
float readChannel(ADS1115_MUX channel) {
  float voltage = 0.0; 
  adc.setCompareChannels(channel);
  adc.startSingleMeasurement();
  while(adc.isBusy()){}
  voltage = adc.getResult_mV(); // alternative: getResult_mV for Millivolt
  return voltage;
  }

// #########################################################################
//               Draw the analogue meter on the screen
// #########################################################################

void analogMeter() {
  // Meter Bezel
  M5.Lcd.fillRect(0, 0, M_SIZE*239, M_SIZE*126, TFT_GREY);
  M5.Lcd.fillRect(5, 3, M_SIZE*230, M_SIZE*119, TFT_WHITE);

  M5.Lcd.setTextColor(TFT_BLACK);  // Text colour

  // Draw ticks every 10 degrees from -50 to +50 degrees (100 deg. FSD swing)
  for (int i = -50; i < 51; i += 10) // i=10  10 graduations
  {
    // Long scale tick length
    int tl = 15;
    // Coodinates of tick to draw
    float sx = cos((i - 90) * 0.0174532925);
    float sy = sin((i - 90) * 0.0174532925);
    uint16_t x0 = sx * (M_SIZE*100 + tl) + M_SIZE*120;
    uint16_t y0 = sy * (M_SIZE*100 + tl) + M_SIZE*140;
    uint16_t x1 = sx * M_SIZE*100 + M_SIZE*120;
    uint16_t y1 = sy * M_SIZE*100 + M_SIZE*140;

    // Short scale tick length
    if (i % 25 != 0) tl = 8;

    // Recalculate coords incase tick lenght changed
    x0 = sx * (M_SIZE*100 + tl) + M_SIZE*120;
    y0 = sy * (M_SIZE*100 + tl) + M_SIZE*140;
    x1 = sx * M_SIZE*100 + M_SIZE*120;
    y1 = sy * M_SIZE*100 + M_SIZE*140;

    // Draw tick
    M5.Lcd.drawLine(x0, y0, x1, y1, TFT_BLACK);

    // Check if labels should be drawn, with position tweaks
    if (i % 10 == 0) {
      // Calculate label positions
      x0 = sx * (M_SIZE*100 + tl + 10) + M_SIZE*120;
      y0 = sy * (M_SIZE*100 + tl + 10) + M_SIZE*140;
      
      // gamme=att // for test
      switch (i / 10) {  // affichage valeurs incluant changement de gamme
        case -5: M5.Lcd.drawCentreString(String(MeterLabel[0]+gamme), x0, y0 - 12, 2); break;
        case -4: M5.Lcd.drawCentreString(String(MeterLabel[1]+gamme), x0, y0 - 9, 2); break;
        case -3: M5.Lcd.drawCentreString(String(MeterLabel[2]+gamme), x0, y0 - 7, 2); break;
        case -2: M5.Lcd.drawCentreString(String(MeterLabel[3]+gamme), x0, y0 - 9, 2); break;
        case -1: M5.Lcd.drawCentreString(String(MeterLabel[4]+gamme), x0, y0 - 12, 2); break;
        case 0: M5.Lcd.drawCentreString(String(MeterLabel[5]+gamme), x0, y0 - 12, 2); break;
        case 1: M5.Lcd.drawCentreString(String(MeterLabel[6]+gamme), x0, y0 - 9, 2); break;
        case 2: M5.Lcd.drawCentreString(String(MeterLabel[7]+gamme), x0, y0 - 7, 2); break;
        case 3: M5.Lcd.drawCentreString(String(MeterLabel[8]+gamme), x0, y0 - 9, 2); break;
        case 4: M5.Lcd.drawCentreString(String(MeterLabel[9]+gamme), x0, y0 - 12, 2); break;
        case 5: M5.Lcd.drawCentreString(String(MeterLabel[10]+gamme), x0, y0 - 12, 2); break;}               
      }
 
    // Now draw the arc of the scale
    sx = cos((i + 10 - 90) * 0.0174532925);
    sy = sin((i + 10 - 90) * 0.0174532925);
    x0 = sx * M_SIZE*100 + M_SIZE*120;
    y0 = sy * M_SIZE*100 + M_SIZE*140;
    // Draw scale arc, don't draw the last part
    if (i < 50) {M5.Lcd.drawLine(x0, y0, x1, y1, TFT_BLACK);}
  } 
    M5.Lcd.drawCentreString("dBm", M_SIZE*120, M_SIZE*70, 4); 
    M5.Lcd.drawRect(5, 3, M_SIZE*230, M_SIZE*119, TFT_BLACK); // Draw bezel line
    plotNeedle(0,0);
    if (moins == true) {plotNeedle(0,0);} else {plotNeedle(99,0);} // reduit le temps de ralliement
}

// #########################################################################
// Update needle position
// This function is blocking while needle moves, time depends on ms_delay
// 10ms minimises needle flicker if text is drawn within needle sweep area
// Smaller values OK if text not in sweep area, zero for instant movement but
// does not look realistic... (note: 100 increments for full scale deflection)
// #########################################################################

void plotNeedle(int value, byte ms_delay) {
   Serial.print("aiguille=");Serial.println(value);
   if (value < -10) value = -10; // Limit value to emulate needle end stops
   if (value > 110) value = 110;

    // Move the needle until new value reached
    while (!(value == old_analog)) {
    if (old_analog < value) old_analog++;
    else old_analog--;

    if (ms_delay == 0) old_analog = value; // Update immediately if delay is 0

    float sdeg = map(old_analog, -10, 110, -150, -30); // Map value to angle
    // Calculate tip of needle coords
    float sx = cos(sdeg * 0.0174532925);
    float sy = sin(sdeg * 0.0174532925);

    // Calculate x delta of needle start (does not start at pivot point)
    float tx = tan((sdeg + 90) * 0.0174532925);

    // Erase old needle image
    M5.Lcd.drawLine(M_SIZE*(120 + 20 * ltx - 1), M_SIZE*(140 - 20), osx - 1, osy, TFT_WHITE);
    M5.Lcd.drawLine(M_SIZE*(120 + 20 * ltx), M_SIZE*(140 - 20), osx, osy, TFT_WHITE);
    M5.Lcd.drawLine(M_SIZE*(120 + 20 * ltx + 1), M_SIZE*(140 - 20), osx + 1, osy, TFT_WHITE);

    // Re-plot text under needle
    M5.Lcd.setTextColor(TFT_BLACK);
    M5.Lcd.drawCentreString("dBm", M_SIZE*120, M_SIZE*70, 4); 
    M5.Lcd.fillRect(120,M_SIZE*90,160,20,TFT_WHITE);  //efface ancienne ligne suivante
    M5.Lcd.drawCentreString(("Att="+String(att)), M_SIZE*120, M_SIZE*90, 4);  
    // Enregistre les coordonnées du bout de aiguille pour le prochain effacement
    ltx = tx;
    osx = M_SIZE*(sx * 98 + 120);
    osy = M_SIZE*(sy * 98 + 140);

    // Draw the needle in the new position, magenta makes needle a bit bolder
    // dessine 3 lignes pour épaissir l'aiguille 
    M5.Lcd.drawLine(M_SIZE*(120 + 20 * ltx - 1), M_SIZE*(140 - 20), osx - 1, osy, TFT_RED);
    M5.Lcd.drawLine(M_SIZE*(120 + 20 * ltx), M_SIZE*(140 - 20), osx, osy, TFT_MAGENTA);
    M5.Lcd.drawLine(M_SIZE*(120 + 20 * ltx + 1), M_SIZE*(140 - 20), osx + 1, osy, TFT_RED);

    // Slow needle down slightly as it approaches new position
    if (abs(old_analog - value) < 10) ms_delay += ms_delay / 5;

    // Wait before next update
    delay(10);
  }
}
void text(){
    if (flag_BtnB ==1){Ctext=TFT_YELLOW; Ctext1=TFT_GREEN;}
    if (flag_BtnB ==2){Ctext=TFT_AZURE; Ctext1=TFT_YELLOW;}
    if (flag_BtnB ==3){Ctext=TFT_WHITE; Ctext1=TFT_AZURE;}
    EEPROM.write(0,flag_BtnB);
    EEPROM.commit();
    delay(100);
    }  
