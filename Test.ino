
#define LED_PIN    13

// Радиобрелки ///////////////////////////////////////////////////////////////////////////////////////
#define HCS_RECIEVER_PIN  2    // пин к которому подключен приемник для брелков

class HCS301 {
public:
  unsigned BattaryLow : 1;  // На брелке села батарейка
  unsigned Repeat : 1; // повторная посылка
  unsigned BtnNoSound : 1;
  unsigned BtnOpen : 1; 
  unsigned BtnClose : 1; 
  unsigned BtnRing : 1;
  unsigned long SerialNum;
  unsigned long Encript;

  void print();
};

volatile boolean  HCS_Listening = true;   
byte        HCS_preamble_count = 0;
uint32_t      HCS_last_change = 0;
//uint32_t      HCS_start_preamble = 0;
uint8_t       HCS_bit_counter;        // счетчик считанных бит данных
uint8_t       HCS_bit_array[66];        // массив считанных бит данных
#define       HCS_TE    400         // типичная длительность имульса Te
#define       HCS_Te2_3 600         // HCS_TE * 3 / 2

HCS301 hcs301;


void setup()
{
  Serial.begin(9600);

  // Брелки
  pinMode(HCS_RECIEVER_PIN, INPUT);
  attachInterrupt(0, HCS_interrupt, CHANGE);

  Serial.println("Setup OK");
}


void loop()
{
  long CurTime = millis();

  // проверяем наличие команды брелка
  if(HCS_Listening == false){
    
    HCS301 msg;
    memcpy(&msg,&hcs301,sizeof(HCS301));

    // включаем слушанье брелков снова
    HCS_Listening = true;

    Serial.println(String("KeyFb#")+String(msg.SerialNum));
  }

}
 
// Функции класса  HCS301 для чтения брелков
void HCS301::print(){
  String btn;

  if (BtnRing == 1) btn += "Ring";
  if (BtnClose == 1) btn += "Close";
  if (BtnOpen == 1) btn += "Open";
  if (BtnNoSound == 1) btn += "NoSound";

  String it2;
  it2 += "Encript ";
  it2 += Encript;
  it2 += " Serial ";
  it2 += SerialNum;
  it2 += " ";
  it2 += btn;
  it2 += " BattaryLow=";
  it2 += BattaryLow;
  it2 += " Rep=";
  it2 += Repeat;

  Serial.println(it2);

}

void HCS_interrupt(){

  if(HCS_Listening == false){
    return;
  }

  uint32_t cur_timestamp = micros();
  uint8_t  cur_status = digitalRead(HCS_RECIEVER_PIN);
  uint32_t pulse_duration = cur_timestamp - HCS_last_change;
  HCS_last_change     = cur_timestamp;

  // ловим преамбулу
  if(HCS_preamble_count < 12){
    if(cur_status == HIGH){
      if( ((pulse_duration > 150) && (pulse_duration < 500)) || HCS_preamble_count == 0){
        // начало импульса преамбулы
        //if(HCS_preamble_count == 0){
        //  HCS_start_preamble = cur_timestamp; // Отметим время старта преамбулы
        //}
      } else {
        // поймали какую то фигню, неправильная пауза между импульсами
        HCS_preamble_count = 0; // сбрасываем счетчик пойманных импульсов преамбулы
        goto exit; 

      }
    } else {
      // конец импульса преамбулы
      if((pulse_duration > 300) && (pulse_duration < 600)){
        // поймали импульс преамбулы
        HCS_preamble_count ++;
        if(HCS_preamble_count == 12){
          // словили преамбулу
          //HCS_Te = (cur_timestamp - HCS_start_preamble) / 23;  // вычисляем длительность базового импульса Te
          //HCS_Te2_3 = HCS_Te * 3 / 2;
          HCS_bit_counter = 0;
          goto exit; 
        }
      } else {
        // поймали какую то фигню
        HCS_preamble_count = 0; // сбрасываем счетчик пойманных импульсов преамбулы
        goto exit; 
      }
    }
  }
  

  // ловим данные
  if(HCS_preamble_count == 12){
    if(cur_status == HIGH){
      if(((pulse_duration > 250) && (pulse_duration < 900)) || HCS_bit_counter == 0){
        // начало импульса данных
      } else {
        // неправильная пауза между импульсами
        HCS_preamble_count = 0;
        goto exit; 
      }
    } else {
      // конец импульса данных
      if((pulse_duration > 250) && (pulse_duration < 900)){
        HCS_bit_array[65 - HCS_bit_counter] = (pulse_duration > HCS_Te2_3) ? 0 : 1; // импульс больше, чем половина от Те * 3 поймали 0, иначе 1
        HCS_bit_counter++;  
        if(HCS_bit_counter == 66){
          // поймали все биты данных
        
          HCS_Listening = false;  // отключем прослушку приемника, отправляем пойманные данные на обработку
          HCS_preamble_count = 0; // сбрасываем счетчик пойманных импульсов преамбулы

          hcs301.Repeat = HCS_bit_array[0];
          hcs301.BattaryLow = HCS_bit_array[1];
          hcs301.BtnNoSound = HCS_bit_array[2];
          hcs301.BtnOpen = HCS_bit_array[3];
          hcs301.BtnClose = HCS_bit_array[4];
          hcs301.BtnRing = HCS_bit_array[5];

          hcs301.SerialNum = 0;
          for(int i = 6; i < 34;i++){
            hcs301.SerialNum = (hcs301.SerialNum << 1) + HCS_bit_array[i];
          };

          uint32_t Encript = 0;
          for(int i = 34; i < 66;i++){
             Encript = (Encript << 1) + HCS_bit_array[i];
          };
          hcs301.Encript = Encript;
        }
      } else {
        // поймали хрень какую то, отключаемся
        HCS_preamble_count = 0;
        goto exit; 
      }
    }
  }
  
  exit:;

  //digitalWrite(LED_PIN,cur_status);
}
