// the regular Adafruit "TouchScreen.h" library only works on AVRs

// different mcufriend shields have Touchscreen on different pins
// and rotation.
// Run the TouchScreen_Calibr_native sketch for calibration of your shield

#include <MCUFRIEND_kbv.h>
MCUFRIEND_kbv tft;       // hard-wired for UNO shields anyway.
#include <TouchScreen.h>
#include <Wire.h> //Essa biblioteca permite que você se comunique com dispositivos I2C.

const int XP=7,XM=A1,YP=A2,YM=6; //ID=0x9320
const int TS_LEFT=197,TS_RT=883,TS_TOP=204,TS_BOT=895;

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);
TSPoint tp;

#define MINPRESSURE 10
#define MAXPRESSURE 2800

int16_t BOXSIZE;
int16_t PENRADIUS = 1;
uint16_t ID, oldcolor, currentcolor;
uint8_t Orientation = 0;    //PORTRAIT
// Assign human-readable names to some common 16-bit color values:
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF


void show_tft(unsigned long q)
{
    tft.setTextColor(WHITE, BLACK);
    unsigned long start;
    unsigned long helper;
    unsigned long endometer = 0;
    unsigned long tobeDiscont = 0;
    int xvel = 0;
    int yvel = 0;
    signed int flagY = 1;
    signed long xholder = 0;
    signed long xSomatorio = 0;
    signed long vxSomatorio = 0;
    signed long yholder = 0;
    signed int x2 = 0;
    int contador = 1;
    unsigned long holder = q;
    while (1) {
        tp = ts.getPoint();
        start = micros();
        if (endometer >= 95000){
              float timer = (start - holder - tobeDiscont)/(float)100000;              
              //Serial.println("cont = " + String(contador));
              if (timer < 0.3){
                timer = 0.3f;
                }
              if (xvel < 0){
                xvel *= -1;
                x2 = 1;
                }
              else{
                x2 = 0;
                }
              if (yvel < 0){
                
                yvel = 0;
                }
              xSomatorio /= contador;
              vxSomatorio /= (contador - 1);
              //Serial.println("somatorioX: " + String(xSomatorio) + "\txvSum: " + String(vxSomatorio) + "\txholder=" +String(xholder));

              if(abs(vxSomatorio) <= 100){
                xholder = (int)xSomatorio;  
              }
              float xvf = (xvel /(float)(contador * timer *2));
              float yvf = (yvel /(float)(contador * timer));              
              int fatorxv = (xvf / 200);
              int restoxv = ((int)xvf % 200);
              int fatorxf = (int(xholder)/200);
              int restoxf = (int(xholder)%200);
              int fatoryf = (int(yholder)/200);
              int restoyf = (int(yholder)%200);
              int x = fatorxf  * 200 + restoxf;
              int y = fatoryf * 200 + restoyf;
              int velx = (fatorxv * 200 + restoxv);
              if (x2){
                velx *= -1;
                }
              int vely = (int) yvf;
              
              if (contador < 3){
                yvf = 0;
                vely = 0;
                }
              
              Serial.println(String(timer));
              Serial.println("xholder: " + String(xholder) + "\tyholder: " + String(yholder));
              Serial.println("x: "+ String(x)+"\tvel x: "+String(velx)+"\ny: "+String(y)+"\tvel y: "+String(vely));
              
              //Serial.println("\t" + String(x2) + "\t" + String(fatorxv) + "\t" + String(restoxv) +"\t" + String(fatorxf)); 
              //Serial.println("\t" + String(restoxf) + "\t" + String(vely) + "\t" + String(fatoryf) +"\t" + String(restoyf));
              Wire.beginTransmission(9); //Inicie uma transmissão para o dispositivo escravo I2C com o endereço fornecido. 
                             //Posteriormente, enfileira os bytes para transmissão com a função write () e os transmita chamando endTransmission () .
              Wire.write(x2);             //Função write faz a escrita dos dados  
              Wire.write(fatorxv);
              Wire.write(restoxv);
              Wire.write(fatorxf);
              Wire.write(restoxf);
              Wire.write(vely);
              Wire.write(fatoryf);
              Wire.write(restoyf);
              Wire.endTransmission();   //Encerra a transmissão 
              delay(1000); 
              break;
        }
        pinMode(XM, OUTPUT);
        pinMode(YP, OUTPUT);
        float timers = (start - holder)/(float)1000000;
        
        tft.setTextSize(5);
        tft.setCursor(tft.width() / 4 , tft.height()/2);
        tft.print(timers);
        if (tp.z < MINPRESSURE || tp.z > MAXPRESSURE) {
            tft.setCursor(0,0);
            tft.setTextSize(2);
            helper = micros();
            endometer += helper - start;
            tobeDiscont = endometer;
            tft.print(1.15f -(endometer/(float)100000));
            continue;
        }

        if (contador == 1)
        {
          xholder = tp.x;
          yholder = tp.y;
          contador++;
          endometer = 0;
          xSomatorio+= xholder;
          
          continue;
          }
        xvel += tp.x - xholder;
        yvel += yholder - tp.y;
        flagY = yholder - tp.y;
        contador++;
        xholder = tp.x;
        yholder = tp.y;
        xSomatorio+= xholder;
        vxSomatorio += xvel;
        endometer = 0;
        if (flagY <= -1){
          yvel = 0;
          contador = 1;
          holder = micros();
          tobeDiscont = 0;
          xSomatorio = 0;
          vxSomatorio = 0;
          //Serial.println("flag = " + String(flagY));
          }
        

    }
}


void setup(void)
{
    uint16_t tmp;
    Wire.begin(); //Inicializa a comunicação
    Serial.begin(9600);
    
    
}

void loop()
{
    tft.reset();
    ID = tft.readID();
    tft.begin(ID);
    tft.setRotation(Orientation);
    tft.setCursor(tft.width() /4 , tft.height()/2);
    tft.setTextColor(BLACK, GREEN);
    tft.fillScreen(BLACK);

    tft.setTextSize(4);
    tft.print("Start");
    
    while(1){

      tp = ts.getPoint();
      Serial.print("");
      if (tp.z >= 400) 
      {
        break;
      }
        
    }
    
  
    unsigned long resetTimer = micros();
    show_tft(resetTimer);
    
}
