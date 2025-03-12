// Interrupção ESP32

//ESP32 Technical reference manual (EN) : https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf
//GPIO API reference : https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/gpio.html

/* Referências:
 * https://github.com/anoochit/esp32-example/blob/master/2080_GPIO_Interrupt/main/gpio_intr.c
 * https://esp32.com/viewtopic.php?t=345
 */

// Biblioteca para GPIO ESP32
#include <driver/gpio.h>


#define input_gpio GPIO_NUM_18

char *tag = "teste";

// ISR
void isr_function(void *arg){
  Serial.println("Do something");
  uint32_t gpio_intr_status = READ_PERI_REG(GPIO_STATUS_REG);
  SET_PERI_REG_MASK(GPIO_STATUS_W1TC_REG, gpio_intr_status);
}

void setup() {

  
  Serial.begin(115200);

  // GPIOs podem ter funções diferentes de GPIO (ver tópico 4.10 do technical reference manual)
  // Definindo a função da GPIO que queremos usar como uma GPIO
  gpio_pad_select_gpio(input_gpio);

  // Selecionar nossa GPIO como OUTPUT ou INPUT ou OUTPUT
  // esp_err_t gpio_set_direction(gpio_num_t gpio_num, gpio_mode_t mode)
  /*
   * gpio_num = GPIO_NUM_X, sendo X = número da sua GPIO 
   * mode = GPIO_MODE_Y, sendo Y = modo da sua GPIO --> DISABLE, INPUT, OUTPUT, OUTPUT_OD (open-drain mode), INPUT_OUTPUT_OD (open-drain mode) e INPUT_OUTPUT
   */
  gpio_set_direction(input_gpio, GPIO_MODE_INPUT);

  // Configurar a GPIO como pull up ou pull down
  // esp_err_t gpio_set_pull_mode(gpio_num_t gpio_num, gpio_pull_mode_t pull)  
  /*
   * gpio_num = GPIO_NUM_X, sendo X = número da sua GPIO
   * pull = GPIO_Z, sendo Z = formas de pull --> PULLUP_ONLY, PULLDOWN_ONLY, PULLUP_UP_PULLDOWN e FLOATING
   */
   
  gpio_set_pull_mode(input_gpio, GPIO_PULLUP_ONLY);

  // Selecionar o tipo de disparador (trigger)
  // esp_err_t gpio_set_intr_type(gpio_num_t gpio_num, gpio_int_type_t intr_type)
  /* 
   *  gpio_num = GPIO_NUM_X, sendo X = número da sua GPIO
   *  intr_type = GPIO_INTR_W, sendo W = forma de interrupção --> DISABLE, POSEDGE (borda de subida), NEGEDGE (borda de descida), ANYEDGE (ambas as bordas),
   *  LOW_LEVEL (dispara com entrada baixa), HIGH_LEVEL (dispara com entrada alta) e MAX.
   */
  gpio_set_intr_type(input_gpio, GPIO_INTR_POSEDGE);

  // Habilitar o módulo de interrupção do sinal da GPIO
  // esp_err_t gpio_intr_enable(gpio_num_t gpio_num)
  /*
   * pio_num = GPIO_NUM_X, sendo X = número da sua GPIO
   */
   /* NOTAS:
    * Não usar interrupção na GPIO36 e 39 quando estiver usando ADC ou WiFi com sleep mode ativo
    */
  gpio_intr_enable(input_gpio);

  // Registrar a Rotina de serviço de interrupção (ISR)
  // esp_err_t gpio_isr_register(void (*fn)(void *), void *arg, int intr_alloc_flags, gpio_isr_handle_t *handle)
  /*
   * fn = Função para a ISR
   * arg = Parametro para passar para a ISR
   * intr_alloc_flags = ESP_INTR_FLAG_Q, sendo Q = (https://github.com/pycom/esp-idf-2.0/blob/master/components/esp32/include/esp_intr_alloc.h) 
   * handle = ponteiro para retornar a ISR
   */
  gpio_isr_register(isr_function, tag, ESP_INTR_FLAG_LEVEL1, NULL);

}


void loop() { 

  

}
