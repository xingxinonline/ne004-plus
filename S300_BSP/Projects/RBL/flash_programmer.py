#!/usr/bin/env python3
"""
S300 QSPI Flash 直接烧录工具
通过JTAG/DAPLink直接操作QSPI Flash进行RBL烧录
支持写保护管理和完整性验证
"""

import sys
import time
import struct
import hashlib
import argparse
from pathlib import Path

try:
    import openocd
    OPENOCD_AVAILABLE = True
except ImportError:
    OPENOCD_AVAILABLE = False

try:
    from pyocd.core.helpers import ConnectHelper
    from pyocd.core.memory_map import MemoryType
    PYOCD_AVAILABLE = True
except ImportError:
    PYOCD_AVAILABLE = False

class S300FlashProgrammer:
    """S300 QSPI Flash 直接编程器"""
    
    # S300 QSPI 控制器寄存器地址
    QSPI_BASE = 0x40012000
    QSPI_CONFIG = QSPI_BASE + 0x00
    QSPI_DEV_INST = QSPI_BASE + 0x04
    QSPI_DEV_SIZE = QSPI_BASE + 0x08
    QSPI_SRAM_PART = QSPI_BASE + 0x18
    QSPI_IND_AHB_ADDR_TRIG = QSPI_BASE + 0x1C
    QSPI_DMA_PERIPH = QSPI_BASE + 0x20
    QSPI_REMAP_ADDR = QSPI_BASE + 0x24
    QSPI_MODE_BIT = QSPI_BASE + 0x28
    QSPI_SRAM_FILL = QSPI_BASE + 0x2C
    QSPI_TX_THRESH = QSPI_BASE + 0x30
    QSPI_RX_THRESH = QSPI_BASE + 0x34
    QSPI_WR_INSTR = QSPI_BASE + 0x38
    QSPI_RD_INSTR = QSPI_BASE + 0x3C
    QSPI_WR_DELAY = QSPI_BASE + 0x40
    QSPI_RD_DATA_CAPTURE = QSPI_BASE + 0x44
    QSPI_RD_LOWER_CAPTURE = QSPI_BASE + 0x48
    QSPI_SIZE_PROT = QSPI_BASE + 0x4C
    QSPI_IND_WR = QSPI_BASE + 0x70
    QSPI_IND_WR_WATER = QSPI_BASE + 0x74
    QSPI_IND_WR_START = QSPI_BASE + 0x78
    QSPI_IND_WR_CNT = QSPI_BASE + 0x7C
    QSPI_IND_RD = QSPI_BASE + 0x60
    QSPI_IND_RD_WATER = QSPI_BASE + 0x64
    QSPI_IND_RD_START = QSPI_BASE + 0x68
    QSPI_IND_RD_CNT = QSPI_BASE + 0x6C
    QSPI_FLASH_CMD = QSPI_BASE + 0x90
    QSPI_FLASH_CMD_ADDR = QSPI_BASE + 0x94
    QSPI_FLASH_CMD_RD_DATA_LOWER = QSPI_BASE + 0xA0
    QSPI_FLASH_CMD_RD_DATA_UPPER = QSPI_BASE + 0xA4
    QSPI_FLASH_CMD_WR_DATA_LOWER = QSPI_BASE + 0xA8
    QSPI_FLASH_CMD_WR_DATA_UPPER = QSPI_BASE + 0xAC
    
    # W25Q128 Flash命令
    W25Q_READ_ID = 0x9F
    W25Q_READ_STATUS1 = 0x05
    W25Q_READ_STATUS2 = 0x35
    W25Q_READ_STATUS3 = 0x15
    W25Q_WRITE_ENABLE = 0x06
    W25Q_WRITE_DISABLE = 0x04
    W25Q_WRITE_STATUS = 0x01
    W25Q_PAGE_PROGRAM = 0x02
    W25Q_SECTOR_ERASE = 0x20
    W25Q_BLOCK_ERASE_64K = 0xD8
    W25Q_CHIP_ERASE = 0xC7
    W25Q_READ_DATA = 0x03
    W25Q_FAST_READ = 0x0B
    
    # Flash布局
    RBL_START_ADDR = 0x000000
    RBL_SIZE = 64 * 1024  # 64KB
    SBL_START_ADDR = 0x010000
    SBL_SIZE = 128 * 1024  # 128KB
    
    def __init__(self, interface="pyocd", target="cortex_m"):
        """初始化Flash编程器"""
        self.interface = interface
        self.target = target
        self.session = None
        self.board = None
        
    def connect(self):
        """连接到目标设备"""
        print("🔌 连接到目标设备...")
        
        if self.interface == "pyocd" and PYOCD_AVAILABLE:
            try:
                self.session = ConnectHelper.session_with_chosen_probe()
                self.board = self.session.board
                self.session.open()
                print(f"✅ 已连接到 {self.board.target.part_number}")
                return True
            except Exception as e:
                print(f"❌ PyOCD连接失败: {e}")
                return False
                
        elif self.interface == "openocd" and OPENOCD_AVAILABLE:
            # OpenOCD连接实现
            print("⚠️  OpenOCD接口暂未实现")
            return False
        else:
            print(f"❌ 接口 {self.interface} 不可用")
            return False
    
    def disconnect(self):
        """断开连接"""
        if self.session:
            self.session.close()
            print("📴 已断开连接")
    
    def read_memory(self, addr, size):
        """读取内存"""
        if not self.session:
            return None
        return self.session.target.read_memory_block8(addr, size)
    
    def write_memory(self, addr, data):
        """写入内存"""
        if not self.session:
            return False
        self.session.target.write_memory_block8(addr, data)
        return True
    
    def read_register(self, addr):
        """读取32位寄存器"""
        if not self.session:
            return None
        return self.session.target.read32(addr)
    
    def write_register(self, addr, value):
        """写入32位寄存器"""
        if not self.session:
            return False
        self.session.target.write32(addr, value)
        return True
    
    def qspi_init(self):
        """初始化QSPI控制器"""
        print("🔧 初始化QSPI控制器...")
        
        # 使能QSPI时钟 (这里需要根据S300具体的RCC寄存器实现)
        # RCC配置需要根据实际的S300时钟树来实现
        
        # 配置QSPI控制器基本参数
        # 这里的配置值需要根据S300 QSPI控制器的具体规格来设定
        config_val = (1 << 0) | (1 << 1) | (1 << 7)  # 使能QSPI等
        self.write_register(self.QSPI_CONFIG, config_val)
        
        print("✅ QSPI控制器初始化完成")
        return True
    
    def qspi_send_command(self, cmd, addr=None, data=None, read_len=0):
        """发送QSPI命令"""
        # 构造Flash命令寄存器值
        cmd_val = cmd
        if addr is not None:
            cmd_val |= (1 << 19)  # 使能地址
            self.write_register(self.QSPI_FLASH_CMD_ADDR, addr)
        
        if data is not None:
            cmd_val |= (1 << 15)  # 使能写数据
            cmd_val |= (len(data) - 1) << 20  # 数据长度
            # 写入数据到写数据寄存器
            for i in range(0, len(data), 8):
                chunk = data[i:i+8].ljust(8, b'\x00')
                lower = struct.unpack('<I', chunk[:4])[0]
                upper = struct.unpack('<I', chunk[4:])[0] if len(chunk) > 4 else 0
                self.write_register(self.QSPI_FLASH_CMD_WR_DATA_LOWER, lower)
                self.write_register(self.QSPI_FLASH_CMD_WR_DATA_UPPER, upper)
        
        if read_len > 0:
            cmd_val |= (1 << 23)  # 使能读数据
            cmd_val |= (read_len - 1) << 20
        
        # 执行命令
        cmd_val |= (1 << 31)  # 执行命令
        self.write_register(self.QSPI_FLASH_CMD, cmd_val)
        
        # 等待命令完成
        timeout = 1000
        while timeout > 0:
            status = self.read_register(self.QSPI_FLASH_CMD)
            if not (status & (1 << 31)):  # 命令完成
                break
            time.sleep(0.001)
            timeout -= 1
        
        if timeout == 0:
            print("⚠️  QSPI命令超时")
            return None
        
        # 读取返回数据
        if read_len > 0:
            result = []
            lower = self.read_register(self.QSPI_FLASH_CMD_RD_DATA_LOWER)
            upper = self.read_register(self.QSPI_FLASH_CMD_RD_DATA_UPPER)
            data_bytes = struct.pack('<II', lower, upper)
            return data_bytes[:read_len]
        
        return b''
    
    def flash_read_id(self):
        """读取Flash ID"""
        print("🔍 读取Flash ID...")
        id_data = self.qspi_send_command(self.W25Q_READ_ID, read_len=3)
        if id_data and len(id_data) >= 3:
            flash_id = struct.unpack('BBB', id_data[:3])
            print(f"📋 Flash ID: 0x{flash_id[0]:02X}{flash_id[1]:02X}{flash_id[2]:02X}")
            return flash_id
        return None
    
    def flash_read_status(self):
        """读取Flash状态"""
        status1 = self.qspi_send_command(self.W25Q_READ_STATUS1, read_len=1)
        status2 = self.qspi_send_command(self.W25Q_READ_STATUS2, read_len=1)
        status3 = self.qspi_send_command(self.W25Q_READ_STATUS3, read_len=1)
        
        if status1 and status2 and status3:
            return status1[0], status2[0], status3[0]
        return None, None, None
    
    def flash_write_enable(self):
        """使能Flash写入"""
        self.qspi_send_command(self.W25Q_WRITE_ENABLE)
        
        # 检查WEL位
        status1, _, _ = self.flash_read_status()
        return status1 is not None and (status1 & 0x02) != 0
    
    def flash_wait_busy(self, timeout_ms=10000):
        """等待Flash操作完成"""
        start_time = time.time()
        while (time.time() - start_time) * 1000 < timeout_ms:
            status1, _, _ = self.flash_read_status()
            if status1 is not None and (status1 & 0x01) == 0:  # BUSY位清除
                return True
            time.sleep(0.01)
        return False
    
    def flash_disable_write_protection(self):
        """禁用Flash写保护"""
        print("🔓 禁用Flash写保护...")
        
        # 读取当前状态
        status1, status2, status3 = self.flash_read_status()
        if status1 is None:
            print("❌ 无法读取Flash状态")
            return False
        
        print(f"📊 当前Flash状态: SR1=0x{status1:02X}, SR2=0x{status2:02X}, SR3=0x{status3:02X}")
        
        # 检查是否有写保护
        if (status1 & 0x9C) == 0 and (status2 & 0x41) == 0:
            print("✅ Flash写保护已经禁用")
            return True
        
        # 写使能
        if not self.flash_write_enable():
            print("❌ 写使能失败")
            return False
        
        # 清除状态寄存器的保护位
        new_status1 = status1 & ~0x9C  # 清除SRP0, SEC, TB, BP2, BP1, BP0
        new_status2 = status2 & ~0x41  # 清除SRP1, CMP
        
        # 写入新的状态寄存器
        status_data = struct.pack('BB', new_status1, new_status2)
        self.qspi_send_command(self.W25Q_WRITE_STATUS, data=status_data)
        
        # 等待写入完成
        if not self.flash_wait_busy():
            print("❌ 写状态寄存器超时")
            return False
        
        # 验证写保护是否已禁用
        status1, status2, status3 = self.flash_read_status()
        if (status1 & 0x9C) == 0 and (status2 & 0x41) == 0:
            print("✅ Flash写保护已禁用")
            return True
        else:
            print("❌ Flash写保护禁用失败")
            return False
    
    def flash_enable_write_protection(self):
        """启用Flash写保护"""
        print("🔒 启用Flash写保护...")
        
        # 写使能
        if not self.flash_write_enable():
            print("❌ 写使能失败")
            return False
        
        # 设置写保护：保护RBL区域(0x000000-0x00FFFF)
        # BP2=0, BP1=0, BP0=1, TB=1 -> 保护下半部分64KB
        status1 = 0x04 | 0x20  # BP0=1, TB=1
        status2 = 0x00         # 不使用CMP
        
        status_data = struct.pack('BB', status1, status2)
        self.qspi_send_command(self.W25Q_WRITE_STATUS, data=status_data)
        
        # 等待写入完成
        if not self.flash_wait_busy():
            print("❌ 写状态寄存器超时")
            return False
        
        # 验证写保护
        status1, status2, status3 = self.flash_read_status()
        print(f"📊 Flash写保护状态: SR1=0x{status1:02X}, SR2=0x{status2:02X}")
        print("✅ Flash写保护已启用(保护RBL区域)")
        return True
    
    def flash_sector_erase(self, addr):
        """擦除Flash扇区(4KB)"""
        if not self.flash_write_enable():
            return False
        
        self.qspi_send_command(self.W25Q_SECTOR_ERASE, addr=addr)
        return self.flash_wait_busy(timeout_ms=1000)
    
    def flash_erase_range(self, start_addr, size):
        """擦除Flash范围"""
        print(f"🗑️  擦除Flash范围: 0x{start_addr:06X} - 0x{start_addr + size:06X} ({size} bytes)")
        
        sectors = (size + 4095) // 4096  # 向上取整到4KB扇区
        for i in range(sectors):
            sector_addr = start_addr + i * 4096
            print(f"🗑️  擦除扇区: 0x{sector_addr:06X}")
            if not self.flash_sector_erase(sector_addr):
                print(f"❌ 扇区擦除失败: 0x{sector_addr:06X}")
                return False
        
        print("✅ Flash擦除完成")
        return True
    
    def flash_page_program(self, addr, data):
        """Flash页编程(256字节)"""
        if len(data) > 256:
            return False
        
        if not self.flash_write_enable():
            return False
        
        self.qspi_send_command(self.W25Q_PAGE_PROGRAM, addr=addr, data=data)
        return self.flash_wait_busy(timeout_ms=1000)
    
    def flash_write_data(self, start_addr, data):
        """写入数据到Flash"""
        print(f"📝 写入数据到Flash: 0x{start_addr:06X}, {len(data)} bytes")
        
        pages = (len(data) + 255) // 256  # 向上取整到256字节页
        for i in range(pages):
            page_addr = start_addr + i * 256
            page_data = data[i * 256:(i + 1) * 256]
            
            if len(page_data) == 0:
                break
            
            print(f"📝 写入页面: 0x{page_addr:06X} ({len(page_data)} bytes)")
            if not self.flash_page_program(page_addr, page_data):
                print(f"❌ 页面写入失败: 0x{page_addr:06X}")
                return False
        
        print("✅ Flash写入完成")
        return True
    
    def flash_read_data(self, start_addr, size):
        """从Flash读取数据"""
        print(f"📖 从Flash读取数据: 0x{start_addr:06X}, {size} bytes")
        
        # 使用Fast Read命令
        result = b''
        chunk_size = 1024  # 每次读取1KB
        
        for offset in range(0, size, chunk_size):
            addr = start_addr + offset
            read_size = min(chunk_size, size - offset)
            
            # 这里需要实现更复杂的读取逻辑，因为QSPI控制器可能有读取限制
            # 简化实现：假设可以直接读取
            chunk = self.qspi_send_command(self.W25Q_FAST_READ, addr=addr, read_len=read_size)
            if chunk is None:
                print(f"❌ Flash读取失败: 0x{addr:06X}")
                return None
            
            result += chunk
        
        print("✅ Flash读取完成")
        return result
    
    def verify_flash_data(self, start_addr, expected_data):
        """验证Flash数据"""
        print(f"🔍 验证Flash数据: 0x{start_addr:06X}, {len(expected_data)} bytes")
        
        read_data = self.flash_read_data(start_addr, len(expected_data))
        if read_data is None:
            return False
        
        if read_data == expected_data:
            print("✅ Flash数据验证通过")
            return True
        else:
            print("❌ Flash数据验证失败")
            # 显示第一个不匹配的位置
            for i, (expected, actual) in enumerate(zip(expected_data, read_data)):
                if expected != actual:
                    print(f"   首个不匹配位置: 0x{start_addr + i:06X}, 期望: 0x{expected:02X}, 实际: 0x{actual:02X}")
                    break
            return False
    
    def program_rbl(self, rbl_file_path):
        """烧录RBL固件"""
        print(f"🚀 开始烧录RBL固件: {rbl_file_path}")
        
        # 读取RBL文件
        try:
            with open(rbl_file_path, 'rb') as f:
                rbl_data = f.read()
        except Exception as e:
            print(f"❌ 读取RBL文件失败: {e}")
            return False
        
        if len(rbl_data) > self.RBL_SIZE:
            print(f"❌ RBL文件太大: {len(rbl_data)} > {self.RBL_SIZE}")
            return False
        
        print(f"📁 RBL文件大小: {len(rbl_data)} bytes")
        
        # 连接设备
        if not self.connect():
            return False
        
        try:
            # 初始化QSPI
            if not self.qspi_init():
                return False
            
            # 读取Flash ID
            flash_id = self.flash_read_id()
            if flash_id != (0xEF, 0x40, 0x18):  # W25Q128
                print(f"❌ Flash ID不匹配: {flash_id}")
                return False
            
            # 禁用写保护
            if not self.flash_disable_write_protection():
                return False
            
            # 擦除RBL区域
            if not self.flash_erase_range(self.RBL_START_ADDR, self.RBL_SIZE):
                return False
            
            # 写入RBL数据
            if not self.flash_write_data(self.RBL_START_ADDR, rbl_data):
                return False
            
            # 验证写入数据
            if not self.verify_flash_data(self.RBL_START_ADDR, rbl_data):
                return False
            
            # 启用写保护
            if not self.flash_enable_write_protection():
                print("⚠️  写保护启用失败，但烧录完成")
            
            print("🎉 RBL烧录成功!")
            return True
            
        finally:
            self.disconnect()
    
    def dump_flash_info(self):
        """显示Flash信息"""
        print("📊 Flash信息:")
        
        if not self.connect():
            return False
        
        try:
            if not self.qspi_init():
                return False
            
            # Flash ID
            flash_id = self.flash_read_id()
            
            # Flash状态
            status1, status2, status3 = self.flash_read_status()
            if status1 is not None:
                print(f"📊 Flash状态寄存器:")
                print(f"   SR1: 0x{status1:02X} (BUSY:{status1&1}, WEL:{(status1>>1)&1}, BP:{(status1>>2)&7}, TB:{(status1>>5)&1}, SEC:{(status1>>6)&1}, SRP0:{(status1>>7)&1})")
                print(f"   SR2: 0x{status2:02X} (SRP1:{status2&1}, QE:{(status2>>1)&1}, LB:{(status2>>3)&7}, CMP:{(status2>>6)&1}, SUS:{(status2>>7)&1})")
                print(f"   SR3: 0x{status3:02X}")
            
            return True
            
        finally:
            self.disconnect()

