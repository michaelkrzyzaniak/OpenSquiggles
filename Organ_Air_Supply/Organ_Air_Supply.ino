#include <EEPROM.h>

#define FAN_INLET_CONTROL_PIN   9   // brown fan wire
#define FAN_OUTLET_CONTROL_PIN  10  // white fan wire
#define FAN_INLET_SENSOR_PIN    2   // yellow fan wire
#define FAN_OUTLET_SENSOR_PIN   3   // purple fan wire
#define PRESSURE_SENSOR_PIN     A1


#define FAN_CONTROL_MAX         320 // 0 to 320 is 0 to 100% duty cycle

#define CLIP(x, MIN, MAX)       (((x)>(MIN)) ? ((x) > (MAX)) ? (MAX) : (x) : (MIN));

volatile uint16_t fan_inlet_sensor_counter   = 0;
volatile uint16_t fan_outlet_sensor_counter  = 0;
volatile uint8_t  fan_sensor_clock_divider   = 0;
volatile float    fan_inlet_sensor_rpm       = 0;
volatile float    fan_outlet_sensor_rpm      = 0;

float    PID_TARGET_PRESSURE     =    2.5; //inches H2O
bool     pid_is_initalized = false;
float    pid_p=0, pid_i=0, pid_d=0;
float    pid_prev_error = 0;
float    pid_integral_error = 0;
uint32_t pid_prev_time = 0;

/*---------------------------------------------------------------*/
//PRIVATE
//determine the fan speed from the fan sensor counters
ISR(TIMER2_OVF_vect)
{
  // this timer ISR runs every 16.384 milliseconds
  // but that is too fast so only update every 8 times,
  // ie  131.072 milliseconds

  ++fan_sensor_clock_divider;
  if((fan_sensor_clock_divider % 8) == 0)
    {
      cli();
      float inlet                = fan_inlet_sensor_counter  * ((1000.0 * 60.0) / (4.0 * 131.072));
      float outlet               = fan_outlet_sensor_counter * ((1000.0 * 60.0) / (4.0 * 131.072));  
      fan_inlet_sensor_counter   = 0;
      fan_outlet_sensor_counter  = 0;
      sei();
      fan_inlet_sensor_rpm       = (inlet * 0.2)  + (fan_inlet_sensor_rpm  * 0.8);
      fan_outlet_sensor_rpm      = (outlet * 0.2) + (fan_outlet_sensor_rpm * 0.8); 
    }
}

/*-------------------------------------------------------------c--*/
//PRIVATE
void fan_inlet_sensor_counter_interrupt()
{
  ++fan_inlet_sensor_counter;
}

/*---------------------------------------------------------------*/
//PRIVATE
void fan_outlet_sensor_counter_interrupt()
{
  ++fan_outlet_sensor_counter;
}

/*---------------------------------------------------------------*/
//PUBLIC
void fan_control_init()
{
  pinMode(FAN_INLET_CONTROL_PIN , OUTPUT);
  pinMode(FAN_OUTLET_CONTROL_PIN, OUTPUT);
  digitalWrite(FAN_INLET_CONTROL_PIN , LOW);
  digitalWrite(FAN_OUTLET_CONTROL_PIN, LOW);

  TCCR1A = 0;                  //clear timer registers
  TCCR1B = 0;
  TCNT1  = 0;

  TCCR1B |= _BV(CS10);         //no prescaler
  ICR1    = FAN_CONTROL_MAX;   //PWM mode counts up 320 then down 320 counts (25kHz)

  fan_set_duty(0, true);
  TCCR1A |= _BV(COM1A1);       //output A clear rising/set falling

  fan_set_duty(0, false);
  TCCR1A |= _BV(COM1B1);       //output B clear rising/set falling

  TCCR1B |= _BV(WGM13);        //PWM mode with ICR1 Mode 10
  TCCR1A |= _BV(WGM11);        //WGM13:WGM10 set 1010  
} 

/*---------------------------------------------------------------*/
//PUBLIC
void fan_sensor_init()
{
  //timer to count 
  //WGM22:0 = 0, normal mode
  TIMSK2 = (TIMSK2 & B11111110) | 0x01; //enable interrupt on overflow
  TCCR2A = (TCCR2B & B11111100) | 0x00; //WGM to normal mode
  TCCR2B = (TCCR2B & B11110000) | 0x07; //set 1024 prescaler and one WGM bit (16.384 milliseconds)

  pinMode(FAN_INLET_SENSOR_PIN , INPUT_PULLUP);
  pinMode(FAN_OUTLET_SENSOR_PIN, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(FAN_INLET_SENSOR_PIN) , fan_inlet_sensor_counter_interrupt , CHANGE); 
  attachInterrupt(digitalPinToInterrupt(FAN_OUTLET_SENSOR_PIN), fan_outlet_sensor_counter_interrupt, CHANGE); 
}

/*---------------------------------------------------------------*/
//PUBLIC
void fan_set_duty(float duty, int is_inlet)
{
  duty = CLIP(duty, 0, 100);
  uint16_t raw = round(FAN_CONTROL_MAX * 0.01 * duty);
  cli();
  if(is_inlet)
    OCR1A = raw;
  else
    OCR1B = raw;
  sei();
}

/*---------------------------------------------------------------*/
//PUBLIC
float fan_get_duty(int is_inlet)
{
  uint16_t raw;
  if(is_inlet)
    raw = OCR1A;
  else
    raw = OCR1B;
    
  return raw * 100 / (float)FAN_CONTROL_MAX;
}

/*---------------------------------------------------------------*/
//PUBLIC
void pressure_sensor_init()
{
  pinMode(PRESSURE_SENSOR_PIN , INPUT); //INPUT_PULLUP?
}

