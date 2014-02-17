/*
===============================================================================
 Name        : main.c
 Author      : chibiegg
 Version     :
 Copyright   : $(copyright)
 Description : main definition
===============================================================================
*/

#ifdef __USE_CMSIS
#include "LPC8xx.h"
#endif

#include <cr_section_macros.h>

#include "lpc8xx_pmu.h"
#include "sct_fsm.h"
#include "swm.h"
#include "iocon.h"

//1回再生完了するとスリープに入るモード
#define ONESHOT

//音源にサンプリングデータを用いる（計算量の問題で単音のみ動作）
//#define USE_SAMPLE


// 音源関係
#ifdef USE_SAMPLE
	#include "sample.h"
#endif
#include "scores/famima.h"
#define SAMPL_FREQ (40000) //サンプリング周波数

uint32_t channel_freq[CHANNELS] = {0};
uint32_t channel_count[CHANNELS] = {0};
uint8_t channel_velocity[CHANNELS] = {0};

void DeepPowerDown(){ //パワーダウンに入る
	LPC_SYSCON->PDRUNCFG &= ~( (1<<0) | (1<<1) | (1<<2));
	__disable_irq();
	PMU_DeepPowerDown();
}

void SCT_Init(void){ //PWM(SCT)の初期化
	// enable the SCT clock
	LPC_SYSCON->SYSAHBCLKCTRL |= (1 << 8);
	sct_fsm_init();
	//NVIC_EnableIRQ(SCT_IRQn);
	// unhalt the SCT by clearing bit 2 of the unified CTRL register
	LPC_SCT->CTRL_U &= ~(1 << 2);
}

void Set_Channel_Freq(uint8_t ch,uint16_t freq,uint8_t velocity)
{
	channel_freq[ch] = ((uint32_t)SAMPL_FREQ)/freq;
	channel_velocity[ch] = velocity/CHANNELS;
}

void SysTick_Handler(void)
{
	/* 楽譜の計算 */
	static uint32_t time_count = 0;
	static uint32_t time = 0;
	static uint16_t note_id = 0;

	if(time==0){
		time = score[0].time;
	}

	time_count++;
	if(time_count>(SAMPL_FREQ/TEMPO*60/4)){
		time_count = 0;
    	if(time==score[note_id].time){
    		while(time==score[note_id].time){
				if(score[note_id].ch < 0){
					note_id = 0;
					time = score[0].time;

#ifdef ONESHOT
					DeepPowerDown();
#endif


					continue;
				}else{
					Set_Channel_Freq(score[note_id].ch,score[note_id].freq,score[note_id].velocity);
					note_id++;
				}
    		}
    	}
    	time++;
	}



	/* 音源の計算 */
	uint8_t ch;
	uint16_t output=0;
	for(ch=0;ch<CHANNELS;ch++){
		if(channel_freq[ch]>0){
			channel_count[ch]++;
			if(channel_count[ch]>=channel_freq[ch]){
				channel_count[ch]=0;
			}

			//出力加算
#ifdef USE_SAMPLE
			//サンプリング音源利用
			uint8_t s = channel_count[ch]*SAMPLES/channel_freq[ch];
			output += sample[s]*channel_velocity[ch]/128;
#else
			//矩形波
			if(channel_count[ch]<(channel_freq[ch]>>1))
				output += channel_velocity[ch];
#endif
		}
	}

	//SCTデューティ比の確定
	sct_fsm_reload_match1(output);
}



int main(void) {

	SwitchMatrix_Init();
	IOCON_Init();
	SCT_Init();

	SystemCoreClockUpdate();
	SysTick_Config( SystemCoreClock/SAMPL_FREQ );

    while(1) {
    }
    return 0 ;
}

