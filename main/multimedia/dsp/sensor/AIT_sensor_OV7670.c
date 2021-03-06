#include	"A8_common.h"
#include	"AIT700_EBIbus.h"
#include	"A8_sensor.h"
#include "A8_vif.h"

#if 1//defined(SENSOR_OV7670)
#define	SENSOR_ID					(0x7673)
#define	SENSOR_I2C_TYPE				(A8_I2C_1A1D)
#define SENSOR_PREVIEW_VH_POLARITY  (A8_SENSOR_HOR_POS|A8_SENSOR_VER_NEG)
#define SENSOR_FULL_VH_POLARITY     (A8_SENSOR_HOR_POS|A8_SENSOR_VER_NEG)
#define SENSOR_PREVIEW_LATCH_COUNT  (A8_PHASE_COUNTER_NUM_2)
#define SENSOR_FULL_LATCH_COUNT     (A8_PHASE_COUNTER_NUM_2)
#define SENSOR_PREVIEW_YUV_FORMAT   (A8_SENSOR_YCBYCR) //(A8_SENSOR_CBYCRY)
#define SENSOR_FULL_YUV_FORMAT      (A8_SENSOR_YCBYCR) //(A8_SENSOR_CBYCRY)
#define SENSOR_LATCH_EDGE           (A8_SENSOR_POS_EDGE)

#define	SENSOR_I2C_ID  			    (0x42)
#define	SENSOR_PREVIEW_WIDTH  	    (640)
#define	SENSOR_PREVIEW_HEIGHT  	    (480)
#define	SENSOR_FULL_WIDTH  		    (640)
#define	SENSOR_FULL_HEIGHT  	    (480)

static t_sensor_mode_attr	full_mode;
static t_sensor_mode_attr	preview_mode;

