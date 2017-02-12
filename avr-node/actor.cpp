#include <util/delay.h>
#include <stdint.h>
#include <avr/io.h>

void delay_ms(unsigned time)
{
	while(time > 0)
	{
		_delay_ms(1);
		--time;
	}
}

void delay_us(unsigned time)
{
	while(time > 0)
	{
		_delay_us(1);
		--time;
	}
}

#define BAUD 9600UL      // Baudrate

// Berechnungen
#define UBRR_VAL ((F_CPU+BAUD*8)/(BAUD*16)-1)   // clever runden
#define BAUD_REAL (F_CPU/(16*(UBRR_VAL+1)))     // Reale Baudrate
#define BAUD_ERROR ((BAUD_REAL*1000)/BAUD) // Fehler in Promille, 1000 = kein Fehler.

#if ((BAUD_ERROR<980) || (BAUD_ERROR>1020))

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
  #error Systematischer Fehler der Baudrate grösser 1% und damit zu hoch!
#endif

void USART_Transmit( uint8_t data )
{
	/* Wait for empty transmit buffer */
	while ( !( UCSRA & (1<<UDRE)) )
	;
	/* Put data into buffer, sends the data */
	UDR = data;
}

uint8_t USART_Receive( void )
{
	/* Wait for data to be received */
	while ( !(UCSRA & (1<<RXC)) )
	;
	/* Get and return received data from buffer */
	return UDR;
}

bool USART_CanRead(void)
{
	return (UCSRA & (1<<RXC));
}

namespace mesh
{
	uint8_t localAddress;
	
	enum class errid : uint8_t
	{
		undefined         = 0x00,
		invalidChecksum   = 0x01,
		invalidRegister   = 0x02,
		invalidDeviceInfo = 0x03,
		unknownCommand    = 0x04,
		dataTooSmall      = 0x05,
	};

	struct 
	{
		uint8_t  devclass;
		uint16_t vendor;
		uint16_t device;
		uint8_t serialNo[6];
		uint8_t hwRevision;
		uint8_t swRevision;
	} __attribute__((packed)) const deviceInfo {
		0x20,
		0x0001, // MQ Productions (or similar :P)
		0x0001, // 8 Bit Switch
		{ 0x12, 0x00, 0xF8, 0xB2, 0x61, 0x84 },
		0xFF, // hw rev 0
		0xBB  // sw rev 0
	};

	const char deviceName[] = "Experimental Actor (8 Bit)";

	struct message_t
	{
		uint8_t magic;
		uint8_t flags;
		uint8_t sender;
		uint8_t receiver;
		uint8_t command;
		uint8_t length;
		void * data;
		uint8_t checksum;
	};
	
	message_t * receive();
	
	// returns always nullptr
	message_t * discard(message_t * in, errid error, uint8_t errinfo);
	
	namespace callback
	{
		void writeReg8(uint8_t regNum, uint8_t value);
		void writeReg16(uint8_t regNum, uint16_t value);
		
		uint8_t readReg8(uint8_t regNum);
		uint16_t readReg16(uint8_t regNum);
	}
}


namespace mesh {
	namespace callback {
		
		void writeReg8(uint8_t reg, uint8_t value)
		{
			uint8_t ind;
			switch(reg)
			{
				case 0x10:
				case 0x11:
				case 0x12:
				case 0x13:
				case 0x14:
				case 0x15:
				case 0x16:
				case 0x17:
					ind = reg - 0x10;
					if(value >= 0x80) {
						PORTB &= ~(1<<ind);
					} else {
						PORTB |=  (1<<ind);
					}
					return;
			}
		}
		
		uint8_t readReg8(uint8_t reg)
		{
			uint8_t ind;
			switch(reg)
			{
				case 0x00: return 0x13; // Bitfield
				case 0x10:
				case 0x11:
				case 0x12:
				case 0x13:
				case 0x14:
				case 0x15:
				case 0x16:
				case 0x17:
					ind = reg - 0x10;
					return (PORTB & (1<<ind)) ? 0x00 : 0xFF;
				default:
					return 0xFF;
			}
		}
		
		void writeReg16(uint8_t reg, uint16_t val)
		{
			switch(reg)
			{
				case 0x80:
					PORTB = ~(val & 0xFF);
					return;
			}
		}
		
		uint16_t readReg16(uint8_t reg)
		{
			switch(reg)
			{
				case 0x80: return ~PORTB;
				case 0x81: return 0x0000;
				case 0x82: return 0xFFFF;
				default: return 0xFFFF;
			}
		}
	}
}

int main()
{
  UCSRB |= (1 << TXEN) | (1 << RXEN);
	UCSRC = (1<<URSEL)|(1 << UCSZ1)|(1 << UCSZ0);
	UBRRL = (UBRR_VAL >> 0) & 0xFF;
	UBRRH = (UBRR_VAL >> 8) & 0xFF;
	
	DDRB = 0xFF;
	
	PORTB = 0x7F;
	
	mesh::localAddress = 0x42;
	
	uint8_t counter = 0;
	
	USART_Transmit(counter);

	while(1)
	{
		mesh::message_t * msg = mesh::receive();
		if(msg) 
		{
			USART_Transmit(++counter);
		
		}
	}
}

// Test Packets:
/*
	Discover any (not broadcast) device:
	77 20 00 FE 01 00 00
	
	Query Device Info:
	77 20 00 FE 02 01 01 01
	
	Query Device Name:
	77 20 00 FE 02 01 02 02
	
	Read register(0):
	77 20 00 FE 06 01 00 00
	
	Read register(15): (LED 5)
	77 20 00 FE 06 01 15 15
	
	Write register(80, AA):
	77 20 00 FE 05 03 80 AA 00 2A
	
	Write register(80, 55):
	77 20 00 FE 05 03 80 55 00 D5
*/

