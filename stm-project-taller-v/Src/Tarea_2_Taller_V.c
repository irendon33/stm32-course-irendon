/*
 * Tarea_2_Taller_V.c
 *
 *  Created on: Jun 12, 2026
 *      Author: ird
 */

#include <stdint.h>
#define STM32F411xE
#include <stm32f4xx.h>

/*
 Variables globales */

/* Valor del contador controlado por las fotocompuertas */
uint16_t val_contador = 0;

/* Bandera que indica qué EXTI se disparó: 1 = EXTI0 (resta), 2 = EXTI2 (suma) */
volatile uint8_t flag_exti = 0;

/* Arreglo con los 4 dígitos descompuestos del contador (miles, centenas, decenas, unidades) */
volatile uint8_t dig[4] = {0, 0, 0, 0};

/* Bandera que se pone en 1 cada vez que el TIM3 genera una interrupción */
volatile uint8_t flag_tim3 = 0;

/* Índice del dígito activo en el multiplexado (0 a 3) */
volatile uint8_t idx_dig = 0;

/*
 Prototipos de funciones */

void config_gpio_display(void);   /* Configura pines de segmentos, transistores y fotocompuertas */
void led_estado(void);            /* Inicializa el LED de estado (blinky) con TIM2 */
void config_exti(void);           /* Configura EXTI0 (bajada, PA0) y EXTI2 (subida, PC2) */
void descomponer_digitos(void);   /* Separa val_contador en sus 4 dígitos decimales */
void config_tim3(void);           /* Configura TIM3 - display */
void escribir_segmentos(void);    /* Activa los segmentos correctos según el dígito actual */
void activar_digito(void);        /* Enciende el transistor del dígito activo y apaga los demás */

/* Main */

int main(void){

    /* Inicialización del hardware */
    led_estado();
    config_gpio_display();
    config_exti();
    config_tim3();

    /* Bucle principal */
    while(1){

        /* Atención a las interrupciones EXTI */
        if(flag_exti == 1){
            flag_exti = 0;        /* Limpia la bandera antes de procesar */
            val_contador--;       /* Fotocompuerta F2 (PA0): decrece el contador */
        }
        else if(flag_exti == 2){
            flag_exti = 0;        /* Limpia la bandera antes de procesar */
            val_contador++;       /* Fotocompuerta F1 (PC2): incrementa el contador */
        }

        /* Actualización del display al ritmo del TIM3 */
        if(flag_tim3 == 1){
            flag_tim3 = 0;        /* Limpia la bandera del timer */
            activar_digito();     /* Muestra el dígito que corresponde al ciclo actual */
        }

        /* Siempre mantiene la des3 del contador actualizada */
        descomponer_digitos();
    }

    return 0;
}

/*
 Definición de funciones
 */

/* Configura todos los GPIO necesarios para el display 7 segmentos:
 * Segmentos A-G, transistores D1-D4, y entradas de fotocompuertas */
