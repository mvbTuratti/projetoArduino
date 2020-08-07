/*Código Arduino 2, controle de matriz :)
 * Como funciona o fluxo?
 * 1 - A primeira coisa que acontece no código é carregar todas as variáveis globais, essas variáveis existem em todos os escopos
 * 2 - Entra na função setup(), todas as variáveis criadas só existem _dentro_ do escopo setup(), a função dela é inicializar periféricos
 * 3 - Entra na função loop(), essa função fica constantemente sendo chamada, por isso o nome loop
*/
#include <stdlib.h>
int latchPin = 8;
int clockPin = 12;
int dataPin = 11;
int alvoG = -1;
int alvo2G = -1;
//Os pinos de alvo 1 e alvo 2 eu fiz uma gambiarra que está explicada mais em baixo do código
int numOfRegisters = 4; 
//número de ci594N
int A = 2, B = 3, C = 4, D = 5, E = 6, F = 7, G = 13; 
//É conveniente usar nomes com significados para as saídas de pinos, ajuda na hora da montagem :)

byte* registerState;
int levelGlobal = 1;
int mistakeG = 0;
/*level global é a variável que controla o nível da fase, mistakeG controla quantos erros em sequência aconteceram
 * essas variáveis precisam ser globais porque é bem incoveniente ficar passando as variáveis (ou cópia delas) entre funções,
 * sendo que elas são usadas constantemente, usualmente não é uma boa prática de programação, mas como eu sei que esse código não vai ser
 * base para novos códigos eu sei que não tem risco de alguém colocar um valor absurdo dentro dessas variáveis o:
 */
#include <Wire.h> //Essa biblioteca permite que você se comunique com dispositivos I2C.

void setup() {
  srand(micros()); //semente pra eventos randomicos, o parâmetro passado é o tempo de execução do código até então na esperança que seja diferente
  Serial.begin(9600);  //Inicializa a comunicação com o Serial Monitor   
  //Initialize array
  registerState = new byte[numOfRegisters];
  for (size_t i = 0; i < numOfRegisters; i++) {
    registerState[i] = 0;
  }
  //set pins to output so you can control the shift register
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  pinMode(A, OUTPUT);
  pinMode(B, OUTPUT);
  pinMode(C, OUTPUT);
  pinMode(D, OUTPUT);
  pinMode(E, OUTPUT);
  pinMode(F, OUTPUT);
  pinMode(G, OUTPUT);
  //Esse bloco ali de cima é uma função do arduino que fala que as saídas numéricas X são para output, então da pra enviar HIGH ou LOW nelas
  
  levelC(1);
  displayLvl();
  //Chama as funções de ligar o display de 7 segmentos + chama a função que acende a matriz de led no nível 1 (pq é o início do jogo)
  Wire.begin(9);  //Inicializa a comunicação              
  Wire.onReceive(receiveEvent); //Essa linha passa uma função como argumento, ou seja, quando o código receber uma chamada ele quer que o fluxo
  //do código seja interrompido e pule pra função chamada receiveEvent()
   
}

void loop() {
  /* Esse é o método que fica fazendo um loop, enquanto não há eventos ocorrendo ele fica sendo constantemente chamado
   * A lógica aqui é bem simples: Ele checa se o nível é acima de 9, se for acima ele abaixa pra 9, isso só aconteceria por alguma cagada
   * a ideia é que NUNCA entre nesse primeiro if
   * a segunda linha checa se está entre 1 e 6, se for, ele chama o método que controla o alvo com base no nível, aqui entra a questão do delay()
   * delay faz uma pausa em milisegundos, ou seja, cada 1000 equivale a 1 segundo de espera no código, a ideia aqui é que o delay máximo 
   * seja 1940ms, ou ~2 segs no nível 6, o alvo então moveria-se a cada 2 segundos na matriz, mas isso é altamente DEPENDENTE da corrente 
   * ou seja, não da pra confiar muito nisso
   * A partir do nível 7 ocorre uma diminuição no parâmetro pra ficar mais rápido
  */
  if(levelGlobal >9){
    levelGlobal = 9;
    }
  if((levelGlobal >0) && (levelGlobal <=6)){
    levelC(levelGlobal);
    delay((10 - levelGlobal)*460);
    }
  else if(levelGlobal >6){
    levelC(levelGlobal);
    delay((10-levelGlobal)*220);
    }
  
  delay(100);
}

