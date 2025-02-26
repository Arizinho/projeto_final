#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/i2c.h"
#include <stdlib.h>
#include "inc/ssd1306.h"
#include "inc/font.h"
#include "hardware/timer.h"

//macro dos pinos utilizados
#define BUTTON_A 5
#define BUTTON_B 6
#define BUTTON_ANALOG 22
#define SERVO_MOTOR 13
#define PH_SENSOR 27

//macro comunicação i2c display oled
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C

volatile bool controle_botao = 0; //flag para controle de botão
volatile uint32_t last_interrupt_time = 0; //variável para contagem de tempo
uint8_t t_abertura_seg; //tempo em segundos de abertura do servo motor

ssd1306_t ssd; // Inicializa a estrutura do display

//inicialização pino do sensor de pH utilizando ADC
void sensor_init(){
    adc_init();
    adc_gpio_init(PH_SENSOR);
    adc_select_input(1);
}

//função para ler o valor do sensor de pH
float sensor_read_value(){
    return (7 - (1.65 - adc_read()*3.3/4095.0) * 4.2424);
}

//função para configurar os botões
void button_init(){
    //configura o botão A
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);

    //configura o botão B
    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);

    //configura o botão do analógico
    gpio_init(BUTTON_ANALOG);
    gpio_set_dir(BUTTON_ANALOG, GPIO_IN);
    gpio_pull_up(BUTTON_ANALOG);
}

//função para inicializar comunicação I2C com display oled
void i2c_oled_init(){
    // I2C Initialisation. Using it at 400Khz.
    i2c_init(I2C_PORT, 400 * 1000);

    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
    gpio_pull_up(I2C_SDA); // Pull up the data line
    gpio_pull_up(I2C_SCL); // Pull up the clock line
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
    ssd1306_config(&ssd); // Configura o display
    ssd1306_send_data(&ssd); // Envia os dados para o display

    // Limpa o display. O display inicia com todos os pixels apagados.
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);
}

//função para configurar o PWM do servo motor
void configura_pwm(void){

    gpio_set_function(SERVO_MOTOR, GPIO_FUNC_PWM); //habilitar o pino GPIO como PWM

    uint slice = pwm_gpio_to_slice_num(SERVO_MOTOR); //obter o canal PWM da GPIO

    pwm_set_clkdiv(slice, 125.0); //define o divisor de clock do PWM 

    pwm_set_wrap(slice, 19999); //definir o valor de wrap (vai contar até 20000 ciclos -> 19999 + 1)

    pwm_set_gpio_level(SERVO_MOTOR, 500); //defini o duty cycle para 2,5% (0 graus)

    pwm_set_enabled(slice, true); //habilita o pwm no slice correspondente

}

//função para desativar o servo motor após o tempo definido
int64_t turn_off_callback(alarm_id_t id, void *user_data) {
    pwm_set_gpio_level(SERVO_MOTOR, 500); //fecha servo (0 graus)
    return 0;
}

//função para o temporizar ciclo de alimentação
bool repeating_timer_callback(struct repeating_timer *t){
    last_interrupt_time = to_ms_since_boot(get_absolute_time()); //armazena o tempo atual para cronometragem
    pwm_set_gpio_level(SERVO_MOTOR, 2400); //abre o servo motor (despeja alimento)
    add_alarm_in_ms(t_abertura_seg * 1000, turn_off_callback, NULL, false); //inicia alarme para fechar servo (para alimentação)
    return true;
}

