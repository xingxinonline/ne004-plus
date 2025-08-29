/**
 * @file sbl_partition.c
 * @brief SBL分区表管理实现 - ESP32兼容分区表
 */

#include "sbl_partition.h"
#include "sbl_config.h"
#include <string.h>
#include <stdio.h>

/* 分区表在Flash中的位置 */
#define PARTITION_TABLE_OFFSET  SBL_PARTITION_TABLE_OFFSET
#define PARTITION_TABLE_SIZE    SBL_PARTITION_TABLE_SIZE

/* 全局变量 */
static esp_partition_info_t g_partition_table[SBL_MAX_PARTITIONS];
static int g_partition_count = 0;
static bool g_partition_initialized = false;

/* 静态函数声明 */
static int sbl_partition_read_flash(uint32_t offset, void* buffer, size_t size);
static bool sbl_partition_is_valid(const esp_partition_info_t* partition);
static uint32_t sbl_partition_crc32(const void* data, size_t length);

/**
 * @brief 初始化分区表系统
 */
int sbl_partition_init(void)
{
    if (g_partition_initialized) {
        return 0;
    }
    
    SBL_LOGI("PART", "Reading partition table from offset 0x%X", PARTITION_TABLE_OFFSET);
    
    /* 读取分区表数据 */
    uint8_t partition_data[PARTITION_TABLE_SIZE];
    if (sbl_partition_read_flash(PARTITION_TABLE_OFFSET, partition_data, PARTITION_TABLE_SIZE) != 0) {
        SBL_LOGE("PART", "Failed to read partition table from flash");
        return -1;
    }
    
    /* 解析分区表 */
    g_partition_count = 0;
    esp_partition_info_t* partitions = (esp_partition_info_t*)partition_data;
    
    for (int i = 0; i < (PARTITION_TABLE_SIZE / sizeof(esp_partition_info_t)); i++) {
        esp_partition_info_t* partition = &partitions[i];
        
        /* 检查分区是否为空（未使用） */
        if (partition->magic != ESP_PARTITION_MAGIC) {
            if (partition->magic == 0xFFFFFFFF) {
                /* 到达分区表末尾 */
                break;
            } else {
                SBL_LOGW("PART", "Invalid partition magic at index %d: 0x%X", i, partition->magic);
                continue;
            }
        }
        
        /* 验证分区有效性 */
        if (!sbl_partition_is_valid(partition)) {
            SBL_LOGW("PART", "Invalid partition at index %d", i);
            continue;
        }
        
        /* 添加到分区列表 */
        if (g_partition_count < SBL_MAX_PARTITIONS) {
            memcpy(&g_partition_table[g_partition_count], partition, sizeof(esp_partition_info_t));
            g_partition_count++;
            
            SBL_LOGD("PART", "Found partition: %s (type=%d, subtype=%d, offset=0x%X, size=0x%X)",
                     partition->label, partition->type, partition->subtype, 
                     partition->offset, partition->size);
        } else {
            SBL_LOGW("PART", "Partition table full, ignoring partition: %s", partition->label);
        }
    }
    
    if (g_partition_count == 0) {
        SBL_LOGE("PART", "No valid partitions found");
        return -1;
    }
    
    SBL_LOGI("PART", "Partition table loaded successfully, %d partitions found", g_partition_count);
    g_partition_initialized = true;
    
    return 0;
}

/**
 * @brief 查找分区
 */
const esp_partition_info_t* sbl_partition_find(esp_partition_type_t type, 
                                               esp_partition_subtype_t subtype, 
                                               const char* label)
{
    if (!g_partition_initialized) {
        SBL_LOGE("PART", "Partition table not initialized");
        return NULL;
    }
    
    for (int i = 0; i < g_partition_count; i++) {
        const esp_partition_info_t* partition = &g_partition_table[i];
        
        /* 检查类型匹配 */
        if (type != ESP_PARTITION_TYPE_ANY && partition->type != type) {
            continue;
        }
        
        /* 检查子类型匹配 */
        if (subtype != ESP_PARTITION_SUBTYPE_ANY && partition->subtype != subtype) {
            continue;
        }
        
        /* 检查标签匹配 */
        if (label != NULL && strcmp(partition->label, label) != 0) {
            continue;
        }
        
        return partition;
    }
    
    return NULL;
}

/**
 * @brief 查找下一个分区
 */
const esp_partition_info_t* sbl_partition_find_next(const esp_partition_info_t* iterator,
                                                    esp_partition_type_t type,
                                                    esp_partition_subtype_t subtype,
                                                    const char* label)
{
    if (!g_partition_initialized) {
        return NULL;
    }
    
    int start_index = 0;
    
    /* 如果提供了迭代器，从下一个分区开始查找 */
    if (iterator != NULL) {
        for (int i = 0; i < g_partition_count; i++) {
            if (&g_partition_table[i] == iterator) {
                start_index = i + 1;
                break;
            }
        }
    }
    
    /* 从指定位置开始查找 */
    for (int i = start_index; i < g_partition_count; i++) {
        const esp_partition_info_t* partition = &g_partition_table[i];
        
        /* 检查类型匹配 */
        if (type != ESP_PARTITION_TYPE_ANY && partition->type != type) {
            continue;
        }
        
        /* 检查子类型匹配 */
        if (subtype != ESP_PARTITION_SUBTYPE_ANY && partition->subtype != subtype) {
            continue;
        }
        
        /* 检查标签匹配 */
        if (label != NULL && strcmp(partition->label, label) != 0) {
            continue;
        }
        
        return partition;
    }
    
    return NULL;
}

/**
 * @brief 获取分区数量
 */