/* receiveEvent é a função que é chamada toda a vez que chega alguma voltagem pelos pinos de i2C
 * O que ela faz?
 * Ela precisa dar sentido aos valores recebidos, pq pelo arduino 1 foram enviados dados de forma ordenada, aqui precisa ser respeitada
 * a ordenação
 * Cada mensagem carregava no máximo 8 bits, (0 até 255), então pra um valor de 550, como você envia?
 * Divide e pega o resto
 * 550 / 255 = 2 
 * 550 % 255 = 40
 * Aqui você junta esses valores ordenados que recebeu
 * 2 * 255 = 510
 * 510 + 40 = 550
*/
void receiveEvent(int howMany) {  
  int vetor[8];
  int flag = 0;
  //Lê os 8 valores enviados
   while(flag < 8){ 
    int x = Wire.read();  //Faz a leitura dos dados   
    vetor[flag] = x;
    flag++;
  }
  if(vetor[7] == 0){
    delay(800);
    return; //valor 7 é a velocidade de Y, se ele é 0 não tem velocidade nenhuma, então cancela o arremesso
  }
  int velx = 0, x = 0, vely = 0, y = 0;

  velx = (vetor[1] *200 + vetor[2])*15;
  if (vetor[0] == 1){
    velx *= -1;
    //O domínio de valores que foram enviados é de 0 até 255, como velocidade de x pode ser negativo é preciso passar um valor que avisa
    //se é um número negativo, se é 1 a velocidade de x é negativa, se for 0 a velocidade de x é positiva
  }
  x = vetor[3]*200 + vetor[4];
  vely = vetor[5];
  y = vetor[6]* 200 +vetor[7];
  //Aqui eu escolhi 200 ao invés de 255(limite superior) por nenhum motivo específico
  
  effectB(x, velx,y,vely);
  //Uma vez que os valores foram reagrupados é passado eles para a função effectB()
  
}