void config_gpio_display(void){

    /* Activacion de relojes para todos los puertos GPIO usados --- */

    /* GPIOA */
    RCC->AHB1ENR &= ~RCC_AHB1ENR_GPIOAEN;
    RCC->AHB1ENR |=  RCC_AHB1ENR_GPIOAEN;

    /* GPIOB */
    RCC->AHB1ENR &= ~RCC_AHB1ENR_GPIOBEN;
    RCC->AHB1ENR |=  RCC_AHB1ENR_GPIOBEN;

    /* GPIOC */
    RCC->AHB1ENR &= ~RCC_AHB1ENR_GPIOCEN;
    RCC->AHB1ENR |=  RCC_AHB1ENR_GPIOCEN;

    /* GPIOD */
    RCC->AHB1ENR &= ~RCC_AHB1ENR_GPIODEN;
    RCC->AHB1ENR |=  RCC_AHB1ENR_GPIODEN;

    /* GPIOH */
    RCC->AHB1ENR &= ~RCC_AHB1ENR_GPIOHEN;
    RCC->AHB1ENR |=  RCC_AHB1ENR_GPIOHEN;

    /*
     Configuración de pines de SEGMENTOS (salidas digitales)
      */

    /* PB8 -> Segmento B */
    GPIOB->MODER   &= ~GPIO_MODER_MODE8;
    GPIOB->MODER   |=  GPIO_MODER_MODE8_0;   /* Salida */
    GPIOB->OTYPER  &= ~GPIO_OTYPER_OT8;      /* Push-pull */
    GPIOB->OSPEEDR &= ~GPIO_OSPEEDR_OSPEED8;
    GPIOB->OSPEEDR |=  GPIO_OSPEEDR_OSPEED8_1; /* Velocidad alta */
    GPIOB->PUPDR   &= ~GPIO_PUPDR_PUPD8;     /* Sin pull */
    GPIOB->ODR     &= ~GPIO_ODR_OD8;         /* Inicialmente apagado */

    /* PC8 -> Segmento F */
    GPIOC->MODER   &= ~GPIO_MODER_MODE8;
    GPIOC->MODER   |=  GPIO_MODER_MODE8_0;
    GPIOC->OTYPER  &= ~GPIO_OTYPER_OT8;
    GPIOC->OSPEEDR &= ~GPIO_OSPEEDR_OSPEED8;
    GPIOC->OSPEEDR |=  GPIO_OSPEEDR_OSPEED8_1;
    GPIOC->PUPDR   &= ~GPIO_PUPDR_PUPD8;
    GPIOC->ODR     &= ~GPIO_ODR_OD8;

    /* PC9 -> Segmento A */
    GPIOC->MODER   &= ~GPIO_MODER_MODE9;
    GPIOC->MODER   |=  GPIO_MODER_MODE9_0;
    GPIOC->OTYPER  &= ~GPIO_OTYPER_OT9;
    GPIOC->OSPEEDR &= ~GPIO_OSPEEDR_OSPEED9;
    GPIOC->OSPEEDR |=  GPIO_OSPEEDR_OSPEED9_1;
    GPIOC->PUPDR   &= ~GPIO_PUPDR_PUPD9;
    GPIOC->ODR     &= ~GPIO_ODR_OD9;

    /* PC10 -> Segmento E */
    GPIOC->MODER   &= ~GPIO_MODER_MODE10;
    GPIOC->MODER   |=  GPIO_MODER_MODE10_0;
    GPIOC->OTYPER  &= ~GPIO_OTYPER_OT10;
    GPIOC->OSPEEDR &= ~GPIO_OSPEEDR_OSPEED10;
    GPIOC->OSPEEDR |=  GPIO_OSPEEDR_OSPEED10_1;
    GPIOC->PUPDR   &= ~GPIO_PUPDR_PUPD10;
    GPIOC->ODR     &= ~GPIO_ODR_OD10;

    /* PC11 -> Segmento C */
    GPIOC->MODER   &= ~GPIO_MODER_MODE11;
    GPIOC->MODER   |=  GPIO_MODER_MODE11_0;
    GPIOC->OTYPER  &= ~GPIO_OTYPER_OT11;
    GPIOC->OSPEEDR &= ~GPIO_OSPEEDR_OSPEED11;
    GPIOC->OSPEEDR |=  GPIO_OSPEEDR_OSPEED11_1;
    GPIOC->PUPDR   &= ~GPIO_PUPDR_PUPD11;
    GPIOC->ODR     &= ~GPIO_ODR_OD11;

    /* PC12 -> Segmento D */
    GPIOC->MODER   &= ~GPIO_MODER_MODE12;
    GPIOC->MODER   |=  GPIO_MODER_MODE12_0;
    GPIOC->OTYPER  &= ~GPIO_OTYPER_OT12;
    GPIOC->OSPEEDR &= ~GPIO_OSPEEDR_OSPEED12;
    GPIOC->OSPEEDR |=  GPIO_OSPEEDR_OSPEED12_1;
    GPIOC->PUPDR   &= ~GPIO_PUPDR_PUPD12;
    GPIOC->ODR     &= ~GPIO_ODR_OD12;

    /* PD2 -> Segmento G */
    GPIOD->MODER   &= ~GPIO_MODER_MODE2;
    GPIOD->MODER   |=  GPIO_MODER_MODE2_0;
    GPIOD->OTYPER  &= ~GPIO_OTYPER_OT2;
    GPIOD->OSPEEDR &= ~GPIO_OSPEEDR_OSPEED2;
    GPIOD->OSPEEDR |=  GPIO_OSPEEDR_OSPEED2_1;
    GPIOD->PUPDR   &= ~GPIO_PUPDR_PUPD2;
    GPIOD->ODR     &= ~GPIO_ODR_OD2;

    /* -------------------------------------------------------
     * Configuración de pines de TRANSISTORES (control de dígitos)
     * Los transistores se activan con nivel bajo (lógica inversa)
     * Se inicializan en HIGH para que todos los dígitos estén apagados
     * ------------------------------------------------------- */

    /* PC6 -> Transistor dígito D1 */
    GPIOC->MODER   &= ~GPIO_MODER_MODE6;
    GPIOC->MODER   |=  GPIO_MODER_MODE6_0;
    GPIOC->OTYPER  &= ~GPIO_OTYPER_OT6;
    GPIOC->OSPEEDR &= ~GPIO_OSPEEDR_OSPEED6;
    GPIOC->OSPEEDR |=  GPIO_OSPEEDR_OSPEED6_1;
    GPIOC->PUPDR   &= ~GPIO_PUPDR_PUPD6;
    GPIOC->ODR     |=  GPIO_ODR_OD6;   /* HIGH = transistor OFF */

    /* PB9 -> Transistor dígito D2 */
    GPIOB->MODER   &= ~GPIO_MODER_MODE9;
    GPIOB->MODER   |=  GPIO_MODER_MODE9_0;
    GPIOB->OTYPER  &= ~GPIO_OTYPER_OT9;
    GPIOB->OSPEEDR &= ~GPIO_OSPEEDR_OSPEED9;
    GPIOB->OSPEEDR |=  GPIO_OSPEEDR_OSPEED9_1;
    GPIOB->PUPDR   &= ~GPIO_PUPDR_PUPD9;
    GPIOB->ODR     |=  GPIO_ODR_OD9;   /* HIGH = transistor OFF */

    /* PC3 -> Transistor dígito D3 */
    GPIOC->MODER   &= ~GPIO_MODER_MODE3;
    GPIOC->MODER   |=  GPIO_MODER_MODE3_0;
    GPIOC->OTYPER  &= ~GPIO_OTYPER_OT3;
    GPIOC->OSPEEDR &= ~GPIO_OSPEEDR_OSPEED3;
    GPIOC->OSPEEDR |=  GPIO_OSPEEDR_OSPEED3_1;
    GPIOC->PUPDR   &= ~GPIO_PUPDR_PUPD3;
    GPIOC->ODR     |=  GPIO_ODR_OD3;   /* HIGH = transistor OFF */

    /* PB7 -> Transistor dígito D4 */
    GPIOB->MODER   &= ~GPIO_MODER_MODE7;
    GPIOB->MODER   |=  GPIO_MODER_MODE7_0;
    GPIOB->OTYPER  &= ~GPIO_OTYPER_OT7;
    GPIOB->OSPEEDR &= ~GPIO_OSPEEDR_OSPEED7;
    GPIOB->OSPEEDR |=  GPIO_OSPEEDR_OSPEED7_1;
    GPIOB->PUPDR   &= ~GPIO_PUPDR_PUPD7;
    GPIOB->ODR     |=  GPIO_ODR_OD7;   /* HIGH = transistor OFF */

    /*
     Configuración de entradas para FOTOCOMPUERTAS (sin pull)
      */

    /* PC2 -> Fotocompuerta F1 (EXTI2, flanco de subida) */
    GPIOC->MODER &= ~GPIO_MODER_MODE2;   /* Entrada */
    GPIOC->PUPDR &= ~GPIO_PUPDR_PUPD2;   /* Sin resistencia */

    /* PA0 -> Fotocompuerta F2 (EXTI0, flanco de bajada) */
    GPIOA->MODER &= ~GPIO_MODER_MODE0;   /* Entrada */
    GPIOA->PUPDR &= ~GPIO_PUPDR_PUPD0;   /* Sin resistencia */
}

