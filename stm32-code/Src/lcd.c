#include <stdint.h>
#include "main.h"
#include "lcd.h"
#include "ports.h"

//Caracter definido por usuario para cargar en la memoria CGRAM del LCD
const int8_t UserFont[8][8] =
{
		{ 0x11, 0x0A, 0x04, 0x1B, 0x11, 0x11, 0x11, 0x0E },
		{ 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10 },
		{ 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18 },
		{ 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C },
		{ 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E },
		{ 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F },
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
};

static void TIM1_Delay_Init(void)
{
    RCC->APB2ENR |= (0x1UL << 11U); // RCC_APB2ENR_TIM1EN
    TIM1->PSC     = 63U;            // 64 MHz / 64 = 1 MHz -> 1 us tick
    TIM1->CR1    &= ~(0x1UL << 0U); // Ensure timer is disabled (CEN=0)
    TIM1->SR      = 0U;             // Clear flags
}

static void delay_us(uint16_t us)
{
    TIM1->ARR = (uint16_t)(us - 1U);
    TIM1->CNT = 0U;
    TIM1->SR  = 0U;
    TIM1->CR1 |= (0x1UL << 0U);     // Enable timer
    while (!(TIM1->SR & (0x1UL << 0U))); // Wait for update interrupt flag (UIF)
    TIM1->CR1 &= ~(0x1UL << 0U);    // Disable timer
    TIM1->SR   = 0U;                // Clear flag
}

static void delay_ms_lcd(uint32_t ms)
{
    for (uint32_t i = 0; i < ms; i++)
        delay_us(1000U);
}

//Funcion que inicializa el LCD a 4 bits
void LCD_Init(void){
	int8_t const *p;

	TIM1_Delay_Init(); // Initialize delay timer

/**
  * Configuracion de todos los pines hacia el LCD general purpose output push-pull, 10 MHz speed
  */
	RCC->APB2ENR	|=	 ( 0x1UL <<  4U );//			IO port C clock enable	
	GPIOC->CRL		&=	~( 0x3UL << 30U ) & ~( 0x2UL << 28U )
					& 	~( 0x3UL << 26U ) & ~( 0x2UL << 24U );
	GPIOC->CRL 		|= 	 ( 0x1UL << 28U )
					|  	 ( 0x1UL << 24U );
	GPIOC->CRH		&=	~( 0x3UL << 18U ) & ~( 0x2UL << 16U )
					& 	~( 0x3UL << 14U ) & ~( 0x2UL << 12U )
					&	~( 0x3UL << 10U ) & ~( 0x2UL <<  8U )
					& 	~( 0x3UL <<  6U ) & ~( 0x2UL <<  4U )
					& 	~( 0x3UL <<  2U ) & ~( 0x2UL <<  0U );
	GPIOC->CRH		|= 	 ( 0x1UL << 16U )
					|  	 ( 0x1UL << 12U )
					| 	 ( 0x1UL <<  8U )
					|  	 ( 0x1UL <<  4U )
					|  	 ( 0x1UL <<  0U );
/**
  * Inicialización del LCD
  * https://web.alfredstate.edu/faculty/weimandn/lcd/lcd_initialization/lcd_initialization_index.html
  * Power ON
  */
	GPIOC->BSRR	 =	 LCD_RS_PIN_LOW;
	GPIOC->BSRR	 =	 LCD_RW_PIN_LOW;
	GPIOC->BSRR	 =	 LCD_EN_PIN_LOW;
	GPIOC->BSRR	 =	 LCD_D4_PIN_LOW;
	GPIOC->BSRR	 =	 LCD_D5_PIN_LOW;
	GPIOC->BSRR	 =	 LCD_D6_PIN_LOW;
	GPIOC->BSRR	 =	 LCD_D7_PIN_LOW;
	delay_ms_lcd(41U);// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! wait > 40ms
	/* Special case of 'Function Set' 				*/
	GPIOC->BSRR	 =	 LCD_D4_PIN_HIGH;
	GPIOC->BSRR	 =	 LCD_D5_PIN_HIGH;
	GPIOC->BSRR	 =	 LCD_D6_PIN_LOW;
	GPIOC->BSRR	 =	 LCD_D7_PIN_LOW;
	LCD_Pulse_EN( );
	delay_ms_lcd(5U);// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! wait > 4.1ms
	/* Special case of 'Function Set' 				*/
	GPIOC->BSRR	 =	 LCD_D4_PIN_HIGH;
	GPIOC->BSRR	 =	 LCD_D5_PIN_HIGH;
	GPIOC->BSRR	 =	 LCD_D6_PIN_LOW;
	GPIOC->BSRR	 =	 LCD_D7_PIN_LOW;
	LCD_Pulse_EN( );
	delay_us(53U);// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!	wait > 53us
	/* Special case of 'Function Set' 				*/
	GPIOC->BSRR	 =	 LCD_D4_PIN_HIGH;
	GPIOC->BSRR	 =	 LCD_D5_PIN_HIGH;
	GPIOC->BSRR	 =	 LCD_D6_PIN_LOW;
	GPIOC->BSRR	 =	 LCD_D7_PIN_LOW;
	LCD_Pulse_EN( );
	delay_us(100U);//						wait instead of busy check (still in 8-bit mode)
	/* Initial 'Function Set' to change 4-bit mode 			*/
	GPIOC->BSRR	 =	 LCD_D4_PIN_LOW;
	GPIOC->BSRR	 =	 LCD_D5_PIN_HIGH;
	GPIOC->BSRR	 =	 LCD_D6_PIN_LOW;
	GPIOC->BSRR	 =	 LCD_D7_PIN_LOW;
	LCD_Pulse_EN( );
	delay_ms_lcd(2U);//						wait instead of busy check (4-bit mode not yet stable)
	/* 'Function Set' (I=1, N and F as required)			*/
	LCD_Write_Cmd( 0x28U );//					2-line display, 5x7 dot
	/* 'Display ON/OFF Control' (D=0, C=0, B=0)			*/
	LCD_Write_Cmd( 0x08U );//					display, cursor and blinking off
	/* 'Clear Display'						*/
	LCD_Write_Cmd( 0x01U );//
	/* 'Entry Mode Set' (I/D and S as required)			*/
	LCD_Write_Cmd( 0x06U );//					cursor increment by 1, shift off
	/* Initialization Ends						*/
	LCD_Write_Cmd( 0x0FU );//					display, cursor and blinking on

	//Cargamos el caracter definido por el usuario en la CGRAM
	LCD_Write_Cmd( 0x40 );//					establece la direccion CGRAM desde 0
	p = &UserFont[0][0];

	for( int i = 0; i < sizeof( UserFont ); i++, p++ )
		LCD_Put_Char( *p );

	/*	Set DDRAM address in address			*/
	LCD_Write_Cmd( 0x80 );//
}

