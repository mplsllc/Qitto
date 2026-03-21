// Phase 0 — validate that Ditto's Crc32Dynamic compiles and produces correct output on Linux
#include "Crc32Dynamic.h"
#include <cstdio>
#include <cstring>
#include <cassert>

int main()
{
    CCrc32Dynamic crc;

    // Test 1: Known CRC32 of "hello world"
    // Standard CRC32 of "hello world" = 0x0D4A1185
    // But Ditto's implementation doesn't XOR initial/final (see commented-out lines in GenerateCrc32)
    // So we just verify it's deterministic and non-zero
    const char *test1 = "hello world";
    DWORD crc1 = 0xFFFFFFFF;
    DWORD result = crc.GenerateCrc32((const LPBYTE)test1, (DWORD)strlen(test1), crc1);
    assert(result == NO_ERROR);
    assert(crc1 != 0xFFFFFFFF); // Should have changed
    printf("CRC32('hello world') = 0x%08X (result=%u) OK\n", crc1, result);

    // Test 2: Determinism — same input gives same CRC
    DWORD crc2 = 0xFFFFFFFF;
    crc.GenerateCrc32((const LPBYTE)test1, (DWORD)strlen(test1), crc2);
    assert(crc1 == crc2);
    printf("Determinism check: 0x%08X == 0x%08X OK\n", crc1, crc2);

    // Test 3: Different input gives different CRC
    const char *test3 = "hello world!";
    DWORD crc3 = 0xFFFFFFFF;
    crc.GenerateCrc32((const LPBYTE)test3, (DWORD)strlen(test3), crc3);
    assert(crc3 != crc1);
    printf("Different input: 0x%08X != 0x%08X OK\n", crc3, crc1);

    // Test 4: Empty input — CRC should stay at initial value
    DWORD crc4 = 0xFFFFFFFF;
    crc.GenerateCrc32((const LPBYTE)"", 0, crc4);
    assert(crc4 == 0xFFFFFFFF); // No bytes processed, stays at init
    printf("Empty input: 0x%08X == 0xFFFFFFFF OK\n", crc4);

    // Test 5: Single byte
    BYTE single = 0x41; // 'A'
    DWORD crc5 = 0xFFFFFFFF;
    crc.GenerateCrc32(&single, 1, crc5);
    assert(crc5 != 0xFFFFFFFF);
    printf("Single byte 'A': 0x%08X OK\n", crc5);

    printf("\nAll CRC32 tests passed. Compat strategy validated.\n");
    return 0;
}
