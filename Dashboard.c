#include <RTL.h>
#include <91x_lib.H>
#include "LCD.h"
#include <string.h>
#include <stdlib.h>

unsigned char A0,A1;
unsigned char B0 = 0,B1 = 0,B2 = 0,B3 = 0,B4 = 0,B5 = 0,B6 = 0,B7 = 0; 
unsigned char AD_in_progress = 0;
const int speedLimit = 300;
const unsigned int blinkperiod = 10;
unsigned int dutyCycle = 0;
unsigned int dutyCyclePct = 0;
int engine = 0;
int door = 0;
short PotentioGlobal;
os_mbx_declare(potmail,16);
os_mbx_declare(slidemail,16);
OS_MUT mutex1[20];
U32 mpoolslide[16*(2*sizeof(U32))/4 + 3];
U32 mpoolpot[16*(2*sizeof(U32))/4 + 3];

char* itoa(int value, char* result, int base) 
{
	char* ptr = result, *ptr1 = result, tmp_char;
	int tmp_value;
	if (base < 2 || base > 36) { *result = '\0'; return result; }
	do {
		tmp_value = value;
		value /= base;
		*ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
	} while ( value );
	if (tmp_value < 0) *ptr++ = '-';
	*ptr-- = '\0';
	while(ptr1 < ptr) {
		tmp_char = *ptr;
		*ptr--= *ptr1;
		*ptr1++ = tmp_char;
	}
	return result;
}

__irq void ADC_IRQ_Handler (void) {    
 	U32 *mptrslide,*mptrpot;
	short SlideSensor,Potentiometer;
	Potentiometer = ADC->DR0 & 0x03FF;    	
	SlideSensor = ADC->DR1 & 0x03FF;         
	mptrslide = _alloc_box (mpoolslide);
	mptrslide[0] = SlideSensor;
	mptrpot = _alloc_box(mpoolpot);
	mptrpot[0] = Potentiometer;
	ADC->CR &= 0xFFFE;                    
	ADC->CR &= 0x7FFF;                   
  VIC0->VAR = 0;                         
  VIC1->VAR = 0;
  AD_in_progress = 0;                  
	if(isr_mbx_check(slidemail) != 0) 
	{
		isr_mbx_send (slidemail, mptrslide);
	}
	if(isr_mbx_check(potmail) != 0)
	{
		isr_mbx_send (potmail, mptrpot);
	}
}

void start_ADC ( )
{
	if (!AD_in_progress)  	
	{             						    						
        AD_in_progress = 1;                 
        ADC->CR |= 0x0423;                  
  }
}

void write_led()
{
  unsigned char mask = 0;
	B0 = 1;
	B1 = 1;
	B2 = 1;
	B3 = 1;
	B4 = 1;
	B5 = 1;
	mask  = (B0<<0);
  mask |= (B1<<1);
  mask |= (B2<<2);
  mask |= (B3<<3);
  mask |= (B4<<4);
  mask |= (B5<<5);
  mask |= (B6<<6);
  mask |= (B7<<7);

  GPIO7->DR[0x3FC] = mask;
}

void clear_led()
{
	unsigned char mask = 0;
	B0 = 0;
	B1 = 0;
	B2 = 0;
	B3 = 0;
	B4 = 0;
	B5 = 0;
  mask  = (B0<<0);
  mask |= (B1<<1);
  mask |= (B2<<2);
  mask |= (B3<<3);
  mask |= (B4<<4);
  mask |= (B5<<5);
  mask |= (B6<<6);
  mask |= (B7<<7);
  GPIO7->DR[0x3FC] = mask;
}

void write_blink_led()
{
	unsigned char mask = 0;
	B6 = 1;
	B7 = 1;
	mask  = (B0<<0);
  mask |= (B1<<1);
  mask |= (B2<<2);
  mask |= (B3<<3);
  mask |= (B4<<4);
  mask |= (B5<<5);
  mask |= (B6<<6);
  mask |= (B7<<7);
	GPIO7->DR[0x3FC] = mask;
}