/* Inicializa el LED de estado en PH1 y configura TIM2 para blinky a 250 ms */
void led_estado(void){

    /* Habilitar reloj GPIOH */
    RCC->AHB1ENR &= ~RCC_AHB1ENR_GPIOHEN;
    RCC->AHB1ENR |=  RCC_AHB1ENR_GPIOHEN;

    /* PH1 como salida push-pull, velocidad alta, sin pull */
    GPIOH->MODER   &= ~GPIO_MODER_MODE1;
    GPIOH->MODER   |=  GPIO_MODER_MODE1_0;
    GPIOH->OTYPER  &= ~GPIO_OTYPER_OT1;
    GPIOH->OSPEEDR &= ~GPIO_OSPEEDR_OSPEED1;
    GPIOH->OSPEEDR |=  GPIO_OSPEEDR_OSPEED1_1;
    GPIOH->PUPDR   &= ~GPIO_PUPDR_PUPD1;
    GPIOH->ODR     |=  GPIO_ODR_OD1;    /* LED encendido al inicio */

    /* Configurar TIM2: reloj 16 MHz, PSC=1599 -> 10 kHz, ARR=2499 -> 250 ms */
    RCC->APB1ENR &= ~RCC_APB1ENR_TIM2EN;
    RCC->APB1ENR |=  RCC_APB1ENR_TIM2EN;

    TIM2->PSC  = 1600 - 1;    /* Prescaler: 16 MHz / 1600 = 10 kHz (tick = 0.1 ms) */
    TIM2->ARR  = 2500 - 1;    /* Auto-reload: 2500 × 0.1 ms = 250 ms */
    TIM2->CNT  = 0;

    TIM2->SR   &= ~TIM_SR_UIF;           /* Limpia bandera de actualización */

    TIM2->DIER &= ~TIM_DIER_UIE;
    TIM2->DIER |=  TIM_DIER_UIE;         /* Habilita interrupción por update-event */

    __NVIC_EnableIRQ(TIM2_IRQn);

    TIM2->CR1  &= ~TIM_CR1_DIR;          /* Conteo ascendente */
    TIM2->CR1  &= ~TIM_CR1_ARPE;
    TIM2->CR1  |=  TIM_CR1_ARPE;         /* Precarga del ARR activa */
    TIM2->CR1  |=  TIM_CR1_CEN;          /* Arranca el timer */
}

