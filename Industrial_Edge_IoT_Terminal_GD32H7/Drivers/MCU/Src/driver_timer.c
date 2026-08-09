/*!
 * 文件名称： driver_timer.c
 * 描    述： timer 驱动文件
 * 版本：     2023-12-03, V1.0
*/

/*
* GD32H757海棠派开发板V1.0
* 聚沃科技官网：https://www.gd32bbs.com/
* B站视频教程：https://space.bilibili.com/475462605
* CSDN文档教程：https://gd32mcu.blog.csdn.net/
* 聚沃开发板购买：https://juwo.taobao.com
* GD技术交流QQ群：859440462
* GD官方开发板、样品购买，咨询Q：87205168  咨询V：13905102619
* GD32 MCU方案移植、方案开发，咨询Q：87205168  咨询V：13905102619
* Copyright@苏州聚沃电子科技有限公司，盗版必究。
*/

#include "driver_timer.h"

//定义示例
//TIMER_CH_DEF(PWM_BEEP,TIMER1,0,TIMER_CH_PWM_HIGH,A,15,AF_PP,GPIO_TIMER1_PARTIAL_REMAP0);
//TIMER_BASE_DEF(TIMER_50US,TIMER5);

/*
*说明 :timer事基配置
*输入[1]              timerx：timer（x=0,1,2,3,4，5,6）
*输入[2]              counter_frequency：计数器频率，配置频率值，比如1K=1000 
*输入[3]              period:配置计数器周期值（重装值） 
*返回值               无
*/
static void driver_timer_base_init(uint32_t timerx,uint32_t counter_frequency,uint16_t period)
{
    timer_parameter_struct  timer_initpara;
    
    SystemCoreClockUpdate();
           
    /* initialize timer0 parameters */
    timer_initpara.prescaler         = SystemCoreClock/2/counter_frequency-1;     /* the prescaler of timer clock: (0+1) */ 
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;       /* Counter aligned mode: center up */
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;              /* Counter direction*/
    timer_initpara.period            = period-1;            /* Inveter PWM carrier frequnece*/
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV4;                /* Timer0 clock division: Deadtimer Clock = Timer clock  */
    timer_initpara.repetitioncounter = 0;                               /* Timer0_CREP: */
    timer_init(timerx, &timer_initpara);                                /* initialize timer0 */
    timer_auto_reload_shadow_enable(timerx);                /* Timer0 auto-reload shadow register */
}


/*
*说明 :timer定时中断配置
*输入[1]              timerx：tiemrx结构体指针
*输入[2]              counter_frequency：计数器频率，配置频率值，比如1K=1000 
*输入[3]              period:配置计数器周期值（重装值） 
*返回值               无
*/
void driver_timer_base_int_init(typdef_timer_base_struct* timerx,uint32_t counter_frequency,uint16_t period)
{
    
    SystemCoreClockUpdate();
    
    rcu_periph_clock_enable(timerx->rcu_timerx);
    
    timer_deinit(timerx->timer_x);
           
    driver_timer_base_init(timerx->timer_x,counter_frequency,period); 

    timer_flag_clear(timerx->timer_x,TIMER_FLAG_UP);
    timer_interrupt_enable(timerx->timer_x,TIMER_INT_UP);
    
    timer_enable(timerx->timer_x);
}

