/**
 * @file sbl_partition.h
 * @brief ESP32兼容的分区表管理
 */

#ifndef SBL_PARTITION_H
#define SBL_PARTITION_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ESP32分区类型定义 */
typedef enum {
    ESP_PARTITION_TYPE_APP = 0x00,      /* 应用程序分区 */
    ESP_PARTITION_TYPE_DATA = 0x01,     /* 数据分区 */
} esp_partition_type_t;

/* ESP32分区子类型定义 */
typedef enum {
    /* APP子类型 */
    ESP_PARTITION_SUBTYPE_APP_FACTORY = 0x00,   /* 出厂APP */
    ESP_PARTITION_SUBTYPE_APP_OTA_MIN = 0x10,   /* OTA_0 */
    ESP_PARTITION_SUBTYPE_APP_OTA_0 = 0x10,     /* OTA_0 */
    ESP_PARTITION_SUBTYPE_APP_OTA_1 = 0x11,     /* OTA_1 */
    ESP_PARTITION_SUBTYPE_APP_OTA_MAX = 0x1F,   /* 最大OTA */
    
    /* DATA子类型 */
    ESP_PARTITION_SUBTYPE_DATA_OTA = 0x00,      /* OTA数据 */
    ESP_PARTITION_SUBTYPE_DATA_NVS = 0x02,      /* NVS */
    ESP_PARTITION_SUBTYPE_DATA_FAT = 0x81,      /* FAT文件系统 */
} esp_partition_subtype_t;

/* 分区表条目结构 */
typedef struct {
    uint16_t magic;              /* 魔数: 0x50AA */
    uint8_t  type;               /* 分区类型 */
    uint8_t  subtype;            /* 分区子类型 */
    uint32_t offset;             /* 分区偏移地址 */
    uint32_t size;               /* 分区大小 */
    char     label[16];          /* 分区标签 */
    uint32_t flags;              /* 分区标志 */
} __attribute__((packed)) esp_partition_info_t;

/* 分区表结构 */
typedef struct {
    esp_partition_info_t partitions[ESP_PARTITION_TABLE_MAX_ENTRIES];
    uint8_t count;               /* 有效分区数量 */
    uint32_t table_crc;          /* 分区表CRC */
} partition_table_t;

/* 分区迭代器 */
typedef struct partition_iterator {
    esp_partition_type_t type;
    esp_partition_subtype_t subtype;
    const char* label;
    const esp_partition_info_t* info;
} esp_partition_iterator_t;

/* 常量定义 */
#define ESP_PARTITION_TABLE_MAX_ENTRIES 95
#define ESP_PARTITION_TABLE_MAX_LEN     (ESP_PARTITION_TABLE_MAX_ENTRIES * sizeof(esp_partition_info_t) + sizeof(uint32_t))
#define ESP_PARTITION_MAGIC             0x50AA
#define ESP_PARTITION_MAGIC_MD5         0xEBEB

/* 函数声明 */

/**
 * @brief 初始化分区表系统
 * @return 0成功，非0失败
 */
int sbl_partition_init(void);

/**
 * @brief 查找指定类型的分区
 * @param type 分区类型
 * @param subtype 分区子类型
 * @param label 分区标签(可选)
 * @return 分区信息指针，NULL表示未找到
 */
const esp_partition_info_t* sbl_partition_find_first(esp_partition_type_t type, 
                                                     esp_partition_subtype_t subtype, 
                                                     const char* label);

/**
 * @brief 获取分区迭代器
 * @param type 分区类型
 * @param subtype 分区子类型  
 * @param label 分区标签(可选)
 * @return 分区迭代器，NULL表示未找到
 */
esp_partition_iterator_t sbl_partition_find(esp_partition_type_t type,
                                           esp_partition_subtype_t subtype,
                                           const char* label);

/**
 * @brief 验证分区表完整性
 * @return true有效，false无效
 */
bool sbl_partition_table_verify(void);

/**
 * @brief 打印分区表信息
 */
void sbl_partition_table_print(void);

/**
 * @brief 获取分区绝对地址
 * @param partition 分区信息
 * @return 分区在Flash中的绝对地址
 */
uint32_t sbl_partition_get_address(const esp_partition_info_t* partition);

/**
 * @brief 检查地址是否在分区范围内
 * @param partition 分区信息
 * @param address 要检查的地址
 * @return true在范围内，false超出范围
 */
bool sbl_partition_address_in_range(const esp_partition_info_t* partition, uint32_t address);

#ifdef __cplusplus
}
#endif

#endif /* SBL_PARTITION_H */