/* effectB é a função que controla o arremesso
 *  Como funciona a matriz?
 *  existe uma representação de 1 dimensão de 0 a 31 que controla cada LED individualmente
 *  a contagem começa de baixo pra cima, da direita pra esquerda, ou seja:
 *  31  23  15  7
 *  30  22  14  6
 *  29  21  13  5
 *  28  20  12  4
 *  27  19  11  3
 *  26  18  10  2
 *  25  17  9   1
 *  24  16  8   0
 *  Essa parte é importante para entender como funciona o código no geral
 *  Relações matemáticas de movimento se resumem a, norte é +1, sul é -1
 *  leste é ir uma posição para a esquerda, veja que o que diferencia o valor entre esqueda e direita é uma diferença de + ou - 8 
 *  ir para a esquerda é -8, ir para a direita +8.
 *  A ideia é sempre aplicar um desses valores cardinais e checar: 
 *  O valor do movimento seguinte está de 0 a 31? Se a resposta for não é pq ele saiu da matriz de LEDs.
*/
void effectB(int x, int vx, int y, int vy){
  bool flagOfHit = false;
  int defineColumn = 0;
  //checa se tem alguma velocidade em Y
  if(vy >0){
    for(int defineRow = 0; defineRow<9; defineRow++){
      //Um for de no máximo 9 checagens, qual a ideia? pra percorrer uma coluna precisa de 8 checagens, se nenhuma delas for acerto
      //esse próximo if simula que o arremesso saiu da matriz e o break; encerra esse for, a variável do for representa a linha ocupada
      if(defineRow == 8){
        delay(2300);
        mistake();
        break;
        }
      /* Como é determinada a coluna da matriz? 
       *  Depois de alguns testes visuais eu cheguei nos alcances listados abaixos,
       *  veja que o valor que eu atribui para a variável defineColumn representa a posição do chão de cada coluna
       *  como o for representa a linha, eu sei que a soma de defineRow e defineColumn define qual posição da matriz o arremesso está
      */
      if((x>120) && (x<290)){
        defineColumn = 24;
        
      }
      else if((x>=290) && (x<390)){
        defineColumn = 16;
      }
      else if((x>=390) && (x<580)){
        defineColumn = 8;
      }
      else if((x>=580)&&(x<720)){
        defineColumn = 0;
      }
      else{
        //Aqui é um else pra caso x > 720 ou x < 120 (saiu pelas laterais)
        if(mistakeG == 1){
          mistakeG = 0;
          levelGlobal = 1;
          displayLvl();
        }
        else{
          mistakeG++;
        }
        //mistake() é a função que faz a animação de erro, o if e else acima checa se o cabra já tinha errado 1x antes, caso sim,
        //reseta pro nível 1 e chama o método de dificuldade 1, else só soma 1 a variável de erro e da break; pra voltar pro loop() 
        //e aguarda um novo arremesso
        mistake();
        break;
      }
      int trueVelocity = 0;  
      if (vy >=100){
        trueVelocity = 200;
        }
      else{
        trueVelocity = (100-vy)*115;
      }
      /* Como entender a velocidade:
       *  Eu chamo delay que serve de um tempo em que uma LED fica acesa, quanto menos tempo acesa antes de mudar de posição, maior a sensação
       *  de velocidade.
       *  A equação acima é bem semelhante a equação do loop(), aqui só está calibrada com valores determinados visualmente
       *  O domínio da função de velocidade de y estava entre 0 e 80 até onde testei, mas CASO alguma bruxaria aconteça e o valor supere 100
       *  eu coloquei uma velocidade mínima
      */
      if(((defineRow + defineColumn) == alvoG)||((defineRow + defineColumn) == alvo2G)){
        strike(defineRow+defineColumn, trueVelocity);
        if(levelGlobal <9){
          levelGlobal++;
        }
        displayLvl();
        mistakeG = 0;
        flagOfHit = true;
        delay(30000);
        levelC(levelGlobal);
        delay(4000);
        break;
        /* Aqui entra a questão de alvo 1 e 2 serem globais, eles seguram aonde na matriz está aceso o alvo, aqui basta checar
         *  se a posição atual equivale a uma posição já ocupada pelos alvos, caso SIM chama-se a função strike() que anima a colisão
         *  um if checa se o nível é inferior a 9, caso seja é aumentado em 1, os erros são resetados (vc passou de fase), uma flag 
         *  é ativada, repare que aqui o delay() toma 30s e 4s, leva esse tempo de verdade? 
         *  Não. 
         *  Pq isso acontece? 
         *  Não sei.
         *  Se tirar isso o que acontece?
         *  Ele colide e já acende o próximo alvo, fica meio estranho
        */  
      }
      
      regWrite(defineRow + defineColumn, HIGH); //Caso a posição dessa linha+coluna não seja igual a algum dos 2 alvos, acende a posição 
      
      delay(trueVelocity); //espera o delay
      if(vy <4){
        /*
         * Se a velocidade é menor que 4 é basicamente parado, ou seja, o movimento morreu, espera uma caralhada de tempo (150s em tese, ~2s na realidade)        
         * depois da espera de tempo, apaga a luz da LED na posição atual.
         * Soma +1 pro erro, caso não tenha nenhum erro
         * Reseta pro nível 1, caso tenha já um erro
         * Depois disso da break; pra sair do for que representa o arremesso, volta pro loop() e espera um novo arremesso
         */
        delay(150000);
        regWrite(defineRow + defineColumn, LOW); //apaga o LED
        if(mistakeG == 1){
          mistakeG = 0;
          levelGlobal = 1;
        }
        else{
          mistakeG++;
        }
        displayLvl();
        mistake();
        break;
      }
      //CASO ainda tenha velocidade suficiente:
      regWrite(defineRow + defineColumn, LOW); //Apaga a luz do LED
      vy -= 3;
      x += vx;
      /* Subtrai 3 da velocidade, esse valor 3 cheguei arbitrariamente no visual mesmo, ele depende muito fortemente do domínio da velocidade y     
       *  Repare que isso age como uma força, pq ele influencia diretamente no valor de velocidade e a velocidade por si influencia na posição.
       *  Aqui também é mudada a posição de X, isso é um pouco mais complicado por 2 motivos:
       *  1- o domínio da vel de X está entre aproximadamente -120 a 320, porquê capta até 3 vezes mais na direção positiva é consequência direta
       *  do quão ruim o hardware é.
       *  2- Existe valor positivo e negativo, então usar uma força de freio precisa checar se é positivo ou negativo
       *  
      */
      if(vx >0){
        vx -= 30;
        if(vx<0){
            vx =0;
          }
      }
      else if(vx <0){
        vx += 10;
        if(vx>0){
          vx = 0;
        }  
      }
    }
    delay(200);
  }
  else{
    //Aqui é o else de velY > 0, caso não seja maior que 0 não teve nem aremesso, então soma um erro ou reseta pro nível 1
    mistake();
    if(mistakeG == 1){
      mistakeG = 0;
      levelGlobal = 1;
      displayLvl();
    }
    else{
      mistakeG++;
    }
    delay(1000);
  }
}