/*
*说明 :timer多通道配置
*输入[1]              timerx_cha：a通道结构体指针
*输入[2]              timerx_chb：b通道结构体指针
*输入[3]              timerx_chc：c通道结构体指针
*输入[4]              timerx_chd：d通道结构体指针
*输入[5]              counter_frequency：计数器频率，配置频率值，比如1K=1000 
*输入[6]              period:配置计数器周期值（重装值） 
*返回值               无
*/
void driver_timer_multi_channel_init(typdef_timer_ch_struct* timerx_cha,typdef_timer_ch_struct* timerx_chb,typdef_timer_ch_struct* timerx_chc,typdef_timer_ch_struct* timerx_chd,uint32_t counter_frequency,uint16_t period)
{
    timer_oc_parameter_struct timer_ocintpara;
    timer_ic_parameter_struct timer_icinitpara;
    typdef_timer_ch_struct* timer_chx[4];
    
    timer_chx[0]=timerx_cha;
    timer_chx[1]=timerx_chb;
    timer_chx[2]=timerx_chc;
    timer_chx[3]=timerx_chd;
     
    for(uint8_t i=0;i<4;i++)
    {
        if(timer_chx[i]!=NULL){
            rcu_periph_clock_enable(timer_chx[i]->rcu_timerx);            
            timer_deinit(timer_chx[i]->timer_x);
            driver_gpio_general_init(timer_chx[i]->timer_ch_gpio); 
            driver_timer_base_init(timer_chx[i]->timer_x,counter_frequency,period);            
        }
    }
   
    for(uint8_t i=0;i<4;i++)
    {
        if(timer_chx[i]!=NULL){    
            if(timer_chx[i]->timer_ch_mode==TIMER_CH_PWM_HIGH || timer_chx[i]->timer_ch_mode==TIMER_CH_PWM_LOW)
            {
                timer_channel_output_struct_para_init(&timer_ocintpara);
                timer_ocintpara.outputstate  = TIMER_CCX_DISABLE;                   
                if(timer_chx[i]->timer_ch_mode==TIMER_CH_PWM_HIGH){
                    timer_ocintpara.ocpolarity   = TIMER_OC_POLARITY_HIGH;             
                }else{
                    timer_ocintpara.ocpolarity   = TIMER_OC_POLARITY_LOW;                          
                }
                
                timer_channel_output_config(timer_chx[i]->timer_x, timer_chx[i]->ch_x, &timer_ocintpara);  /* PFC_TIMER channle 2 output mode */
                timer_channel_output_pulse_value_config(timer_chx[i]->timer_x, timer_chx[i]->ch_x, 0);   /* initialze channel 0 compare register (CH0CV) */
                timer_channel_output_mode_config(timer_chx[i]->timer_x, timer_chx[i]->ch_x, TIMER_OC_MODE_PWM0);   /* channnel 0 mode: PWM1 */
                /* PFC_TIMER channel 2 shadow register enable */
                timer_channel_output_shadow_config(timer_chx[i]->timer_x, timer_chx[i]->ch_x, TIMER_OC_SHADOW_ENABLE); 
                if(timer_chx[i]->timer_x==TIMER0 || timer_chx[i]->timer_x ==TIMER7){
                    timer_primary_output_config(timer_chx[i]->timer_x, ENABLE);                
                }
            }else if(timer_chx[i]->timer_ch_mode==TIMER_CH_INPUT_CAPTURE_LOW || timer_chx[i]->timer_ch_mode==TIMER_CH_INPUT_CAPTURE_HIGH)
            {        
                timer_channel_input_struct_para_init(&timer_icinitpara);
                /* TIMER2 configuration */
                /* TIMER2 CH0 PWM input capture configuration */
                /* TIMER2  configuration */
                /* TIMER2 CH0 input capture configuration */
                timer_icinitpara.icpolarity  = TIMER_IC_POLARITY_RISING;
                timer_icinitpara.icselection = TIMER_IC_SELECTION_DIRECTTI;
                timer_icinitpara.icprescaler = TIMER_IC_PSC_DIV1;
                timer_icinitpara.icfilter    = 0x0;
                timer_input_capture_config(timer_chx[i]->timer_x,timer_chx[i]->ch_x,&timer_icinitpara);

                /* clear channel 0 interrupt bit */
                timer_interrupt_flag_clear(timer_chx[i]->timer_x,timer_chx[i]->TIMER_FLAG_CHx);
                /* channel 0 interrupt enable */
                timer_interrupt_enable(timer_chx[i]->timer_x,timer_chx[i]->TIMER_INT_CHx); 
                
            }else if(timer_chx[i]->timer_ch_mode==TIMER_CH_INPUT_CAPTURE_PWM || timer_chx[i]->ch_x==TIMER_CH_0 || timer_chx[i]->ch_x==TIMER_CH_1)
            {        
                timer_channel_input_struct_para_init(&timer_icinitpara);
                /* TIMER2 configuration */
                /* TIMER2 CH0 PWM input capture configuration */
                timer_icinitpara.icpolarity  = TIMER_IC_POLARITY_RISING;
                timer_icinitpara.icselection = TIMER_IC_SELECTION_DIRECTTI;
                timer_icinitpara.icfilter    = 0x0;
                timer_input_pwm_capture_config(timer_chx[i]->timer_x,timer_chx[i]->ch_x,&timer_icinitpara);
                
                /* slave mode selection: TIMER2 */
                timer_input_trigger_source_select(timer_chx[i]->timer_x,TIMER_SMCFG_TRGSEL_CI0FE0);
                timer_slave_mode_select(timer_chx[i]->timer_x,TIMER_SLAVE_MODE_RESTART);

                /* select the master slave mode */
                timer_master_slave_mode_config(timer_chx[i]->timer_x,TIMER_MASTER_SLAVE_MODE_ENABLE);

                /* clear channel 0 interrupt bit */
                timer_interrupt_flag_clear(timer_chx[i]->timer_x,timer_chx[i]->TIMER_FLAG_CHx);
                /* channel 0 interrupt enable */
                timer_interrupt_enable(timer_chx[i]->timer_x,timer_chx[i]->TIMER_INT_CHx);                    

            } 
        }
    } 
    for(uint8_t i=0;i<4;i++)
    {
        if(timer_chx[i]!=NULL){      
            timer_enable(timer_chx[i]->timer_x);
        }
    }
}

