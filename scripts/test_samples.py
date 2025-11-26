#!/usr/bin/env python3
import os
import subprocess
import sys
import glob

# --- 配置 ---
COMPILER_PATH = "./build/bin/cactc"
SAMPLES_DIR = "tests/samples"

# --- 颜色 ---
class Colors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'

def main():
    # 1. 检查编译器是否存在
    if not os.path.isfile(COMPILER_PATH):
        print(f"{Colors.FAIL}Error: Compiler not found at {COMPILER_PATH}{Colors.ENDC}")
        print("Please run 'make' first.")
        sys.exit(1)

    # 2. 获取所有 .cact 文件并排序
    files = sorted(glob.glob(os.path.join(SAMPLES_DIR, "*.cact")))
    if not files:
        print(f"{Colors.WARNING}No .cact files found in {SAMPLES_DIR}{Colors.ENDC}")
        sys.exit(0)

    print(f"{Colors.HEADER}=== Running Integration Tests ({len(files)} files) ==={Colors.ENDC}")

    passed = 0
    failed = 0
    
    # 3. 遍历测试
    for filepath in files:
        filename = os.path.basename(filepath)
        
        # 判定预期行为
        # 包含 "false" -> 预期失败 (Exit Code != 0)
        # 包含 "true"  -> 预期成功 (Exit Code == 0)
        expect_success = "true" in filename and "false" not in filename
        
        # 特殊情况处理：比如 15_true_syntax_false_semantic.cact
        # 这个名字里既有 true 又有 false。根据语义分析逻辑，如果语义错误，编译器应该报错退出。
        # 所以只要包含 "false"，我们就预期它失败。
        if "false" in filename:
            expect_success = False

        # 运行编译器
        # capture_output=True 捕获 stdout/stderr，避免刷屏
        result = subprocess.run(
            [COMPILER_PATH, filepath], 
            capture_output=True, 
            text=True
        )
        
        actual_success = (result.returncode == 0)
        
        # 判断结果
        is_pass = (expect_success == actual_success)
        
        if is_pass:
            passed += 1
            status = f"{Colors.OKGREEN}[PASS]{Colors.ENDC}"
        else:
            failed += 1
            status = f"{Colors.FAIL}[FAIL]{Colors.ENDC}"

        # 打印单行结果
        print(f"{status} {filename:<35} ", end="")
        
        # 如果失败了，打印详细原因
        if not is_pass:
            print(f"\n    {Colors.WARNING}Expected exit code {'0' if expect_success else 'non-zero'}, got {result.returncode}{Colors.ENDC}")
            print(f"    {Colors.FAIL}Stderr output:{Colors.ENDC}")
            # 缩进打印 stderr
            for line in result.stderr.splitlines():
                print(f"      {line}")
        else:
            print("") # 换行

    # 4. 总结
    print("-" * 50)
    if failed == 0:
        print(f"{Colors.OKGREEN}All {passed} tests passed!{Colors.ENDC}")
        sys.exit(0)
    else:
        print(f"{Colors.FAIL}{failed} tests failed.{Colors.ENDC} ({passed} passed)")
        sys.exit(1)

if __name__ == "__main__":
    main()