/*LevelC define quais alvos serão ligados
 * Eu ia fazer um switch case pra adicionar coisas específicas pra cada level, depois escolhi fazer só uma mudança de velocidade
 * essa função deveria ser somente um if else.
*/
void levelC(int level){
  if(alvoG != -1){
    regWrite(alvoG, LOW);
  }
  if(alvo2G != -1){
    regWrite(alvo2G, LOW);
  }
  //Checa se os alvos estão ativos (diferente de -1), caso estejam, apague-os
  int alvo = rand() % 32;
  int alvo2 = rand() %32;
  //seleciona dois alvos aleatórios entre 0 e 31
  switch(level){
    case 1:
      do{
        alvo2 = rand() %32;
      }while(alvo == alvo2);      
      regWrite(alvo, HIGH);
      regWrite(alvo2, HIGH);
      alvoG = alvo;
      alvo2G = alvo2; //no nível 1, acende ambos alvos gerados depois de garantir que eles são diferentes entre si
      break;
    case 2:
      alvo = rand()%32;
      regWrite(alvo, HIGH);
      alvoG = alvo;
      alvo2G = -1; //acende só 1 deles e joga o alvo 2 pra valor inválido (-1)
      break;
    default:
      alvo2G = -1;
      alvo = rand()%32;
      regWrite(alvo, HIGH);
      alvoG = alvo; //mesma coisa que nível 2
      break;
    }  
}