/* ISR del TIM2: hace toggle del LED de estado cada 250 ms */
void TIM2_IRQHandler(void){
    if(TIM2->SR && TIM_SR_UIF){
        TIM2->SR  &= ~TIM_SR_UIF;        /* Baja la bandera */
        GPIOH->ODR ^= GPIO_ODR_OD1;      /* Toggle del LED de estado */
    }
}

/* Configura EXTI0 (PA0, flanco de bajada) y EXTI2 (PC2, flanco de subida) */
void config_exti(void){

    /* Habilitar SYSCFG para map de EXTI */
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

    /* EXTI0 mapeado a PA0 (valor 0000 por defecto, solo se limpia) */
    SYSCFG->EXTICR[0] &= ~SYSCFG_EXTICR1_EXTI0;

    /* EXTI0: detecta flanco de bajada, deshabilita flanco de subida */
    EXTI->FTSR |=  EXTI_FTSR_TR0;
    EXTI->RTSR &= ~EXTI_RTSR_TR0;

    /* EXTI2 mapeado a PC2 */
    SYSCFG->EXTICR[0] &= ~SYSCFG_EXTICR1_EXTI2;
    SYSCFG->EXTICR[0] |=  SYSCFG_EXTICR1_EXTI2_PC;

    /* EXTI2: detecta flanco de subida, deshabilita flanco de bajada */
    EXTI->RTSR |=  EXTI_RTSR_TR2;
    EXTI->FTSR &= ~EXTI_FTSR_TR2;

    /* Limpiar banderas pendientes antes de activar */
    EXTI->PR |= EXTI_PR_PR0;
    EXTI->PR |= EXTI_PR_PR2;

    /* Habilitar interrupciones en el registro de máscara */
    EXTI->IMR |= EXTI_IMR_MR0;
    EXTI->IMR |= EXTI_IMR_MR2;

    /* Registrar en el NVIC */
    __NVIC_EnableIRQ(EXTI0_IRQn);
    __NVIC_EnableIRQ(EXTI2_IRQn);
}