void clear_blink_led()
{
	unsigned char mask = 0;
	B6 = 0;
	B7 = 0;
	mask  = (B0<<0);
  mask |= (B1<<1);
  mask |= (B2<<2);
  mask |= (B3<<3);
  mask |= (B4<<4);
  mask |= (B5<<5);
  mask |= (B6<<6);
  mask |= (B7<<7);
	GPIO7->DR[0x3FC] = mask;
}

OS_TID task1_id;
OS_TID task2_id;
OS_TID task5_id;
OS_TID task6_id;
OS_TID task3_id;
OS_TID task4_id;
OS_TID task7_id;
OS_TID engine_id;
OS_TID door_id;
OS_TID alarm_id;

__task void task1(void){
	const unsigned int period = 10;
	os_itv_set(period);
	for(;;)
	{
			start_ADC();
			os_itv_wait();
	}
}	

__task void task2(void)
{
	short Potentiometer;
	U32 *rptr, rec_val;
	_init_box (mpoolpot, sizeof(mpoolpot), sizeof(U32));
	os_mbx_init (potmail, sizeof(potmail));
	os_dly_wait(100);
	while(1)
	{
	os_mbx_wait (potmail, (void *)&rptr, 0xFFFF);
	rec_val = rptr[0];
	Potentiometer = rec_val;
	PotentioGlobal = Potentiometer;
	if(Potentiometer>=0 && Potentiometer<=204)
	{
		dutyCyclePct = 100;
	}
	else if(Potentiometer>204 && Potentiometer<=408)
	{
		dutyCyclePct = 80;
	}
	else if(Potentiometer>408 && Potentiometer<=612)
	{
		dutyCyclePct = 60;
	}
	else if(Potentiometer>612 && Potentiometer<=816)
	{
		dutyCyclePct = 40;
	}
	else if(Potentiometer>816 && Potentiometer<=1023)
	{
		dutyCyclePct = 20;
	}
	dutyCycle =(unsigned int)((dutyCyclePct*blinkperiod)/100);
	os_evt_set(0x0003,task4_id);
	_free_box (mpoolpot, rptr);
	}
}

__task void task3(void)
{	
	unsigned int dutyRest = 0;
	while(1)
	{
		dutyRest = blinkperiod - dutyCycle;
		write_led();
		os_dly_wait(dutyCycle);
		clear_led();
		os_dly_wait(dutyRest);
	}
}

__task void task4(void)
{	
	char display[10];
	char disStr[20];
	strcpy(disStr,"LIGHT:");
	while(1)
	{
		os_evt_wait_or(0x0003,0xFFFF);
		itoa(PotentioGlobal,display,10);
		strcat(disStr,display);
		os_mut_wait(&mutex1, 0xFFFF);
		LCD_gotoxy(1,2);
		LCD_puts((U8 *)disStr);
		os_mut_release(&mutex1);
		strcpy(disStr,"LIGHT:");
	}
}

__task void task5(void)
{
	short SlideSensor;
	char display1[10];
	char disStr[20];
	U32 *rptr, rec_val;
	strcpy(disStr,"SPEED:");
	_init_box (mpoolslide, sizeof(mpoolslide), sizeof(U32));
	os_mbx_init (slidemail, sizeof(slidemail));
	os_dly_wait(100);
	while(1)
	{
	os_mbx_wait (slidemail, (void *)&rptr, 0xFFFF);
	rec_val = rptr[0];
	SlideSensor = rec_val;
	itoa(SlideSensor,display1,10);
	strcat(disStr,display1);
	LCD_cls();
	os_mut_wait(&mutex1, 0xFFFF);
	LCD_gotoxy(1,1);
	LCD_puts((U8 *)disStr);
	os_mut_release(&mutex1);
	strcpy(disStr,"SPEED:");
	if(SlideSensor>speedLimit)
	{
		os_evt_set (0x0004, task6_id);
	}
	_free_box (mpoolslide, rptr);
	}
}

