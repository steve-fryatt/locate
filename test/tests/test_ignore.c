/* Copyright 2026, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of Locate:
 *
 *   http://www.stevefryatt.org.uk/software/
 *
 * Licensed under the EUPL, Version 1.2 only (the "Licence");
 * You may not use this work except in compliance with the
 * Licence.
 *
 * You may obtain a copy of the Licence at:
 *
 *   http://joinup.ec.europa.eu/software/page/eupl
 *
 * Unless required by applicable law or agreed to in
 * writing, software distributed under the Licence is
 * distributed on an "AS IS" basis, WITHOUT WARRANTIES
 * OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * See the Licence for the specific language governing
 * permissions and limitations under the Licence.
 */

/**
 * \file: test_textdump.c
 *
 * Unit Tests for the textdump.c
 */

/* ANSI C Header files. */

#include <string.h>

/* Acorn C Header files. */

#include "flex.h"

/* SFLib Header files. */

#include "sflib/heap.h"

/* Unity Header files. */

#include "unity.h"

/* Locate Application header file. */

#include "ignore.h"

/**
 * Unit Test setup.
 */

void setUp(void)
{
	flex_init("Unit Tests", 0, 0);
	heap_initialise();
}

/**
 * Unit Test teardown.
 */

void tearDown(void)
{ }



void test_match_leaf_to_leaf_empty_ci(void)
{
	struct ignore_block *test = ignore_create();
	TEST_ASSERT_NOT_NULL(test);
	TEST_ASSERT_TRUE(ignore_add_node_for_test(test, "foo", FALSE));

	TEST_ASSERT_FALSE(ignore_compare_string_for_test(test, ""));
}

void test_match_leaf_to_leaf_short1_ci(void)
{
	struct ignore_block *test = ignore_create();
	TEST_ASSERT_NOT_NULL(test);
	TEST_ASSERT_TRUE(ignore_add_node_for_test(test, "foo", FALSE));

	TEST_ASSERT_FALSE(ignore_compare_string_for_test(test, "f"));
}

void test_match_leaf_to_leaf_short2_ci(void)
{
	struct ignore_block *test = ignore_create();
	TEST_ASSERT_NOT_NULL(test);
	TEST_ASSERT_TRUE(ignore_add_node_for_test(test, "foo", FALSE));

	TEST_ASSERT_FALSE(ignore_compare_string_for_test(test, "fo"));
}

void test_match_leaf_to_leaf_ci(void)
{
	struct ignore_block *test = ignore_create();
	TEST_ASSERT_NOT_NULL(test);
	TEST_ASSERT_TRUE(ignore_add_node_for_test(test, "foo", FALSE));

	TEST_ASSERT_TRUE(ignore_compare_string_for_test(test, "foo"));
}

void test_match_leaf_to_leaf_long_ci(void)
{
	struct ignore_block *test = ignore_create();
	TEST_ASSERT_NOT_NULL(test);
	TEST_ASSERT_TRUE(ignore_add_node_for_test(test, "foo", FALSE));

	TEST_ASSERT_FALSE(ignore_compare_string_for_test(test, "fooo"));
}

void test_match_leaf_to_path1_ci(void)
{
	struct ignore_block *test = ignore_create();
	TEST_ASSERT_NOT_NULL(test);
	TEST_ASSERT_TRUE(ignore_add_node_for_test(test, "foo", FALSE));

	TEST_ASSERT_TRUE(ignore_compare_string_for_test(test, "foo."));
}

void test_match_leaf_to_path2_ci(void)
{
	struct ignore_block *test = ignore_create();
	TEST_ASSERT_NOT_NULL(test);
	TEST_ASSERT_TRUE(ignore_add_node_for_test(test, "foo", FALSE));

	TEST_ASSERT_FALSE(ignore_compare_string_for_test(test, "bar."));
}

void test_match_leaf_to_path3_ci(void)
{
	struct ignore_block *test = ignore_create();
	TEST_ASSERT_NOT_NULL(test);
	TEST_ASSERT_TRUE(ignore_add_node_for_test(test, "foo", FALSE));

	TEST_ASSERT_TRUE(ignore_compare_string_for_test(test, "foo.bar"));
}

void test_match_leaf_to_leaf_empty_case_ci(void)
{
	struct ignore_block *test = ignore_create();
	TEST_ASSERT_NOT_NULL(test);
	TEST_ASSERT_TRUE(ignore_add_node_for_test(test, "foo", FALSE));

	TEST_ASSERT_FALSE(ignore_compare_string_for_test(test, ""));
}

void test_match_leaf_to_leaf_short1_case_ci(void)
{
	struct ignore_block *test = ignore_create();
	TEST_ASSERT_NOT_NULL(test);
	TEST_ASSERT_TRUE(ignore_add_node_for_test(test, "foo", FALSE));

	TEST_ASSERT_FALSE(ignore_compare_string_for_test(test, "F"));
}

void test_match_leaf_to_leaf_short2_case_ci(void)
{
	struct ignore_block *test = ignore_create();
	TEST_ASSERT_NOT_NULL(test);
	TEST_ASSERT_TRUE(ignore_add_node_for_test(test, "foo", FALSE));

	TEST_ASSERT_FALSE(ignore_compare_string_for_test(test, "Fo"));
}

