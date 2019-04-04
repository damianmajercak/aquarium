#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include<EEPROM.h>
#include <Keypad.h>

#define TIME_LCD_BACK_LIGHT 60000 // after one minute no activit lcd backlight off
///------------------------eprom defines
#define EEPROM_TURN_ON_HOURS 0
#define EEPROM_TURN_ON_MINUTES 1
#define EEPROM_TURN_OFF_HOURS 2
#define EEPROM_TURN_OFF_MINUTES 3
#define EEPROM_RISE_TIME 4
#define EEPROM_STATE_OF_LIGHT 5
//-------------------------------
#define DS3231_I2C_ADDRESS 0x68
#define TEMPERATURE_SENSOR 8
#define LED_STRIP 10
#define ARROW_UP 2
#define ARROW_DOWN 3
#define LIGHT_SYM 4
#define TURN_ON_HOURS_POS_COL 4
#define TURN_ON_HOURS_POS_ROW 2
#define TURN_ON_MINUTES_POS_COL 7
#define TURN_ON_MINUTES_POS_ROW 2
#define TURN_OFF_HOURS_POS_COL 15
#define TURN_OFF_HOURS_POS_ROW 2
#define TURN_OFF_MINUTES_POS_COL 18
#define TURN_OFF_MINUTES_POS_ROW 2
#define RISE_TIME_POS_COL 7
#define RISE_TIME_POS_ROW 3
#define UPDATE_DEFAULT_VIEW_TIME 5000
//define pre pozicie pri nastaveni casu
#define ST_DAY_OF_WEEK_COL 0
#define ST_DAY_OF_WEEK_ROW 2
#define ST_DAY_OF_MONTH_COL 5
#define ST_DAY_OF_MONTH_ROW 2
#define ST_MONTH_COL 8
#define ST_MONTH_ROW 2
#define ST_YEAR_COL 11
#define ST_YEAR_ROW 2
#define ST_HOUR_COL 15
#define ST_HOUR_ROW 2
#define ST_MINUTE_COL 18
#define ST_MINUTE_ROW 2

//stringy na vypis
#define S_SET_DIMMING_TIME "       svetlo       "
#define SETTING_TIME_TITLE "        cas         "
#define MENU_TITLE "        MENU        "
#define CONFIRM_SET_DIMMING_TIME "Hodnoty ulozene"
#define CONFIRM_SET_CURRENT_TIME "Cas bol nastaveny"
//prototype
void default_view();
void set_light();
void check_light();
void set_current_time();
void showStatusLight();

OneWire oneWire(TEMPERATURE_SENSOR);
DallasTemperature sensors(&oneWire);
LiquidCrystal_I2C lcd(0x3F, 20,4);//POSITIVE); // platformio ide
//LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // arduino ide
  //idem skusit git


String dni[7] = {"Ned.","Pon.","Utr.","Str.","Stv.","Pia.","Sob."};
int val = 0; //asi zbytocne
int a = 0;  // taktiez asi zbytocne

byte stateOfLight;
typedef struct time{
    byte second,minute,hour,dayOfWeek,dayOfMonth,month,year;
}TIME;
TIME actualTime;

const byte ROWS = 1; //four rows
const byte COLS = 5; //three columns
char keys[ROWS][COLS] = {
  {'e','r','u','d','l'}
};
byte rowPins[ROWS] = {2}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {7, 6, 5, 4, 3}; //connect to the column pinouts of the keypad
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

byte lightSymbol[8] = {
  B00000,
  B01110,
  B10001,
  B10001,
  B01110,
  B01110,
  B01110,
  B00100
};

byte termometer[8] = {
    B00100,
    B01010,
    B01010,
    B01110,
    B01110,
    B11111,
    B11111,
    B01110};
byte arrowUp[8] = {
  B00100,
  B01110,
  B10101,
  B00100,
  B00100,
  B00100,
  B00100,
  B00100};
byte arrowDown[8] = {
  B00100,
  B00100,
  B00100,
  B00100,
  B00100,
  B10101,
  B01110,
  B00100};




///------------------SECTION DS3231------------------------------
// Convert normal decimal numbers to binary coded decimal
byte decToBcd(byte val){
  return( (val/10*16) + (val%10) );
}
// Convert binary coded decimal to normal decimal numbers
byte bcdToDec(byte val){
  return( (val/16*10) + (val%16) );
}

