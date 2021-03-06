#ifndef NAND_H
#define NAND_H

int nand_init(void);
int nand_erase_block(int block);
int nand_read_id(unsigned char *buf, int len);
int nand_read_page(int page, unsigned char *buf, int len);
int nand_write_page(int page, unsigned char *buf, int len);
void nand_bbt_scan(void);
unsigned int nand_page_size(void);
unsigned int nand_block_size(void);
unsigned int nand_chip_size(void);
unsigned int nand_block_to_addr(unsigned int block);
unsigned int nand_page_to_addr(unsigned int page);
unsigned int nand_addr_to_block(unsigned int addr);
unsigned int nand_addr_to_page(unsigned int addr);

#endif
