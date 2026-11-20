/*
 * Regression test for user_data_registered_itu_t_t35() header bounds (#2468).
 * Mirrors the production guard and initial header reads from avc_functions.c.
 *
 * Build:
 *   gcc -O0 -g -fsanitize=address,undefined -fno-sanitize-recover=all \
 *       -o t35_header_short_test t35_header_short_test.c
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static void parse_t35_header(unsigned char *userbuf, unsigned char *userend)
{
	unsigned char *tbuf = userbuf;

	if (userend - userbuf < 3)
		return;

	int itu_t_t35_country_code = *((uint8_t *)tbuf);
	tbuf++;
	int itu_t_35_provider_code = *tbuf * 256 + *(tbuf + 1);
	tbuf += 2;

	if (itu_t_t35_country_code != 0xB5)
		return;

	switch (itu_t_35_provider_code)
	{
		case 0x0031:
		case 0x002F:
			/* Full caption parsing needs more bytes; not exercised here. */
			break;
		default:
			break;
	}
}

static void expect_no_crash(const char *label, unsigned char *buf, size_t len)
{
	printf("  %s (len=%zu)... ", label, len);
	fflush(stdout);
	parse_t35_header(buf, buf + len);
	printf("ok\n");
}

int main(void)
{
	unsigned char empty[1];
	unsigned char one[1] = {0xB5};
	unsigned char two[2] = {0xB5, 0x00};
	/* Unsupported provider: only the mandatory 3-byte header is consumed. */
	unsigned char three[3] = {0xB5, 0x99, 0x99};

	printf("=== ITU-T T.35 short header (ASan) ===\n");
	expect_no_crash("empty payload", empty, 0);
	expect_no_crash("1-byte payload", one, 1);
	expect_no_crash("2-byte payload", two, 2);
	expect_no_crash("3-byte header only", three, 3);
	printf("All cases passed.\n");
	return EXIT_SUCCESS;
}