extern	u_char	gbSensorInited;
__align(2)const static u_char	Sensor_Init[]=
{
	0x11, 0x80,// double PLL
	0x3a, 0x04,// Y-CB-Y-CR
	0x12, 0x00,
	0x17, 0x13,
	0x18, 0x01,
	0x32, 0xb6,
	0x19, 0x02,
	0x1a, 0x7a,
	0x03, 0x0a,
	0x0c, 0x00,
	0x3e, 0x00,
	0x70, 0x3a,
	0x71, 0x35,
	0x72, 0x11,
	0x73, 0xf0,
	0xa2, 0x02,
	0x7a, 0x20,
	0x7b, 0x1c,
	0x7c, 0x28,
	0x7d, 0x3c,
	0x7e, 0x5a,
	0x7f, 0x68,
	0x80, 0x76,
	0x81, 0x80,
	0x82, 0x88,
	0x83, 0x8f,
	0x84, 0x96,
	0x85, 0xa3,
	0x86, 0xaf,
	0x87, 0xc4,
	0x88, 0xd7,
	0x89, 0xe8,
	0x13, 0xe0,
	0x01, 0x80,
	0x02, 0x80,
	0x00, 0x00,
	0x10, 0x00,
	0x0d, 0x40,
	0x14, 0x18,  
	0xa5, 0x05,
	0xab, 0x07,
	0x24, 0x78,
	0x25, 0x68,
	0x26, 0xc3,
	0x55, 0x00,
	0x9f, 0xA0,
	0xa0, 0x90,
	0xa1, 0x0b,
	0xa6, 0xd8,
	0xa7, 0xd8,
	0xa8, 0xf0,
	0xa9, 0x90,
	0xaa, 0x14,//0x94, can not brightness
	0x13, 0xe5,
	0x0e, 0x61,
	0x0f, 0x4b,
	0x16, 0x02,
	0x1e, 0x07, 
	0x21, 0x02,
	0x22, 0x91,
	0x29, 0x07,
	0x33, 0x0b, 
	0x35, 0x0b,
	0x37, 0x1d,
	0x38, 0x71,
	0x39, 0x2a, 
	0x3c, 0x78,
	0x4d, 0x40,// stable
	0x4e, 0x20,
	0x69, 0x00,
	0x6b, 0x0a,
	0x74, 0x19,
	0x8d, 0x4f,
	0x8e, 0x00,
	0x8f, 0x00,
	0x90, 0x00,
	0x91, 0x00,
	0x92, 0x00,//0x19,
	0x96, 0x00,
	0x9a, 0x80,
	0xb0, 0x84, 
	0xb1, 0x0c,
	0xb2, 0x0e,
	0xb3, 0x7c,
	0xb8, 0x0a,
	0x43, 0x14,
	0x44, 0xf0,
	0x45, 0x34,
	0x46, 0x58,
	0x47, 0x28,
	0x48, 0x3a,
	0x59, 0x88,
	0x5a, 0x88,
	0x5b, 0x44,
	0x5c, 0x67,
	0x5d, 0x49,
	0x5e, 0x0e,
	0x64, 0x04,
	0x65, 0x20,
	0x66, 0x05,
	0x94, 0x04,
	0x95, 0x08,
	0x6c, 0x0a,
	0x6d, 0x55,
	0x6e, 0x11,
	0x6f, 0x9f, //;9e for advance AWB
	0x6a, 0x40,
	0x13, 0xe7,
	0x4f, 0x80,
	0x50, 0x80,
	0x51, 0x00,
	0x52, 0x22,
	0x53, 0x5e,
	0x54, 0x80,
	0x58, 0x9e,
	0x41, 0x18,//0x08,
	0x3f, 0x02,//0x00,
	0x75, 0x04,
	0x76, 0xe1,
	0x4c, 0x00,
	0x77, 0x01,
	0x3d, 0xc0, // Y-CB-Y-CR
	0x4b, 0x09,
	0xc9, 0x60,
	0x41, 0x38,
	0x56, 0x40,
	0x34, 0x11,
	0x3b, 0x0a,//60HZ:0x02,//50HZ:0x0a,  // night mode:0xCA
	0xa4, 0x88,
	0x96, 0x00,
	0x97, 0x30,
	0x98, 0x20,
	0x99, 0x30,
	0x9a, 0x84,
	0x9b, 0x29,
	0x9c, 0x03,
	0x9d, 0x4D,//0x52,
	0x9e, 0x40,//0x44,
	0x78, 0x04,
	0x79, 0x01,
	0xc8, 0xf0,
	0x79, 0x0f,
	0xc8, 0x00,
	0x79, 0x10,
	0xc8, 0x7e,
	0x79, 0x0a,
	0xc8, 0x80,
	0x79, 0x0b,
	0xc8, 0x01,
	0x79, 0x0c,
	0xc8, 0x0f,
	0x79, 0x0d,
	0xc8, 0x20,
	0x79, 0x09,
	0xc8, 0x80,
	0x79, 0x02,
	0xc8, 0xc0,
	0x79, 0x03,
	0xc8, 0x40,
	0x79, 0x05,
	0xc8, 0x30,
	0x79, 0x26,
	       
	#if 0  
	// for 15fps under 52/3 MHZ
	0x2d, 0x00, 
	0x2e, 0x00,
	0x2a, 0x10,//0x40,//0x10, 
	0x2b, 0x5C,//0x84,//0x10,
	#endif
	#if 0  
	// for 15fps under 19.2 MHZ
	0x2d, 0x00, 
	0x2e, 0x00,
	0x2a, 0x10, 
	0x2b, 0xD7,
	#endif 
	#if 1
	// for 15fps under 52/3 MHZ
	0x2d, 0x54, 
	0x2e, 0x02,
	0x2a, 0x00,//0x40,//0x10, 
	0x2b, 0x00,//0x84,//0x10,	
	#endif
	
	0xFF, 0xFF,
};