__task void task6(void)
{
	for(;;)
	{
		os_evt_wait_or(0x0004,0xFFFF);
		write_blink_led();
		os_dly_wait(150);
		clear_blink_led();			
		os_dly_wait(150);
	}
}
					
__task void ENGINE(void)
{
	int prevCase = 0;
	char display[20];
	while(1)
	{
		os_dly_wait(5);
		A1 = !(GPIO3->DR[0x100]>>6);
		switch(A1)
		{
			case 1:
				if(prevCase!=1)
				{
					if(engine == 0)
					{
						engine = 1;
					}
					else if(engine == 1)
					{
						engine = 0;
					}
				}
				prevCase = 1;
				break;
			case 0:
				prevCase = 0;
				break;
		}
		if(engine == 0)
		{
			strcpy(display,"E:OFF");
		}
		else if(engine == 1)
		{
			strcpy(display,"E:ON");
			if(door == 1)
			{
				os_evt_set (0x0002, alarm_id);
			}
		}
		LCD_gotoxy(12,1);
		LCD_puts((U8 *)display);
	}
}

__task void DOOR(void)
{
	int prevCase = 0;
	char display[20];
	while(1)
	{
		os_dly_wait(5);
		A0 = !(GPIO3->DR[0x080]>>5);
		switch(A0)
		{
			case 1:
				if(prevCase!=1)
				{
					if(door == 0)
					{
						door = 1;
					}
					else if(door == 1)
					{
						door = 0;
					}
				}
				prevCase = 1;
				break;
			case 0:
				prevCase = 0;
				break;
		}
		if(door == 0)
		{
			strcpy(display,"D:CL");
		}
		else if(door == 1)
		{
			strcpy(display,"D:OP");
		}
		LCD_gotoxy(12,2);
		LCD_puts((U8 *)display);
	}
}

__task void ALARM(void)
{
	while(1)
	{
			os_evt_wait_or(0x0002,0xFFFF);
			write_blink_led();
			os_dly_wait(150);
			clear_blink_led();			
			os_dly_wait(150);
	}
}

__task void init (void) {
  unsigned int n = 0;
  SCU->GPIOIN[4]  |= 0x07;               
  SCU->GPIOOUT[4] &= 0xFFC0;
  GPIO4->DDR      &= 0xF8;               
  SCU->GPIOANA    |= 0x0007;              
		
	ADC->CR         |= 0x0042;              
  for (n = 0; n < 100000; n ++);         

  ADC->CR         &= 0xFFB7;              
  for (n = 0; n < 1500; n ++);           

  ADC->CR         |= 0x0423;             
  ADC->CCR         = 0x003F;
  VIC0->VAiR[15]  = (unsigned int)ADC_IRQ_Handler; 
  VIC0->VCiR[15] |= 0x20;                 
  VIC0->VCiR[15] |= 15;                  
  VIC0->INTER    |= (1<<15);
  SCU->GPIOOUT[7]  = 0x5555;
  GPIO7->DDR       = 0xFF;
  GPIO7->DR[0x3FC] = 0x00;  
  GPIO8->DDR       = 0xFF;
  GPIO9->DDR       = 0x07; 
  SCU->GPIOIN[3]  |= 0x60;
  SCU->GPIOOUT[3] &= 0xC3FF;
  GPIO3->DDR      &= 0x9F;
  LCD_init(); 
  LCD_cur_off(); 
  LCD_cls();	
	os_mut_init (&mutex1);
	task5_id = os_tsk_create(task5,20);
	task2_id = os_tsk_create(task2,20);
	task4_id = os_tsk_create(task4,20);
	task1_id = os_tsk_create(task1,10);
	task3_id = os_tsk_create(task3,5);
	task6_id = os_tsk_create(task6,5);
	engine_id = os_tsk_create(ENGINE,5);
	door_id = os_tsk_create(DOOR,5);
	alarm_id = os_tsk_create(ALARM,5);
	os_tsk_delete_self ();
}

int main (void) {

  os_sys_init (init);                   
  return 0;
}
