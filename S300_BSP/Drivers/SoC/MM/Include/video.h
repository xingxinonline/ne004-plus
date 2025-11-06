#ifndef _VIDEO_H_
#define _VIDEO_H_

#include <stdint.h>
#include "s300.h"

/* 兼容旧代码中使用的基本类型与寄存器访问宏 */
#ifndef UINT32
typedef uint32_t UINT32;
#endif
#ifndef UINT8
typedef uint8_t UINT8;
#endif
#ifndef REG32
#define REG32(addr) (*(volatile uint32_t *)(uintptr_t)(addr))
#endif

#define RD_SOURCE_FRAME_START_X (0)
#define RD_SOURCE_FRAME_START_Y (0)

#define BINNING_SIZE            (1) //binning = 4 @ Sensor image size (1280*1920) 尽量binning到足够小

/* binning */
#define BINNING_IMAGE_WIDTH         (288)
#define BINNING_IMAGE_HEIGHT        (360)
/* sensor */
#define SENSOR_IMAGE_WIDTH          (BINNING_IMAGE_WIDTH * (1U << BINNING_SIZE))
#define SENSOR_IMAGE_HEIGHT         (BINNING_IMAGE_HEIGHT * (1U << BINNING_SIZE))
/* downscale */
#define DOWNSCALE_IMAGE_WIDTH       (240)
#define DOWNSCALE_IMAGE_HEIGHT      (320)
/* display  */
#define DISP_START_X                (0)
#define DISP_START_Y                (0)
#define DISP_IMAGE_WIDTH            (240)
#define DISP_IMAGE_HEIGHT           (320)
/* snap */
#define SNAP_IMAGE_WIDTH            (240)
#define SNAP_IMAGE_HEIGHT           (320)

#define BINNING_LINE_MAX_SIZE       (1280)
#define DOWNSCALE_FACTOR            (8192)

// #define OFFLINE_IMAGE_BASE_ADDRESS  (0x44000000)

#define DISP_WFRAME0_ADDR           (0x80000000U)//0x80000000;
#define DISP_WFRAME1_ADDR           (0x80000000U)//0x80100000;
#define DISP_RFRAME0_ADDR           (0x80000000U + (SNAP_IMAGE_WIDTH * SNAP_IMAGE_HEIGHT * 2))//0x44035000;//0x80200000
#define DISP_RFRAME1_ADDR           (0x80000000U + (SNAP_IMAGE_WIDTH * SNAP_IMAGE_HEIGHT * 4))//0x44035000;//0x80300000;
#define DISP_RALPHA0_ADDR           (0x80000000U + (DISP_IMAGE_WIDTH * DISP_IMAGE_HEIGHT * 6))//0x44060000;//0x80250000;
#define DISP_RALPHA1_ADDR           (0x80000000U + (DISP_IMAGE_WIDTH * DISP_IMAGE_HEIGHT * 6))//0x44060000;//0x80350000;

// 定义结构体
typedef struct
{
    int width;
    int height;
} ImageDimensions;

typedef struct
{
    float width;
    float height;
} DownscaleRatio;

typedef struct
{
    int x;
    int y;
} CropCoordinates;

// 定义枚举类型用于裁剪方式
typedef enum
{
    CROP_CENTER, // 裁剪中心
    CROP_TOP_LEFT,  // 裁剪左上角
    CROP_BOTTOM_RIGHT  // 裁剪右下角
} CropType;

/* *
 *	1.config the clock for PLL
 *	int init_mm_pll(stPLLPRO pro);
 *    int init_dsp_pll(stPLLPRO pro);
 *    set_mm_clock_enable(emBoolean en);
 *
 *	2. Configure the i2c function，Refer to the programming manual for specific serial port pins, see i2c.h
 *	 make sure the define of SLV_ADDR in ov5640.h,the SLV_ADDR is address for i2c extern device,there is the camera 
 *
 * 	3.Configure the gpio function，Refer to the programming manual for specific serial port pins, see gpio.h
 *		a.make sure the pin in  for CAMREA_RESETB and  CAMREA_PWDN to control the camera 
 *		b.Define initialization variables, gpio ports, pin numbers, and reuse function arrays 
 *		emGPIO portGPIO[] = {GPIOA,GPIOA};
 *		UINT8 pinGPIO[] = {12,13};// 12:CAMREA_PWDN ,13:CAMREA_RESETB
 *		emGPIOFUNC gpiofunc[] = {FUNCTION_2,FUNCTION_2};
 *		c. Example Initialize the gpio function
 *         	for(i = 0;i < sizeof(pinGPIO);i++)
 *             set_gpio_function(portGPIO[i],pinGPIO[i],gpiofunc[i]);
 *  
 *	4.init the externdevice of camera,there is a sample for ov5640
 *		init_camera(EM_DVP,EM_I2C0,SLV_ADDR,portGPIO,pinGPIO, CAMREA_YUV422)
 *	5.init the video
 *		void init_video(EM_DVP,CAMREA_YUV422,C1080X720P);
 *  6.Developers can develop according to demand
*/
typedef enum _DVP_{
    EM_DVP  = 0x0000
}emDVP;

typedef enum _camera_format_description_{
    CAMREA_RGB565  = 0x0000,
    CAMREA_YUV422  = 0x0001
}emCameraFormatPro;

typedef enum _mm_{
    EM_MM0 = 0x0000
}emMM;

typedef enum _mm_video_config_{
    C1080X720P      = 0x1,
    BIG_PIC_MODE    = 0x2,
    USE_DVP_CLK     = 0x4
}emMMProcessPro;



void init_video(emDVP dvp,emCameraFormatPro formatPro,emMMProcessPro mmPro);
void init_mm_subsystem(emMM mm,emCameraFormatPro cameraPro,emMMProcessPro mmPro);
void init_mm_subsystem_small(emMM mm,emCameraFormatPro cameraPro,emMMProcessPro mmPro);
void delay_ms(UINT32 d);
void init_high_camera_st77_lcd(emMM mm,emCameraFormatPro cameraPro,emMMProcessPro mmPro);

#endif //_VIDEO_H_