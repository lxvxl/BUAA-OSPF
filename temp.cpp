#include <iostream>
#include <stdint.h>
uint8_t data[] = {0x00, 0x01, 
    0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
    0x01, 0x01, 0x80, 0x00, 0x00, 0x0e, 0x00, 0x00,
    0x00, 0x24, 0x00, 0x00, 0x00, 0x01, 0xc0, 0xa8,
    0x40, 0x00, 0xff, 0xff, 0xff, 0x00, 0x03, 0x00,
    0x00, 0x01};
int size = 32;
uint16_t fletcher16(uint8_t *data, int count) {
    uint16_t sum1 = 0;
    uint16_t sum2 = 0;
    
    for (int i = 0; i < count; ++i) {
        sum1 = (sum1 + data[i]) % 255;
        sum2 = (sum2 + sum1) % 255;
    }

    return (sum2 << 8) | sum1;
}

uint16_t fletcher_checksum(const void* data, size_t len, int checksum_offset) {
    const uint8_t* ptr = static_cast<const uint8_t*>(data);
    int length = len;

    int32_t x, y;
	uint32_t mul;
	uint32_t c0 = 0, c1 = 0;
	uint16_t checksum = 0;

	for (int index = 0; index < length; index++) {
		if (index == checksum_offset ||
			index == checksum_offset+1) {
            // in case checksum has not set 0 before
			c1 += c0;
			ptr++;
		} else {
			c0 = c0 + *(ptr++);
			c1 += c0;
		}
	}

	c0 = c0 % 255;
	c1 = c1 % 255;	
    mul = (length - checksum_offset)*(c0);
  
	x = mul - c0 - c1;
	y = c1 - mul - 1;

	if ( y >= 0 ) y++;
	if ( x < 0 ) x--;

	x %= 255;
	y %= 255;

	if (x == 0) x = 255;
	if (y == 0) y = 255;
	y &= 0x00FF;
	return (x << 8) | y;
}
int main() {
    std::cout << "Fletcher-16 checksum: 0x" 
        << std::hex << fletcher_checksum(data + 2, size - 2,  14) << std::endl
        << std::hex << fletcher16(data + 2, size - 2) << std::endl;
    return 0;
}