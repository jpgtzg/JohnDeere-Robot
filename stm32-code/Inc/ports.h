#ifndef PORTS_H_
#define PORTS_H_

#include <stdint.h>
#include <sys/types.h>

/* Flash memory interface registers */
typedef struct {
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
typedef struct {
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
typedef struct {
  volatile uint32_t CRL;
  volatile uint32_t CRH;
  volatile uint32_t IDR;
  volatile uint32_t ODR;
  volatile uint32_t BSRR;
  volatile uint32_t BRR;
  volatile uint32_t LCKR;
} GPIO_TypeDef;

/* Alternate Function I/O registers */
typedef struct {
  volatile uint32_t EVCR;
  volatile uint32_t MAPR;
  volatile uint32_t EXTICR[4];
  volatile uint32_t reserved;
  volatile uint32_t MAPR2;
} AFIO_TypeDef;

typedef struct {
  volatile uint32_t SR;
  volatile uint32_t DR;
  volatile uint32_t BRR;
  volatile uint32_t CR1;
  volatile uint32_t CR2;
  volatile uint32_t CR3;
  volatile uint32_t GTPR;
} USART_TypeDef;

typedef struct {
  volatile uint32_t ISER[3U];
  volatile uint32_t RESERVD0[29U];
  volatile uint32_t ICER[3U];
  volatile uint32_t RESERVED1[29U];
  volatile uint32_t ISPR[3U];
  volatile uint32_t RESERVED2[29U];
  volatile uint32_t ICPR[3U];
  volatile uint32_t RESERVED3[29U];
  volatile uint32_t IABR[3U];
  volatile uint32_t RESERVED4[61U];
  volatile uint32_t IP[84U];
  volatile uint32_t RESERVED5[683U];
  volatile uint32_t STIR;
} NVIC_TypeDef;

typedef struct {
  volatile uint32_t CR1;
  volatile uint32_t CR2;
  volatile uint32_t SMCR;
  volatile uint32_t DIER;
  volatile uint32_t SR;
  volatile uint32_t EGR;
  volatile uint32_t CCMR1;
  volatile uint32_t CCMR2;
  volatile uint32_t CCER;
  volatile uint32_t CNT;
  volatile uint32_t PSC;
  volatile uint32_t ARR;
  volatile uint32_t RCR; // Reserved
  volatile uint32_t CCR1;
} TIM_TypeDef;

typedef struct {
  volatile uint32_t SR;
  volatile uint32_t CR1;
  volatile uint32_t CR2;
  volatile uint32_t SMPR1;
  volatile uint32_t SMPR2;
  volatile uint32_t JOFR1;
  volatile uint32_t JOFR2;
  volatile uint32_t JOFR3;
  volatile uint32_t JOFR4;
  volatile uint32_t HTR;
  volatile uint32_t LTR;
  volatile uint32_t SQR1;
  volatile uint32_t SQR2;
  volatile uint32_t SQR3;
  volatile uint32_t JSQR;
  volatile uint32_t JDR1;
  volatile uint32_t JDR2;
  volatile uint32_t JDR3;
  volatile uint32_t JDR4;
  volatile uint32_t DR;
} ADC_TypeDef;

#define FLASH_BASE 0x40022000UL //		FLASH base address
#define RCC_BASE 0x40021000UL   //		RCC base address
#define GPIOA_BASE 0x40010800UL //		GPIO Port A base address
#define GPIOB_BASE 0x40010C00UL //		GPIO Port B base address
#define GPIOC_BASE 0x40011000UL //		GPIO Port C base address
#define AFIO_BASE 0x40010000UL
#define USART1_BASE 0x40013800UL
#define USART2_BASE 0x40004400UL
#define TIM2_BASE 0x40000000UL //      TIM2 base address
#define TIM3_BASE 0x40000400UL //      TIM3 base address
#define NVIC_BASE 0xE000E100UL
#define ADC1_BASE 0x40012400UL

#define AFIO ((AFIO_TypeDef *)AFIO_BASE)
#define FLASH ((FLASH_TypeDef *)FLASH_BASE)
#define RCC ((RCC_TypeDef *)RCC_BASE)
#define GPIOA ((GPIO_TypeDef *)GPIOA_BASE)
#define GPIOB ((GPIO_TypeDef *)GPIOB_BASE)
#define GPIOC ((GPIO_TypeDef *)GPIOC_BASE)
#define USART1 ((USART_TypeDef *)USART1_BASE)
#define USART2 ((USART_TypeDef *)USART2_BASE)
#define NVIC ((NVIC_TypeDef *)NVIC_BASE)
#define TIM2 ((TIM_TypeDef *)TIM2_BASE)
#define TIM3 ((TIM_TypeDef *)TIM3_BASE)
#define ADC1 ((ADC_TypeDef *)ADC1_BASE)

#endif