void setDS3231time(byte second, byte minute, byte hour, byte dayOfWeek, byte dayOfMonth, byte month, byte year){
    // sets time and date data to DS3231
    Wire.beginTransmission(DS3231_I2C_ADDRESS);
    Wire.write(0); // set next input to start at the seconds register
    Wire.write(decToBcd(second)); // set seconds
    Wire.write(decToBcd(minute)); // set minutes
    Wire.write(decToBcd(hour)); // set hours
    Wire.write(decToBcd(dayOfWeek)); // set day of week (1=Sunday, 7=Saturday)
    Wire.write(decToBcd(dayOfMonth)); // set date (1 to 31)
    Wire.write(decToBcd(month)); // set month
    Wire.write(decToBcd(year)); // set year (0 to 99)
    Wire.endTransmission();
}
void readDS3231time(byte *second, byte *minute, byte *hour, byte *dayOfWeek, byte *dayOfMonth, byte *month, byte *year){
    Wire.beginTransmission(DS3231_I2C_ADDRESS);
    Wire.write(0); // set DS3231 register pointer to 00h
    Wire.endTransmission();
    Wire.requestFrom(DS3231_I2C_ADDRESS, 7);
    // request seven bytes of data from DS3231 starting from register 00h
    *second = bcdToDec(Wire.read() & 0x7f);
    *minute = bcdToDec(Wire.read());
    *hour = bcdToDec(Wire.read() & 0x3f);
    *dayOfWeek = bcdToDec(Wire.read());
    *dayOfMonth = bcdToDec(Wire.read());
    *month = bcdToDec(Wire.read());
    *year = bcdToDec(Wire.read());
}

void read_time(){
  readDS3231time(&actualTime.second,&actualTime.minute,&actualTime.hour,&actualTime.dayOfWeek,&actualTime.dayOfMonth,&actualTime.month,&actualTime.year);
}

String timeToShow(){
  byte second,minute,hour,dayOfWeek,dayOfMonth,month,year;
  hour =actualTime.hour;
  minute = actualTime.minute;
  dayOfMonth= actualTime.dayOfMonth;
  dayOfWeek= actualTime.dayOfWeek;
  month=actualTime.month;
  second= actualTime.second ;
  year = actualTime.year;

  String output="";
  output += dni[dayOfWeek];
  output += " ";
  if(dayOfMonth<10){
    output += '0';
    }
  output += dayOfMonth;
  output += '.';
  if(month<10){
    output += '0';
    }
  output += month;
  output += '.';
  output += year;
  output += "' ";
  if(hour<10){
    output +='0';
    }
  output += hour;
  output += ':';
  if(minute<10){
    output += '0';
    }
  output += minute;
  output += ' ';
  return output;

}

///---------------------------------------------------------------

//---------------------SECTION TEMPERATURE------------------------
float read_temperature(){
  sensors.requestTemperatures();
  float temperature = sensors.getTempCByIndex(0);
  return temperature;
}


void showTemperatureOnLCD(){
 lcd.setCursor(12,1);
 lcd.write(1);
 lcd.print(" ");
 float temp = read_temperature();
 if(temp < -120){
   lcd.print("chyba  ");
 }
 else{
  lcd.print(temp,1);
  lcd.print((char)223); //degree sign
  lcd.print("C");
  }
}
//------------------------------------------------------------------


//---------------------SECTION LIGHT ------------------------------
void setValueToDimming(byte value){
  analogWrite(LED_STRIP,value);  
}

  //-----------------------------------------------------------------
long cas;
long timeUpdateLight;
long timeLcdBackglight;
typedef struct TurnTime{
  byte hour;
  byte minute;
}TURNTIME;
TURNTIME turnOn;
TURNTIME turnOff;
byte rise_time;

int interval_update_dimming_light; // v microsekundach cas update ktory bude spustat zvysovanie hodnoty v dimmingu
bool LightTurnningOn;
bool LightTurnningOff;

void save_to_eeprom(int index,byte value){
      EEPROM.write(index,value);
}

void startup_initiazlize_variable(){
  turnOn.hour = EEPROM.read(EEPROM_TURN_ON_HOURS);
  turnOn.minute = EEPROM.read(EEPROM_TURN_ON_MINUTES);
  turnOff.hour = EEPROM.read(EEPROM_TURN_OFF_HOURS);
  turnOff.minute = EEPROM.read(EEPROM_TURN_OFF_MINUTES);
  rise_time = EEPROM.read(EEPROM_RISE_TIME);
  stateOfLight = EEPROM.read(EEPROM_STATE_OF_LIGHT);
}


