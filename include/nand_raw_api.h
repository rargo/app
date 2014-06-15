#ifndef NAND_RAW_API_H
#define NAND_RAW_API_H
int nand_read_raw(unsigned int addr, unsigned char *buf, unsigned int len);
int nand_erase_raw(unsigned int addr, unsigned int len);
int nand_write_raw(unsigned int addr, const unsigned char *buf, unsigned int len);
#endif