/*
*说明 :timer通道配置
*输入[1]              timerx_ch： 通道结构体指针
*输入[2]              counter_frequency：计数器频率，配置频率值，比如1K=1000 
*输入[3]              period:配置计数器周期值（重装值） 
*返回值               无
*/
void driver_timer_channel_init(typdef_timer_ch_struct* timerx_ch,uint32_t counter_frequency,uint16_t period)
{
    timer_oc_parameter_struct timer_ocintpara;
    timer_ic_parameter_struct timer_icinitpara;
    static __IO uint8_t flag = 0;
    
    rcu_periph_clock_enable(timerx_ch->rcu_timerx);
    
    flag++;    
    if(flag==0){
        timer_deinit(timerx_ch->timer_x);  
    }

    driver_gpio_general_init(timerx_ch->timer_ch_gpio);    
    
    driver_timer_base_init(timerx_ch->timer_x,counter_frequency,period);  
    
    if(timerx_ch->timer_ch_mode==TIMER_CH_PWM_HIGH || timerx_ch->timer_ch_mode==TIMER_CH_PWM_LOW)
    {
        timer_channel_output_struct_para_init(&timer_ocintpara);
        timer_ocintpara.outputstate  = TIMER_CCX_DISABLE;                   
        if(timerx_ch->timer_ch_mode==TIMER_CH_PWM_HIGH){
            timer_ocintpara.ocpolarity   = TIMER_OC_POLARITY_HIGH;             
        }else{
            timer_ocintpara.ocpolarity   = TIMER_OC_POLARITY_LOW;                          
        }
        
        timer_channel_output_config(timerx_ch->timer_x, timerx_ch->ch_x, &timer_ocintpara);  /* PFC_TIMER channle 2 output mode */
        timer_channel_output_pulse_value_config(timerx_ch->timer_x, timerx_ch->ch_x, 0);   /* initialze channel 0 compare register (CH0CV) */
        timer_channel_output_mode_config(timerx_ch->timer_x, timerx_ch->ch_x, TIMER_OC_MODE_PWM0);   /* channnel 0 mode: PWM1 */
        /* PFC_TIMER channel 2 shadow register enable */
        timer_channel_output_shadow_config(timerx_ch->timer_x, timerx_ch->ch_x, TIMER_OC_SHADOW_ENABLE); 
        if(timerx_ch->timer_x==TIMER0 || timerx_ch->timer_x ==TIMER7){
            timer_primary_output_config(timerx_ch->timer_x, ENABLE);                
        }
    }else if(timerx_ch->timer_ch_mode==TIMER_CH_INPUT_CAPTURE_LOW || timerx_ch->timer_ch_mode==TIMER_CH_INPUT_CAPTURE_HIGH)
    {        
        timer_channel_input_struct_para_init(&timer_icinitpara);
        /* TIMER2 configuration */
        /* TIMER2 CH0 PWM input capture configuration */
        /* TIMER2  configuration */
        /* TIMER2 CH0 input capture configuration */
        timer_icinitpara.icpolarity  = TIMER_IC_POLARITY_RISING;
        timer_icinitpara.icselection = TIMER_IC_SELECTION_DIRECTTI;
        timer_icinitpara.icprescaler = TIMER_IC_PSC_DIV1;
        timer_icinitpara.icfilter    = 0x0;
        timer_input_capture_config(timerx_ch->timer_x,timerx_ch->ch_x,&timer_icinitpara);

        /* clear channel 0 interrupt bit */
        timer_interrupt_flag_clear(timerx_ch->timer_x,timerx_ch->TIMER_FLAG_CHx);
        /* channel 0 interrupt enable */
        timer_interrupt_enable(timerx_ch->timer_x,timerx_ch->TIMER_INT_CHx); 
        
    }else if(timerx_ch->timer_ch_mode==TIMER_CH_INPUT_CAPTURE_PWM || timerx_ch->ch_x==TIMER_CH_0 || timerx_ch->ch_x==TIMER_CH_1)
    {        
        timer_channel_input_struct_para_init(&timer_icinitpara);
        /* TIMER2 configuration */
        /* TIMER2 CH0 PWM input capture configuration */
        timer_icinitpara.icpolarity  = TIMER_IC_POLARITY_RISING;
        timer_icinitpara.icselection = TIMER_IC_SELECTION_DIRECTTI;
        timer_icinitpara.icfilter    = 0x0;
        timer_input_pwm_capture_config(timerx_ch->timer_x,timerx_ch->ch_x,&timer_icinitpara);
        
        /* slave mode selection: TIMER2 */
        timer_input_trigger_source_select(timerx_ch->timer_x,TIMER_SMCFG_TRGSEL_CI0FE0);
        timer_slave_mode_select(timerx_ch->timer_x,TIMER_SLAVE_MODE_RESTART);

        /* select the master slave mode */
        timer_master_slave_mode_config(timerx_ch->timer_x,TIMER_MASTER_SLAVE_MODE_ENABLE);

        /* clear channel 0 interrupt bit */
        timer_interrupt_flag_clear(timerx_ch->timer_x,timerx_ch->TIMER_FLAG_CHx);
        /* channel 0 interrupt enable */
        timer_interrupt_enable(timerx_ch->timer_x,timerx_ch->TIMER_INT_CHx);                    

    }   
    timer_enable(timerx_ch->timer_x);
}