void test_match_leaf_to_leaf_case_ci(void)
{
	struct ignore_block *test = ignore_create();
	TEST_ASSERT_NOT_NULL(test);
	TEST_ASSERT_TRUE(ignore_add_node_for_test(test, "foo", FALSE));

	TEST_ASSERT_TRUE(ignore_compare_string_for_test(test, "Foo"));
}

void test_match_leaf_to_leaf_long_case_ci(void)
{
	struct ignore_block *test = ignore_create();
	TEST_ASSERT_NOT_NULL(test);
	TEST_ASSERT_TRUE(ignore_add_node_for_test(test, "foo", FALSE));

	TEST_ASSERT_FALSE(ignore_compare_string_for_test(test, "Fooo"));
}

void test_match_leaf_to_path1_case_ci(void)
{
	struct ignore_block *test = ignore_create();
	TEST_ASSERT_NOT_NULL(test);
	TEST_ASSERT_TRUE(ignore_add_node_for_test(test, "foo", FALSE));

	TEST_ASSERT_TRUE(ignore_compare_string_for_test(test, "Foo."));
}

void test_match_leaf_to_path2_case_ci(void)
{
	struct ignore_block *test = ignore_create();
	TEST_ASSERT_NOT_NULL(test);
	TEST_ASSERT_TRUE(ignore_add_node_for_test(test, "foo", FALSE));

	TEST_ASSERT_FALSE(ignore_compare_string_for_test(test, "Bar."));
}

void test_match_leaf_to_path3_case_ci(void)
{
	struct ignore_block *test = ignore_create();
	TEST_ASSERT_NOT_NULL(test);
	TEST_ASSERT_TRUE(ignore_add_node_for_test(test, "foo", FALSE));

	TEST_ASSERT_TRUE(ignore_compare_string_for_test(test, "Foo.Bar"));
}

void test_match_leaf_to_leaf_empty_cs(void)
{
	struct ignore_block *test = ignore_create();
	TEST_ASSERT_NOT_NULL(test);
	TEST_ASSERT_TRUE(ignore_add_node_for_test(test, "foo", TRUE));

	TEST_ASSERT_FALSE(ignore_compare_string_for_test(test, ""));
}

void test_match_leaf_to_leaf_short1_cs(void)
{
	struct ignore_block *test = ignore_create();
	TEST_ASSERT_NOT_NULL(test);
	TEST_ASSERT_TRUE(ignore_add_node_for_test(test, "foo", TRUE));

	TEST_ASSERT_FALSE(ignore_compare_string_for_test(test, "f"));
}

void test_match_leaf_to_leaf_short2_cs(void)
{
	struct ignore_block *test = ignore_create();
	TEST_ASSERT_NOT_NULL(test);
	TEST_ASSERT_TRUE(ignore_add_node_for_test(test, "foo", TRUE));

	TEST_ASSERT_FALSE(ignore_compare_string_for_test(test, "fo"));
}

void test_match_leaf_to_leaf_cs(void)
{
	struct ignore_block *test = ignore_create();
	TEST_ASSERT_NOT_NULL(test);
	TEST_ASSERT_TRUE(ignore_add_node_for_test(test, "foo", TRUE));

	TEST_ASSERT_TRUE(ignore_compare_string_for_test(test, "foo"));
}

void test_match_leaf_to_leaf_long_cs(void)
{
	struct ignore_block *test = ignore_create();
	TEST_ASSERT_NOT_NULL(test);
	TEST_ASSERT_TRUE(ignore_add_node_for_test(test, "foo", TRUE));

	TEST_ASSERT_FALSE(ignore_compare_string_for_test(test, "fooo"));
}

void test_match_leaf_to_path1_cs(void)
{
	struct ignore_block *test = ignore_create();
	TEST_ASSERT_NOT_NULL(test);
	TEST_ASSERT_TRUE(ignore_add_node_for_test(test, "foo", TRUE));

	TEST_ASSERT_TRUE(ignore_compare_string_for_test(test, "foo."));
}

void test_match_leaf_to_path2_cs(void)
{
	struct ignore_block *test = ignore_create();
	TEST_ASSERT_NOT_NULL(test);
	TEST_ASSERT_TRUE(ignore_add_node_for_test(test, "foo", TRUE));

	TEST_ASSERT_FALSE(ignore_compare_string_for_test(test, "bar."));
}

void test_match_leaf_to_path3_cs(void)
{
	struct ignore_block *test = ignore_create();
	TEST_ASSERT_NOT_NULL(test);
	TEST_ASSERT_TRUE(ignore_add_node_for_test(test, "foo", TRUE));

	TEST_ASSERT_TRUE(ignore_compare_string_for_test(test, "foo.bar"));
}

void test_match_leaf_to_leaf_empty_case_cs(void)
{
	struct ignore_block *test = ignore_create();
	TEST_ASSERT_NOT_NULL(test);
	TEST_ASSERT_TRUE(ignore_add_node_for_test(test, "foo", TRUE));

	TEST_ASSERT_FALSE(ignore_compare_string_for_test(test, ""));
}

