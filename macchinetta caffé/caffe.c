#include "stm32_unict_lib.h"
#include <stdio.h>

//int toggle_PC2_led_time = 0;
//int toggle_PC3_led_time = 0;

int flashingYellow = 0; 
int flashingGreen = 0;

//in ms
int time_delay = 5;

//idealmente sarebbe un array di oggetti da 3 attributi ciascuno
char names[5] = {'K','E','T','C','L'}; 
int cost[5] = {50,20,30,20,10};
int preptime[5] = {15000,5000,10000,5000,5000};

// la "i" per scorrere i tre array di cui sopra
int choiceNum = 0;

int prepping = 0; 

int money_inserted = 0;
int rest = 0; 
int deposit_money = 0;

//"duplicati" perché wait_money e sugarLev verranno modificati, salvo copia valore standard

//in ms
int max_wait = 30000;
int wait_money = 30000;

int standardSugar = 3;
int sugarLev = 3;

typedef enum{
    st_SCELTA,
    st_ZUCC,
    st_MONEY,
    st_PREP,
    st_RITORNO
} t_state;

t_state current_state; 


void setup(void){
    current_state = st_SCELTA;

    DISPLAY_init();


    TIM_init(TIM2);
    TIM_config_timebase(TIM2,8400,2500);
    TIM_enable_irq(TIM2,IRQ_UPDATE);
    TIM_set(TIM2,0);
    TIM_on(TIM2);


    GPIO_init(GPIOB);
    GPIO_init(GPIOC);

    GPIO_config_output(GPIOB, 0); //LED ROSSO
    GPIO_config_output(GPIOC,2);  //LED GIALLO
    GPIO_config_output(GPIOC, 3); //LED VERDE

    GPIO_config_input(GPIOB, 10); //PB X
    GPIO_config_input(GPIOB, EXTI10);
	EXTI_enable(EXTI10, FALLING_EDGE);

    GPIO_config_input(GPIOB, 4);  //PB Y
    GPIO_config_input(GPIOB, EXTI4);
	EXTI_enable(EXTI4, FALLING_EDGE);


    GPIO_config_input(GPIOB, 5);  //PB Z
    GPIO_config_input(GPIOB, EXTI5);
	EXTI_enable(EXTI5, FALLING_EDGE);


    GPIO_config_input(GPIOB, 6); //PB T
    GPIO_config_EXTI(GPIOB,EXTI6);
    EXTI_enable(EXTI6, FALLING_EDGE);
}