__align(2)const static u_char	Sensor_DSC_Preview[] =
{
NULL
};
__align(2)const static u_char	Sensor_Video_Preview[]=
{
NULL
};
__align(2)const static u_char	Sensor_Normal_Capture[] = 
{
NULL
};
__align(2)const static u_char  Sensor_Full_Capture[] = 
{
NULL,
};
__align(2)const static u_char Sensor_Full_Capture_ScaleUp[] =
{
NULL,
};
__align(2)const static u_char  SENSOR_EFFECT_NORMAL[] = 
{	//Normal
	0x3A, 0x04,
	0x67, 0x80,
	0x68, 0x80,
};
__align(2)const static u_char  SENSOR_EFFECT_1[] = 
{	//BW
	0x3A, 0x14,
	0x67, 0xC0,
	0x68, 0x80,
};
__align(2)const static u_char  SENSOR_EFFECT_2[] = 
{	// Bluish 2
	0x3A, 0x24,
	0x67, 0x80,
	0x68, 0x80,
};
__align(2)const static u_char  SENSOR_EFFECT_3[] = 
{	// Negative 3
	0x3A, 0x14,
	0x67, 0x80,
	0x68, 0xC0,
};
__align(2)const static u_char  SENSOR_EFFECT_4[] = 
{	// Reddish 4
	0x3A, 0x14,
	0x67, 0x80,
	0x68, 0x80,
};
__align(2)const static u_char  SENSOR_EFFECT_5[] = 
{	// Antique 5
	0x3A, 0x14,
	0x67, 0xA0,
	0x68, 0x40,
};
__align(2)const static u_char  SENSOR_EFFECT_6[] = 
{	// Greenish 6
	0x3A, 0x14,
	0x67, 0x40,
	0x68, 0x40,
};
__align(2)const static u_char  SENSOR_EFFECT_7[] = 
{
NULL
};

__align(2)const static u_char  SENSOR_EV_0[] = 
{	// EV -2
	0x24, 0x34,
	0x25, 0x2c,
	0x55,0xb8,
};
__align(2)const static u_char  SENSOR_EV_1[] = 
{	// EV -1.5
	0x24, 0x44,
	0x25, 0x3c,
	0x55,0xb0,
};
__align(2)const static u_char  SENSOR_EV_2[] = 
{	// EV -1
	0x24, 0x54,
	0x25, 0x4c,
	0x55,0xa0,
};
__align(2)const static u_char  SENSOR_EV_3[] = 
{	// EV -0.5
	0x24, 0x64,
	0x25, 0x5c,
	0x55,0x90,
};
__align(2)const static u_char  SENSOR_EV_4[] = 
{	// EV 0
	0x24, 0x74,
	0x25, 0x6c,
	0x55,0x00,
};
__align(2)const static u_char  SENSOR_EV_5[] = 
{	// EV +0.5
	0x24, 0x84,
	0x25, 0x7c,
	0x55,0x10,
};
__align(2)const static u_char  SENSOR_EV_6[] = 
{	// EV +1
	0x24, 0x94,
	0x25, 0x8c,
	0x55,0x20,
};
__align(2)const static u_char  SENSOR_EV_7[] = 
{	// EV +1.5
	0x24, 0xa4,
	0x25, 0x9c,
	0x55,0x30,
};
__align(2)const static u_char  SENSOR_EV_8[] = 
{	// EV +2
	0x24, 0xb4,
	0x25, 0xac,
	0x55,0x38,
};

__align(2)const static u_char  SENSOR_WB_AUTO[] = 
{	// Auto 0
#if 0
	0x45, 0x34,
	0x46, 0x58,
	0x47, 0x28,
	0x48, 0x3A,
	0x59, 0x88,
	0x5A, 0x88,
	0x5B, 0x44,
	0x5C, 0x67,
	0x5D, 0x49,
	0x5E, 0x0E,
#else
	0x13,0xe7,
#endif
};	
__align(2)const static u_char  SENSOR_WB_1[] = 
{	// Sunny 1
#if 0
	0x45, 0x38,
	0x46, 0x51,
	0x47, 0x47,
	0x48, 0x48,
	0x59, 0x88,
	0x5A, 0x80,
	0x5B, 0x33,
	0x5C, 0x5C,
	0x5D, 0x54,
	0x5E, 0x0B,
#else
	0x13,0xe5,
	0x01,0x30,
	0x02,0x70,
	0x6a,0x42,
	0x3b,0x0a,
#endif
};	
__align(2)const static u_char  SENSOR_WB_2[] = 
{	// Cloudy 2
#if 0
	0x45, 0x38,
	0x46, 0x66,
	0x47, 0x43,
	0x48, 0x44,
	0x59, 0x88,
	0x5A, 0x80,
	0x5B, 0x33,
	0x5C, 0x73,
	0x5D, 0x50,
	0x5E, 0x0C,
#else
	0x13,0xe5,
	0x01,0x48,//change
	0x02,0x60,//
	0x6a,0x40,
	0x3b,0x0a,
#endif
};	
__align(2)const static u_char  SENSOR_WB_3[] = 
{	// Indoor home 3
#if 0
	0x45, 0x77,
	0x46, 0x7B,
	0x47, 0x24,
	0x48, 0x3E,
	0x59, 0x80,
	0x5A, 0x94,
	0x5B, 0x42,
	0x5C, 0x84,
	0x5D, 0x44,
	0x5E, 0x08,
#else
	0x13,0xe5,
	0x01,0x7a,
	0x02,0x44,
	0x6a,0x4a,
	0x3b,0x0a,
#endif
};	
__align(2)const static u_char  SENSOR_WB_4[] = 
{	// Indoor office 4
#if 0
	0x45, 0x71,
	0x46, 0x73,
	0x47, 0x3D,
	0x48, 0x3F,
	0x59, 0x80,
	0x5A, 0x80,
	0x5B, 0x44,
	0x5C, 0x82,
	0x5D, 0x4E,
	0x5E, 0x0E,
#else
	0x13,0xe5,
	0x01,0x5c,
	0x02,0x44,//change
	0x6a,0x40,
	0x3b,0x0a,	
#endif
};	
__align(2)const static u_char  SENSOR_WB_5[] = 
{
NULL
};
__align(2)const static u_char  SENSOR_WB_6[] = 
{
NULL
};