void blink(int col,int row,String s){
  String blank = "";
  if(millis() % 1000 > 0 && millis() % 1000 < 500){ // zaciatok - 10
            //blikanie setni cursor podla toho co ma blikat
            lcd.setCursor(col,row);
            for(int a = 0;a<s.length();a++){
              blank += " ";
            } 
            lcd.print(blank);                
    }
    else{
      lcd.setCursor(col,row);
      lcd.print(s); 
    }
}

void set_dimming_time(){
  
  char input;
  int walker = 0;
  String output;
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(S_SET_DIMMING_TIME);
  lcd.setCursor(0,2);
  lcd.print("zap.");
  if(turnOn.hour<10){
     lcd.print("0");
  }
  lcd.print(turnOn.hour);
  lcd.print(":");
  if(turnOn.minute<10){
    lcd.print("0");
  }
  lcd.print(turnOn.minute);
  //turnoff
  lcd.print("  ");
  lcd.print("vyp.");
  if(turnOff.hour<10){
    lcd.print("0");
  }
  lcd.print(turnOff.hour);
  lcd.print(":");
  if(turnOff.minute<10){
    lcd.print("0");
  }
  lcd.print(turnOff.minute);

  lcd.setCursor(0,3);
  lcd.print("nabeh: ");
  if(rise_time<10){
    lcd.print("0");
  }
  lcd.print(rise_time);
  lcd.print(" min");

  while((input = keypad.getKey())!='e'){
    switch(input){
      case 'l':
          if(walker == 0){lcd.setCursor(TURN_ON_HOURS_POS_COL,TURN_ON_HOURS_POS_ROW);if(turnOn.hour<10){lcd.print("0");}lcd.print(turnOn.hour);}
          if(walker == 1){lcd.setCursor(TURN_ON_MINUTES_POS_COL,TURN_ON_MINUTES_POS_ROW);if(turnOn.minute<10){lcd.print("0");}lcd.print(turnOn.minute);}
          if(walker == 2){lcd.setCursor(TURN_OFF_HOURS_POS_COL,TURN_OFF_HOURS_POS_ROW);if(turnOff.hour<10){lcd.print("0");}lcd.print(turnOff.hour);}
          if(walker == 3){lcd.setCursor(TURN_OFF_MINUTES_POS_COL,TURN_OFF_HOURS_POS_ROW);if(turnOff.minute<10){lcd.print("0");}lcd.print(turnOff.minute);}
          if(walker == 4){lcd.setCursor(RISE_TIME_POS_COL,RISE_TIME_POS_ROW);if(rise_time<10){lcd.print("0");}lcd.print(rise_time);}

          if(walker > 0){
            walker --;
          }
          else{
            walker = 4;
          }
          break;
      case 'r':
           if(walker == 0){lcd.setCursor(TURN_ON_HOURS_POS_COL,TURN_ON_HOURS_POS_ROW);if(turnOn.hour<10){lcd.print("0");}lcd.print(turnOn.hour);}
          if(walker == 1){lcd.setCursor(TURN_ON_MINUTES_POS_COL,TURN_ON_MINUTES_POS_ROW);if(turnOn.minute<10){lcd.print("0");}lcd.print(turnOn.minute);}
          if(walker == 2){lcd.setCursor(TURN_OFF_HOURS_POS_COL,TURN_OFF_HOURS_POS_ROW);if(turnOff.hour<10){lcd.print("0");}lcd.print(turnOff.hour);}
          if(walker == 3){lcd.setCursor(TURN_OFF_MINUTES_POS_COL,TURN_OFF_HOURS_POS_ROW);if(turnOff.minute<10){lcd.print("0");}lcd.print(turnOff.minute);}
          if(walker == 4){lcd.setCursor(RISE_TIME_POS_COL,RISE_TIME_POS_ROW);if(rise_time<10){lcd.print("0");}lcd.print(rise_time);}
          if(walker < 4){
            walker++;
          }
          else{
            walker = 0;
          }
          break;
      case 'u':
          if(walker==0){
            if(turnOn.hour<23){
              turnOn.hour++;
            }
            else{
              turnOn.hour = 0;
            }
          }
          if(walker==1){
            if(turnOn.minute<59){
              turnOn.minute++;
            }
            else{
              turnOn.minute = 0;
            }
          }
          if(walker==2){
            if(turnOff.hour<23){
              turnOff.hour++;
            }
            else{
              turnOff.hour = 0;
            }
          }
          if(walker==3){
            if(turnOff.minute<59){
              turnOff.minute++;
            }
            else{
              turnOff.minute = 0;
            }
          }
          if(walker == 4){if(rise_time<59){rise_time++;}else{rise_time=0;}}
          break;
      case 'd':
        if(walker==0){
            if(turnOn.hour>0){
              turnOn.hour--;
            }
            else{
              turnOn.hour = 23;
            }
          }
          if(walker==1){
            if(turnOn.minute>0){
              turnOn.minute--;
            }
            else{
              turnOn.minute = 59;
            }
          }
          if(walker==2){
            if(turnOff.hour>0){
              turnOff.hour--;
            }
            else{
              turnOff.hour = 23;
            }
          }
          if(walker==3){
            if(turnOff.minute>0){
              turnOff.minute--;
            }
            else{
              turnOff.minute = 59;
            }
          }
          if(walker == 4){if(rise_time>0){rise_time--;}else{rise_time=59;}}
          break;

    }
    switch (walker){
      case 0:
        output = String(turnOn.hour);
        if(turnOn.hour<10){
          output = "0"+output;
        }
        blink(TURN_ON_HOURS_POS_COL,TURN_ON_HOURS_POS_ROW,output);
        /* code */
        break;
      case 1:
        output = String(turnOn.minute);
        if(turnOn.minute<10){
          output = "0"+output;
        }
        blink(TURN_ON_MINUTES_POS_COL,TURN_ON_MINUTES_POS_ROW,output);
        break;
      case 2:
        output = String(turnOff.hour);
        if(turnOff.hour<10){
          output = "0"+output;
        }
        blink(TURN_OFF_HOURS_POS_COL,TURN_OFF_HOURS_POS_ROW,output);
        break;
      case 3:
        output = String(turnOff.minute);
        if(turnOff.minute<10){
          output = "0"+output;
        }
        blink(TURN_OFF_MINUTES_POS_COL,TURN_OFF_MINUTES_POS_ROW,output);
        break;
      case 4:
        output = String(rise_time);
        if(rise_time<10){
          output = "0"+output;
        }
        blink(RISE_TIME_POS_COL,RISE_TIME_POS_ROW,output);
        break;
      default:
        break;
    }
   
  }
  lcd.clear();
  lcd.setCursor(0,1);
  lcd.print(CONFIRM_SET_DIMMING_TIME);
  save_to_eeprom(EEPROM_TURN_ON_HOURS,turnOn.hour);
  save_to_eeprom(EEPROM_TURN_ON_MINUTES,turnOn.minute);
  save_to_eeprom(EEPROM_TURN_OFF_HOURS,turnOff.hour);
  save_to_eeprom(EEPROM_TURN_OFF_MINUTES,turnOff.minute);
  save_to_eeprom(EEPROM_RISE_TIME,rise_time);
  delay(1000);
  lcd.clear();
  default_view();
}


