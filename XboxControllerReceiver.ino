/* Arduino code to communicate with xbox 360 RF module.
  Original work by (yaywoop)
  Additional ideas from Alexander Martinez
  Modified by dilandou (www.dilandou.com, www.diru.org/wordpress)
  Controller turn off added by Martin Ballantyne using research from http://tkkrlab.nl/wiki/XBOX_360_RF_Module
First sends LED initialisation code followed by LED startup animation code, then sleeps until a button press for sync command.
If the sync button is held down and released after 1000ms the connected controllers will be turned off. Otherwise the sync command will initiate.
RF module must be powered with 3.3V, two diodes in series with USB 5v will do. Connect the USB wires to a host computer, and the data and serial wires to Arduino.
of course, make sure to have a common ground */

#include <avr/sleep.h>

#define sync_pin 2 //power button repurposed for sync button (pin 5 on the module)
#define data_pin 3 //data line (pin 6 on the module)
#define clock_pin 4 //clock line (pin 7 on module) 

int led_cmd[10] =          {0,0,1,0,0,0,0,1,0,0}; //Activates/initialises the LEDs, leaving the center LED lit.
int led_timer_red_1[10] =  {0,0,1,0,1,1,1,0,0,0}; //Set quadrant 1 to red
int led_timer_red_2[10] =  {0,0,1,0,1,1,1,1,0,0}; //Set quadrant 1 and 2 to red
int led_timer_red_3[10] =  {0,0,1,0,1,1,1,1,0,1}; //Set quadrant 1, 2 and 4 to red
int led_timer_red_4[10] =  {0,0,1,0,1,1,1,1,1,1}; //Set quadrant 1, 2, 3 and 4 to red
int led_red_off[10] =      {0,0,1,0,1,1,0,0,0,0}; //Set quadrant 1, 2, 3 and 4 to off

int anim_cmd[10] =         {0,0,1,0,0,0,0,1,0,1}; //Makes the startup animation on the ring of light.
int sync_cmd[10] =         {0,0,0,0,0,0,0,1,0,0}; //Initiates the sync process.
int turn_off_cmd[10] =     {0,0,0,0,0,0,1,0,0,1}; //Turns off the connected controllers.

volatile boolean sync_pressed = 0;
int sync_hold_time = 0;
boolean turn_off_controllers = false;

void sendData(int command[])
{
  pinMode(data_pin, OUTPUT);
  digitalWrite(data_pin, LOW);    //start sending data.
  
  int previous_clock = 1;  
  for(int i = 0; i < 10; i++)
  {
    while (previous_clock == digitalRead(clock_pin)){} //detects change in clock
    previous_clock = digitalRead(clock_pin);
    
    // should be after downward edge of clock, so send bit of data now
    digitalWrite(data_pin, command[i]);

    while (previous_clock == digitalRead(clock_pin)){} //detects upward edge of clock
    previous_clock = digitalRead(clock_pin);
  }
  
  digitalWrite(data_pin, HIGH);
  pinMode(data_pin, INPUT);
  
  delay(50);
}

void setHeldLEDs(int held_time)
{
  if(held_time >= 1000)
  {
    sendData(led_timer_red_4);
  }
  else if(held_time >= 750)
  {
    sendData(led_timer_red_3);
  }
  else if(held_time >= 500)
  {
    sendData(led_timer_red_2);
  }
  else if(held_time >= 250)
  {
    sendData(led_timer_red_1);
  }
  else
  {
    sendData(led_red_off);
  }
}

void initLEDs()
{
  sendData(led_cmd);
  sendData(anim_cmd);
}

void wakeUp()
{
  sync_pressed = 1;
}

void sleepNow()
{
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // set sleep mode
  sleep_enable(); //enable sleep bit
  attachInterrupt(0, wakeUp, LOW);
  sleep_mode();
  sleep_disable(); //disable sleep bit
  detachInterrupt(0); // disables interrupt 0 on pin 2
}

void setup() 
{
  Serial.begin(9600);
  pinMode(sync_pin, INPUT);
  digitalWrite(sync_pin,HIGH);
  pinMode(data_pin, INPUT);
  pinMode(clock_pin, INPUT);
  delay(2000);

  initLEDs();
}

void loop()
{
  // Only sleep if the sync button is not held down
  if(!sync_pressed)
  {
    sleepNow();
  }
  
  delay(200);
  
  if(sync_pressed)
  {    
    Serial.print("Sync held time (ms): ");
    Serial.println(sync_hold_time, DEC);
    
    setHeldLEDs(sync_hold_time);
    
    // Count 1000ms, if elapsed the user wants to turn off their controllers
    if(sync_hold_time >= 1000)
    {
      turn_off_controllers = true;
      sync_hold_time = 1000;
    }
    
    // Initiate the user's action when they release the sync button
    if (digitalRead(sync_pin))
    {
      setHeldLEDs(0); 
      
      if(turn_off_controllers)
      {
        Serial.println("Turning off controllers.");
        sendData(turn_off_cmd);
        
        turn_off_controllers = false;
      }
      else
      {
        Serial.println("Syncing controllers.");
        sendData(sync_cmd);  
      }
      
      // Action complete, reset hold time and button state
      sync_hold_time = 0;
      sync_pressed = false;
    }
    else
    {    
      sync_hold_time += 200;
    }
  }
}
