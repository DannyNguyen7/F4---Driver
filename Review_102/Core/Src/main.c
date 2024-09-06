
#include <stdint.h>
#include "main.h"
#include <string.h>
unsigned int tick = 0;
uint32_t* SYST_CR = (uint32_t*) (0xe000e010);
uint32_t* SYST_RV = (uint32_t*) (0xe000e014);
char rx_buffer[128];
int rx_index = 0;
int rx_amount = 0;

#define GPIOD_BASE_ADDRESS 0x40020c00
#define GPIOA_BASE_ADDRESS 0x40020000
void delay_init(){
    // int systick
    *SYST_RV = 16000/8 - 1;
    *SYST_CR |= 1 | ( 1<< 1);
}

void delay(unsigned int time){
	tick = 0;
    while ((tick < time));
    *SYST_CR &= ~1;
}

void Systick_Handler(){
    tick++;
    *SYST_CR &= ~(1 << 16);
}

void Led_init(){
	__HAL_RCC_GPIOA_CLK_ENABLE();
	uint32_t* GPIOD_MODER = (uint32_t*) (GPIOD_BASE_ADDRESS + 0x00);
	uint32_t* GPIOD_OTYPER = (uint32_t*) (GPIOD_BASE_ADDRESS + 0x04);
	*GPIOD_MODER &= ~(0xff << 24);
	*GPIOD_MODER |= (0b01 << 24) | (0b01 << 26) | (0b01 << 28) | (0b01 << 30);
	*GPIOD_OTYPER &= ~(0xf << 12);
}

void Button_init(){
	__HAL_RCC_GPIOA_CLK_ENABLE();
	uint32_t* GPIOD_MODER = (uint32_t*) (GPIOA_BASE_ADDRESS + 0x00);
	uint32_t* GPIOD_PUPDR = (uint32_t*) (GPIOA_BASE_ADDRESS + 0x0c);
	*GPIOD_MODER &= ~(0b11); // Set PA0 in input mode
	*GPIOD_PUPDR &= ~(0b11);
}

typedef enum{
	ON, OFF
}Led_state_t;

void Led_control(int led_num, Led_state_t state){
	uint32_t* GPIOD_ODR = (uint32_t*) (GPIOD_BASE_ADDRESS + 0x14);
	if(state == ON)
		*GPIOD_ODR |= (1 << (12 + led_num));
	else
		*GPIOD_ODR &= ~(1 << (12 + led_num));
}

char Get_button(){
	uint32_t* GPIOA_IDR = (uint32_t*) (GPIOA_BASE_ADDRESS + 0x10);
	return *GPIOA_IDR & 0b00000001;
}
void EXTI0_init(){
    uint32_t* EXTICR1 = (uint32_t*)0x40013808;
	*EXTICR1 &= ~(0b1111 << 0);
	uint32_t* FTSR = (uint32_t*)0x40013c0c;
	*FTSR |= (1<<0);
	uint32_t* IMR = (uint32_t*)0x40013c00;
	*IMR |= (1 << 0);

	// xu ly trong bo nvic
	uint32_t* NVIC_ISER0 = (uint32_t*)0xe000e100;
	*NVIC_ISER0 |= (1 << 6);
}

void EXTI0_IRQHandler(){
  __asm("NOP");
  uint32_t* PR = (uint32_t*)0x40013c14;
  *PR |= (1 << 0);
}

void custom_Handler(){
	uint32_t* PR = (uint32_t*)0x40013c14;
	 *PR |= (1 << 0);
}

void USART2_IRQHandler(){
	uint32_t *DR = (uint32_t*) 0x40004404;
	rx_buffer[rx_index] = *DR;
	rx_index++;
	if(strstr(rx_buffer, "on")!= 0){
		Led_control(2, ON);
		memset(rx_buffer, 0, sizeof(rx_buffer));
		rx_index = 0;
	}
	else if(strstr(rx_buffer, "off")!= 0){
			Led_control(2, OFF);
			memset(rx_buffer, 0, sizeof(rx_buffer));
			rx_index = 0;
	}
}

