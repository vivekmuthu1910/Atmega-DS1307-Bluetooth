#include "Driver_I2C.h"

uint8_t slave_add, *I2C_dat_master, I2C_num, I2C_num_slave_write, I2C_num_slave_read, *I2C_dat_slave_write, *I2C_dat_slave_read;
volatile uint8_t I2C_flag=0;

ISR(TWI_vect)
{
	static uint8_t i=0;
	switch(TW_STATUS)
	{
		case TW_START:	
		case TW_REP_START:	
								if ((I2C_flag & I2C_Mode_Mask) == I2C_Master_Write)
									TWDR = (slave_add<<1) | TW_WRITE;
								else if((I2C_flag & I2C_Mode_Mask) == I2C_Master_Read)
									TWDR = (slave_add<<1) | TW_READ;
								TWCR = 1<<TWINT | 1<<TWEN | 1<<TWIE;
								I2C_flag &= ~(I2C_Error_Mask);
								break;
		case TW_MT_SLA_ACK: 
		case TW_MT_DATA_ACK:	
								if (I2C_num == i)
								{
									TWCR = 1<<TWINT | 1<<TWSTO | 1<<TWEN;
									I2C_flag &= ~(I2C_Busy);
									i=0;
									break;
								}
								TWDR = I2C_dat_master[i++];
								TWCR = 1<<TWINT | 1<<TWEN | 1<<TWIE;
								break;
			
		case TW_MT_SLA_NACK:	
								I2C_flag &= ~(I2C_Error_Mask);
								I2C_flag |= I2C_Master_Write_Slave_NACK;
								TWCR = 1<<TWINT | 1<<TWSTA | 1<<TWEN | 1<<TWIE;
								break;
		case TW_MT_DATA_NACK:						
								TWDR = I2C_dat_master[i];
								TWCR = 1<<TWINT | 1<<TWSTO | 1<<TWEN;
								I2C_flag &= ~(I2C_Busy | I2C_Error_Mask);
								i=0;
								break;
		case TW_MT_ARB_LOST:	
								I2C_flag &= ~(I2C_Error_Mask);
								if((I2C_flag & I2C_Mode_Mask) == I2C_Master_Write)
									I2C_flag |=  I2C_Master_Transmit_Arb_Lost;
								else if((I2C_flag & I2C_Mode_Mask) == I2C_Master_Read)
									I2C_flag |=  I2C_Master_Receive_Arb_Lost;
									
								if(I2C_num != i)
								{
									if ((I2C_flag & I2C_Mode_Mask) == I2C_Master_Write)
									TWDR = (slave_add<<1) | TW_WRITE;
									else if((I2C_flag & I2C_Mode_Mask) == I2C_Master_Read)
									TWDR = (slave_add<<1) | TW_READ;
									TWCR = 1<<TWINT | 1<<TWEN | 1<<TWIE;
								}
								break;
		case TW_MR_SLA_ACK:
								TWCR = 1<<TWINT | 1<<TWIE | 1<<TWEN | 1<<TWEA;
								break;
		case TW_MR_DATA_ACK:
								I2C_dat_master[i++] = TWDR;
								if (I2C_num == (i+1))
									TWCR = 1<<TWINT | 1<<TWIE | 1<<TWEN;
								else
									TWCR = 1<<TWINT | 1<<TWIE | 1<<TWEN | 1<<TWEA;
								break;
		case TW_MR_DATA_NACK:
								I2C_dat_master[i++] = TWDR;
								TWCR = 1<<TWINT | 1<<TWSTO | 1<<TWEN;
								I2C_flag &= ~(I2C_Busy | I2C_Error_Mask);
								i=0;
								break;
		case TW_MR_SLA_NACK:	
								I2C_flag &= ~(I2C_Error_Mask);
								I2C_flag |= I2C_Master_Read_Slave_NACK;
								TWCR = 1<<TWINT | 1<<TWSTA | 1<<TWEN | 1<<TWIE;
								break;
	}
}

void I2C_Master_Transmit(uint8_t SLA_ADD, uint8_t *dat, uint8_t num)
{
	sei();
	slave_add = SLA_ADD;
	I2C_dat_master = dat;
	I2C_num = num;
	I2C_flag = I2C_Master_Write | I2C_Busy;
	TWCR = 1<<TWINT | 1<<TWSTA | 1<<TWEN | 1<<TWIE;
}

void I2C_Master_Receive(uint8_t SLA_ADD, uint8_t *dat, uint8_t num)
{
	TWAR = 0;
	sei();
	slave_add = SLA_ADD;
	I2C_dat_master = dat;
	I2C_num = num;
	I2C_flag = I2C_Master_Read | I2C_Busy;
	TWCR = 1<<TWINT | 1<<TWSTA | 1<<TWEN | 1<<TWIE;
}

void I2C_Slave_Transmit(uint8_t slave_ad, uint8_t *dat, uint8_t num)
{
	TWAR = slave_ad<<1;
	I2C_dat_slave_write = dat;
	I2C_num_slave_write = num;
	sei();
	TWCR = 1<<TWINT | 1<<TWEA | 1<<TWEN | 1<<TWIE;
}

void I2C_Slave_Receive(uint8_t slave_ad, uint8_t gen_call_en, uint8_t *dat, uint8_t num)
{
	TWAR = slave_ad<<1 | gen_call_en;
	I2C_dat_slave_read = dat;
	I2C_num_slave_read = num;
	sei();
	TWCR = 1<<TWINT | 1<<TWEA | 1<<TWEN | 1<<TWIE;
}

void I2C_Set_Speed(uint32_t Speed){
	TWBR =(F_CPU/Speed-16)/8;
}