void setup() {
    
  pinMode(LED_STRIP,OUTPUT);
  Wire.begin(); 
  lcd.begin(20,4);
  //EEPROM.write(EEPROM_STATE_OF_LIGHT,0);
  lcd.createChar(1,termometer);
  lcd.createChar(ARROW_UP,arrowUp);
  lcd.createChar(ARROW_DOWN,arrowDown);
  lcd.createChar(LIGHT_SYM,lightSymbol);
  Serial.begin(9600);
  sensors.begin();
  startup_initiazlize_variable();
  lcd.setCursor(0,0);
  lcd.setBacklight(1);
  lcd.print("aquarium controller");
  lcd.setCursor(0,1);
  lcd.print("Damian Majercak");
  lcd.setCursor(0,2);
  lcd.print("2019");
  delay(5000);
  lcd.clear();
  lcd.setCursor(0,0);
  read_time();
  lcd.print(timeToShow());
  showStatusLight();
  showTemperatureOnLCD();
  cas =  millis();
  timeUpdateLight = cas;
  timeLcdBackglight = millis();
}

void default_view(){
  cas = millis();
  read_time();
  showTemperatureOnLCD();
  lcd.setCursor(0,0);
  lcd.print(timeToShow());
  check_light();
  showStatusLight();

  
}
void showStatusLight(){
  lcd.setCursor(0,1);
  lcd.print("          ");
  lcd.setCursor(0,1);
  lcd.write(LIGHT_SYM);
  lcd.print(" ");
  float percent = float(stateOfLight) * 0.3922; //konstanta ktora mapuje 255 do
  lcd.print(percent,1);
  lcd.print("% ");
  if(LightTurnningOn){
    lcd.write(ARROW_UP);
  }
  else if(LightTurnningOff){
    lcd.write(ARROW_DOWN);
  }
}