void loop(void){
    char s[5];

    switch(current_state){

        case st_SCELTA:

        //azzeramento valori/spegnimento led precedenti
        wait_money = max_wait;
        GPIO_write(GPIOB,0,0);

        //flashing con timer
        flashingYellow = 1;

        // display nome bevanda 
        DISPLAY_putc(0,names[choiceNum]);
        
        //flashing con delay
        /*delay_ms(time_delay);
        toggle_PC2_led_time+=time_delay; 
        if(toggle_PC2_led_time>=500){
            GPIO_toggle(GPIOC,2);
            toggle_PC2_led_time = 0;
        }
        */
            break; 

        case st_ZUCC:

        //azzeramento valori/gestione led 
        flashingYellow = 0;
        //toggle_PC2_led_time = 0;
        GPIO_write(GPIOC,2,1);

        // display quantità zucchero
        sprintf(s,"%4d",sugarLev);
        DISPLAY_puts(0,s);

            break; 

        case st_MONEY:

        //azzeramento valori/gestione led 
        flashingYellow = 0;
        //toggle_PC2_led_time = 0;
        GPIO_write(GPIOC,2,1);

        //display importo aggiunto
        sprintf(s,"%4d",money_inserted);
        DISPLAY_puts(0,s);

        delay_ms(time_delay);
        wait_money-=time_delay; 
        if(wait_money == 0){        //se ha aspettato troppo

            // "ritorna" soldi
            money_inserted = 0;
            wait_money = max_wait;
            current_state = st_SCELTA;
        }
        
        //inserita una quantità di soldi uguale o maggiore al costo bevanda nel tempo limite
        if(money_inserted >= cost[choiceNum]){
            current_state = st_PREP;
        }

            break;

        case st_PREP:
        
        //gestione led 
        GPIO_write(GPIOC,2,0);
        flashingGreen = 1;

        // display "PREP"
        sprintf(s,"%4d","PREP");
        DISPLAY_puts(0,s);

        delay_ms(time_delay);

        //led flashing with time delay
        /*toggle_PC3_led_time+=time_delay; 
        if(toggle_PC3_led_time>=500){
            GPIO_toggle(GPIOC,2);
            toggle_PC3_led_time = 0;
        }
        */

        prepping+=time_delay;   //preparando tramite delay

        //se la bevanda è preparata
        if(prepping>=preptime[choiceNum]){
            prepping = 0;
            current_state = st_RITORNO;
        }

            break;

        case st_RITORNO:

        //gestione led
        flashingGreen = 0;
        GPIO_write(GPIOC,3,0);
        GPIO_write(GPIOB,0,1);
        
        rest = money_inserted - cost[choiceNum];

        // "ritornare" resto
        sprintf(s,"R%4d",rest);
        DISPLAY_puts(0,s);

        // "consegnare" bevanda

        //metto il costo della bevanda nel deposito della macchinetta
        deposit_money += rest; 

        //azzero lo zucchero
        sugarLev = standardSugar;
            break;

    }

}

//handler del timer per il lampeggio led
void TIM2_IRQHandler(void){
    if(flashingYellow){
        GPIO_toggle(GPIOC,2);
    }else if(flashingGreen){
        GPIO_toggle(GPIOC,3);
    }else{
        GPIO_write(GPIOC,3,0);
        GPIO_write(GPIOC,2,0);
    }
    TIM_update_clear(TIM2);
}

void EXTI4_IRQHandler(void){
    //PB Y
    if(EXTI_isset(EXTI4)){
        
        if(current_state == st_SCELTA){
            if(choiceNum<5){
                choiceNum++;
            }
        }

        if(current_state == st_ZUCC){
            if(sugarLev>0){
                sugarLev--; 
            }
        }

        if(current_state == st_MONEY){
            money_inserted+=20; 
        }

        EXTI_clear(EXTI4);
    }
}

void EXTI9_5_IRQHandler(void){
    //PB Z
    if(EXTI_isset(EXTI5)){

        if(current_state == st_SCELTA){ //andata menu zucchero
            current_state = st_ZUCC;
        }
        
        if(current_state == st_ZUCC){ //ritorno menu scelta
            current_state = st_SCELTA;
        }

        if(current_state == st_MONEY){
            money_inserted+=100; 
        }

        EXTI_clear(EXTI5);
    }

    //PB T
    if(EXTI_isset(EXTI6)){
        
        if(current_state == st_SCELTA){ // "conferma bevanda"
            current_state = st_MONEY;
        }

        if(current_state == st_MONEY){ 
            money_inserted+=50; 
        }

        if(current_state == st_RITORNO){ // inizio nuova sessione
            current_state = st_SCELTA;
        }

        EXTI_clear(EXTI6);
    }
}

void EXTI15_10_IRQHandler(void){
    //PB X
    if(EXTI_isset(EXTI10)){

        if(current_state == st_SCELTA){
            if(choiceNum>0){
                choiceNum--;
            }
        }

        if(current_state == st_ZUCC){
            if(sugarLev<5){
                sugarLev++; 
            }
        }

        if(current_state == st_MONEY){
            money_inserted+=10; 
        }

        EXTI_clear(EXTI10);
    }
}

int main(){
    setup();
    for(;;){
        loop();
    }
    return 0;
}