/* Função de acerto!
 *  Essa é um pouco mais complicada pelas posições não serem matriciais, a ideia é gerar um movimento pro norte, nordeste e noroeste do 
 *  arremesso.
 *  Pq é mais difícil?
 *  Pq tem que garantir que não superou algum valor superiores de uma das colunas, por exemplo, se explodir e um dos alvos for a posição 15
 *  somar mais 1 no sentido norte traria o número 16, que é a posição de baixo da coluna do lado esquerdo.
 *  Como contorno isso? 
 *  Dou um valor arbitrário inicial, depois tento modificar e verifico se a posição é uma das de baixo, caso seja ignore.
 *  
*/
void strike(int posit, int vel){  
  if((alvoG != -1) && (alvoG != posit)){
    regWrite(alvoG, LOW);
  }
  if((alvo2G != -1) &&( alvoG != posit)){
    regWrite(alvo2G, LOW);
  }
  //Quando acerta tem que apagar o outro alvo aceso (no nível 1)
  delay(vel);
  regWrite(posit, LOW);
  signed int leste = 50, oeste = 50, noroeste = 50, nordeste = 50, norte = 50; //50 foi um valor arbitrário, só pra avisar que ta na 1a ainda
  for(signed int i = 1; i<4; i++){
    if((norte <31)||(norte == 50)){
      norte = posit + i;
    }
    if((oeste<31)||(oeste == 50)){
      oeste = posit+(i*8);
    }
    if((leste>-1)||(leste == 50)){
      leste = posit-(i*8);
    }
    if((noroeste<31)||(noroeste == 50)){
      noroeste = oeste+i;
    }
    if((nordeste>-1)||(nordeste == 50)){
      nordeste = leste +i;
    }
    if(nortificar(norte)){
      regWrite(norte, HIGH);
    }else{norte=32;}
    if(nortificar(leste)){
      //regWrite(leste, HIGH);    
    }else{leste=-1;}
    if(nortificar(oeste)){
      //regWrite(oeste, HIGH);    
    }else{oeste=32;}
    if(nortificar(nordeste)){
      regWrite(nordeste, HIGH);   
    }else{norte=-1;}
    if(nortificar(noroeste)){
      regWrite(noroeste, HIGH); 
    }else{noroeste=32;}
    vel+=3200;
    delay(vel);
    //nortificar() checa se é > 0 e < 31 e é diferente das posições inferiores
    for(int j = 0; j < 32; j++){
      regWrite(j, LOW);  
    }
    //esse daqui apaga tudo na matriz pq tava com preguiça de guardar 3 valores pra serem apagados depois 
  }
  delay(4500);
}

bool nortificar(int pos){
    if(((pos < 31) && (pos >= 0))||(pos == 50)){
      if((pos != 8)&&(pos!= 16)&&(pos !=24)&&(pos!=32)){
        return true;          
      }
      else{
        return false;
      }
    }
    else{
      return false;  
    }  
}

/* Função de erro!
 *  Ele acende no sentido positivo do for e depois no sentido negativo, fiz uma logicazinha pra fazer uma espécie de arco
 *  a logica com if basicamente fala pra pular a 1a iteração do for e compensa isso no segundo for.
*/
void mistake(){
  delay(8000);
  for(int i = 0; i< 8; i++){
    if(i!=0){
      
      regWrite(i-1, HIGH);
      regWrite(i+23,HIGH);
    }
    regWrite(i+8, HIGH);
    regWrite(i+16, HIGH);
    delay(8000);
    
    if(i!=0){
      
      regWrite(i-1, LOW);
      regWrite(i+23,LOW);  
    }
    regWrite(i+8, LOW);
    regWrite(i+16, LOW);
        
  }
  regWrite(7, HIGH);
  regWrite(31,HIGH);

  for(int i = 7; i>-1; i--){
    
    if(i!=7){
      regWrite(i+1, HIGH);
      regWrite(i+25,HIGH);
    }
    
    regWrite(i+8, HIGH);
    regWrite(i+16, HIGH);
    
    delay(8000);
    
    if(i!=7){
      regWrite(i+1, LOW);
      regWrite(i+25,LOW);    
    }else{
      regWrite(7, LOW);
      regWrite(31,LOW);   
    }
    regWrite(i+8, LOW);
    regWrite(i+16, LOW);
    
  }
  //Basicamente se você quer acender ou apagar um pino você tem que chamar o regWrite()
  levelC(1);
  delay(10000);
}