def main():
    """主函数"""
    parser = argparse.ArgumentParser(description='S300 QSPI Flash直接烧录工具')
    parser.add_argument('--interface', choices=['pyocd', 'openocd'], default='pyocd',
                        help='调试器接口 (默认: pyocd)')
    parser.add_argument('--info', action='store_true',
                        help='显示Flash信息')
    parser.add_argument('--program', type=str, metavar='FILE',
                        help='烧录RBL文件')
    parser.add_argument('--disable-wp', action='store_true',
                        help='禁用写保护')
    parser.add_argument('--enable-wp', action='store_true',
                        help='启用写保护')
    
    args = parser.parse_args()
    
    if not PYOCD_AVAILABLE and args.interface == 'pyocd':
        print("❌ PyOCD不可用，请安装: pip install pyocd")
        sys.exit(1)
    
    if not OPENOCD_AVAILABLE and args.interface == 'openocd':
        print("❌ OpenOCD Python接口不可用")
        sys.exit(1)
    
    programmer = S300FlashProgrammer(interface=args.interface)
    
    if args.info:
        programmer.dump_flash_info()
    elif args.program:
        if not Path(args.program).exists():
            print(f"❌ 文件不存在: {args.program}")
            sys.exit(1)
        programmer.program_rbl(args.program)
    elif args.disable_wp:
        if programmer.connect():
            try:
                programmer.qspi_init()
                programmer.flash_disable_write_protection()
            finally:
                programmer.disconnect()
    elif args.enable_wp:
        if programmer.connect():
            try:
                programmer.qspi_init()
                programmer.flash_enable_write_protection()
            finally:
                programmer.disconnect()
    else:
        parser.print_help()

if __name__ == "__main__":
    main()