/*---------------------------------------------------------------*/
//PUBLIC
float pressure_sensor_read()
{
  // theoretical output range 0 to 5 inches h20 
  // 5% to 80% of input voltage; 51.15 to 819.2 raw analogRead.
  //empirical: sensor reads 54 (raw analogRead) when 0 pressure,
  //reading average 762.27 (raw analogRead) when 4.375 inches measared with u-tube manometer
  //this is more like 5.3% to 84% (54 to 864) raw analogRead
  int raw = analogRead(PRESSURE_SENSOR_PIN);

//4 3/8 reading 4.6
//4.375 inches reading 4.627 inches
  
  float inches = (raw - 54) * 5.0 / (864 - 54);
  //inches = CLIP(inches, 0, 5);
  return inches;
}

/*---------------------------------------------------------------*/
//PUBLIC
float pid_init()
{
  uint8_t magic_number = 0x69;
  uint8_t data = EEPROM.read(0);
  if(data != magic_number)
    {
      EEPROM.write(0, magic_number);
      pid_set_p(pid_p);
      pid_set_i(pid_i);
      pid_set_d(pid_d);
      pid_set_target_pressure(PID_TARGET_PRESSURE);
    }

  EEPROM.get(1, pid_p);
  EEPROM.get(5, pid_i);
  EEPROM.get(9, pid_d);
  EEPROM.get(13, PID_TARGET_PRESSURE);
}

/*---------------------------------------------------------------*/
void pid_set_p(float p)
{
  pid_p = p;
  EEPROM.put(1, p);
}

/*---------------------------------------------------------------*/
void pid_set_i(float i)
{
  pid_i = i;
  EEPROM.put(5, i);
}

/*---------------------------------------------------------------*/
void pid_set_d(float d)
{
  pid_d = d;
  EEPROM.put(9, d);
}

/*---------------------------------------------------------------*/
void pid_set_target_pressure(float target_pressure)
{
  PID_TARGET_PRESSURE = target_pressure;
  EEPROM.put(13, target_pressure);
}

/*---------------------------------------------------------------*/
//PUBLIC
//returns the fan pwm duty cycle adjustment
float pid_update(float pressure, float target_pressure)
{
  float    e  = target_pressure - pressure;
  uint32_t t  = millis();
  uint32_t dt = pid_prev_time - t; //correct even when millis overflows
  float    deriv = (dt > 0) ? ((pid_prev_error - e) / (float)dt) : 0;
  
  pid_prev_time  = t;
  pid_prev_error = e;

  if(!pid_is_initalized)
    {
      pid_is_initalized = true;
      return 0;  
    }

  //trapezoidal integral
  pid_integral_error += (e+pid_prev_error) * 0.5 * (float)dt;

  return pid_p*e + pid_i*pid_integral_error + pid_d*deriv;
}

/*---------------------------------------------------------------*/
void setup() 
{
  Serial.begin(115200);
  fan_control_init();
  fan_sensor_init();
  pressure_sensor_init();
  pid_init();
}

/*---------------------------------------------------------------*/
void loop() 
{
  while(Serial.available())
    {
      switch(Serial.read())
        {
          case 'q':
            fan_set_duty(fan_get_duty(true) + 5, true);
            break;
          case 'a':
            fan_set_duty(fan_get_duty(true) - 5, true);
            break; 
          case 'w':
            fan_set_duty(fan_get_duty(false) + 5, false);
            break;
          case 's':
            fan_set_duty(fan_get_duty(false) - 5, false);
            break; 

          case 'p':
            pid_set_p(pid_p - 0.01);
            Serial.print("pid_p = ");
            Serial.println(pid_p);
            break;
          case 'P':
            pid_set_p(pid_p + 0.01);
            Serial.print("pid_p = ");
            Serial.println(pid_p);
            break; 
          case 'i':
            pid_set_i(pid_i - 0.01);
            Serial.print("pid_i = ");
            Serial.println(pid_i);
            break;
          case 'I':
            pid_set_i(pid_i + 0.01);
            Serial.print("pid_i = ");
            Serial.println(pid_i);
            break; 
          case 'd':
            pid_set_d(pid_d - 0.01);
            Serial.print("pid_d = ");
            Serial.println(pid_d);
            break;
          case 'D':
            pid_set_d(pid_d + 0.01);
            Serial.print("pid_d = ");
            Serial.println(pid_d);
            break; 
          case '=': /*cascade*/
          case '+':
            pid_set_target_pressure(PID_TARGET_PRESSURE - 0.01);
            Serial.print("target pressure = ");
            Serial.print(PID_TARGET_PRESSURE);
            Serial.println(" inches H2O");
            break;
          case '-': /*cascade*/
          case '_':
            pid_set_target_pressure(PID_TARGET_PRESSURE + 0.01);
            Serial.print("target pressure = ");
            Serial.print(PID_TARGET_PRESSURE);
            Serial.println(" inches H2O");
          default: break;
        }
    }

    Serial.println("***********************");
    Serial.print("INLET FAN: \t");
    Serial.print(round(fan_get_duty(true)));
    Serial.print("%\t");
    Serial.print(round(fan_inlet_sensor_rpm));
    Serial.println(" rpm");
    Serial.print("OUTLET FAN: \t");
    Serial.print(round(fan_get_duty(false)));
    Serial.print("%\t");
    Serial.print(round(fan_outlet_sensor_rpm));
    Serial.println(" rpm");
    Serial.print("PRESSURE: \t");
    Serial.print(pressure_sensor_read());
    Serial.println(" inches H2O");

    delay(250);
    
/*
    float adjustment = pid_update(pressure_sensor_read(), PID_TARGET_PRESSURE);
    fan_set_duty(adjustment + fan_get_duty(true) , true);
    fan_set_duty(adjustment + fan_get_duty(false), false);
*/
}
