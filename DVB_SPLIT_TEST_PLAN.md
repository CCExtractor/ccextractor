# DVB Split Feature Test Plan

## Quick Verification Checklist

### 1. File Creation Test
```bash
./ccextractor multiprogram_spain.ts --split-dvb-subs -o /tmp/test
```
**Check:** Split files created with correct naming pattern
- ✅ `test_spa_0x00D3.srt` exists
- ✅ `test_spa_0x05E7.srt` exists
- ❌ FAIL if 0 bytes

### 2. Content Test
```bash
wc -l /tmp/test_*.srt
head -20 /tmp/test_spa_*.srt
```
**Check:**
- ✅ Non-zero line count
- ✅ Valid SRT format (numbered entries, timestamps, text)
- ❌ FAIL if empty or malformed

### 3. No Repetition Test (Bug 1)
```bash
cat /tmp/test_spa_0x00D3.srt | grep -E "^[A-Za-z]" | sort | uniq -c | awk '$1>1'
```
**Check:** No duplicate lines → ✅ Bug 1 fixed

### 4. Comparison Test
```bash
# Without split
./ccextractor multiprogram_spain.ts -o /tmp/nosplit.srt
wc -l /tmp/nosplit.srt

# With split  
./ccextractor multiprogram_spain.ts --split-dvb-subs -o /tmp/split
cat /tmp/split_*.srt | wc -l
```
**Check:** Split output should have similar total content

### 5. Edge Cases
| Test | Command | Expected |
|------|---------|----------|
| No DVB subs | `./ccextractor BBC1.mp4 --split-dvb-subs` | Graceful exit, no crash |
| Single stream | `./ccextractor mp_spain_C49.ts --split-dvb-subs` | One split file |
| Large file | Use 11GB test file | No timeout, no crash |

## Pass Criteria
- [ ] All split files > 0 bytes
- [ ] Valid SRT content in each file
- [ ] No repeated subtitle text
- [ ] No crashes on edge cases