int sbl_partition_get_count(void)
{
    return g_partition_initialized ? g_partition_count : 0;
}

/**
 * @brief 获取分区表指针
 */
const esp_partition_info_t* sbl_partition_get_table(void)
{
    return g_partition_initialized ? g_partition_table : NULL;
}

/**
 * @brief 打印分区表信息
 */
void sbl_partition_table_print(void)
{
    if (!g_partition_initialized) {
        SBL_LOGE("PART", "Partition table not initialized");
        return;
    }
    
    printf("\r\n");
    printf("==================== Partition Table ====================\r\n");
    printf("%-16s %-8s %-8s %-10s %-10s %-8s\r\n", 
           "Name", "Type", "SubType", "Offset", "Size", "Flags");
    printf("-----------------------------------------------------------\r\n");
    
    for (int i = 0; i < g_partition_count; i++) {
        const esp_partition_info_t* partition = &g_partition_table[i];
        
        /* 类型字符串 */
        const char* type_str = "unknown";
        switch (partition->type) {
            case ESP_PARTITION_TYPE_APP:
                type_str = "app";
                break;
            case ESP_PARTITION_TYPE_DATA:
                type_str = "data";
                break;
        }
        
        /* 子类型字符串 */
        const char* subtype_str = "unknown";
        if (partition->type == ESP_PARTITION_TYPE_APP) {
            switch (partition->subtype) {
                case ESP_PARTITION_SUBTYPE_APP_FACTORY:
                    subtype_str = "factory";
                    break;
                case ESP_PARTITION_SUBTYPE_APP_OTA_0:
                    subtype_str = "ota_0";
                    break;
                case ESP_PARTITION_SUBTYPE_APP_OTA_1:
                    subtype_str = "ota_1";
                    break;
            }
        } else if (partition->type == ESP_PARTITION_TYPE_DATA) {
            switch (partition->subtype) {
                case ESP_PARTITION_SUBTYPE_DATA_OTA:
                    subtype_str = "ota";
                    break;
                case ESP_PARTITION_SUBTYPE_DATA_NVS:
                    subtype_str = "nvs";
                    break;
                case ESP_PARTITION_SUBTYPE_DATA_FAT:
                    subtype_str = "fat";
                    break;
            }
        }
        
        printf("%-16s %-8s %-8s 0x%08X 0x%08X 0x%04X\r\n",
               partition->label, type_str, subtype_str,
               partition->offset, partition->size, partition->flags);
    }
    
    printf("===========================================================\r\n");
    printf("Total partitions: %d\r\n", g_partition_count);
    printf("\r\n");
}

/**
 * @brief 读取Flash数据
 */
static int sbl_partition_read_flash(uint32_t offset, void* buffer, size_t size)
{
    /* 计算绝对地址 */
    uint32_t flash_addr = SBL_FLASH_BASE_ADDR + offset;
    
    /* 检查地址对齐 */
    if ((flash_addr & 3) != 0 || (size & 3) != 0) {
        SBL_LOGE("PART", "Flash read address or size not aligned");
        return -1;
    }
    
    /* 检查地址范围 */
    if (flash_addr < SBL_FLASH_BASE_ADDR || 
        (flash_addr + size) > (SBL_FLASH_BASE_ADDR + SBL_FLASH_SIZE)) {
        SBL_LOGE("PART", "Flash read address out of range");
        return -1;
    }
    
    /* 直接从XIP地址读取数据 */
    memcpy(buffer, (void*)flash_addr, size);
    
    return 0;
}

/**
 * @brief 验证分区有效性
 */
static bool sbl_partition_is_valid(const esp_partition_info_t* partition)
{
    if (partition == NULL) {
        return false;
    }
    
    /* 检查魔数 */
    if (partition->magic != ESP_PARTITION_MAGIC) {
        return false;
    }
    
    /* 检查偏移和大小对齐 */
    if ((partition->offset & 0xFFF) != 0 || (partition->size & 0xFFF) != 0) {
        SBL_LOGW("PART", "Partition %s not 4KB aligned", partition->label);
        return false;
    }
    
    /* 检查偏移和大小范围 */
    if (partition->offset < SBL_FLASH_SIZE && 
        (partition->offset + partition->size) <= SBL_FLASH_SIZE &&
        partition->size > 0) {
        return true;
    }
    
    SBL_LOGW("PART", "Partition %s has invalid offset/size", partition->label);
    return false;
}

/**
 * @brief 计算CRC32校验和
 */
static uint32_t sbl_partition_crc32(const void* data, size_t length)
{
    static const uint32_t crc32_table[256] = {
        0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F,
        0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
        0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2,
        0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
        0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9,
        0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
        0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C,
        0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
        0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423,
        0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
        0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190, 0x01DB7106,
        0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
        0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D,
        0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
        0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950,
        0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
        0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7,
        0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
        0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9, 0x5005713C, 0x270241AA,
        0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
        0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81,
        0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
        0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84,
        0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
        0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB,
        0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
        0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 0xD6D6A3E8, 0xA1D1937E,
        0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
        0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55,
        0x316E8EEF, 0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
        0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28,
        0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
        0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F,
        0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
        0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 0x86D3D2D4, 0xF1D4E242,
        0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
        0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69,
        0x616BFFD3, 0x166CCF45, 0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
        0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC,
        0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
        0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693,
        0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
        0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
    };
    
    const uint8_t* bytes = (const uint8_t*)data;
    uint32_t crc = 0xFFFFFFFF;
    
    for (size_t i = 0; i < length; i++) {
        crc = crc32_table[(crc ^ bytes[i]) & 0xFF] ^ (crc >> 8);
    }
    
    return crc ^ 0xFFFFFFFF;
}
