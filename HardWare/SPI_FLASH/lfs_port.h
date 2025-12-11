#ifndef LFS_PORT_H
#define LFS_PORT_H

#include "HeaderFiles.h"
#include "lfs.h"

#define GD25Q40E_SECTOR_SIZE 4096
#define GD25Q40E_SECTOR_NUM 128
#define GD25Q40E_ID 0x00C84013


extern lfs_t lfs;             // lfs 文件系统对象
extern lfs_file_t file;       // lfs 文件对象
extern struct lfs_config cfg; // lfs 文件系统配置结构体

int lfs_spi_flash_init(struct lfs_config *cfg);
int lfs_spi_flash_read(const struct lfs_config *cfg, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size);
int lfs_spi_flash_prog(const struct lfs_config *cfg, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size);
int lfs_spi_flash_erase(const struct lfs_config *cfg, lfs_block_t block);
int lfs_spi_flash_sync(const struct lfs_config *cfg);

#endif