void handleMenu(){
    char input=0;
    lcd.clear();
    String menuChoices[3] ={"- nastavenie casu","- nastavenie svetla","        EXIT"}; 
    byte walker= 0;
    lcd.setCursor(0,0);
    lcd.print(MENU_TITLE);
    for(int i = 1;i<=3;i++){
        lcd.setCursor(0,i);
        lcd.print(menuChoices[i-1]);
    }
    // zakladny vypis hotovy
    while((input=keypad.getKey())!= 'e'){
      blink(0,walker+1,menuChoices[walker]);
      switch (input)
      {
        case 'd':
           lcd.setCursor(0,walker+1);
           lcd.print(menuChoices[walker]);
          walker  = (walker +1)%3;
          break;
        case 'u':
            lcd.setCursor(0,walker+1);
            lcd.print(menuChoices[walker]);
            if(walker > 0){
              walker --;
            }
            else {
              walker = 2;
            }
          break;
        default:
          break;
      }


    }
  switch (walker)
  {
    case 0:
      set_current_time();
      break;
    case 1:
      set_dimming_time();
    case 2:
      lcd.clear();
      return;
    default:
      break;
  }
  lcd.clear();
}

void loop() {
  char input;
 if((millis()-cas) > UPDATE_DEFAULT_VIEW_TIME){
    default_view();
  }
  set_light();
  input = keypad.getKey();
  if(input == 'e'){
    lcd.setBacklight(1);
    handleMenu();
    timeLcdBackglight = millis();
    default_view();
  }
  if((input == 'u'||input == 'l')|| (input == 'r'||input == 'd')){
    lcd.setBacklight(1);
    timeLcdBackglight = millis();
 }

 if((millis() - timeLcdBackglight) > TIME_LCD_BACK_LIGHT){
   lcd.setBacklight(0);
 }
 
}

void set_light(){
  if(LightTurnningOn){
    if((millis()-timeUpdateLight)>interval_update_dimming_light){ // ak som mal updatovat hodnotu svetla
      if(stateOfLight<byte(255)){
        stateOfLight++;
        setValueToDimming(stateOfLight);
        /*Serial.print("zvysenie svietivosti: ");
        Serial.print(stateOfLight);
        Serial.print("\n");*/
      }
      else {
        LightTurnningOn = false;
        //Serial.println("svetlo zapnute na plno");
      }
      timeUpdateLight = millis();
    }
  }

  if(LightTurnningOff){
     if((millis()-timeUpdateLight)>long(interval_update_dimming_light)){
       if(stateOfLight>0){
         stateOfLight--;
         setValueToDimming(stateOfLight);
         /*Serial.print("zniezenie svietivosti: ");
         Serial.print(stateOfLight);
         Serial.print("\n");*/
         
       }
       else{
         LightTurnningOff = false;
         //Serial.print("svetlo vypnute uplne\n");
       }
       timeUpdateLight = millis();
     }

  }
}