__align(2)const static u_char  SENSOR_NIGHT_MODE_ON[] = 
{ 
	0x13,0xe7,
	0x3b,0x0a,
	0x2d,0x54,
	0x2e,0x02,
	0x3b,0xea,
};
__align(2)const static u_char  SENSOR_NIGHT_MODE_OFF[] = 
{
	0x13,0xe7,
	0x3b,0x0a,
	0x2d,0x54,
	0x2e,0x02,
};

__align(2)const static u_char  SENSOR_DEBAND_50[] = 
{
NULL
};
__align(2)const static u_char  SENSOR_DEBAND_60[] = 
{
NULL
};

__align(2)const static u_char  SENSOR_ROTATE_NOMAL[] = 
{
	0x03, 0x00,
	0x11, 0x80,
};
__align(2)const static u_char  SENSOR_ROTATE_MIRROR[] = 
{
	0x03, 0x00,
	0x11, 0x81,
};
__align(2)const static u_char  SENSOR_ROTATE_FLIP[] = 
{
	0x03, 0x00,
	0x11, 0x82,
};
__align(2)const static u_char  SENSOR_ROTATE_MIRROR_FLIP[] = 
{
	0x03, 0x00,
	0x11, 0x83,
};

static u_short	Sensor_Effect(u_short EffectMode)
{
	extern u_char	A8L_SendSensorSettingTable(u_short* tbl,u_short table_length);
	u_short	*tab_addr,tab_len;
    
	switch(EffectMode)
	{
		case	1:
				tab_addr = (u_short*)SENSOR_EFFECT_1;
				tab_len = sizeof(SENSOR_EFFECT_1);
				break;
		case	2:
				tab_addr = (u_short*)SENSOR_EFFECT_2;
				tab_len = sizeof(SENSOR_EFFECT_2);
				break;
		case	3:
				tab_addr = (u_short*)SENSOR_EFFECT_3;
				tab_len = sizeof(SENSOR_EFFECT_3);
				break;
		case	4:
				tab_addr = (u_short*)SENSOR_EFFECT_4;
				tab_len = sizeof(SENSOR_EFFECT_4);
				break;
		case	5:
				tab_addr = (u_short*)SENSOR_EFFECT_5;
				tab_len = sizeof(SENSOR_EFFECT_5);
				break;	
		case	6:
				tab_addr = (u_short*)SENSOR_EFFECT_6;
				tab_len = sizeof(SENSOR_EFFECT_6);
				break;																			
		default:
				tab_addr = (u_short*)SENSOR_EFFECT_NORMAL;
				tab_len = sizeof(SENSOR_EFFECT_NORMAL);
				break;				
	}

   	return	A8L_SendSensorSettingTable(tab_addr,tab_len);
}