/*
*说明 :timer pwm占空比调节
*输入[1]              timerx_ch： 通道结构体指针
*输入[2]              duty_value:占空比：0——100
*返回值               无
*/
void driver_timer_pwm_duty_set(typdef_timer_ch_struct* timerx_ch,uint16_t duty_value)
{
    uint32_t chxcv_reg=0;
    timer_update_event_disable(timerx_ch->timer_x);     
    chxcv_reg=(TIMER_CAR(timerx_ch->timer_x)+1)*duty_value/100;    
    timer_channel_output_pulse_value_config(timerx_ch->timer_x, timerx_ch->ch_x, chxcv_reg);
    timer_update_event_enable(timerx_ch->timer_x);     
}

/*
*说明 :timer pwm周期调节
*输入[1]              timerx_ch： 通道结构体指针
*输入[2]              period:周期值：0——0xffff
*返回值               无
*/
void driver_timer_pwm_period_set(typdef_timer_ch_struct* timerx_ch,uint16_t period)
{   
    timer_update_event_disable(timerx_ch->timer_x);     
    TIMER_CAR(timerx_ch->timer_x)=period-1;    
    timer_update_event_enable(timerx_ch->timer_x);     
}

/*
*说明 :timer pwm周期、占空比同步调节
*输入[1]              timerx_ch： 通道结构体指针
*输入[2]              period:周期值：0——0xffff
*输入[3]              duty_value:占空比：0——100
*返回值               无
*/
void driver_timer_pwm_period_duty_set(typdef_timer_ch_struct* timerx_ch,uint16_t period,uint16_t duty_value)
{  
    uint32_t chxcv_reg=0;
    
    chxcv_reg=period/100*duty_value;  
   
    timer_update_event_disable(timerx_ch->timer_x);   
    
    TIMER_CAR(timerx_ch->timer_x)=period; 
    
    timer_channel_output_pulse_value_config(timerx_ch->timer_x, timerx_ch->ch_x, chxcv_reg); 
    
    timer_update_event_enable(timerx_ch->timer_x);    
}

/*
*说明 :timer pwm通道开
*输入[1]              timerx_ch： 通道结构体指针
*返回值               无
*/
void driver_timer_pwm_on(typdef_timer_ch_struct* timerx_ch)
{
    timer_channel_output_state_config(timerx_ch->timer_x, timerx_ch->ch_x,TIMER_CCX_ENABLE);
}

/*
*说明 :timer pwm通道关
*输入[1]              timerx_ch： 通道结构体指针
*返回值               无
*/
void driver_timer_pwm_off(typdef_timer_ch_struct* timerx_ch)
{
    timer_channel_output_state_config(timerx_ch->timer_x, timerx_ch->ch_x,TIMER_CCX_DISABLE);
}

/*
*说明 :timer 定时中断处理
*输入[1]              timerx_ch： 通道结构体指针
*返回值               无
*/
void driver_timer_updata_int_handler(typdef_timer_base_struct* timerx)
{
    if(timer_interrupt_flag_get(timerx->timer_x,TIMER_FLAG_UP)!=RESET)
    {
        timer_interrupt_flag_clear(timerx->timer_x,TIMER_FLAG_UP);
        
        if(timerx->timer_updata_callback!=NULL)
        {
            timerx->timer_updata_callback();
        }
        
    }
}

/*
*说明 :timer 通道（捕获、比较）处理函数
*输入[1]      timerx_ch： 通道结构体指针
*返回值       无
*/
void driver_timer_channel_int_handler(typdef_timer_ch_struct* timerx_ch)
{
    if(timer_interrupt_flag_get(timerx_ch->timer_x,timerx_ch->TIMER_FLAG_CHx)!=RESET)
    {
        timer_interrupt_flag_clear(timerx_ch->timer_x,timerx_ch->TIMER_FLAG_CHx);
        if(timerx_ch->timer_ch_callback!=NULL)
        {
            timerx_ch->timer_ch_callback(timerx_ch);
        }        
        
    }
}