void check_light(){
  if((turnOn.hour == 0 && turnOff.hour ==0)&&(turnOn.minute ==0 && turnOff.minute==0)){
    //ak su vsetky veci nastavene na nulu tak svetlo sa nebude zapinat
    setValueToDimming(0);
    stateOfLight = 0;
    return;
  }
  if((turnOn.hour == 23 && turnOff.hour ==23)&&(turnOn.minute ==59 && turnOff.minute==59)){
    //manualne zapnutie
    setValueToDimming(255);
    stateOfLight = 255;
    return;
  }

  if(turnOn.hour == actualTime.hour && turnOn.minute == actualTime.minute){  // ak nadisiel cas zapnutia
    if(rise_time ==0){ // ak cas nabehu sa rovna 0 
      setValueToDimming(255);
      stateOfLight = 255;
      return;
    }
    if(LightTurnningOn == false){
      float a = (float(rise_time) * 60 )/255;

      //Serial.print("v sekundach update interval time");
      //Serial.print(a,5);
      //Serial.print("\n");
      interval_update_dimming_light = int((a * 1000));
      //Serial.print("v microsekundach update interval time: ");
      //Serial.print(interval_update_dimming_light);
      //Serial.print("\n");
      LightTurnningOn = true;
      LightTurnningOff = false;
    }
          //prepocitaj cas update ktory sa bude volat
  }
  if(turnOff.hour == actualTime.hour && turnOff.minute == actualTime.minute){  // ak nadisiel cas zapnutia
    if(rise_time == 0){ // ak cas nabehu sa rovna 0 
      setValueToDimming(0);
      stateOfLight = 0;
      return;
    }
    if(LightTurnningOff == false){
       float a = (float(rise_time) *60)/255;
      //Serial.print("v sekundach update interval time znizenia");
      //Serial.print(a,5);
      //Serial.print("\n");
      interval_update_dimming_light = int((a * 1000));
      //Serial.print("v microsekundach update interval time znizenia : ");
      //Serial.print(interval_update_dimming_light);
      //Serial.print("\n");
      LightTurnningOff = true;
      LightTurnningOn = false;
    }
          //prepocitaj cas update ktory sa bude volat
  }
}