//Funcion que genera un strobe en el LCD
void LCD_Out_Data4(uint8_t val){
	if( ( val & 0x01U ) == 0x01U )//				Bit[0]
		GPIOC->BSRR	=	LCD_D4_PIN_HIGH;
	else
		GPIOC->BSRR	=	LCD_D4_PIN_LOW;

	if( ( val & 0x02U ) == 0x02U )//				Bit[1]
		GPIOC->BSRR	=	LCD_D5_PIN_HIGH;
	else
		GPIOC->BSRR	=	LCD_D5_PIN_LOW;

	if( ( val & 0x04U ) == 0x04U )//				Bit[2]
		GPIOC->BSRR	=	LCD_D6_PIN_HIGH;
	else
		GPIOC->BSRR	=	LCD_D6_PIN_LOW;

	if( ( val & 0x08U ) == 0x08U )//				Bit[3]
		GPIOC->BSRR	=	LCD_D7_PIN_HIGH;
	else
		GPIOC->BSRR	=	LCD_D7_PIN_LOW;
}

//Funcion que escribe 1 byte de datos en el LCD
void LCD_Write_Byte(uint8_t val){
	LCD_Out_Data4( ( val >> 4 ) & 0x0FU );
	LCD_Pulse_EN( );
	LCD_Out_Data4( val & 0x0FU );
	LCD_Pulse_EN( );
	delay_ms_lcd(2U);
}

//Funcion que escribe un comando en el LCD
void LCD_Write_Cmd(uint8_t val){
	GPIOC->BSRR	=	LCD_RS_PIN_LOW;//			RS=0 (seleccion de comando)
	LCD_Write_Byte( val );
}

//Escribe un caracter ASCII en el LCD
void LCD_Put_Char(uint8_t c){
	GPIOC->BSRR	=	LCD_RS_PIN_HIGH;//			RS=1 (seleccion de caracteres)
	LCD_Write_Byte( c );
}

