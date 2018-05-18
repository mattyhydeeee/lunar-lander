/* Game Controller 

/* apropriate libraries 
#include <mbed.h>
#include <EthernetInterface.h>
#include <rtos.h>
#include <mbed_events.h>
#include <FXOS8700Q.h>
#include <C12832.h>
#include <math.h>

/* display 
C12832 lcd(D11, D13, D12, D7, D10);

/* wrapper functions for leds being on and off 
enum { Red, Green, Blue};
DigitalOut LED[] = {
  DigitalOut(PTB22,1),
  DigitalOut(PTE26,1),
  DigitalOut(PTB21,1)
};

void LEDon(int n) {
  LED[n].write(0);
}
void LEDoff(int n) {
  LED[n].write(1);
}

/* speaker 
PwmOut speaker(D6);

/* event queue and thread support 
Thread dispatch;
EventQueue periodic;

/* Accelerometer 
I2C i2c(PTE25, PTE24);
FXOS8700QAccelerometer acc(i2c, FXOS8700CQ_SLAVE_ADDR1);

/* Input from Potentiometer 
AnalogIn  left(A0);

/* wrapper functions for if buttons are pressed or not 
enum { Btn1, Btn2, sw_down, sw_left, sw_right};
struct pushbutton {
    DigitalIn sw;
    bool invert;
} buttons[] = {
  {DigitalIn(SW2),true},
  {DigitalIn(SW3),true},
  {DigitalIn(A3),false},
  {DigitalIn(A4),false},
  {DigitalIn(A5),false}
};

/* a switching modes interrupt 
InterruptIn sw_up(A2);

bool ispressed(int b) {
  return (buttons[b].sw.read())^buttons[b].invert;
}

/* is user in automatic or not 
bool automatic = false;

/* a function to switch modes 
void swapMode(void){
  automatic = !automatic;
}

/* User input states 
float rollRate;
float throttle = 0.0f;

/* Task for polling sensors 
void user_input(void){
  /* if the appropriate button is pressed the users control mode will change 
    if (automatic){
    /* acquiring the Accelerometer rating 
    motion_data_units_t a;
    acc.getAxis(a);
    /* a deadband to ignore readings near 0 
    if (asin(a.x) < 0.1f && asin(a.x) > -0.1f){
      rollRate = 0.0f;
    }
    /* acquring a more accurate reading if the Accelerometer is acceptable 
    else{
    float magnitude = sqrt( a.x*a.x + a.x*a.x + a.x*a.x );
    a.x = a.x/magnitude;
    rollRate = (asin(a.x) * -1.0f);
    }
    /* a failsafe if the Accelerometer returns nan 
    if (isnan(rollRate)){
      rollRate = 0.0f;
    }
    /* getting the reading from the Potentiometer 
    if (left >= 0.0f){
      throttle = (left * 100.0f);
    }
    }
    /* getting inputs for manual mode 
    else if(!automatic){
      rollRate = 0.0f;
      if(ispressed(3)){
        rollRate -= 1.0f;
      }
      else if(ispressed(4)){
        rollRate += 1.0f;
      }
      else if(ispressed(0)){
        throttle++;
      }
      else if(ispressed(1)){
        throttle--;
      }
    }
    /* allowing the user to set throttle to 0 
    if (ispressed(2)){
      throttle = 0.0f;
    }
    sw_up.rise(swapMode);
} */







/* States from Lander */
float altitude = 0.0, fuel= 100.00;
int orientation, Vx, Vy;
bool isFlying, isCrashed;

/* The addresses of the lander and dashboard */
SocketAddress lander("192.168.0.16",65200);
SocketAddress dash("192.168.0.16",65250);

/* the UDP and eth connections */
EthernetInterface eth;
UDPSocket udp;