//displayLvl() controla o display de 7 segmentos na raça, ele faz isso dizendo em quais saídas vão ter de ser acendidas com base no número 
//decimal
void displayLvl(){
   switch(levelGlobal){
    case 1:
      digitalWrite(A, LOW);
      digitalWrite(B, HIGH);
      digitalWrite(C, HIGH);
      digitalWrite(D, LOW);
      digitalWrite(E, LOW);
      digitalWrite(F, LOW);
      digitalWrite(G, LOW);
      break;
    case 2:
      digitalWrite(A, HIGH);
      digitalWrite(B, HIGH);
      digitalWrite(C, LOW);
      digitalWrite(D, HIGH);
      digitalWrite(E, HIGH);
      digitalWrite(F, LOW);
      digitalWrite(G, HIGH);
      break;
    case 3:
      digitalWrite(A, HIGH);
      digitalWrite(B, HIGH);
      digitalWrite(C, HIGH);
      digitalWrite(D, HIGH);
      digitalWrite(E, LOW);
      digitalWrite(F, LOW);
      digitalWrite(G, HIGH);
      break;
    case 4:
      digitalWrite(A, LOW);
      digitalWrite(B, HIGH);
      digitalWrite(C, HIGH);
      digitalWrite(D, LOW);
      digitalWrite(E, LOW);
      digitalWrite(F, HIGH);
      digitalWrite(G, HIGH);
      break;
    case 5:
      digitalWrite(A, HIGH);
      digitalWrite(B, LOW);
      digitalWrite(C, HIGH);
      digitalWrite(D, HIGH);
      digitalWrite(E, LOW);
      digitalWrite(F, HIGH);
      digitalWrite(G, HIGH);
      break;
    case 6:
      digitalWrite(A, HIGH);
      digitalWrite(B, LOW);
      digitalWrite(C, HIGH);
      digitalWrite(D, HIGH);
      digitalWrite(E, HIGH);
      digitalWrite(F, HIGH);
      digitalWrite(G, HIGH);
      break;
    case 7:
      digitalWrite(A, HIGH);
      digitalWrite(B, HIGH);
      digitalWrite(C, HIGH);
      digitalWrite(D, LOW);
      digitalWrite(E, LOW);
      digitalWrite(F, LOW);
      digitalWrite(G, LOW);
      break;
    case 8:
      digitalWrite(A, HIGH);
      digitalWrite(B, HIGH);
      digitalWrite(C, HIGH);
      digitalWrite(D, HIGH);
      digitalWrite(E, HIGH);
      digitalWrite(F, HIGH);
      digitalWrite(G, HIGH);
      break;
    default:
      digitalWrite(A, HIGH);
      digitalWrite(B, HIGH);
      digitalWrite(C, HIGH);
      digitalWrite(D, HIGH);
      digitalWrite(E, LOW);
      digitalWrite(F, HIGH);
      digitalWrite(G, HIGH);
      break;
   }
}

//regWrite é a função que controla os CIs, ele funciona com base em HIGH e LOWs em pinos específicos do CI, dependendo da sequência 
//que você faz no CI ele interpreta como uma funcionalidade diferente, com base no datasheet da pra fazer varias coisas
//nesse daqui ele está basicamente povoando de forma serial quais pinos serão HIGH e LOW e depois confirmando via enable que é pra mostrar 
//os valores atualizados.
void regWrite(int pin, bool state){
  //Determines register
  int reg = pin / 8;
  //Determines pin for actual register
  int actualPin = pin - (8 * reg);

  //Begin session
  digitalWrite(latchPin, LOW);

  for (int i = 0; i < numOfRegisters; i++){
    //Get actual states for register
    byte* states = &registerState[i];

    //Update state
    if (i == reg){
      bitWrite(*states, actualPin, state);
    }

    //Write
    shiftOut(dataPin, clockPin, MSBFIRST, *states);
  }

  //End session
  digitalWrite(latchPin, HIGH);
}