//Funcion que establece el cursor en una posicion de la pantalla del LCD
//Minimum values for line and column must be 1
void LCD_Set_Cursor(uint8_t line, uint8_t column){
	uint8_t address;
	column--;
	line--;
	address = ( line * 0x40U ) + column;
	address = 0x80U + ( address & 0x7FU );
	LCD_Write_Cmd( address );
}

//Funcion que envia una cadena de caracteres ASCII al LCD
void LCD_Put_Str(char * str){
	for( int16_t i = 0; i < 16 && str[ i ] != 0; i++ )
		LCD_Put_Char( str[ i ] );//				envia 1 byte al LCD
}

//Funcion que envia un caracter numerico al LCD
//El número debe ser entero y de 5 dígitos máximo
void LCD_Put_Num(int16_t num){
	int16_t p;
	int16_t f = 0;
	int8_t ch[ 5 ];

	for( int16_t i = 0; i < 5; i++ ){
		p = 1;
		for( int16_t j = 4 - i; j > 0; j-- )
			p = p * 10;
		ch[ i ] = ( num / p );
		if( num >= p && !f )
			f = 1;
		num = num - ch[ i ] * p;
		ch[ i ] = ch[ i ] + 48;
		if( f )
			LCD_Put_Char( ch[ i ] );
	}
}

//Funcion que provoca tiempos de espera en el LCD
char LCD_Busy(void){
	char busy;
/**
  * Configuracion de D7 as input floating
  */
	GPIOC->CRH	&=	~( 0x2UL << 18U ) & ~( 0x3UL << 16U );
	GPIOC->CRH	|=   ( 0x1UL << 18U );
	GPIOC->BSRR	 =	 LCD_RS_PIN_LOW;
	GPIOC->BSRR	 =	 LCD_RW_PIN_HIGH;
	/* Read high nibble (contains busy flag on D7) */
	GPIOC->BSRR	 =	 LCD_EN_PIN_HIGH;
	delay_us(100U);
	busy = ( GPIOC->IDR & LCD_D7_PIN_HIGH ) ? 1 : 0;
	GPIOC->BSRR	 =	 LCD_EN_PIN_LOW;
	delay_us(10U);
	/* Read low nibble (dummy, required to keep 4-bit state machine in sync) */
	GPIOC->BSRR	 =	 LCD_EN_PIN_HIGH;
	delay_us(100U);
	GPIOC->BSRR	 =	 LCD_EN_PIN_LOW;
	GPIOC->BSRR	 =	 LCD_RW_PIN_LOW;
/**
  * Configuracion de D7 as output push-pull, 10 MHz speed
  */
	GPIOC->CRH	&=	~( 0x3UL << 18U ) & ~( 0x2UL << 16U );
	GPIOC->CRH	|=   ( 0x1UL << 16U );
	return busy;
}

//Funcion que genera un pulso en el pin EN del LCD
void LCD_Pulse_EN(void){
	GPIOC->BSRR	=	LCD_EN_PIN_LOW;//
	delay_us(10U);// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! wait for	10us
	GPIOC->BSRR	=	LCD_EN_PIN_HIGH;//			habilita pin EN ON
	delay_us(10U);// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! wait for	10us
	GPIOC->BSRR	=	LCD_EN_PIN_LOW;//			habilita pin EN OFF
	delay_ms_lcd(1U);// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! wait for	1ms
}

/*
 * Funcion que muestra un caracter grafico en el LCD
 * en 'value' el valor de su posicion en CGRAM y
 * en 'size' especificamos su tamaño
 */
void LCD_BarGraphic(int16_t value, int16_t size){
	value = value * size / 20;//					matriz de 5x8 pixeles
	for( int16_t i = 0; i < size; i++ ){
		if( value > 5 ){
			LCD_Put_Char( 0x05U );
			value -= 5;
		} else {
			LCD_Put_Char( value );
			break;
		}
	}
}

/*
 * Funcion que muestra un caracter grafico en el LCD especificando 
 * la posicion pos_x horizontal de inicio y la posicion pos_y vertical de la pantalla LCD
 */
void LCD_BarGraphicXY(int16_t pos_x, int16_t pos_y, int16_t value){
	LCD_Set_Cursor( pos_x, pos_y );
	for( int16_t i = 0; i < 16; i++ ){
		if( value > 5 ){
			LCD_Put_Char( 0x05U );
			value -= 5;
		} else {
			LCD_Put_Char( value );
			while( i++ < 16 )
				LCD_Put_Char( 0 );
		}
	}
}