/* parse/f-info */

#include "unit-test.h"
#include "unit-test-data.h"
#include "init.h"


int setup_tests(void **state) {
	*state = init_parse_feat();
	return !*state;
}

int teardown_tests(void *state) {
	struct feature *f = parser_priv(state);
	string_free(f->die_msg);
	string_free(f->hurt_msg);
	string_free(f->run_msg);
	string_free(f->walk_msg);
	string_free(f->mimic);
	string_free(f->desc);
	string_free(f->name);
	mem_free(f);
	parser_destroy(state);
	return 0;
}

static int test_name0(void *state) {
	enum parser_error r = parser_parse(state, "name:Test Feature");
	struct feature *f;

	eq(r, PARSE_ERROR_NONE);
	f = parser_priv(state);
	require(f);
	require(streq(f->name, "Test Feature"));
	ok;
}

static int test_graphics0(void *state) {
	enum parser_error r = parser_parse(state, "graphics:::red");
	struct feature *f;

	eq(r, PARSE_ERROR_NONE);
	f = parser_priv(state);
	require(f);
	eq(f->d_char, L':');
	eq(f->d_attr, COLOUR_RED);
	ok;
}

static int test_mimic0(void *state) {
	enum parser_error r = parser_parse(state, "mimic:marshmallow");
	struct feature *f;

	eq(r, PARSE_ERROR_NONE);
	f = parser_priv(state);
	require(f);
	require(streq(f->mimic, "marshmallow"));
	ok;
}

static int test_priority0(void *state) {
	enum parser_error r = parser_parse(state, "priority:2");
	struct feature *f;

	eq(r, PARSE_ERROR_NONE);
	f = parser_priv(state);
	require(f);
	eq(f->priority, 2);
	ok;
}

static int test_flags0(void *state) {
	enum parser_error r = parser_parse(state, "flags:LOS | PERMANENT | DOWNSTAIR");
	struct feature *f;

	eq(r, PARSE_ERROR_NONE);
	f = parser_priv(state);
	require(f);
	require(f->flags);
	ok;
}

static int test_info0(void *state) {
	enum parser_error r = parser_parse(state, "info:9:2:0");
	struct feature *f;

	eq(r, PARSE_ERROR_NONE);
	f = parser_priv(state);
	require(f);
	eq(f->forge_bonus, 9);
	eq(f->dig, 2);
	eq(f->pit_difficulty, 0);
	ok;
}

static int test_walk_msg0(void *state) {
	enum parser_error r = parser_parse(state, "walk-msg:lookout ");
	struct feature *f;

	eq(r, PARSE_ERROR_NONE);
	f = parser_priv(state);
	require(f);
	require(streq(f->walk_msg, "lookout "));
	ok;
}

static int test_run_msg0(void *state) {
	enum parser_error r = parser_parse(state, "run-msg:lookout! ");
	struct feature *f;

	eq(r, PARSE_ERROR_NONE);
	f = parser_priv(state);
	require(f);
	require(streq(f->run_msg, "lookout! "));
	ok;
}

static int test_hurt_msg0(void *state) {
	enum parser_error r = parser_parse(state, "hurt-msg:ow!");
	struct feature *f;

	eq(r, PARSE_ERROR_NONE);
	f = parser_priv(state);
	require(f);
	require(streq(f->hurt_msg, "ow!"));
	ok;
}

static int test_die_msg0(void *state) {
	enum parser_error r = parser_parse(state, "die-msg:aargh");
	struct feature *f;

	eq(r, PARSE_ERROR_NONE);
	f = parser_priv(state);
	require(f);
	require(streq(f->die_msg, "aargh"));
	ok;
}

static int test_dig_msg0(void *state) {
	enum parser_error r = parser_parse(state, "dig-msg:urgh");
	struct feature *f;

	eq(r, PARSE_ERROR_NONE);
	f = parser_priv(state);
	require(f);
	require(streq(f->dig_msg, "urgh"));
	ok;
}

static int test_fail_msg0(void *state) {
	enum parser_error r = parser_parse(state, "fail-msg:D'oh");
	struct feature *f;

	eq(r, PARSE_ERROR_NONE);
	f = parser_priv(state);
	require(f);
	require(streq(f->fail_msg, "D'oh"));
	ok;
}

static int test_str_msg0(void *state) {
	enum parser_error r = parser_parse(state, "str-msg:hnnnh");
	struct feature *f;

	eq(r, PARSE_ERROR_NONE);
	f = parser_priv(state);
	require(f);
	require(streq(f->str_msg, "hnnnh"));
	ok;
}

static int test_confused_msg0(void *state) {
	enum parser_error r = parser_parse(state, "confused-msg:huh?");
	struct feature *f;

	eq(r, PARSE_ERROR_NONE);
	f = parser_priv(state);
	require(f);
	require(streq(f->confused_msg, "huh?"));
	ok;
}

const char *suite_name = "parse/f-info";
struct test tests[] = {
	{ "name0", test_name0 },
	{ "graphics0", test_graphics0 },
	{ "mimic0", test_mimic0 },
	{ "priority0", test_priority0 },
	{ "flags0", test_flags0 },
	{ "info0", test_info0 },
	{ "walk_msg0", test_walk_msg0 },
	{ "run_msg0", test_run_msg0 },
	{ "hurt_msg0", test_hurt_msg0 },
	{ "die_msg0", test_die_msg0 },
	{ "dig_msg0", test_dig_msg0 },
	{ "fail_msg0", test_fail_msg0 },
	{ "str_msg0", test_str_msg0 },
	{ "confused_msg0", test_confused_msg0 },
	{ NULL, NULL }
};