/* ISR de EXTI0: fotocompuerta F2 (PA0), activa decrece del contador */
void EXTI0_IRQHandler(void){
    if(EXTI->PR && EXTI_PR_PR0){
        EXTI->PR |= EXTI_PR_PR0;   /* Limpia la bandera escribiendo 1 */
        __NOP();
        flag_exti = 1;             /* Solicita decrece en el main loop */
    }
}

/* ISR de EXTI2: fotocompuerta F1 (PC2), activa incremento del contador */
void EXTI2_IRQHandler(void){
    if(EXTI->PR && EXTI_PR_PR2){
        EXTI->PR |= EXTI_PR_PR2;   /* Limpia la bandera escribiendo 1 */
        __NOP();
        flag_exti = 2;             /* Solicita incremento en el main loop */
    }
}

/* Separa val_contador en sus cuatro dígitos decimales (módulo 10000) */
void descomponer_digitos(void){
    uint16_t tmp = val_contador % 10000;
    dig[0] =  tmp / 1000;            /* Dígito de los miles */
    dig[1] = (tmp % 1000) / 100;     /* Dígito de las centenas */
    dig[2] = (tmp % 100)  / 10;      /* Dígito de las decenas */
    dig[3] =  tmp % 10;              /* Dígito de las unidades */
}

/* Configura TIM3: PSC=1599, ARR=59 -> periodo ~6 ms para refresco del display */
void config_tim3(void){

    RCC->APB1ENR &= ~RCC_APB1ENR_TIM3EN;
    RCC->APB1ENR |=  RCC_APB1ENR_TIM3EN;

    TIM3->PSC  = 1600 - 1;    /* 16 MHz / 1600 = 10 kHz (tick = 0.1 ms) */
    TIM3->ARR  = 60 - 1;      /* 60 ticks × 0.1 ms = 6 ms por dígito */
    TIM3->CNT  = 0;

    TIM3->SR   &= ~TIM_SR_UIF;           /* Limpia bandera */

    TIM3->DIER &= ~TIM_DIER_UIE;
    TIM3->DIER |=  TIM_DIER_UIE;         /* Interrupción por update-event */

    __NVIC_EnableIRQ(TIM3_IRQn);

    TIM3->CR1  &= ~TIM_CR1_DIR;          /* Conteo ascendente */
    TIM3->CR1  &= ~TIM_CR1_ARPE;
    TIM3->CR1  |=  TIM_CR1_ARPE;         /* Precarga del ARR activa */
    TIM3->CR1  |=  TIM_CR1_CEN;          /* Arranca el timer */
}

/* ISR del TIM3: señaliza al main loop que debe actualizar el display */
void TIM3_IRQHandler(void){
    if(TIM3->SR && TIM_SR_UIF){
        TIM3->SR  &= ~TIM_SR_UIF;   /* Baja la bandera */
        flag_tim3  = 1;             /* Solicita refresco del dígito activo */
    }
}