//função para configurar o intervalo de alimentação (em dias e horas)
uint32_t configura_intervalo(void){
    uint32_t last_time = 0, dias_horas_em_ms;
    uint8_t numero_display = 0, dias, horas;

    //mostra configuração de dias no display
    ssd1306_fill(&ssd, false); // Limpa o display
    ssd1306_draw_string(&ssd, "Ciclo de alimenta\1\2o:", 0, 0);
    ssd1306_draw_char(&ssd, 48+0, 40, 24);
    ssd1306_draw_string(&ssd, " dias", 48, 24);
    ssd1306_draw_char(&ssd, 48+0, 32, 40);
    ssd1306_draw_char(&ssd, 48+0, 40, 40);
    ssd1306_draw_string(&ssd, " horas", 48, 40);
    ssd1306_send_data(&ssd); // Atualiza o display

    //configura número de dias (espera botão do analógico ser pressionado para confirmar)
    while (!controle_botao){
        ssd1306_inv_draw_char(&ssd, 48 + numero_display, 40, 24);
        ssd1306_send_data(&ssd);

        //incrementa número de dias se botão A for pressionado + debounce de 200 ms
        if (!gpio_get(BUTTON_A) && (to_ms_since_boot(get_absolute_time())-last_time > 200)){
            last_time = to_ms_since_boot(get_absolute_time());
            numero_display = (numero_display - 1 + 10)%10;
        }

        //dencrementa número de dias se botão B for pressionado + debounce de 200 ms
        else if(!gpio_get(BUTTON_B) && (to_ms_since_boot(get_absolute_time())-last_time > 200)){
            last_time = to_ms_since_boot(get_absolute_time());
            numero_display = (numero_display + 1)%10;
        }
    }
    //exibe número final de dias
    ssd1306_draw_char(&ssd, 48+numero_display, 40, 24);
    //salva valor de dias
    dias = numero_display;

    //reseta variáveis de controle
    numero_display = 0;
    controle_botao = 0;

    //configura dezena do número de horas (espera botão do analógico ser pressionado para confirmar)
    while (!controle_botao){
        ssd1306_inv_draw_char(&ssd, 48 + numero_display, 32, 40);
        ssd1306_send_data(&ssd);

        //incrementa dezena do número de horas se botão A for pressionado + debounce de 200 ms
        if (!gpio_get(BUTTON_A) && (to_ms_since_boot(get_absolute_time())-last_time > 200)){
            last_time = to_ms_since_boot(get_absolute_time());
            numero_display = (numero_display - 1 + 3)%3;
        }
        //decrementa dezena do número de horas se botão B for pressionado + debounce de 200 ms
        else if(!gpio_get(BUTTON_B) && (to_ms_since_boot(get_absolute_time())-last_time > 200)){
            last_time = to_ms_since_boot(get_absolute_time());
            numero_display = (numero_display + 1)%3;
        }
    }
    //exibe número final de dezenas de horas
    ssd1306_draw_char(&ssd, 48 + numero_display, 32, 40);
    //salva dezena de horas
    horas = 10*numero_display;

    //reseta variáveis de controle
    numero_display = 0;
    controle_botao = 0;

    //lógica para impedir que número de dias + horas seja nulo
    if(dias + horas == 0){
        numero_display = 1;
    }

    //configura unidades de horas
    switch (horas < 20)
    {
    //para valores menores que 20:
    case true:
        while (!controle_botao){
            ssd1306_inv_draw_char(&ssd, 48 + numero_display, 40, 40);
            ssd1306_send_data(&ssd);
            //incrementa unidade do número de horas se botão A for pressionado + debounce de 200 ms
            if (!gpio_get(BUTTON_A) && (to_ms_since_boot(get_absolute_time())-last_time > 200)){
                last_time = to_ms_since_boot(get_absolute_time());
                numero_display = (numero_display - 1 + 10)%10;
                if((dias + horas == 0) && numero_display == 0){
                    numero_display = 9;
                }
            }
            //decrementa dezena do número de horas se botão B for pressionado + debounce de 200 ms
            else if(!gpio_get(BUTTON_B) && (to_ms_since_boot(get_absolute_time())-last_time > 200)){
                last_time = to_ms_since_boot(get_absolute_time());
                numero_display = (numero_display + 1)%10;
                if((dias + horas == 0) && numero_display == 0){
                    numero_display = 1;
                }
            }
        }
        break;
    
    //para valores maiores ou igual a 20:
    case false:
        while (!controle_botao){
            ssd1306_inv_draw_char(&ssd, 48 + numero_display, 40, 40);
            ssd1306_send_data(&ssd);
            //incrementa unidade do número de horas se botão A for pressionado + debounce de 200 ms
            if (!gpio_get(BUTTON_A) && (to_ms_since_boot(get_absolute_time())-last_time > 200)){
                last_time = to_ms_since_boot(get_absolute_time());
                numero_display = (numero_display - 1 + 4)%4;
            }
            //decrementa unidade do número de horas se botão A for pressionado + debounce de 200 ms
            else if(!gpio_get(BUTTON_B) && (to_ms_since_boot(get_absolute_time())-last_time > 200)){
                last_time = to_ms_since_boot(get_absolute_time());
                numero_display = (numero_display + 1)%4;
            }
        }
        break;
    }
    //reseta variavel de controle
    controle_botao = 0;
    //salva valor final de horas
    horas += numero_display;
    //calcula tempo total em ms
    dias_horas_em_ms = (24*dias + horas)*60*60*1000;

    //retorna valor configurado em ms
    return dias_horas_em_ms;
}

