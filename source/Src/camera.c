#include "camera.h"
#include "i2c.h"
#include "usart.h"


IMG_State img_state=IMG_Start;

extern DMA_HandleTypeDef hdma_tim8_ch4_trig_com;

uint8_t camera_buffer[CAMERA_H*CAMERA_W/8];

reg_s ov7725_eagle_reg[] =
{
    //寄存器，寄存器值
    {OV7725_COM4         , 0xC1},
    {OV7725_CLKRC        , 0x02},   //50帧，如果要150帧，将02改成00
    {OV7725_COM2         , 0x03},
    {OV7725_COM3         , 0xD0},
    {OV7725_COM7         , 0x40},
    {OV7725_COM10        , 0x20},
    {OV7725_HSTART       , 0x3F},
    {OV7725_HSIZE        , 0x50},
    {OV7725_VSTRT        , 0x03},
    {OV7725_VSIZE        , 0x78},
    {OV7725_HREF         , 0x00},
    {OV7725_SCAL0        , 0x0A},
    {OV7725_AWB_Ctrl0    , 0xE0},
    {OV7725_DSPAuto      , 0xff},
    {OV7725_DSP_Ctrl2    , 0x0C},
    {OV7725_DSP_Ctrl3    , 0x00},
    {OV7725_DSP_Ctrl4    , 0x00},

#if (CAMERA_W == 80)
    {OV7725_HOutSize     , 0x14},
#elif (CAMERA_W == 160)
    {OV7725_HOutSize     , 0x28},
#elif (CAMERA_W == 240)
    {OV7725_HOutSize     , 0x3c},
#elif (CAMERA_W == 320)
    {OV7725_HOutSize     , 0x50},
#else

#endif

#if (CAMERA_H == 60 )
    {OV7725_VOutSize     , 0x1E},
#elif (CAMERA_H == 120 )
    {OV7725_VOutSize     , 0x3c},
#elif (CAMERA_H == 180 )
    {OV7725_VOutSize     , 0x5a},
#elif (CAMERA_H == 240 )
    {OV7725_VOutSize     , 0x78},
#else

#endif

    {OV7725_EXHCH        , 0x00},
    {OV7725_GAM1         , 0x0c},
    {OV7725_GAM2         , 0x16},
    {OV7725_GAM3         , 0x2a},
    {OV7725_GAM4         , 0x4e},
    {OV7725_GAM5         , 0x61},
    {OV7725_GAM6         , 0x6f},
    {OV7725_GAM7         , 0x7b},
    {OV7725_GAM8         , 0x86},
    {OV7725_GAM9         , 0x8e},
    {OV7725_GAM10        , 0x97},
    {OV7725_GAM11        , 0xa4},
    {OV7725_GAM12        , 0xaf},
    {OV7725_GAM13        , 0xc5},
    {OV7725_GAM14        , 0xd7},
    {OV7725_GAM15        , 0xe8},
    {OV7725_SLOP         , 0x20},
    {OV7725_LC_RADI      , 0x00},
    {OV7725_LC_COEF      , 0x13},
    {OV7725_LC_XC        , 0x08},
    {OV7725_LC_COEFB     , 0x14},
    {OV7725_LC_COEFR     , 0x17},
    {OV7725_LC_CTR       , 0x05},
    {OV7725_BDBase       , 0x99},
    {OV7725_BDMStep      , 0x03},
    {OV7725_SDE          , 0x04},
    {OV7725_BRIGHT       , 0x00},
    {OV7725_CNST         , 0xFF},
    {OV7725_SIGN         , 0x06},
    {OV7725_UVADJ0       , 0x11},
    {OV7725_UVADJ1       , 0x02},

};

extern DMA_HandleTypeDef hdma_memtomem_dma2_stream0;
extern TIM_HandleTypeDef htim8;
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
  TIM_HandleTypeDef * htim=&htim8;
  if(img_state==IMG_Start){
    if(GPIO_Pin==GPIO_PIN_8){
      HAL_NVIC_DisableIRQ(EXTI9_5_IRQn);
      HAL_DMA_Start_IT(&hdma_tim8_ch4_trig_com,(uint32_t)&(GPIOD->IDR),(uint32_t)camera_buffer,sizeof(camera_buffer));
      __HAL_TIM_ENABLE_DMA(htim, TIM_DMA_CC4);
    }
  }
}

uint8_t sccb_write_byte(uint8_t address,uint8_t data){
  return I2C_Write_Buffer(OV7725_ADDRESS,address,&data,1);
}
  
uint8_t sccb_read_byte(uint8_t reg){
  uint8_t data=0;
  I2C_Read_Buffer(OV7725_ADDRESS,reg,&data,1);
  return data;
}

void camera_init(){
  int cfg_size;
  uint8_t id;
  uint8_t result=0xf1;
  result=sccb_write_byte(OV7725_COM7,0x80);
  uprintf("%d\r\n",result);
  HAL_Delay(50);
  id=sccb_read_byte(OV7725_VER);
  if(id!=OV7725_ID){
    //error!
    uprintf("error read camera id:%d!\r\n",id);
    return ;
  }
  
  cfg_size=sizeof(ov7725_eagle_reg)/sizeof(reg_s);
  
  for(int i=0;i<cfg_size;++i){
    result=sccb_write_byte(ov7725_eagle_reg[i].addr,ov7725_eagle_reg[i].val);
    if(sccb_read_byte(ov7725_eagle_reg[i].addr)!=ov7725_eagle_reg[i].val){
      uprintf("error!\r\n");
    }
  }
  uprintf("successful write camera regs!\r\n");
}

void IMG_OK_Callback(DMA_HandleTypeDef *_hdma){
  img_state=IMG_OK;
  __HAL_TIM_DISABLE_DMA(&htim8, TIM_DMA_CC4);  
  //震惊，没加这一句的话，右边的图像会跑的左边
  //令人费解，因为已经设置了DMA模式为NORMAL，应该传输完后就停止了阿
}

void vcan_sendimg(void *imgaddr, uint32_t imgsize)
{
    uint8_t cmdf[2] = {0x01, 0xFE};    //山外上位机 使用的命令
    uint8_t cmdr[2] = {0xFE, 0x01};    //山外上位机 使用的命令

    HAL_UART_Transmit(&huart2,cmdf, sizeof(cmdf),1000);
    HAL_UART_Transmit(&huart2,(uint8_t *)imgaddr, imgsize,1000);
    HAL_UART_Transmit(&huart2,cmdr, sizeof(cmdr),1000);
}

void get_img(){
  img_state=IMG_Start;
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
}