/* Apaga todos los transistores, configura segmentos y enciende el dígito activo */
void activar_digito(void){

    /* Paso 1: Apagar TODOS los dígitos (transistores en HIGH) */
    GPIOC->ODR |= GPIO_ODR_OD6;   /* D1 OFF */
    GPIOB->ODR |= GPIO_ODR_OD9;   /* D2 OFF */
    GPIOC->ODR |= GPIO_ODR_OD3;   /* D3 OFF */
    GPIOB->ODR |= GPIO_ODR_OD7;   /* D4 OFF */

    /* Paso 2: Configurar los segmentos para el dígito actual */
    escribir_segmentos();

    /* Paso 3: Encender únicamente el transistor del dígito activo */
    switch(idx_dig){
        case 0:
            GPIOC->ODR &= ~GPIO_ODR_OD6;   /* D1 ON */
            break;
        case 1:
            GPIOB->ODR &= ~GPIO_ODR_OD9;   /* D2 ON */
            break;
        case 2:
            GPIOC->ODR &= ~GPIO_ODR_OD3;   /* D3 ON */
            break;
        case 3:
            GPIOB->ODR &= ~GPIO_ODR_OD7;   /* D4 ON */
            break;
    }

    /* Avanza al siguiente dígito en el ciclo (0 -> 1 -> 2 -> 3 -> 0 ...) */
    idx_dig = (idx_dig + 1) % 4;
}

