//25mmHg joetronica@gmx.net, Kazoosh, 2021

/* Optimized half-duplex serial uart implementation
 * timing within 2% using at 230.4kbps @ 8Mhz 
 * @author: Ralph Doncaster
 * @orginalversion: BasicSerial3.h
 * link: https://nerdralph.blogspot.com/2014/01/avr-half-duplex-software-uart.html
 * link: https://nerdralph.blogspot.com/2013/12/writing-avr-assembler-code-with-arduino.html
 * link to newer version: https://github.com/nerdralph/nerdralph/blob/master/avr/picoUART.h
 * shrink to TX only by 25mmHg
 * use: printString("\nH E L L O\n");
 * use: printHex(10);
 */
 
#define BAUD_RATE 115200

#ifdef F_CPU
  /* account for integer truncation by adding 3/2 = 1.5 */
  #define TXDELAY (((F_CPU/BAUD_RATE)-7 +1.5)/3)
#else
  #error CPU frequency F_CPU undefined
#endif

extern "C"
{
	static void TxTimedByte(char, char);
}

class SerialOut
{
	public:

		SerialOut() {}

		static void printString(const char* str)
		{
			while (*str) TxTimedByte(*str++, TXDELAY);
		}

		static void printHex(uint16_t data2hex)
		{
			//convert unsigned integer numbers 2 HEX and print it 
			static const uint16_t mask = 0xF;
			for(uint8_t i=0; i<4; i++)
			{
				uint8_t temp = (uint8_t)((data2hex >> (4*(3-i))) & mask);
				temp = (temp < 0xA ? temp+'0' : temp-0xA+'A');
				TxTimedByte(temp, TXDELAY);
			}
		}	
};