static u_short	Sensor_EV(u_short EvStep)
{
	u_short	*tab_addr,tab_len;
//    u_short fRet = 0;
    
	switch(EvStep)
	{
		case	0:
				tab_addr = (u_short*)SENSOR_EV_0;
				tab_len = sizeof(SENSOR_EV_0);		
				break;	
		case	1:
				tab_addr = (u_short*)SENSOR_EV_1;
				tab_len = sizeof(SENSOR_EV_1);		
				break;	
		case	2:
				tab_addr = (u_short*)SENSOR_EV_2;
				tab_len = sizeof(SENSOR_EV_2);		
				break;	
		case	3:
				tab_addr = (u_short*)SENSOR_EV_3;
				tab_len = sizeof(SENSOR_EV_3);		
				break;	
		case	4:
				tab_addr = (u_short*)SENSOR_EV_4;
				tab_len = sizeof(SENSOR_EV_4);		
				break;	
		case	5:
				tab_addr = (u_short*)SENSOR_EV_5;
				tab_len = sizeof(SENSOR_EV_5);		
				break;	
		case	6:
				tab_addr = (u_short*)SENSOR_EV_6;
				tab_len = sizeof(SENSOR_EV_6);		
				break;	
		case	7:
				tab_addr = (u_short*)SENSOR_EV_7;
				tab_len = sizeof(SENSOR_EV_7);		
				break;	
		case	8:
				tab_addr = (u_short*)SENSOR_EV_8;
				tab_len = sizeof(SENSOR_EV_8);		
				break;																																																			
		default:
				tab_addr = (u_short*)SENSOR_EV_4;
				tab_len = sizeof(SENSOR_EV_4);
				break;	
	}	
	
   	return	A8L_SendSensorSettingTable(tab_addr,tab_len);					
}


static u_short	Sensor_WhiteBalance(u_short WbMode)
{
	u_short	*tab_addr,tab_len;
//    u_short fRet = 0;
    
	switch(WbMode)
	{
		case	0:
				tab_addr = (u_short*)SENSOR_WB_AUTO;
				tab_len = sizeof(SENSOR_WB_AUTO);		
				break;	
		case	1:
				tab_addr = (u_short*)SENSOR_WB_1;
				tab_len = sizeof(SENSOR_WB_1);			
				break;	
		case	2:
				tab_addr = (u_short*)SENSOR_WB_2;
				tab_len = sizeof(SENSOR_WB_2);			
				break;	
		case	3:
				tab_addr = (u_short*)SENSOR_WB_3;
				tab_len = sizeof(SENSOR_WB_3);		
				break;	
		case	4:
				tab_addr = (u_short*)SENSOR_WB_4;
				tab_len = sizeof(SENSOR_WB_4);				
				break;	
		case	5:
				tab_addr = (u_short*)SENSOR_WB_5;
				tab_len = sizeof(SENSOR_WB_5);			
				break;	
		case	6:
				tab_addr = (u_short*)SENSOR_WB_6;
				tab_len = sizeof(SENSOR_WB_6);		
				break;																																																	
		default:
				tab_addr = (u_short*)SENSOR_WB_AUTO;
				tab_len = sizeof(SENSOR_WB_AUTO);
				break;	
	}	

   	return	A8L_SendSensorSettingTable(tab_addr,tab_len);		
}

static u_short	Sensor_NightMode(mmp_sensor_night_mode NightMode)
{
	u_short	*tab_addr,tab_len;
    
	switch(NightMode)
	{
		case	A8_SENSOR_NIGHT_MODE_OFF:
				tab_addr = (u_short*)SENSOR_NIGHT_MODE_OFF;
				tab_len = sizeof(SENSOR_NIGHT_MODE_OFF);		
				break;	;																																																			
		default:
				tab_addr = (u_short*)SENSOR_NIGHT_MODE_ON;
				tab_len = sizeof(SENSOR_NIGHT_MODE_ON);
				break;	
	}				
	
   	return	A8L_SendSensorSettingTable(tab_addr,tab_len);	
}

static u_short	Sensor_DeBand(mmp_sensor_deband DeBandMode)
{
	u_short	*tab_addr,tab_len;
    
	switch(DeBandMode)
	{
		case	A8_SENSOR_DEBAND_60:
				tab_addr = (u_short*)SENSOR_DEBAND_60;
				tab_len = sizeof(SENSOR_DEBAND_60);		
				break;	;																																																			
		default:
				tab_addr = (u_short*)SENSOR_DEBAND_50;
				tab_len = sizeof(SENSOR_DEBAND_50);
				break;		
	}
	
   	return	A8L_SendSensorSettingTable(tab_addr,tab_len);		
}

