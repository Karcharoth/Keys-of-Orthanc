/* monster/monster 
 *
 * Tests for monster/monster?.c
 *
 * Created by: myshkin
 *             26 Apr 2011
 */

#include "unit-test.h"
#include "unit-test-data.h"
#include "test-utils.h"
#include "mon-util.h"

int setup_tests(void **state) {
	read_edit_files();
	*state = 0;
	return 0;
}

int teardown_tests(void *state) {
	mem_free(state);
	return 0;
}

/* Regression test for #1409 */
static int test_match_monster_bases(void *state) {
	struct monster_base *base;

	/* The third monster - Orc */
	base = (&r_info[3])->base;
	require(match_monster_bases(base, "orc", NULL));
	require(match_monster_bases(base, "zephyr hound", "orc", NULL));
	require(!match_monster_bases(base, "angel", NULL));
	require(!match_monster_bases(base, "lich", "vampire", "wraith", NULL));

	/* Saruman */
	base = (lookup_monster("Saruman of Many Colours"))->base;
	require(!match_monster_bases(base, "canine", NULL));
	require(!match_monster_bases(base, "lich", "vampire", "wraith", NULL));
	require(match_monster_bases(base, "person", "horror", NULL));
	require(match_monster_bases(base, "person", NULL));

	ok;
}

const char *suite_name = "monster/monster";
struct test tests[] = {
	{ "match_monster_bases", test_match_monster_bases },
	{ NULL, NULL }
};