void UART_init(){
	__HAL_RCC_GPIOA_CLK_ENABLE();
	uint32_t *GPIOA_MODER = (uint32_t*)0x40020000;
	*GPIOA_MODER &= ~(0xf << 4);
	*GPIOA_MODER |= (0x1010 << 4);

	uint32_t *AFRL = (uint32_t*)0x40020020;
	//*AFRL &= ~(0xff << 8);
	*AFRL |= (7 << 8) | (7 << 12);

	__HAL_RCC_USART2_CLK_ENABLE();
	uint32_t *BRR = (uint32_t*) 0x40004408;
	*BRR |= (104 << 4) | (3);
	uint32_t *CR1 = (uint32_t*)0x4000440c;
	*CR1 |= (1 <<3) | (1 << 2) | (1 << 13);

	uint32_t *CR3 = (uint32_t*) 0x40004414;
	*CR3 |= (1 << 6);
//	// Set bit 5 to enable RXNEIE: RXNE interrupt enable => Generate 1 interrupt to send NVIC
//	*CR1 |= (1<< 5);
//
//	// Set interrupt of NVIC
//	uint32_t *ISER1 = (uint32_t*)0xe000e104;
//	*ISER1 |= (1 << (38 - 32));

}

void DMA_init(){
	__HAL_RCC_DMA1_CLK_ENABLE();
	uint32_t *S5PAR = (uint32_t*) 0x40026090;
	*S5PAR = 0x40004404; // vào thanh ghi DR của UART2 copy dữ liệu;

	uint32_t *S5M0AR = (uint32_t*) 0x40026094; // Địa chỉ của memory address
	*S5M0AR = (uint32_t) rx_buffer;

	uint32_t *S5NDTR = (uint32_t*) 0x4002608c; //
	//*S5NDTR = sizeof(rx_buffer);
	*S5NDTR = 10; // nhận 10 byte th, 128 lâu quá

	uint32_t *S5CR = (uint32_t*) 0x40026088;
	*S5CR &= ~(0b1111 << 25);
	*S5CR |= (4 << 25) | (1 << 10)| (1 << 4) | 1; // Set channel 4 và Memmory increment mode và enable steam
	uint32_t *NVIC_ISER0 = (uint32_t*)0xe000e100;
	*NVIC_ISER0 |= (1 << 16);

}

void DMA1_Stream5_IRQHandler (){
	__asm ("NOP");
}

void timer (){

}


void UART_Send_Byte(char data){
	uint32_t *DR = (uint32_t*) 0x40004404;
	uint32_t *SR = (uint32_t*) 0x40004400;
	while(((*SR << 7) & 1) != 1);
	*DR = data;
	while (((*SR << 6) & 1) != 1);
}

void UART_Send_Multy_Bytes(char *arr, int size){
	for(int i = 0; i < size; i++){
		UART_Send_Byte(arr[i]);
	}
}
// Output is 1 Data 8 bit
char UART_Recv_Byte(){
	uint32_t *DR = (uint32_t*) 0x40004404;
	uint32_t *SR = (uint32_t*) 0x40004400;
	while(((*SR >> 5) & 1) != 1);
	char data = *DR; // Read the DR register and it's cleared register at the same time
	return data;
}

int main()
{
	delay_init();
	Led_init();
	Button_init();
//	EXTI0_init();
//	UART_init();
//	DMA_init();
//
//  char* flash = 0;
//  char* ram = (char*) 0x20000000;
//  for(int i = 0; i < 0x198; i++){
//	  ram[i] = flash[i];
//  }
//  uint32_t* VTOR = (uint32_t*) 0xe000ed08;
//  *VTOR = 0x20000000;
//
//  // register VTTB
//  uint32_t* temp =(uint32_t*) 0x20000058;
//  *temp =(uint32_t) custom_Handler | 1;
//
//  char msg[] = "Hello, ku";
//  uint32_t *S5NDTR = (uint32_t*) 0x4002608c;

  while (1)
  {
//	  // When to receive the Data, Data will save in the rx_buffer
//	  rx_buffer[rx_index]= UART_Recv_Byte();
//	  rx_index++;

	  Led_control(1, ON);
	  delay(1000);
	  Led_control(1, OFF);
	  delay(1000);
	//  rx_amount = *S5NDTR;

//	  if(Get_button() == 1)
//		  Led_control(2, ON);
//	  else
//		  Led_control(2, OFF);
  }
  return 0;
}