/* Activa los segmentos A-G del 7 segmentos según el valor del dígito activo */
void escribir_segmentos(void){

    uint8_t num = dig[idx_dig];   /* Valor numérico del dígito a mostrar */

    /*
     * Mapa de pines de segmentos:
     *   A = PC9  |  B = PB8  |  C = PC11
     *   D = PC12 |  E = PC10 |  F = PC8  |  G = PD2
     *
     * Lógica activa en bajo (0 = encendido, 1 = apagado)
     */

    switch(num){

        case 0:  /*  Segmentos: A B C D E F    (G apagado) */
            GPIOC->ODR &= ~GPIO_ODR_OD9;    /* A ON  */
            GPIOB->ODR &= ~GPIO_ODR_OD8;    /* B ON  */
            GPIOC->ODR &= ~GPIO_ODR_OD11;   /* C ON  */
            GPIOC->ODR &= ~GPIO_ODR_OD12;   /* D ON  */
            GPIOC->ODR &= ~GPIO_ODR_OD10;   /* E ON  */
            GPIOC->ODR &= ~GPIO_ODR_OD8;    /* F ON  */
            GPIOD->ODR |=  GPIO_ODR_OD2;    /* G OFF */
            break;

        case 1:  /* Segmentos: B C */
            GPIOC->ODR |=  GPIO_ODR_OD9;    /* A OFF */
            GPIOB->ODR &= ~GPIO_ODR_OD8;    /* B ON  */
            GPIOC->ODR &= ~GPIO_ODR_OD11;   /* C ON  */
            GPIOC->ODR |=  GPIO_ODR_OD12;   /* D OFF */
            GPIOC->ODR |=  GPIO_ODR_OD10;   /* E OFF */
            GPIOC->ODR |=  GPIO_ODR_OD8;    /* F OFF */
            GPIOD->ODR |=  GPIO_ODR_OD2;    /* G OFF */
            break;

        case 2:  /* Segmentos: A B D E G */
            GPIOC->ODR &= ~GPIO_ODR_OD9;    /* A ON  */
            GPIOB->ODR &= ~GPIO_ODR_OD8;    /* B ON  */
            GPIOC->ODR |=  GPIO_ODR_OD11;   /* C OFF */
            GPIOC->ODR &= ~GPIO_ODR_OD12;   /* D ON  */
            GPIOC->ODR &= ~GPIO_ODR_OD10;   /* E ON  */
            GPIOC->ODR |=  GPIO_ODR_OD8;    /* F OFF */
            GPIOD->ODR &= ~GPIO_ODR_OD2;    /* G ON  */
            break;

        case 3:  /* Segmentos: A B C D G */
            GPIOC->ODR &= ~GPIO_ODR_OD9;    /* A ON  */
            GPIOB->ODR &= ~GPIO_ODR_OD8;    /* B ON  */
            GPIOC->ODR &= ~GPIO_ODR_OD11;   /* C ON  */
            GPIOC->ODR &= ~GPIO_ODR_OD12;   /* D ON  */
            GPIOC->ODR |=  GPIO_ODR_OD10;   /* E OFF */
            GPIOC->ODR |=  GPIO_ODR_OD8;    /* F OFF */
            GPIOD->ODR &= ~GPIO_ODR_OD2;    /* G ON  */
            break;

        case 4:  /* Segmentos: B C F G */
            GPIOC->ODR |=  GPIO_ODR_OD9;    /* A OFF */
            GPIOB->ODR &= ~GPIO_ODR_OD8;    /* B ON  */
            GPIOC->ODR &= ~GPIO_ODR_OD11;   /* C ON  */
            GPIOC->ODR |=  GPIO_ODR_OD12;   /* D OFF */
            GPIOC->ODR |=  GPIO_ODR_OD10;   /* E OFF */
            GPIOC->ODR &= ~GPIO_ODR_OD8;    /* F ON  */
            GPIOD->ODR &= ~GPIO_ODR_OD2;    /* G ON  */
            break;

        case 5:  /* Segmentos: A C D F G */
            GPIOC->ODR &= ~GPIO_ODR_OD9;    /* A ON  */
            GPIOB->ODR |=  GPIO_ODR_OD8;    /* B OFF */
            GPIOC->ODR &= ~GPIO_ODR_OD11;   /* C ON  */
            GPIOC->ODR &= ~GPIO_ODR_OD12;   /* D ON  */
            GPIOC->ODR |=  GPIO_ODR_OD10;   /* E OFF */
            GPIOC->ODR &= ~GPIO_ODR_OD8;    /* F ON  */
            GPIOD->ODR &= ~GPIO_ODR_OD2;    /* G ON  */
            break;

        case 6:  /* Segmentos: A C D E F G */
            GPIOC->ODR &= ~GPIO_ODR_OD9;    /* A ON  */
            GPIOB->ODR |=  GPIO_ODR_OD8;    /* B OFF */
            GPIOC->ODR &= ~GPIO_ODR_OD11;   /* C ON  */
            GPIOC->ODR &= ~GPIO_ODR_OD12;   /* D ON  */
            GPIOC->ODR &= ~GPIO_ODR_OD10;   /* E ON  */
            GPIOC->ODR &= ~GPIO_ODR_OD8;    /* F ON  */
            GPIOD->ODR &= ~GPIO_ODR_OD2;    /* G ON  */
            break;

        case 7:  /* Segmentos: A B C */
            GPIOC->ODR &= ~GPIO_ODR_OD9;    /* A ON  */
            GPIOB->ODR &= ~GPIO_ODR_OD8;    /* B ON  */
            GPIOC->ODR &= ~GPIO_ODR_OD11;   /* C ON  */
            GPIOC->ODR |=  GPIO_ODR_OD12;   /* D OFF */
            GPIOC->ODR |=  GPIO_ODR_OD10;   /* E OFF */
            GPIOC->ODR |=  GPIO_ODR_OD8;    /* F OFF */
            GPIOD->ODR |=  GPIO_ODR_OD2;    /* G OFF */
            break;

        case 8:  /* Segmentos: A B C D E F G (todos) */
            GPIOC->ODR &= ~GPIO_ODR_OD9;    /* A ON  */
            GPIOB->ODR &= ~GPIO_ODR_OD8;    /* B ON  */
            GPIOC->ODR &= ~GPIO_ODR_OD11;   /* C ON  */
            GPIOC->ODR &= ~GPIO_ODR_OD12;   /* D ON  */
            GPIOC->ODR &= ~GPIO_ODR_OD10;   /* E ON  */
            GPIOC->ODR &= ~GPIO_ODR_OD8;    /* F ON  */
            GPIOD->ODR &= ~GPIO_ODR_OD2;    /* G ON  */
            break;

        case 9:  /* Segmentos: A B C D F G */
            GPIOC->ODR &= ~GPIO_ODR_OD9;    /* A ON  */
            GPIOB->ODR &= ~GPIO_ODR_OD8;    /* B ON  */
            GPIOC->ODR &= ~GPIO_ODR_OD11;   /* C ON  */
            GPIOC->ODR &= ~GPIO_ODR_OD12;   /* D ON  */
            GPIOC->ODR |=  GPIO_ODR_OD10;   /* E OFF */
            GPIOC->ODR &= ~GPIO_ODR_OD8;    /* F ON  */
            GPIOD->ODR &= ~GPIO_ODR_OD2;    /* G ON  */
            break;
    }
}