void test_match_leaf_to_leaf_short1_case_cs(void)
{
	struct ignore_block *test = ignore_create();
	TEST_ASSERT_NOT_NULL(test);
	TEST_ASSERT_TRUE(ignore_add_node_for_test(test, "foo", TRUE));

	TEST_ASSERT_FALSE(ignore_compare_string_for_test(test, "F"));
}

void test_match_leaf_to_leaf_short2_case_cs(void)
{
	struct ignore_block *test = ignore_create();
	TEST_ASSERT_NOT_NULL(test);
	TEST_ASSERT_TRUE(ignore_add_node_for_test(test, "foo", TRUE));

	TEST_ASSERT_FALSE(ignore_compare_string_for_test(test, "Fo"));
}

void test_match_leaf_to_leaf_case_cs(void)
{
	struct ignore_block *test = ignore_create();
	TEST_ASSERT_NOT_NULL(test);
	TEST_ASSERT_TRUE(ignore_add_node_for_test(test, "foo", TRUE));

	TEST_ASSERT_FALSE(ignore_compare_string_for_test(test, "Foo"));
}

void test_match_leaf_to_leaf_long_case_cs(void)
{
	struct ignore_block *test = ignore_create();
	TEST_ASSERT_NOT_NULL(test);
	TEST_ASSERT_TRUE(ignore_add_node_for_test(test, "foo", TRUE));

	TEST_ASSERT_FALSE(ignore_compare_string_for_test(test, "Fooo"));
}

void test_match_leaf_to_path1_case_cs(void)
{
	struct ignore_block *test = ignore_create();
	TEST_ASSERT_NOT_NULL(test);
	TEST_ASSERT_TRUE(ignore_add_node_for_test(test, "foo", TRUE));

	TEST_ASSERT_FALSE(ignore_compare_string_for_test(test, "Foo."));
}

void test_match_leaf_to_path2_case_cs(void)
{
	struct ignore_block *test = ignore_create();
	TEST_ASSERT_NOT_NULL(test);
	TEST_ASSERT_TRUE(ignore_add_node_for_test(test, "foo", TRUE));

	TEST_ASSERT_FALSE(ignore_compare_string_for_test(test, "Bar."));
}

void test_match_leaf_to_path3_case_cs(void)
{
	struct ignore_block *test = ignore_create();
	TEST_ASSERT_NOT_NULL(test);
	TEST_ASSERT_TRUE(ignore_add_node_for_test(test, "foo", TRUE));

	TEST_ASSERT_FALSE(ignore_compare_string_for_test(test, "Foo.Bar"));
}

/**
 * The main test runner.
 */

int main(void)
{
	UNITY_BEGIN();
	RUN_TEST(test_match_leaf_to_leaf_empty_ci);
	RUN_TEST(test_match_leaf_to_leaf_short1_ci);
	RUN_TEST(test_match_leaf_to_leaf_short2_ci);
	RUN_TEST(test_match_leaf_to_leaf_ci);
	RUN_TEST(test_match_leaf_to_leaf_long_ci);
	RUN_TEST(test_match_leaf_to_path1_ci);
	RUN_TEST(test_match_leaf_to_path2_ci);
	RUN_TEST(test_match_leaf_to_path3_ci);
	RUN_TEST(test_match_leaf_to_leaf_empty_case_ci);
	RUN_TEST(test_match_leaf_to_leaf_short1_case_ci);
	RUN_TEST(test_match_leaf_to_leaf_short2_case_ci);
	RUN_TEST(test_match_leaf_to_leaf_case_ci);
	RUN_TEST(test_match_leaf_to_leaf_long_case_ci);
	RUN_TEST(test_match_leaf_to_path1_case_ci);
	RUN_TEST(test_match_leaf_to_path2_case_ci);
	RUN_TEST(test_match_leaf_to_path3_case_ci);
	RUN_TEST(test_match_leaf_to_leaf_empty_cs);
	RUN_TEST(test_match_leaf_to_leaf_short1_cs);
	RUN_TEST(test_match_leaf_to_leaf_short2_cs);
	RUN_TEST(test_match_leaf_to_leaf_cs);
	RUN_TEST(test_match_leaf_to_leaf_long_cs);
	RUN_TEST(test_match_leaf_to_path1_cs);
	RUN_TEST(test_match_leaf_to_path2_cs);
	RUN_TEST(test_match_leaf_to_path3_cs);
	RUN_TEST(test_match_leaf_to_leaf_empty_case_cs);
	RUN_TEST(test_match_leaf_to_leaf_short1_case_cs);
	RUN_TEST(test_match_leaf_to_leaf_short2_case_cs);
	RUN_TEST(test_match_leaf_to_leaf_case_cs);
	RUN_TEST(test_match_leaf_to_leaf_long_case_cs);
	RUN_TEST(test_match_leaf_to_path1_case_cs);
	RUN_TEST(test_match_leaf_to_path2_case_cs);
	RUN_TEST(test_match_leaf_to_path3_case_cs);
	return UNITY_END();
}