static u_short	Sensor_Rotate(mmp_sensor_rotate Rotate)
{
	u_short	*tab_addr,tab_len;
    
	switch(Rotate)
	{
		case	A8_SENSOR_ROTATE_MIRROR:
				tab_addr = (u_short*)SENSOR_ROTATE_MIRROR;
				tab_len = sizeof(SENSOR_ROTATE_MIRROR);		
				break;	
		case	A8_SENSOR_ROTATE_FILP:
				tab_addr = (u_short*)SENSOR_ROTATE_FLIP;
				tab_len = sizeof(SENSOR_ROTATE_FLIP);		
				break;	
		case	A8_SENSOR_ROTATE_MIRROR_FLIP:
				tab_addr = (u_short*)SENSOR_ROTATE_MIRROR_FLIP;
				tab_len = sizeof(SENSOR_ROTATE_MIRROR_FLIP);		
				break;																																																											
		default:
				tab_addr = (u_short*)SENSOR_ROTATE_NOMAL;
				tab_len = sizeof(SENSOR_ROTATE_NOMAL);
				break;	
	}				
	
   	return	A8L_SendSensorSettingTable(tab_addr,tab_len);		
}

static u_short 	Sensor_FrameRate(u_short FrameRate)
{return 0;}

static void	Sensor_Enable(u_short enable)
{
	extern void A8L_VIF_Init(t_sensor_mode_attr* sensor_mode);
	if(enable) {
		A8L_VIF_Init(&preview_mode);
	    //sensor power LDO
	    A8L_SetSensorPowerPin(1);
	    sys_IF_ait_delay1ms(1);

	    //sensor PWDN
	    A8L_SetSensorEnablePin(0);
	    sys_IF_ait_delay1ms(1);  
		
//		A810H_PresetSensor(0, SENSOR_I2C_ID, SENSOR_I2C_TYPE, 3);
//		A810H_SetPinPull(0x69D1, PIN_PULL_HIGH);	//PSEN pull up
//		A810H_SetPinPull(0x69D2, PIN_PULL_HIGH);	//PSDA pull up
//		A810H_SetPinPull(0x69D3, PIN_PULL_HIGH);	//PSCK pull up
//		AITS_I2C_Init();

        A8L_SetSensorResetPin(1); 
        sys_IF_ait_delay1ms(1);
        A8L_SetSensorResetPin(0);
        sys_IF_ait_delay1ms(1); 
        A8L_SetSensorResetPin(1); 
	//Initial I2C I/F
	A8L_SetSensorIF(A8_SENSOR_SCK_PIN, A8_ON);
	A8L_SetSensorIF(A8_SENSOR_SDA_PIN, A8_ON);
	sys_IF_ait_delay1ms(1);

	/// Set OV7670's COM7: Reset all register and set to default value
	if (A8L_SetSenReg(0x12, 0x80))
	{
//		return 1;
	}
	sys_IF_ait_delay1ms(10);
	} else {
#if 0	
		SetA8RegB(0x7150,GetA8RegB(0x7150)|0x07);
		sys_IF_ait_delay1ms(100);	
		//Set to low first
		//If hardware sensor power can not disable, it is better to set SCK and SDA to high or floating
		SetA8RegB(0x7151,GetA8RegB(0x7151)&(~0x07));
		
		//A810H_SetRegB(0x6050,A810H_GetRegB(0x6050)|0x01);
		//A810H_SetRegB(0x6051,A810H_GetRegB(0x6051)&0xFE);
		sys_IF_ait_delay1ms(1);
		SetA8RegB(0x7151,GetA8RegB(0x7151)|0x01);
		sys_IF_ait_delay1ms(1);
	    //sensor PWDN
    	A8L_SetSensorEnablePin(1);
        sys_IF_ait_delay1ms(10);
	    //sensor power LDO
	    A8L_SetSensorPowerPin(0);
	    sys_IF_ait_delay1ms(10);        
		gbSensorInited = 0;	
#else
        A8L_SetSensorResetPin(0);
	    sys_IF_ait_delay1ms(1);  
	SetA8RegB(VIF_SEN_CLK_CTL,0);		
	    //sensor PWDN
	    A8L_SetSensorEnablePin(1);
	    sys_IF_ait_delay1ms(1);  	
		//For power consumption issue
		A8L_SetSensorIF(A8_SENSOR_SCK_PIN, A8_ON);
		A8L_SetSensorIF(A8_SENSOR_SDA_PIN, A8_ON);		
		gbSensorInited = 0;	
#endif
	}	
}
static u_short	Get_Sensor_ID(u_short*	SensorID)
{
	u_short	fRet;
	if(!gbSensorInited)
		Sensor_Enable(1);
	A8L_GetSenReg(0x0a,&fRet);
	*SensorID = fRet << 8;
	A8L_GetSenReg(0x0b,&fRet);
	*SensorID += fRet;
	if(!gbSensorInited)
		Sensor_Enable(0);
	return	fRet;
}

