#!/usr/bin/env python3
"""
S300 RBL 完整性测试脚本
自动化测试RBL的各项功能
"""

import sys
import time
import os
import subprocess
import tempfile
import struct
from pathlib import Path

class RBLTester:
    """RBL完整性测试器"""
    
    def __init__(self, project_path):
        """初始化测试器"""
        self.project_path = Path(project_path)
        self.test_results = []
        
    def log_test(self, test_name, passed, details=""):
        """记录测试结果"""
        status = "✅ PASS" if passed else "❌ FAIL"
        self.test_results.append({
            'name': test_name,
            'passed': passed,
            'details': details
        })
        print(f"{status} {test_name}")
        if details:
            print(f"    {details}")
    
    def test_build_system(self):
        """测试构建系统"""
        print("\n🔨 测试构建系统...")
        
        try:
            # 测试清理
            result = subprocess.run([
                str(self.project_path / "build.sh"), "clean"
            ], capture_output=True, text=True, cwd=self.project_path)
            
            # 构建清理可能失败(没有文件需要清理)，这是正常的
            self.log_test("构建清理", True, 
                         "清理完成" if result.returncode == 0 else "无需清理")
            
            # 测试构建
            result = subprocess.run([
                str(self.project_path / "build.sh"), "build"
            ], capture_output=True, text=True, cwd=self.project_path)
            
            # 如果构建脚本不存在，尝试直接使用make
            if result.returncode != 0:
                result = subprocess.run([
                    "make", "-C", str(self.project_path / "GCC")
                ], capture_output=True, text=True)
            
            self.log_test("项目构建", result.returncode == 0,
                         "成功生成RBL镜像" if result.returncode == 0 else f"构建失败: {result.stderr[:200]}...")
            
            # 检查输出文件
            gcc_dir = self.project_path / "GCC"
            build_dir = gcc_dir / "build"
            
            expected_files = [
                "rbl.elf",
                "rbl.bin", 
                "rbl.map",
                "s300_rbl_complete.bin"
            ]
            
            for file in expected_files:
                file_path = build_dir / file
                exists = file_path.exists()
                size = file_path.stat().st_size if exists else 0
                self.log_test(f"输出文件: {file}", exists and size > 0,
                             f"大小: {size} bytes" if exists else "文件不存在")
            
        except Exception as e:
            self.log_test("构建系统", False, str(e))
    
    def test_code_quality(self):
        """测试代码质量"""
        print("\n📊 测试代码质量...")
        
        # 检查TODO/FIXME等标记
        todo_count = 0
        fixme_count = 0
        
        for file_path in self.project_path.rglob("*.c"):
            try:
                with open(file_path, 'r', encoding='utf-8') as f:
                    content = f.read()
                    todo_count += content.upper().count("TODO")
                    fixme_count += content.upper().count("FIXME")
            except:
                pass
        
        for file_path in self.project_path.rglob("*.h"):
            try:
                with open(file_path, 'r', encoding='utf-8') as f:
                    content = f.read()
                    todo_count += content.upper().count("TODO")
                    fixme_count += content.upper().count("FIXME")
            except:
                pass
        
        self.log_test("待完成项目", todo_count < 5, f"TODO: {todo_count}, FIXME: {fixme_count}")
        
        # 统计代码行数
        total_lines = 0
        c_files = 0
        h_files = 0
        
        for file_path in self.project_path.rglob("*.c"):
            try:
                with open(file_path, 'r', encoding='utf-8') as f:
                    lines = len(f.readlines())
                    total_lines += lines
                    c_files += 1
            except:
                pass
        
        for file_path in self.project_path.rglob("*.h"):
            try:
                with open(file_path, 'r', encoding='utf-8') as f:
                    lines = len(f.readlines())
                    total_lines += lines
                    h_files += 1
            except:
                pass
        
        self.log_test("代码规模", total_lines > 1000, 
                     f"总行数: {total_lines}, C文件: {c_files}, H文件: {h_files}")
    
    def test_binary_analysis(self):
        """测试二进制文件分析"""
        print("\n🔍 测试二进制文件...")
        
        bin_path = self.project_path / "GCC" / "build" / "rbl.bin"
        complete_path = self.project_path / "GCC" / "build" / "s300_rbl_complete.bin"
        
        if not bin_path.exists():
            self.log_test("二进制文件", False, "rbl.bin不存在")
            return
        
        # 检查文件大小
        bin_size = bin_path.stat().st_size
        self.log_test("RBL大小检查", bin_size < 64*1024, 
                     f"大小: {bin_size} bytes ({bin_size/1024:.1f}KB)")
        
        if complete_path.exists():
            complete_size = complete_path.stat().st_size
            self.log_test("完整镜像", complete_size > bin_size,
                         f"完整镜像: {complete_size} bytes")
        
        # 检查二进制内容
        try:
            with open(bin_path, 'rb') as f:
                data = f.read(16)
                
            # 检查ARM向量表
            stack_ptr = struct.unpack('<I', data[0:4])[0]
            reset_handler = struct.unpack('<I', data[4:8])[0]
            
            # 栈指针应该在SRAM范围内 (S300: 0x20000000-0x20040000)
            stack_valid = 0x20000000 <= stack_ptr <= 0x20040000
            # Reset handler应该是奇数(Thumb模式)且在合理范围
            reset_valid = (reset_handler & 1) == 1 and 0x20000000 <= reset_handler <= 0x20040000
            
            self.log_test("ARM向量表", stack_valid and reset_valid,
                         f"SP: 0x{stack_ptr:08X}, Reset: 0x{reset_handler:08X}")
            
        except Exception as e:
            self.log_test("二进制分析", False, str(e))
    
    def test_documentation(self):
        """测试文档完整性"""
        print("\n📚 测试文档...")
        
        required_docs = [
            "README.md",
            "YMODEM_GUIDE.md", 
            "SECURITY_GUIDE.md",
            "PROJECT_SUMMARY.md"
        ]
        
        for doc in required_docs:
            doc_path = self.project_path / doc
            exists = doc_path.exists()
            size = doc_path.stat().st_size if exists else 0
            self.log_test(f"文档: {doc}", exists and size > 1000,
                         f"大小: {size} bytes" if exists else "文件不存在")
    
    def test_tools(self):
        """测试工具脚本"""
        print("\n🛠️ 测试工具脚本...")
        
        scripts = [
            "build.sh",
            "flash_program.sh",
            "ymodem_test.py",
            "flash_programmer.py"
        ]
        
        for script in scripts:
            script_path = self.project_path / script
            exists = script_path.exists()
            executable = script_path.stat().st_mode & 0o111 if exists else False
            
            self.log_test(f"脚本: {script}", exists,
                         f"可执行: {'是' if executable else '否'}" if exists else "文件不存在")
    
    def test_configuration(self):
        """测试配置文件"""
        print("\n⚙️ 测试配置...")
        
        config_files = [
            "Inc/rbl_config.h",
            "s300_flash.cfg",
            "GCC/Makefile"
        ]
        
        for config in config_files:
            config_path = self.project_path / config
            exists = config_path.exists()
            size = config_path.stat().st_size if exists else 0
            
            self.log_test(f"配置: {config}", exists and size > 0,
                         f"大小: {size} bytes" if exists else "文件不存在")
    
    def generate_report(self):
        """生成测试报告"""
        print("\n" + "="*60)
        print("🎯 RBL完整性测试报告")
        print("="*60)
        
        total_tests = len(self.test_results)
        passed_tests = sum(1 for test in self.test_results if test['passed'])
        failed_tests = total_tests - passed_tests
        
        print(f"总测试数: {total_tests}")
        print(f"通过: {passed_tests}")
        print(f"失败: {failed_tests}")
        print(f"通过率: {passed_tests/total_tests*100:.1f}%")
        
        if failed_tests > 0:
            print(f"\n❌ 失败的测试:")
            for test in self.test_results:
                if not test['passed']:
                    print(f"  - {test['name']}: {test['details']}")
        
        # 生成JSON报告
        report_path = self.project_path / "test_report.json"
        try:
            import json
            with open(report_path, 'w') as f:
                json.dump({
                    'timestamp': time.strftime('%Y-%m-%d %H:%M:%S'),
                    'total_tests': total_tests,
                    'passed_tests': passed_tests,
                    'failed_tests': failed_tests,
                    'pass_rate': passed_tests/total_tests*100,
                    'tests': self.test_results
                }, f, indent=2)
            print(f"\n📄 详细报告已保存到: {report_path}")
        except:
            pass
        
        print("="*60)
        
        if passed_tests == total_tests:
            print("🎉 所有测试通过! RBL项目完整性良好!")
            return True
        else:
            print("⚠️  部分测试失败，请检查上述问题")
            return False
    
    def run_all_tests(self):
        """运行所有测试"""
        print("🚀 开始RBL完整性测试...")
        print(f"📁 项目路径: {self.project_path}")
        
        self.test_build_system()
        self.test_code_quality()
        self.test_binary_analysis()
        self.test_documentation()
        self.test_tools()
        self.test_configuration()
        
        return self.generate_report()

def main():
    """主函数"""
    if len(sys.argv) > 1:
        project_path = sys.argv[1]
    else:
        project_path = os.path.dirname(os.path.abspath(__file__))
    
    tester = RBLTester(project_path)
    success = tester.run_all_tests()
    
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()
