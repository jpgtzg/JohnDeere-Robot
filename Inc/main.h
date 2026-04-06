#ifndef MAIN_H_
#define MAIN_H_

/* Flash memory interface registers */
typedef struct{
	volatile uint32_t ACR;
	volatile uint32_t KEYR;
	volatile uint32_t OPTKEYR;
	volatile uint32_t SR;
	volatile uint32_t CR;
	volatile uint32_t AR;
	volatile uint32_t reserved;
	volatile uint32_t OBR;
	volatile uint32_t WRPR;
} FLASH_TypeDef;

/* Reset and Clock Control registers */
typedef struct
{
	volatile uint32_t CR;
	volatile uint32_t CFGR;
	volatile uint32_t CIR;
	volatile uint32_t APB2RSTR;
	volatile uint32_t APB1RSTR;
	volatile uint32_t AHBENR;
	volatile uint32_t APB2ENR;
	volatile uint32_t APB1ENR;
	volatile uint32_t BDCR;
	volatile uint32_t CSR;
} RCC_TypeDef;

/* General Purpose I/O registers */
typedef struct
{
	volatile uint32_t CRL;
	volatile uint32_t CRH;
	volatile uint32_t IDR;
	volatile uint32_t ODR;
	volatile uint32_t BSRR;
	volatile uint32_t BRR;
	volatile uint32_t LCKR;
} GPIO_TypeDef;

/* Alternate Function I/O registers */
typedef struct
{
    volatile uint32_t EVCR;
    volatile uint32_t MAPR;
    volatile uint32_t EXTICR[4];
    volatile uint32_t reserved;
    volatile uint32_t MAPR2;
} AFIO_TypeDef;

typedef struct{
	volatile uint32_t SR;
	volatile uint32_t DR;
	volatile uint32_t BRR;
	volatile uint32_t CR1;
	volatile uint32_t CR2;
	volatile uint32_t CR3;
	volatile uint32_t GTPR;
} USART_TypeDef;

#define FLASH_BASE	0x40022000UL//		FLASH base address
#define RCC_BASE	0x40021000UL//		RCC base address
#define GPIOA_BASE	0x40010800UL//		GPIO Port A base address
#define GPIOB_BASE	0x40010C00UL//		GPIO Port B base address
#define AFIO_BASE   0x40010000UL
#define USART1_BASE 0x40013800UL 

#define AFIO        ((AFIO_TypeDef *)AFIO_BASE)
#define FLASH		(( FLASH_TypeDef *)FLASH_BASE )// 	FLASH base address points to FLASH structure
#define RCC         (( RCC_TypeDef *)RCC_BASE )//		RCC base address points to RCC structure
#define GPIOA		(( GPIO_TypeDef *)GPIOA_BASE )//	GPIO Port A base address points to GPIO structure
#define GPIOB		(( GPIO_TypeDef *)GPIOB_BASE )//	GPIO Port B base address points to GPIO structure
#define USART1      (( USART_TypeDef *)USART1_BASE )

void USER_GPIO_Init( void );
void USER_Delay_10ms( void );
void USER_Delay_1sec( void );

#endif /* MAIN_H_ */