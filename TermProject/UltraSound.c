#include "stm32f10x.h"
#include "stm32f10x_exti.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_rcc.h"
#include "touch.h"
#include "misc.h"
#include <stdio.h>

// PE4 : Trig (송신부 - OUTPUT)
// PE3 : Echo (수신부 - INPUT)
uint32_t usTime=0;

void RCC_Configure(void);
void GPIO_Configure(void);
void TIM_Configure(void);
void NVIC_Configure(void);
void TIM2_IRQHandler(void);
int Read_Distance(void);

void RCC_Configure(void) {
  // Alternate Function IO clock enable
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

  // TIM2 clock enable
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
  // port E RCC ENABLE
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);

  /* ADC1 Enable */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
}

void GPIO_Configure(void) {
  // UltraSound
  GPIO_InitTypeDef GPIO_InitStructure;

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4; // PE4 : Trig (송신부 - OUTPUT)
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOE, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3; // PE3 : Echo (수신부 - INPUT)
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
  GPIO_Init(GPIOE, &GPIO_InitStructure);
}

void TIM_Configure(void) {
  // uint16_t prescale = (uint16_t)(SystemCoreClock / 10000);

  // TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
  // TIM_TimeBaseStructure.TIM_Period = 10000;
  // TIM_TimeBaseStructure.TIM_Prescaler = prescale;
  // TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  // TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Down; 
  // TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

  // TIM_ARRPreloadConfig(TIM2, ENABLE);
  // // TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE); 
  // TIM_Cmd(TIM2, ENABLE);

  //set 1us
  TIM_TimeBaseInitTypeDef TIM_InitStructure;
  TIM_InitStructure.TIM_Prescaler = 72;
  TIM_InitStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_InitStructure.TIM_Period = 1;
  TIM_InitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
  TIM_TimeBaseInit(TIM2, &TIM_InitStructure);
  
  TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE); 
  TIM_Cmd(TIM2, ENABLE);
}

void NVIC_Configure(void) {
    
     NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
     NVIC_InitTypeDef NVIC_InitStructure;
     /* Enable TIM2 Global Interrupt */
     NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
     NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
     NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
     NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
     NVIC_Init(&NVIC_InitStructure);

}

void TIM2_IRQHandler(void) {
  if(TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) {
    usTime++;
    TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
  }
}

void Delay(void) {
    int i;
    for (i = 0; i < 500; i++) {}
}

int Read_Distance(void){
  uint32_t prev=0;
  GPIO_SetBits(GPIOE,GPIO_Pin_4);
  GPIO_ResetBits(GPIOE, GPIO_Pin_3);
  Delay();
  GPIO_ResetBits(GPIOE,GPIO_Pin_4);
  uint8_t val = GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_3);
  prev = usTime;
  //초기값은 ECHO가 RESET일테니까.
  while(val == RESET){ //바로 SET되지 않고 RESET인 경우에
    if(usTime - prev >= 10000) break; // 5ms 동안
    else{
      val = GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_3); //계속 갱신하면서 5ms가 넘으면 빠져나옴.
    }
  }
  //빠져나왔는데
  if(val == SET) { // 5ms안에 SET이 되었으면
    prev = usTime;
    while(GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_3) != RESET)
    {
    }
    return (usTime - prev) * 34 / 1000; // 다시 SET -> RESET이 될때까지 시간 (usTime -prev)으로 distance계산해서 반환.
  }else{
      //5ms안에 감지가 안됐으면
      //너무 거리가 멀다는 의미니까 큰값 반환.
      return usTime - prev;
  }
}

gint main(void){
  SystemInit();
  RCC_Configure();
  GPIO_Configure();
  TIM_Configure();
  NVIC_Configure();

  while(1){
    int distance = Read_Distance();
    printf("\tdistance : %d\n", distance);
  }

  return 0;
}
