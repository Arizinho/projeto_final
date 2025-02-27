# Sistema de Alimentação Automática para Peixes

Este projeto implementa um sistema de alimentação automática para peixes utilizando um sensor de pH, um servo motor para liberar a comida, um display OLED para exibir informações e botões para configuração. O sistema é baseado na Raspberry Pi Pico e permite configurar o intervalo de alimentação e o tempo de abertura do servo motor.

---

## **Configuração dos Pinos GPIO**

Os pinos GPIO da Raspberry Pi Pico estão configurados conforme a tabela abaixo:

| GPIO   | Componente       | Função                                                                           |
| ------ | ---------------- | -------------------------------------------------------------------------------- |
| **5**  | Botão A          | Entrada digital para incrementar valores de configuração                         |
| **6**  | Botão B          | Entrada digital para decrementar valores de configuração                         |
| **13** | Servo Motor      | Saída PWM para controlar a abertura do servo motor                               |
| **14** | I2C SDA          | Linha de dados para comunicação I2C com o display OLED                           |
| **15** | I2C SCL          | Linha de clock para comunicação I2C com o display OLED                           |
| **22** | Botão Analógico  | Entrada digital para confirmar valores de configuração e reconfigurar timers     |
| **27** | Sensor de pH     | Entrada analógica para leitura do valor de pH da água                            |


---

## **Funcionamento do Código**

1. **Inicialização:**

   - Configura os pinos GPIO para os botões, servo motor e sensor de pH.
   - Inicializa a comunicação I2C para o display OLED.
   - Configura o PWM para controle do servo motor.
   - Configura interrupções no botão analógico para confirmar valores de configuração.

2. **Configuração do Intervalo de Alimentação:**

   - O usuário pode configurar o intervalo de alimentação em dias e horas usando os botões A e B.
   - O botão analógico é usado para confirmar os valores selecionados.

3. **Configuração do Tempo de Abertura do Servo Motor:**

   - O usuário pode configurar o tempo de abertura do servo motor em segundos usando os botões A e B.
   - O botão analógico é usado para confirmar os valores selecionados.

4. **Controle do Servo Motor:**

   - O sistema abre o servo motor para liberar a comida no intervalo de tempo configurado.
   - Após o tempo de abertura configurado, o servo motor é fechado automaticamente.

5. **Leitura do Sensor de pH:**

   - O sistema realiza a leitura analógica do sensor de pH.
   - O valor de leitura analógica é convertido para um valor de pH.
     
6. **Exibição no Display OLED:**

   - O display OLED exibe o tempo restante para a próxima alimentação e o valor do pH da água.
   - O usuário pode reconfigurar os intervalos de alimentação e o tempo de abertura do servo motor pressionando o botão analógico.

7. **Tratamento de Interrupção:**

   - O botão analógico é configurado para gerar interrupção ao ser pressionado.
   - Um debounce de 200 ms é aplicado para evitar leituras erradas.

---

## **Como Usar**

1. **Configuração Inicial:**
   - Conecte os componentes conforme a tabela de pinos GPIO.
   - Compile e carregue o código na Raspberry Pi Pico.

2. **Configuração do Intervalo de Alimentação:**
   - Use os botões A e B para ajustar o número de dias e horas.
   - Pressione o botão analógico para confirmar.

3. **Configuração do Tempo de Abertura do Servo Motor:**
   - Use os botões A e B para ajustar o tempo de abertura em segundos.
   - Pressione o botão analógico para confirmar.

4. **Monitoramento:**
   - O display OLED exibirá o tempo restante para a próxima alimentação e o valor do pH da água.
   - Para reconfigurar, pressione o botão analógico e siga os passos 2 e 3.

---

## **Link do Vídeo**

- Youtube: https://www.youtube.com/watch?v=aO68k0g_5qM
