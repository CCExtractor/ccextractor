import os
import subprocess
import glob
import sys

# Configuration
CC_BINARY = "./build_cmake/ccextractor"
TEST_DIR = "/home/rahul/Desktop/all_tests"
OUT_DIR = "./test_verification_output"

# Test Cases
# Format: (Filename, [Keywords to check in specific outputs])
TEST_CASES = [
    {
        "filename": "multiprogram_spain.ts",
        "description": "Repetition Bug & Stream Splitting",
        "args": ["--split-dvb-subs"],
        "checks": [
            {
                "stream_suffix": "spa_0x006F.srt",
                "contains": ["continuar con este debate", "Podemos deberia cambiar"],
                "max_occurrences": {"continuar con este debate": 2} # Ensure no infinite loop
            },
            {
                "stream_suffix": "spa_0x00D3.srt",
                "contains": ["No me enseñe nada", "la manta a la cabeza"]
            }
        ]
    },
    {
        "filename": "04e47919de5908edfa1fddc522a811d56bc67a1d4020f8b3972709e25b15966c.ts",
        "description": "Crash/Empty File Bug (Chinese Streams)",
        "args": ["--split-dvb-subs"],
        "checks": [
            {
                # We expect at least some files to be generated.
                # We will check if ANY generated .srt file has size > 0
                "any_file_valid": True
            }
        ]
    }
]

def validate_srt_content(filepath):
    """
    Checks if a file has valid SRT structure:
    1. Numeric counter
    2. Timestamp line (00:00:00,000 --> 00:00:00,000)
    3. Some content (text or empty for DVB sometimes, but usually text if OCR'd)
    Returns (True, "OK") or (False, Reason)
    """
    try:
        with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
            lines = f.readlines()
        
        if not lines:
            return False, "Empty file"
        
        # Simple state machine or regex check for at least one valid block
        has_valid_block = False
        i = 0
        while i < len(lines):
            line = lines[i].strip()
            if line.isdigit():
                # Found index
                if i + 1 < len(lines):
                    time_line = lines[i+1].strip()
                    if "-->" in time_line and ":" in time_line:
                        has_valid_block = True
                        break
            i += 1
            
        if has_valid_block:
            return True, "OK"
        else:
            return False, "No valid SRT blocks found"
            
    except Exception as e:
        return False, f"Exception reading file: {e}"

def run_test():
    if not os.path.exists(OUT_DIR):
        os.makedirs(OUT_DIR)
    
    overall_success = True

    for test in TEST_CASES:
        input_file = os.path.join(TEST_DIR, test["filename"])
        print(f"Testing: {test['filename']} ({test['description']})...")
        
        # Cleanup previous output for this file
        base_name = os.path.splitext(test["filename"])[0]
        for f in glob.glob(os.path.join(OUT_DIR, f"{base_name}*")):
            os.remove(f)

        cmd = [CC_BINARY, input_file] + test["args"] + ["-o", os.path.join(OUT_DIR, base_name)]
        
        try:
            # Capture output to avoid cluttering screen, but print on failure
            result = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=120, text=True)
            
            if result.returncode != 0 and result.returncode != 10: # 10 is often returned by CCX on success with some warnings
                print(f"  [FAIL] CCExtractor returned code {result.returncode}")
                print(result.stderr)
                overall_success = False
                continue

        except subprocess.TimeoutExpired:
            print("  [FAIL] Timed out! Infinite loop suspected.")
            overall_success = False
            continue

        # Validation
        generated_files = glob.glob(os.path.join(OUT_DIR, f"{base_name}*.srt"))
        if not generated_files:
            print("  [FAIL] No output files generated.")
            overall_success = False
            continue

        print(f"  [INFO] Generated {len(generated_files)} files.")
        
        test_success = True
        
        for check in test.get("checks", []):
            if "any_file_valid" in check:
                non_empty = []
                for f in generated_files:
                    if os.path.getsize(f) > 0:
                        is_valid, reason = validate_srt_content(f)
                        if is_valid:
                            non_empty.append(f)
                        else:
                            print(f"  [WARN] File {f} has size > 0 but invalid content: {reason}")

                if not non_empty:
                    print("  [FAIL] All generated files are empty or invalid SRT. Crash verification failed.")
                    test_success = False
                else:
                    print(f"  [PASS] Found {len(non_empty)} valid non-empty files.")

            if "stream_suffix" in check:
                target_file = None
                for gf in generated_files:
                    if gf.endswith(check["stream_suffix"]):
                        target_file = gf
                        break
                
                if not target_file:
                    print(f"  [FAIL] Expected stream file ending in {check['stream_suffix']} not found.")
                    test_success = False
                    continue
                
                with open(target_file, 'r', encoding='utf-8', errors='ignore') as f:
                    content = f.read()
                    
                    # Check keywords
                    for keyword in check.get("contains", []):
                        if keyword not in content:
                            print(f"  [FAIL] File {target_file} missing keyword: '{keyword}'")
                            test_success = False
                        else:
                            print(f"  [PASS] File {target_file} contains: '{keyword}'")

                    # Check max occurrences (Repetition bug)
                    for phrase, max_count in check.get("max_occurrences", {}).items():
                        count = content.count(phrase)
                        if count > max_count:
                            print(f"  [FAIL] Repetition detected! '{phrase}' found {count} times (Limit: {max_count})")
                            test_success = False
                        else:
                            print(f"  [PASS] Repetition check: '{phrase}' found {count} times.")

        if test_success:
            print("  -> Test Case Verified!")
        else:
            print("  -> Test Case FAILED.")
            overall_success = False
        print("-" * 40)

    if overall_success:
        print("\nAll tests passed successfully.")
        sys.exit(0)
    else:
        print("\nSome tests failed.")
        sys.exit(1)

if __name__ == "__main__":
    run_test()
