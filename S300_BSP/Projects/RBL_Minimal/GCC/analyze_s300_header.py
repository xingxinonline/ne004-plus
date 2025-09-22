#!/usr/bin/env python3
"""
S300 Header Format Analysis Tool
分析当前生成的header是否符合S300规范
"""

import struct
import sys


def analyze_s300_header(file_path):
    """分析S300 header文件"""
    print(f"🔍 分析S300 header: {file_path}")

    # 读取文件
    try:
        with open(file_path, 'rb') as f:
            header = f.read()
    except FileNotFoundError:
        print(f"❌ 文件不存在: {file_path}")
        return False

    # 检查文件大小 - 如果大于256字节，只取前256字节作为header
    if len(header) < 256:
        print(f"❌ 文件太小: {len(header)} bytes (需要至少256字节)")
        return False
    elif len(header) > 256:
        print(f"ℹ️  文件大小: {len(header)} bytes，提取前256字节作为header")
        header = header[:256]
    else:
        print(f"✅ Header size: {len(header)} bytes")

    # === Cortex-M4段信息分析 (0x00-0x1F) ===
    print("🔍 Cortex-M4 (控制系统) 段信息 (0x00-0x1F)")
    print("-" * 50)

    cortex_m4_pro = struct.unpack_from('<I', header, 0x00)[0]
    cortex_m4_addr = struct.unpack_from('<I', header, 0x04)[0]
    cortex_m4_exe = struct.unpack_from('<I', header, 0x08)[0]
    cortex_m4_len = struct.unpack_from('<I', header, 0x0C)[0]
    cortex_m4_check = struct.unpack_from('<I', header, 0x10)[0]

    print(f"0x00 Pro字段:       0x{cortex_m4_pro:08X}")

    # 解析Pro字段
    check_mode = cortex_m4_pro & 0x3
    boot_mode = (cortex_m4_pro >> 2) & 0x3F
    big_endian = (cortex_m4_pro >> 8) & 0x1
    address_bytes = (cortex_m4_pro >> 11) & 0x1
    qpi = (cortex_m4_pro >> 12) & 0x1
    dtr = (cortex_m4_pro >> 13) & 0x1
    mode = (cortex_m4_pro >> 14) & 0x1
    base_xip = (cortex_m4_pro >> 15) & 0x1
    div = (cortex_m4_pro >> 16) & 0xF
    debug = (cortex_m4_pro >> 20) & 0x1
    run_type = (cortex_m4_pro >> 21) & 0xF

    if check_mode == 1:
        desc_check_mode = 'CRC32'
    elif check_mode == 0:
        desc_check_mode = 'Checksum'
    else:
        desc_check_mode = 'Unknown'
    print(f"  Check Mode:     {check_mode} ({desc_check_mode})")
    if boot_mode == 0:
        desc_boot_mode = 'XIP Flash'
    elif boot_mode == 1:
        desc_boot_mode = 'RAM0'
    elif boot_mode == 2:
        desc_boot_mode = 'RAM1'
    else:
        desc_boot_mode = 'Unknown'
    print(f"  Boot Mode:      {boot_mode} ({desc_boot_mode})")
    print(f"  Big Endian:     {big_endian}")
    print(f"  Address Bytes:  {address_bytes}")
    print(f"  QPI Support:    {qpi}")
    print(f"  DTR Support:    {dtr}")
    print(f"  Mode:           {mode}")
    print(f"  Base XIP:       {base_xip}")
    print(f"  Div:            {div}")
    print(f"  Debug:          {debug}")
    print(f"  Run Type:       {run_type}")

    print(f"0x04 Addr:          0x{cortex_m4_addr:08X} (Flash地址)")
    print(f"0x08 Exe Addr:      0x{cortex_m4_exe:08X} (执行地址)")
    print(f"0x0C Len:           {cortex_m4_len} bytes (RBL大小)")
    print(f"0x10 Check:         0x{cortex_m4_check:08X} (CRC32校验)")

    # 版本信息
    version_bytes = header[0x14:0x20]
    version_str = version_bytes.rstrip(
        b'\x00').decode('ascii', errors='ignore')
    print(f"0x14 Version:       '{version_str}'")
    print()

    # === Cortex-M0段信息分析 (0x20-0x3F) ===
    print("🔍 Cortex-M0 段信息 (0x20-0x3F)")
    print("-" * 50)

    cortex_m0_pro = struct.unpack_from('<I', header, 0x20)[0]
    cortex_m0_addr = struct.unpack_from('<I', header, 0x24)[0]
    cortex_m0_exe = struct.unpack_from('<I', header, 0x28)[0]
    cortex_m0_len = struct.unpack_from('<I', header, 0x2C)[0]
    cortex_m0_check = struct.unpack_from('<I', header, 0x30)[0]

    m0_status = '未使用' if cortex_m0_pro == 0 else '已配置'
    print(f"0x20 Pro字段:       0x{cortex_m0_pro:08X} ({m0_status})")
    print(f"0x24 Addr:          0x{cortex_m0_addr:08X}")
    print(f"0x28 Exe Addr:      0x{cortex_m0_exe:08X}")
    print(f"0x2C Len:           {cortex_m0_len} bytes")
    print(f"0x30 Check:         0x{cortex_m0_check:08X}")

    version_m0_bytes = header[0x34:0x40]
    version_m0_str = version_m0_bytes.rstrip(
        b'\x00').decode('ascii', errors='ignore')
    print(f"0x34 Version:       '{version_m0_str}'")
    print()

    # === CPT段信息分析 (0x40-0xBF) ===
    print("🔍 CPT 段信息 (0x40-0xBF)")
    print("-" * 50)

    cpt_pro = struct.unpack_from('<I', header, 0x40)[0]
    cpt_addr = struct.unpack_from('<I', header, 0x44)[0]
    cpt_exe = struct.unpack_from('<I', header, 0x48)[0]
    cpt_len = struct.unpack_from('<I', header, 0x4C)[0]

    cpt_status = '未使用' if cpt_pro == 0 else '已配置'
    print(f"0x40 Pro字段:       0x{cpt_pro:08X} ({cpt_status})")
    print(f"0x44 Addr:          0x{cpt_addr:08X}")
    print(f"0x48 Exe Addr:      0x{cpt_exe:08X}")
    print(f"0x4C Len:           {cpt_len} bytes")

    # CPT RAM映射信息
    print("\nCPT RAM映射信息:")
    ram_names = ["PTCM", "DTCM", "SRAM0", "SRAM1", "PSRAM"]
    for i in range(5):
        offset = 0x50 + i * 20
        addr = struct.unpack_from('<I', header, offset)[0]
        ram_addr = struct.unpack_from('<I', header, offset + 4)[0]
        ram_map = struct.unpack_from('<I', header, offset + 8)[0]
        size = struct.unpack_from('<I', header, offset + 12)[0]
        check = struct.unpack_from('<I', header, offset + 16)[0]

        print(f"  {ram_names[i]:>6}: Addr=0x{addr:08X}, RAM=0x{ram_addr:08X}")
        print(
            "             Map=0x%08X, Size=%d, Check=0x%08X" %
            (ram_map, size, check)
        )
    print()

    # === 时钟配置分析 (0xE0-0xFF) ===
    print("🔍 时钟和PLL配置 (0xE0-0xFF)")
    print("-" * 50)

    # 根据NE005 S300规范解析
    reserved1 = struct.unpack_from('<I', header, 0xE0)[0]
    reserved2 = struct.unpack_from('<I', header, 0xE4)[0]
    reserved3 = struct.unpack_from('<I', header, 0xE8)[0]
    ref_clock = struct.unpack_from('<I', header, 0xEC)[0]    # Table 15
    fout_clock = struct.unpack_from('<I', header, 0xF0)[0]   # Table 16
    clkconfig0 = struct.unpack_from('<I', header, 0xF4)[0]   # Table 17
    clkconfig1 = struct.unpack_from('<I', header, 0xF8)[0]   # Table 18

    print(f"0xE0 保留字段1:     0x{reserved1:08X}")
    print(f"0xE4 保留字段2:     0x{reserved2:08X}")
    print(f"0xE8 保留字段3:     0x{reserved3:08X}")
    print(f"0xEC REFclock:      {ref_clock} Hz ({ref_clock/1000000:.1f} MHz)")
    print(
        f"0xF0 foutclock:     {fout_clock} Hz ({fout_clock/1000000:.1f} MHz)")

    # 解析Clkconfig0 (Table 17)
    timeout = postdiv2 = postdiv1 = fbdiv = 0  # 初始化变量
    if clkconfig0 != 0:
        timeout = (clkconfig0 >> 18) & 0x3FFF
        postdiv2 = (clkconfig0 >> 15) & 0x7
        postdiv1 = (clkconfig0 >> 12) & 0x7
        fbdiv = clkconfig0 & 0xFFF

        print(f"0xF4 Clkconfig0:    0x{clkconfig0:08X}")
        print(f"  timeout:          {timeout} (实际延时: {timeout * 256})")
        print(f"  postdiv2:         {postdiv2}")
        print(f"  postdiv1:         {postdiv1}")
        print(f"  fbdiv:            {fbdiv}")
    else:
        print(f"0xF4 Clkconfig0:    0x{clkconfig0:08X} (未配置)")

    # 解析Clkconfig1 (Table 18)
    en = refdiv = frac = 0  # 初始化变量
    if clkconfig1 != 0:
        en = (clkconfig1 >> 30) & 0x3
        refdiv = (clkconfig1 >> 24) & 0x3F
        frac = clkconfig1 & 0xFFFFFF

        print(f"0xF8 Clkconfig1:    0x{clkconfig1:08X}")
        print(f"  PLL使能:          {en} ({'启用' if en == 0x3 else '禁用'})")
        print(f"  refdiv:           {refdiv}")
        print(f"  frac:             {frac} (浮点分频，保留)")

        # PLL计算验证
        if (en == 0x3 and clkconfig0 != 0 and ref_clock != 0 and
                refdiv != 0 and fbdiv != 0):
            try:
                fvco = ref_clock * fbdiv // refdiv
                if postdiv1 != 0 and postdiv2 != 0:
                    calc_fout = fvco // (postdiv1 * postdiv2 * 2)
                    print("  计算结果:")
                    print(f"    FVCO:           {fvco} Hz " +
                          f"({fvco/1000000:.1f} MHz)")
                    print(f"    计算输出频率:    {calc_fout} Hz " +
                          f"({calc_fout/1000000:.1f} MHz)")

                    if abs(calc_fout - fout_clock) < 1000:  # 1kHz容差
                        print("    ✅ PLL配置计算正确")
                    else:
                        print(f"    ❌ PLL配置计算错误 (期望{fout_clock}Hz)")

                    # 检查FVCO范围 (800MHz - 3200MHz)
                    if 800000000 <= fvco <= 3200000000:
                        print("    ✅ FVCO频率在有效范围内")
                    else:
                        print("    ❌ FVCO频率超出范围 (800M-3200M)")
                else:
                    print("    ⚠️  postdiv参数为0，无法计算")

            except (ZeroDivisionError, NameError):
                print("    ⚠️  PLL参数不完整，无法计算")
    else:
        print(f"0xF8 Clkconfig1:    0x{clkconfig1:08X} (PLL禁用)")

    print()

    # === Header CRC32验证 (0xFC) ===
    print("🔍 Header CRC32验证 (0xFC)")
    print("-" * 50)

    header_crc32_stored = struct.unpack_from('<I', header, 0xFC)[0]
    print(f"0xFC Header CRC32:  0x{header_crc32_stored:08X}")

    # 使用 ROM 同款 CRC32 算法：非反射，poly=0x04C11DB7，init=0，xorout=0。
    # 规则：先将 header[0xFC..0xFF] 清零，再对完整 256 字节计算 CRC32。
    def _crc32_rom(data: bytes, init: int = 0) -> int:
        poly = 0x04C11DB7
        # 构建查找表（非反射）
        table = []
        for i in range(256):
            c = i << 24
            for _ in range(8):
                if c & 0x80000000:
                    c = ((c << 1) ^ poly) & 0xFFFFFFFF
                else:
                    c = (c << 1) & 0xFFFFFFFF
            table.append(c)

        crc = init & 0xFFFFFFFF
        for b in data:
            idx = ((crc >> 24) ^ b) & 0xFF
            crc = ((crc << 8) ^ table[idx]) & 0xFFFFFFFF
        return crc & 0xFFFFFFFF

    header_z = bytearray(header)
    header_z[0xFC:0x100] = b"\x00\x00\x00\x00"
    header_crc32_calc = _crc32_rom(header_z, 0)
    print(f"     ROM-Calculated: 0x{header_crc32_calc:08X} (ROM算法)")

    if header_crc32_stored == header_crc32_calc:
        print("✅ Header CRC32 校验通过 (ROM算法一致)")
        crc_ok = True
    else:
        print("❌ Header CRC32 校验失败 (ROM算法不一致)")
        crc_ok = False

    print()

    # === 总体评估 ===
    print("🎯 S300规范符合性评估")
    print("-" * 50)

    issues = []

    # 检查必要字段
    if cortex_m4_pro == 0:
        issues.append("❌ Cortex-M4 Pro字段为0，应该配置控制系统信息")
    else:
        print("✅ Cortex-M4段已正确配置")

    if check_mode == 1:
        print("✅ 使用CRC32校验模式")
    else:
        issues.append(f"⚠️  校验模式为{check_mode}，建议使用CRC32(1)")

    if boot_mode == 0:
        print("✅ 使用XIP模式运行")
    elif boot_mode == 2:
        print("✅ 使用RAM1模式运行 (RBL标准配置)")
    else:
        issues.append(f"⚠️  启动模式为{boot_mode}，标准为XIP(0)或RAM1(2)")

    if ref_clock == 24000000:
        print("✅ 参考时钟配置正确 (24MHz)")
    else:
        issues.append(f"⚠️  参考时钟为{ref_clock}Hz，标准为24MHz")

    # 检查PLL配置
    if en == 0x3:
        print("✅ PLL已启用")
        if fout_clock == 168000000:
            print("✅ PLL输出频率配置正确 (168MHz)")
        else:
            issues.append(f"⚠️  PLL输出频率为{fout_clock//1000000}MHz")
    else:
        print("✅ PLL已禁用 (按配置要求)")
        if fout_clock == ref_clock:
            print("✅ 输出频率等于参考频率 (PLL禁用模式)")
        else:
            issues.append("⚠️  PLL禁用时输出频率应等于参考频率")

    if cortex_m4_addr == 0x100:
        print("✅ RBL Flash地址正确 (紧跟Header)")
    else:
        issues.append(f"⚠️  RBL Flash地址为0x{cortex_m4_addr:08X}，建议为0x100")

    if boot_mode == 0 and cortex_m4_exe == 0x08000100:
        print("✅ RBL XIP执行地址正确")
    elif boot_mode == 2 and cortex_m4_exe == 0x20000000:
        print("✅ RBL SRAM执行地址正确")
    else:
        issues.append(f"⚠️  执行地址为0x{cortex_m4_exe:08X}，与启动模式不匹配")

    if crc_ok:
        print("✅ Header CRC32校验正确")

    if len(header) == 256:
        print("✅ Header大小符合规范 (256字节)")

    print()

    if issues:
        print("🔧 发现的问题:")
        for issue in issues:
            print(f"   {issue}")
        print()

    print("📊 总结:")
    print(f"   - Header大小: {len(header)} bytes")
    print(f"   - RBL大小: {cortex_m4_len} bytes")
    print(f"   - 总镜像大小: {len(header) + cortex_m4_len} bytes")
    print(f"   - 校验状态: {'通过' if crc_ok else '失败'}")
    print(f"   - 规范符合度: {max(0, 100 - len(issues) * 10)}%")

    return len(issues) == 0 and crc_ok


def main():
    if len(sys.argv) != 2:
        print("Usage: python3 analyze_s300_header.py <header_file>")
        return 1

    header_path = sys.argv[1]

    try:
        if analyze_s300_header(header_path):
            print("\n🎉 S300 Header格式分析完成 - 符合规范!")
            return 0
        else:
            print("\n⚠️  S300 Header格式分析完成 - 存在问题!")
            return 1
    except Exception as e:
        print(f"❌ 分析失败: {e}")
        return 1


if __name__ == "__main__":
    sys.exit(main())
