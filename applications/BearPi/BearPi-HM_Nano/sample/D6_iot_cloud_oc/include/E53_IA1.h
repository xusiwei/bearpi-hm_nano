
#ifndef __E53_IA1_H__
#define __E53_IA1_H__

/***************************************************************
* 名		称: GasStatus_ENUM
* 说    明：枚举状态结构体
***************************************************************/
typedef enum
{
	OFF = 0,
	ON
} E53_IA1_Status_ENUM;

/* E53_IA1传感器数据类型定义 ------------------------------------------------------------*/
typedef struct
{
	float    Lux;							//光照强度
	float    Humidity;        //湿度
	float    Temperature;     //温度
} E53_IA1_Data_TypeDef;



/* 寄存器宏定义 --------------------------------------------------------------------*/
#define SHT30_Addr 0x44

#define BH1750_Addr 0x23


void E53_IA1_Init(void);
void E53_IA1_Read_Data(E53_IA1_Data_TypeDef *ReadData);
void Light_StatusSet(E53_IA1_Status_ENUM status);
void Motor_StatusSet(E53_IA1_Status_ENUM status);


#endif /* __E53_IA1_H__ */