static u_short ChangeSensorAE(void)
{return 0;}

static u_short SetSensorResolution(mmp_sensor_mode sensor_mode)
{
	u_short	*tab_addr,tab_len;
    u_short fRet = 0;
	switch(sensor_mode)
	{
		case	SENSOR_INIT:
                A8L_VIF_Init(&preview_mode); 
                A8L_SetSensorAttr(&preview_mode);            
				tab_addr = (u_short *)Sensor_Init;
				tab_len = sizeof(Sensor_Init);
                gbSensorInited = 1;
			break;	
		case	SENSOR_DSC_PREVIEW:
				tab_addr = (u_short *)Sensor_DSC_Preview;
				tab_len = sizeof(Sensor_DSC_Preview);				
			break;
        case    SENSOR_VIDEO_PREVIEW:
				tab_addr = (u_short *)Sensor_Video_Preview;
				tab_len = sizeof(Sensor_Video_Preview);				
			break;            
		case	SENSOR_NORMAL_CAP_MODE:
				tab_addr = (u_short *)Sensor_Normal_Capture;
				tab_len = sizeof(Sensor_Normal_Capture);				
			break;
		case	SENSOR_FULL_RESOL_MODE:
                A8L_VIF_Init(&full_mode); 
                A8L_SetSensorAttr(&full_mode);            
				tab_addr = (u_short *)Sensor_Full_Capture;
				tab_len = sizeof(Sensor_Full_Capture);				
			break;
		case	SENSOR_SCALE_UP_MODE:
                fRet = ChangeSensorAE();
				tab_addr = (u_short *)Sensor_Full_Capture_ScaleUp;
				tab_len = sizeof(Sensor_Full_Capture_ScaleUp);				
            break;			
	}
    if(!fRet)
    	return	A8L_SendSensorSettingTable(tab_addr,tab_len);
    else
        return  fRet;
}

static t_sensor_mode_attr	preview_mode =
{
	SENSOR_PREVIEW_VH_POLARITY,
	SENSOR_PREVIEW_LATCH_COUNT,
	SENSOR_PREVIEW_YUV_FORMAT,
	SENSOR_LATCH_EDGE,
	1,
	1,
	SENSOR_PREVIEW_WIDTH,
	SENSOR_PREVIEW_HEIGHT,
	0,
	SENSOR_PREVIEW_LATCH_COUNT
};
static t_sensor_mode_attr	full_mode =
{
	SENSOR_FULL_VH_POLARITY,
	SENSOR_FULL_LATCH_COUNT,
	SENSOR_FULL_YUV_FORMAT,
	SENSOR_LATCH_EDGE,
	1,
	1,
	SENSOR_FULL_WIDTH,
	SENSOR_FULL_HEIGHT,
	0,
	SENSOR_FULL_LATCH_COUNT 
};
t_sensor_manager sensor_ov7670 = 
{
    SENSOR_ID,
	SENSOR_I2C_ID,
	SENSOR_I2C_TYPE,
	&preview_mode,
	&full_mode,
	SetSensorResolution,
	Sensor_Enable,
	Get_Sensor_ID,
	Sensor_Effect,
	Sensor_EV,
	Sensor_WhiteBalance,
	Sensor_NightMode,
	Sensor_DeBand,
	Sensor_Rotate,
	Sensor_FrameRate
};
#endif