void set_current_time(){
  char input;
  byte walker = 0;
  String output;
  TIME newTime;
  //----------
  read_time();
  newTime.dayOfMonth = actualTime.dayOfMonth;
  newTime.dayOfWeek = actualTime.dayOfWeek;
  newTime.hour = actualTime.hour;
  newTime.minute = actualTime.minute;
  newTime.month = actualTime.month;
  newTime.second = actualTime.second;
  newTime.year = actualTime.year;

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(SETTING_TIME_TITLE);
  lcd.setCursor(0,2);
  lcd.print(dni[actualTime.dayOfWeek]);
  lcd.print(" ");
  if(actualTime.dayOfMonth<10) lcd.print("0");
  lcd.print(actualTime.dayOfMonth);
  lcd.print(".");
  if(actualTime.month<10) lcd.print("0");
  lcd.print(actualTime.month);
  lcd.print(".");
  lcd.print(actualTime.year);
  lcd.print("' ");
  if(actualTime.hour<10) lcd.print("0");
  lcd.print(actualTime.hour);
  lcd.print(":");
  if(actualTime.minute < 10)lcd.print("0");
  lcd.print(actualTime.minute);
  //---------------
  while((input = keypad.getKey())!= 'e'){
    switch(input){
      case 'l':
          if(walker == 0){lcd.setCursor(ST_DAY_OF_WEEK_COL,ST_DAY_OF_WEEK_ROW);lcd.print(dni[newTime.dayOfWeek]);} //zobrazenie dna
          if(walker == 1){lcd.setCursor(ST_DAY_OF_MONTH_COL,ST_DAY_OF_MONTH_ROW);if(newTime.dayOfMonth<10){lcd.print("0");}lcd.print(newTime.dayOfMonth);}
          if(walker == 2){lcd.setCursor(ST_MONTH_COL,ST_MONTH_ROW);if(newTime.month<10){lcd.print("0");}lcd.print(newTime.month);}
          if(walker == 3){lcd.setCursor(ST_YEAR_COL,ST_YEAR_ROW);if(newTime.year<10){lcd.print("0");}lcd.print(newTime.year);}
          if(walker == 4){lcd.setCursor(ST_HOUR_COL,ST_HOUR_ROW);if(newTime.hour<10){lcd.print("0");}lcd.print(newTime.hour);}
          if(walker == 5){lcd.setCursor(ST_MINUTE_COL,ST_MINUTE_ROW);if(newTime.minute<10){lcd.print("0");}lcd.print(newTime.minute);}

          if(walker > 0){
            walker --;
          }
          else{
            walker = 6;
          }
          break;
      case 'r':
          if(walker == 0){lcd.setCursor(ST_DAY_OF_WEEK_COL,ST_DAY_OF_WEEK_ROW);lcd.print(dni[newTime.dayOfWeek]);} //zobrazenie dna
          if(walker == 1){lcd.setCursor(ST_DAY_OF_MONTH_COL,ST_DAY_OF_MONTH_ROW);if(newTime.dayOfMonth<10){lcd.print("0");}lcd.print(newTime.dayOfMonth);}
          if(walker == 2){lcd.setCursor(ST_MONTH_COL,ST_MONTH_ROW);if(newTime.month<10){lcd.print("0");}lcd.print(newTime.month);}
          if(walker == 3){lcd.setCursor(ST_YEAR_COL,ST_YEAR_ROW);if(newTime.year<10){lcd.print("0");}lcd.print(newTime.year);}
          if(walker == 4){lcd.setCursor(ST_HOUR_COL,ST_HOUR_ROW);if(newTime.hour<10){lcd.print("0");}lcd.print(newTime.hour);}
          if(walker == 5){lcd.setCursor(ST_MINUTE_COL,ST_MINUTE_ROW);if(newTime.minute<10){lcd.print("0");}lcd.print(newTime.minute);}
          if(walker < 5){
            walker++;
          }
          else{
            walker = 0;
          }
          break;
      case 'u':
          if(walker==0){
            if(newTime.dayOfWeek<6){
              newTime.dayOfWeek++;
            }
            else{
              newTime.dayOfWeek = 0;
            }
          }
          if(walker==1){
            if(newTime.dayOfMonth<31){  //dummy nieje osetreny rozsah
              newTime.dayOfMonth++;
            }
            else{
              newTime.dayOfMonth = 1;
            }
          }
          if(walker==2){
            if(newTime.month<12){
              newTime.month++;
            }
            else{
              newTime.month = 1;
            }
          }
          if(walker==3){
            if(newTime.year<99){
              newTime.year++;
            }
            else{
              newTime.year = 0;
            }
          }
          if(walker==4){
            if(newTime.hour<23){
              newTime.hour++;
            }
            else{
              newTime.hour = 0;
            }
          }
          if(walker==5){
            if(newTime.minute<59){
              newTime.minute++;
            }
            else{
              newTime.minute = 0;
            }
          }
          break;
      case 'd':
       if(walker==0){
            if(newTime.dayOfWeek>0){
              newTime.dayOfWeek--;
            }
            else{
              newTime.dayOfWeek = 6;
            }
          }
          if(walker==1){
            if(newTime.dayOfMonth>1){  //dummy nieje osetreny rozsah
              newTime.dayOfMonth--;
            }
            else{
              newTime.dayOfMonth = 31;
            }
          }
          if(walker==2){
            if(newTime.month>1){
              newTime.month--;
            }
            else{
              newTime.month = 12;
            }
          }
          if(walker==3){
            if(newTime.year>0){
              newTime.year--;
            }
            else{
              newTime.year = 99;
            }
          }
          if(walker==4){
            if(newTime.hour>0){
              newTime.hour--;
            }
            else{
              newTime.hour = 23;
            }
          }
          if(walker==5){
            if(newTime.minute>0){
              newTime.minute--;
            }
            else{
              newTime.minute = 59;
            }
          }
          break;

    }
    switch (walker){
      case 0:
        output = dni[newTime.dayOfWeek];
        blink(ST_DAY_OF_WEEK_COL,ST_DAY_OF_WEEK_ROW,output);
        /* code */
        break;
      case 1:
        output = String(newTime.dayOfMonth);
        if(newTime.dayOfMonth<10){
          output = "0"+output;
        }
        blink(ST_DAY_OF_MONTH_COL,ST_DAY_OF_MONTH_ROW,output);
        break;
      case 2:
        output = String(newTime.month);
        if(newTime.month<10){
          output = "0"+output;
        }
        blink(ST_MONTH_COL,ST_MONTH_ROW,output);
        break;
      case 3:
        output = String(newTime.year);
        if(newTime.year<10){
          output = "0"+output;
        }
        blink(ST_YEAR_COL,ST_YEAR_ROW,output);
        break;
      case 4:
        output = String(newTime.hour);
        if(newTime.hour<10){
          output = "0"+output;
        }
        blink(ST_HOUR_COL,ST_HOUR_ROW,output);
        break;
      case 5:
        output = String(newTime.minute);
        if(newTime.minute<10){
          output = "0"+output;
        }
        blink(ST_MINUTE_COL,ST_MINUTE_ROW,output);
        break;
      default:
        break;
    }
  }
  lcd.clear();
  lcd.setCursor(0,1);
  lcd.print(CONFIRM_SET_CURRENT_TIME);
  setDS3231time(0,newTime.minute,newTime.hour,newTime.dayOfWeek,newTime.dayOfMonth,newTime.month,newTime.year);
  delay(1000);
  lcd.clear();
}