namespace mesh
{
	static uint8_t messageBuffer[256];
	
	static message_t receivedMessage = 
	{
		0x00, 0x00,
		0x00, 0x00,
		0x00, 0x00,
		messageBuffer,
		0x00
	};
	
	static message_t * receiveRaw(uint8_t & checksum)
	{
		message_t * msg = &receivedMessage;
		msg->magic = USART_Receive();
		if(msg->magic != 0x77) {
			return nullptr;
		}
		msg->flags = USART_Receive();
		msg->receiver = USART_Receive();
		msg->sender = USART_Receive();
		msg->command = USART_Receive();
		msg->length = USART_Receive();
		
		checksum = 0;
		for(int i = 0; i < msg->length; i++) {
			messageBuffer[i] = USART_Receive();
			checksum += messageBuffer[i];
		}
		msg->checksum = USART_Receive();
		
		return msg;
	}
	
	static uint8_t write(void const * buffer, uint16_t len)
	{
		uint8_t cs = 0;
		uint8_t const * buf = reinterpret_cast<uint8_t const *>(buffer);
		for(uint16_t i = 0; i < len; i++)
		{
			cs += buf[i];
			USART_Transmit(buf[i]);
		}
		return cs;
	}
	
	message_t * discard(message_t * in, errid error, uint8_t errinfo)
	{
		USART_Transmit(0x77); // magic
		USART_Transmit(0x21); // v1.0 rsp
		USART_Transmit(in->sender); 
		USART_Transmit(localAddress);
		USART_Transmit(0x04); // cmd
		USART_Transmit(0x03); // len
		USART_Transmit(in->command);
		USART_Transmit(uint8_t(error));
		USART_Transmit(errinfo);
		USART_Transmit(in->command + uint8_t(error) + errinfo); // cs
		return nullptr;
	}
	
	message_t * receive()
	{
		uint8_t checksum;
		message_t * msg = receiveRaw(checksum);
		if(msg == nullptr) {
			return nullptr;
		}
		if(msg->receiver != localAddress &&
		   msg->receiver != 0x00 &&
			 msg->receiver != 0xFF)
		{
			// This message was not meant for us
			return nullptr;
		}
		
		if(msg->checksum != checksum) {
			return discard(msg, errid::invalidChecksum, checksum);
		}
		
		if(msg->flags & 0x01) {
			// Always return responses
			return msg;
		}
	
		uint8_t cs;
		switch(msg->command)
		{
			case 0x01: // discover
				USART_Transmit(0x77); // magic
				USART_Transmit(0x21); // v1.0 rsp
				USART_Transmit(msg->sender); 
				USART_Transmit(localAddress);
				USART_Transmit(0x01); // cmd
				USART_Transmit(0x00); // len
				USART_Transmit(0x00); // cs
				return nullptr;
			case 0x02:
				if(msg->length < 1) {
					return nullptr;
				}
				switch(*((uint8_t*)msg->data))
				{
					case 0x01: // query device info
						USART_Transmit(0x77); // magic
						USART_Transmit(0x21); // v1.0 rsp
						USART_Transmit(msg->sender); 
						USART_Transmit(localAddress);
						USART_Transmit(0x01); // cmd
						USART_Transmit(sizeof(deviceInfo) + 1); // len
						USART_Transmit(0x01); // id
						cs = write(&deviceInfo, sizeof(deviceInfo));
						USART_Transmit(cs + 0x01); // cs
						return nullptr;
					case 0x02: // Query device name
						USART_Transmit(0x77); // magic
						USART_Transmit(0x21); // v1.0 rsp
						USART_Transmit(msg->sender); 
						USART_Transmit(localAddress);
						USART_Transmit(0x01); // cmd
						USART_Transmit(sizeof(deviceName) + 1); // len
						USART_Transmit(0x02); // id
						cs = write(&deviceName, sizeof(deviceName));
						USART_Transmit(cs + 0x02); // cs
						return nullptr;
					default:
						return discard(msg, errid::invalidDeviceInfo, *((uint8_t*)msg->data));
				}
			case 0x05: // write
			{
				if(msg->length < 3) {
					return nullptr;
				}
				
				struct data {
					uint8_t reg;
					uint16_t val;
				};
				
				data * d = reinterpret_cast<data*>(msg->data);
				
				if(d->reg < 0x80) {
					callback::writeReg8(d->reg, uint8_t(d->val));
				} else {
					callback::writeReg16(d->reg, d->val);
				}
				
				return nullptr;
			}
			case 0x06: // read
			{
				if(msg->length < 1) {
					return nullptr;
				}
				
				uint8_t * reg = reinterpret_cast<uint8_t*>(msg->data);
				uint16_t value;
				if(*reg < 0x80) {
					value = callback::readReg8(*reg);
				} else {
					value = callback::readReg16(*reg);
				}
				
				USART_Transmit(0x77); // magic
				USART_Transmit(0x21); // v1.0 rsp
				USART_Transmit(msg->sender); 
				USART_Transmit(localAddress);
				USART_Transmit(0x06); // cmd
				USART_Transmit(0x03); // len
				USART_Transmit(*reg); // register
				cs = write(&value, sizeof value);
				USART_Transmit(cs + *reg); // cs
				
				return nullptr;
			}
			default:
				return msg;
		}
	}
}