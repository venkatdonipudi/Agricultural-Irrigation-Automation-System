#include "stm32g070xx.h"
#include<string.h>

//Function prototypes
void uart_init(void);
void uart_send_char(char c);
void uart_send_string(char *str);
char uart_receive_char(void);
void gpio_init(void);
void relay_on(void);
void relay_off(void);
void check_sms(void);

int main(void) {
    uart_init();      
    gpio_init();      

    //  AT commands to SIM800L
    uart_send_string("AT\r");            // Basic check
    uart_send_string("AT+CMGF=1\r");     // Set SMS mode to text

    while (1) {
        check_sms();  // Poll for incoming SMS commands to control relay
    }
}

// UART Initialization 
void uart_init(void) {
    RCC->IOPENR |= (1 << 0);           // Enable GPIOA clock
    RCC->APBENR1 |= (1 << 17);         // Enable USART2 clock

    // Configure PA2 (TX) and PA3 (RX) for UART2
    GPIOA->MODER &= ~(3 << (2 * 2));   // Clear PA2
    GPIOA->MODER |= (2 << (2 * 2));    // Set PA2 to alternate function
    GPIOA->AFR[0] |= (1 << (4 * 2));   // Set PA2 AF1 for USART2_TX

    GPIOA->MODER &= ~(3 << (2 * 3));   // Clear PA3
    GPIOA->MODER |= (2 << (2 * 3));    // Set PA3 to alternate function
    GPIOA->AFR[0] |= (1 << (4 * 3));   // Set PA3 AF1 for USART2_RX

    USART2->BRR = 0x0683;              // Baud rate = 9600, assuming 8MHz clock
    USART2->CR1 |= USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;  // Enable TX, RX, UART
}

// Send a single character over UART
void uart_send_char(char c) {
    while (!(USART2->ISR &(1<<7)));  // Wait until TX is empty
    USART2->TDR = c;
}

//  string over UART
void uart_send_string(char *str) {
    while (*str) {
        uart_send_char(*str++);
    }
}

// Receive a character over UART
char uart_receive_char(void) {
    while (!(USART2->ISR &(1<<5)));  // Wait until data is received
    return USART2->RDR;
}

// Initialize GPIO for relay control
void gpio_init(void) {
    RCC->IOPENR |= (1 << 0);           // Enable GPIOA clock
    GPIOA->MODER &= ~(3 << (2 * 0));   // Set PA0 as output for relay control
}

// Turn relay ON
void relay_on(void) {
    GPIOA->ODR |= (1 << 0);            // Set PA0 high
}

// Turn relay OFF
void relay_off(void) {
    GPIOA->ODR &= ~(1 << 0);           // Set PA0 low
}

// Check for incoming SMS to control the relay
void check_sms(void) {
    char buffer[100] = {0};            // Buffer to store incoming message
    char *cmd;

    // Request the first SMS in memory
    uart_send_string("AT+CMGR=1\r");   // Read the first message
    
    // Read the response into the buffer
    for (int i = 0; i < 100; i++) {
        buffer[i] = uart_receive_char();
        if (buffer[i] == '\n') break;  // Stop reading at line end
    }

    // Check if  message contains "ON" or "OFF"
    if ((cmd = strstr(buffer, "ON"))) {
        relay_on();   // Turn the relay ON
    } else if ((cmd = strstr(buffer, "OFF"))) {
        relay_off();  // Turn the relay OFF
    }
}
//uff finally i get output