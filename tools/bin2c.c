/*
 * Unloved program to convert a binary on stdin to a C include on stdout
 *
 * Jan 1999 Matt Mackall <mpm@selenic.com>
 * Jun 2017 Juan Pedro Bolivar Puente <raskolnikov@gnu.org>
 *
 * This software may be used and distributed according to the terms
 * of the GNU General Public License, incorporated herein by reference.
 */

#include <stdio.h>

int main(int argc, char *argv[])
{
	int ch, total = 0;

	if (argc > 1)
		printf("const std::uint8_t %s[] %s= {\n",
			argv[1], argc > 2 ? argv[2] : "");

	do {
		while ((ch = getchar()) != EOF) {
			total++;
			printf("0x%02x,", ch);
			if (total % 16 == 0)
				break;
		}
	} while (ch != EOF);

	if (argc > 1)
		printf("\n};\nconst std::size_t %s_size = %d;\n",
		       argv[1], total);

	return 0;
}