/* Task for synchronous UDP communications with lander */
void communications(void){
    /* variables for fomratting the message */
    SocketAddress source;
    char buffer[512];

    /* formatting the message to be sent to the lander */
    sprintf(buffer,"command:!\nthrottle:%f\nroll:%f\n", throttle, rollRate);

    /* sending the message */
    udp.sendto( lander, buffer, strlen(buffer));

    /* recieving the message */
    nsapi_size_or_error_t  n =
     udp.recvfrom(&source, buffer, sizeof(buffer));
    buffer[n] = '\0';

    /* Unpack incomming message */
    char *nextline, *line;
    for(
        line = strtok_r(buffer, "\r\n", &nextline);
        line != NULL;
        line = (strtok_r(NULL, "\r\n", &nextline))
    ){
      char *key, *value;
      key = strtok(line, ":");
      value = strtok(NULL, ":");
      /* acquring appropriate variables */
      if(strcmp(key,"altitude")==0 ) {
        altitude = atof(value);
      }
      else if (strcmp(key,"fuel")==0){
        fuel = atof(value);
      }
      else if (strcmp(key, "flying") == 0){
        if (atoi(value) == 1){
          isFlying = true;
        }
        if (atoi(value) == 0){
          isFlying = false;
        }
      }
      else if (strcmp(key,"crashed")==0){
        if (atoi(value) == 1){
          isCrashed = true;
        }
        else if(atoi(value) == 0){
          isCrashed = false;
        }
      }
      else if(strcmp(key,"orientation")==0){
          orientation = atoi(value);
      }
      else if(strcmp(key,"Vx")==0){
          Vx = atoi(value);
      }
      else if(strcmp(key,"Vy")==0){
        Vy = atoi(value);
      }
    }
}

/* Task for asynchronous UDP communications with dashboard */
void dashboard(void){
    /* formatting and sending message to dashboard */
    char buffer[512];
    sprintf(buffer, "altitude:%f\nfuel:%f\nflying:%d\noVx:%d\noVy:%d\nautomatic:%d", altitude, fuel, isFlying,Vx,Vy,automatic);
    udp.sendto( dash, buffer, strlen(buffer));
}

int main() {
    /* enabling the Accelerometer */
    acc.enable();

    /* initialising appropriate variables */
    throttle = 0.00f;
    isFlying = false;
    isCrashed = false;

    /* ethernet connection : usually takes a few seconds */
    printf("conecting \n");
    eth.connect();
    /* write obtained IP address to serial monitor */
    const char *ip = eth.get_ip_address();
    printf("IP address is: %s\n", ip ? ip : "No IP");

    /* open udp for communications on the ethernet */
    udp.open( &eth);

    printf("lander is on %s/%d\n",lander.get_ip_address(),lander.get_port() );
    printf("dash   is on %s/%d\n",dash.get_ip_address(),dash.get_port() );

    dispatch.start( callback(&periodic, &EventQueue::dispatch_forever) );

    /* periodic tasks */
    periodic.call_every(50, user_input);
    periodic.call_every(50, communications);
    periodic.call_every(50, dashboard);

    /* start event dispatching thread */
    dispatch.start( callback(&periodic, &EventQueue::dispatch_forever) );

    while(1) {
        /* setting the speaker and leds to off */
        speaker.write(0);
        lcd.locate(0,0);
        LEDoff(0); //red
        LEDoff(1); // green
        LEDoff(1); // blue
        /* functions to do only if the lander is flying */
        if(isFlying){
          lcd.printf("Velocity X: %i\nVelocity Y: %i", Vx, Vy);
          lcd.locate(0,20);
          lcd.printf("orientation:%d", orientation);
          /* changing led if the user is in manual mode*/
          if (automatic == false){
            LEDoff(0);
            LEDoff(2);
            wait(0.5);
            LEDon(2);
            wait(0.5);
          }
          else if (automatic == true){
          LEDoff(0);
          LEDoff(2);
          wait(0.5);
          LEDon(1);
          wait(0.5);
          LEDoff(1);
          }
        }
        /* changing details displayed for when lander is crashed */
        if(isCrashed){
          lcd.cls();
          lcd.locate(0,20);
          lcd.printf("Oh no you've crashed!\n");
          LEDoff(1);
          LEDoff(2);
          wait(0.5);
          LEDon(0);
          wait(0.5);
          for (int f=20.0; f<100; f+=10) {
            speaker.period(1.0/f);
            speaker.write(0.5);
            wait(0.1);
          }
          /* changing details if lander is landed */
          if (!isCrashed && !isFlying){
            lcd.cls();
            lcd.locate(0,20);
            lcd.printf("You've landed!\n");
            LEDoff(2);
            LEDoff(0);
            LEDoff(1);
            wait(0.5);
            LEDon(1);
            wait(0.5);
          }
          lcd.cls();
        }
        wait(0.5);/* The speed at which the main loop runs */
    }
}