//função para configurar o tempo de abertura do servo motor
void configura_abertura(){
    uint32_t last_time = 0;
    uint8_t numero_display = 0;

    //lógica para capturar o tempo de abertura usando os botões (similar à função configura_intervalo())
    ssd1306_fill(&ssd, false); // Limpa o display
    ssd1306_draw_string(&ssd, "Tempo de abertura:", 0, 0);
    ssd1306_draw_char(&ssd, 48+0, 26, 24);
    ssd1306_draw_char(&ssd, 48+0, 36, 24);
    ssd1306_draw_string(&ssd, " segundos", 46, 24);

    //captura dezena de segundos
    while (!controle_botao){
        ssd1306_inv_draw_char(&ssd, 48 + numero_display, 26, 24);
        ssd1306_send_data(&ssd);
        if (!gpio_get(BUTTON_A) && (to_ms_since_boot(get_absolute_time())-last_time > 200)){
            last_time = to_ms_since_boot(get_absolute_time());
            numero_display = (numero_display - 1 + 10)%10;
        }
        else if(!gpio_get(BUTTON_B) && (to_ms_since_boot(get_absolute_time())-last_time > 200)){
            last_time = to_ms_since_boot(get_absolute_time());
            numero_display = (numero_display + 1)%10;
        }
    }
    ssd1306_draw_char(&ssd, 48 + numero_display, 26, 24);
    controle_botao = 0;
    t_abertura_seg = 10*numero_display;
    numero_display = 0;
    
    //impede que tempo em segundos seja nulo
    if(t_abertura_seg == 0){
        numero_display = 1;
    }

    //captura unidades de segundos
    while (!controle_botao){
        ssd1306_inv_draw_char(&ssd, 48 + numero_display, 36, 24);
        ssd1306_send_data(&ssd);
        if (!gpio_get(BUTTON_A) && (to_ms_since_boot(get_absolute_time())-last_time > 200)){
            last_time = to_ms_since_boot(get_absolute_time());
            numero_display = (numero_display - 1 + 10)%10;
            if((t_abertura_seg == 0) && numero_display == 0){
                numero_display = 9;
            }
        }
        else if(!gpio_get(BUTTON_B) && (to_ms_since_boot(get_absolute_time())-last_time > 200)){
            last_time = to_ms_since_boot(get_absolute_time());
            numero_display = (numero_display + 1)%10;
            if((t_abertura_seg == 0) && numero_display == 0){
                numero_display = 1;
            }
        }
    }
    controle_botao = 0;
    t_abertura_seg += numero_display;
}

//função para tratar interrupção no botão do analógico
void gpio_irq_handler(uint gpio, uint32_t events){
    //variável para guardar tempo da ocorrência da última interrupção
    static uint32_t last_time = 0;

    //debounce por delay de 200ms
    if(to_ms_since_boot(get_absolute_time())-last_time > 200){
        //atualiza last_time com tempo atual
        last_time = to_ms_since_boot(get_absolute_time());
        //ativa flag de controle do botão
        controle_botao = 1;
    }
}

int main(){
    uint32_t intervalo_alarme, remain_time;
    uint8_t dias, horas, min, seg;
    char leitura_str[5], tempo_str[17];

    //inicializa periféricos
    stdio_init_all();
    sensor_init();
    i2c_oled_init();
    button_init();
    configura_pwm();

    //habilita interrupção por botão do analógico
    gpio_set_irq_enabled_with_callback(BUTTON_ANALOG, GPIO_IRQ_EDGE_FALL, 1, gpio_irq_handler);

    //configura intervalo de alimentação dos peixes
    intervalo_alarme = configura_intervalo();
    //configura o tempo de abertura do alimentador
    configura_abertura();

    //salva tempo atual para realizar cronometragem para alimentação
    last_interrupt_time = to_ms_since_boot(get_absolute_time());

    //habilita timer para realizar a alimentação periódica
    struct repeating_timer timer;
    add_repeating_timer_ms(intervalo_alarme, repeating_timer_callback, NULL, &timer);

    while (true) {
        //clcula dias, horas, minutos e segundos restantes para alimentação
        remain_time = (intervalo_alarme - (to_ms_since_boot(get_absolute_time()) - last_interrupt_time));
        dias = remain_time/(24*60*60*1000);
        remain_time -= dias*(24*60*60*1000);
        horas = remain_time/(60*60*1000);
        remain_time -= horas*(60*60*1000);
        min = remain_time/(60*1000);
        remain_time -= min*(60*1000);
        seg = remain_time/(1000);
        //formata a string para exibir no display
        sprintf(tempo_str, "%i dias %02i:%02i:%02i\n", dias, horas, min, seg);
        
        //exibe tempo no display OLED
        ssd1306_fill(&ssd, false); // Limpa o display
        ssd1306_draw_string(&ssd, "Pr\4x.", 0, 0);
        ssd1306_draw_string(&ssd, "al", 42, 0);
        ssd1306_draw_string(&ssd, "imenta\1\2o", 54, 0);
        ssd1306_draw_string(&ssd, tempo_str, 0, 10);

        //lê o valor do pH e formata
        sprintf(leitura_str, "%.2f", sensor_read_value()); 
        //exipe valor de pH no display
        ssd1306_draw_string(&ssd, "pH da \3gua: ", 1, 30);
        ssd1306_draw_string(&ssd, leitura_str, 87, 30);
        ssd1306_send_data(&ssd);

        //reconfigura timers se botão do analógico for pressionado
        if(controle_botao){
            controle_botao = 0;
            cancel_repeating_timer(&timer);
            intervalo_alarme = configura_intervalo();
            configura_abertura();
            last_interrupt_time = to_ms_since_boot(get_absolute_time());
            add_repeating_timer_ms(intervalo_alarme, repeating_timer_callback, NULL, &timer);
        }
        sleep_ms(200);
    }
}
