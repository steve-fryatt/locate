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
 * \file: test_flexutils.c
 *
 * Unit Tests for the flexutils.c code.
 */

/* ANSI C Header files. */

#include <string.h>

/* Acorn C Header files. */

#include "flex.h"

/* SFLib Header files. */

/* Unity Header files. */

#include "unity.h"

/* Locate Application header file. */

#include "../src/flexutils.h"

/**
 * Unit Test setup.
 */

void setUp(void)
{
	flex_init("Unit Tests", 0, 0);
}

/**
 * Unit Test teardown.
 */

void tearDown(void)
{ }

/**
 * The Unit Tests.
 */

/*** Tests with some invalid inputs. ***/

/* Various combinations with no flex_ptr, which should just fail by default. */

void test_no_flex_copy_null_string(void)
{
	TEST_ASSERT_FALSE_MESSAGE(flexutils_store_string((flex_ptr) NULL, NULL, NULL), "Call to flexutils returns false");
}

void test_no_flex_copy_null_string_with_wrapper(void)
{
	TEST_ASSERT_FALSE_MESSAGE(flexutils_store_string((flex_ptr) NULL, NULL, ""), "Call to flexutils returns false");
}

void test_no_flex_copy_valid_string(void)
{
	TEST_ASSERT_FALSE_MESSAGE(flexutils_store_string((flex_ptr) NULL, "", NULL), "Call to flexutils returns false");
}

void test_no_flex_copy_valid_string_with_wrapper(void)
{
	TEST_ASSERT_FALSE_MESSAGE(flexutils_store_string((flex_ptr) NULL, "", ""), "Call to flexutils returns false");
}

/* Various combinations with a NULL flex_ptr and a NULL text pointer. */

void test_null_flex_copy_null_string(void)
{
	char *text = NULL;

	TEST_ASSERT_FALSE_MESSAGE(flexutils_store_string((flex_ptr) &text, NULL, NULL), "Call to flexutils returns false");
}

void test_null_flex_copy_null_string_with_wrapper(void)
{
	char *text = NULL;

	TEST_ASSERT_FALSE_MESSAGE(flexutils_store_string((flex_ptr) &text, NULL, ""), "Call to flexutils returns false");
}

/* Various combinations with an existing flex_ptr and a NULL text pointer. */

void test_valid_flex_copy_null_string(void)
{
	char *text = NULL;

	TEST_ASSERT_EQUAL_INT_MESSAGE(1, flex_alloc((flex_ptr) &text, 5), "Allocate dummy flex block");
	TEST_ASSERT_NOT_NULL_MESSAGE(text, "Flex block now points to memory");

	TEST_ASSERT_FALSE_MESSAGE(flexutils_store_string((flex_ptr) &text, NULL, NULL), "Call to flexutils returns false");
}

void test_valid_flex_copy_null_string_with_wrapper(void)
{
	char *text = NULL;

	TEST_ASSERT_EQUAL_INT_MESSAGE(1, flex_alloc((flex_ptr) &text, 5), "Allocate dummy flex block");
	TEST_ASSERT_NOT_NULL_MESSAGE(text, "Flex block now points to memory");

	TEST_ASSERT_FALSE_MESSAGE(flexutils_store_string((flex_ptr) &text, NULL, ""), "Call to flexutils returns false");
}

/*** Tests with valid inputs. ***/

/* Copy an empty string with no wrapper. */

void test_empty_string_into_null_ptr(void)
{
	char *text = NULL;

	TEST_ASSERT_TRUE_MESSAGE(flexutils_store_string((flex_ptr) &text, "", NULL), "Call to flexutils returns true");

	TEST_ASSERT_NOT_NULL_MESSAGE(text, "A flex block was allocated");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("", text, "The correct buffer contents returned");
	TEST_ASSERT_EQUAL_INT_MESSAGE(1, flex_size((flex_ptr) &text), "The returned block was the correct size");
}

void test_empty_string_into_existing_ptr(void)
{
	char *text = NULL;

	TEST_ASSERT_EQUAL_INT_MESSAGE(1, flex_alloc((flex_ptr) &text, 5), "Allocate dummy flex block");
	TEST_ASSERT_NOT_NULL_MESSAGE(text, "Flex block now points to memory");

	TEST_ASSERT_TRUE_MESSAGE(flexutils_store_string((flex_ptr) &text, "", NULL), "Call to flexutils returns true");

	TEST_ASSERT_NOT_NULL_MESSAGE(text, "A flex block was allocated");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("", text, "The correct buffer contents returned");
	TEST_ASSERT_EQUAL_INT_MESSAGE(1, flex_size((flex_ptr) &text), "The returned block was the correct size");
}

/* Copy a single character string with no wrapper. */

void test_1char_string_into_null_ptr(void)
{
	char *text = NULL;

	TEST_ASSERT_TRUE_MESSAGE(flexutils_store_string((flex_ptr) &text, "A", NULL), "Call to flexutils returns true");

	TEST_ASSERT_NOT_NULL_MESSAGE(text, "A flex block was allocated");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("A", text, "The correct buffer contents returned");
	TEST_ASSERT_EQUAL_INT_MESSAGE(2, flex_size((flex_ptr) &text), "The returned block was the correct size");
}

void test_1char_string_into_existing_ptr(void)
{
	char *text = NULL;

	TEST_ASSERT_EQUAL_INT_MESSAGE(1, flex_alloc((flex_ptr) &text, 5), "Allocate dummy flex block");
	TEST_ASSERT_NOT_NULL_MESSAGE(text, "Flex block now points to memory");

	TEST_ASSERT_TRUE_MESSAGE(flexutils_store_string((flex_ptr) &text, "A", NULL), "Call to flexutils returns true");

	TEST_ASSERT_NOT_NULL_MESSAGE(text, "A flex block was allocated");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("A", text, "The correct buffer contents returned");
	TEST_ASSERT_EQUAL_INT_MESSAGE(2, flex_size((flex_ptr) &text), "The returned block was the correct size");
}

/* Copy a 10 character string with no wrapper. */

void test_10char_string_into_null_ptr(void)
{
	char *text = NULL;

	TEST_ASSERT_TRUE_MESSAGE(flexutils_store_string((flex_ptr) &text, "ABCDEFGHIJ", NULL), "Call to flexutils returns true");

	TEST_ASSERT_NOT_NULL_MESSAGE(text, "A flex block was allocated");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("ABCDEFGHIJ", text, "The correct buffer contents returned");
	TEST_ASSERT_EQUAL_INT_MESSAGE(11, flex_size((flex_ptr) &text), "The returned block was the correct size");
}

void test_10char_string_into_existing_ptr(void)
{
	char *text = NULL;

	TEST_ASSERT_EQUAL_INT_MESSAGE(1, flex_alloc((flex_ptr) &text, 5), "Allocate dummy flex block");
	TEST_ASSERT_NOT_NULL_MESSAGE(text, "Flex block now points to memory");

	TEST_ASSERT_TRUE_MESSAGE(flexutils_store_string((flex_ptr) &text, "ABCDEFGHIJ", NULL), "Call to flexutils returns true");

	TEST_ASSERT_NOT_NULL_MESSAGE(text, "A flex block was allocated");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("ABCDEFGHIJ", text, "The correct buffer contents returned");
	TEST_ASSERT_EQUAL_INT_MESSAGE(11, flex_size((flex_ptr) &text), "The returned block was the correct size");
}

/* Copy an empty string with empty wrapper. */

void test_empty_string_and_empty_wrapper_into_null_ptr(void)
{
	char *text = NULL;

	TEST_ASSERT_TRUE_MESSAGE(flexutils_store_string((flex_ptr) &text, "", ""), "Call to flexutils returns true");

	TEST_ASSERT_NOT_NULL_MESSAGE(text, "A flex block was allocated");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("", text, "The correct buffer contents returned");
	TEST_ASSERT_EQUAL_INT_MESSAGE(1, flex_size((flex_ptr) &text), "The returned block was the correct size");
}

void test_empty_string_and_empty_wrapper_into_existing_ptr(void)
{
	char *text = NULL;

	TEST_ASSERT_EQUAL_INT_MESSAGE(1, flex_alloc((flex_ptr) &text, 5), "Allocate dummy flex block");
	TEST_ASSERT_NOT_NULL_MESSAGE(text, "Flex block now points to memory");

	TEST_ASSERT_TRUE_MESSAGE(flexutils_store_string((flex_ptr) &text, "", ""), "Call to flexutils returns true");

	TEST_ASSERT_NOT_NULL_MESSAGE(text, "A flex block was allocated");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("", text, "The correct buffer contents returned");
	TEST_ASSERT_EQUAL_INT_MESSAGE(1, flex_size((flex_ptr) &text), "The returned block was the correct size");
}

/* Copy a single character string with empty wrapper. */

void test_1char_string_and_empty_wrapper_into_null_ptr(void)
{
	char *text = NULL;

	TEST_ASSERT_TRUE_MESSAGE(flexutils_store_string((flex_ptr) &text, "A", ""), "Call to flexutils returns true");

	TEST_ASSERT_NOT_NULL_MESSAGE(text, "A flex block was allocated");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("A", text, "The correct buffer contents returned");
	TEST_ASSERT_EQUAL_INT_MESSAGE(2, flex_size((flex_ptr) &text), "The returned block was the correct size");
}

void test_1char_string_and_empty_wrapper_into_existing_ptr(void)
{
	char *text = NULL;

	TEST_ASSERT_EQUAL_INT_MESSAGE(1, flex_alloc((flex_ptr) &text, 5), "Allocate dummy flex block");
	TEST_ASSERT_NOT_NULL_MESSAGE(text, "Flex block now points to memory");

	TEST_ASSERT_TRUE_MESSAGE(flexutils_store_string((flex_ptr) &text, "A", ""), "Call to flexutils returns true");

	TEST_ASSERT_NOT_NULL_MESSAGE(text, "A flex block was allocated");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("A", text, "The correct buffer contents returned");
	TEST_ASSERT_EQUAL_INT_MESSAGE(2, flex_size((flex_ptr) &text), "The returned block was the correct size");
}

/* Copy a 10 character string with empty wrapper. */

void test_10char_string_and_empty_wrapper_into_null_ptr(void)
{
	char *text = NULL;

	TEST_ASSERT_TRUE_MESSAGE(flexutils_store_string((flex_ptr) &text, "ABCDEFGHIJ", ""), "Call to flexutils returns true");

	TEST_ASSERT_NOT_NULL_MESSAGE(text, "A flex block was allocated");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("ABCDEFGHIJ", text, "The correct buffer contents returned");
	TEST_ASSERT_EQUAL_INT_MESSAGE(11, flex_size((flex_ptr) &text), "The returned block was the correct size");
}

void test_10char_string_and_empty_wrapper_into_existing_ptr(void)
{
	char *text = NULL;

	TEST_ASSERT_EQUAL_INT_MESSAGE(1, flex_alloc((flex_ptr) &text, 5), "Allocate dummy flex block");
	TEST_ASSERT_NOT_NULL_MESSAGE(text, "Flex block now points to memory");

	TEST_ASSERT_TRUE_MESSAGE(flexutils_store_string((flex_ptr) &text, "ABCDEFGHIJ", ""), "Call to flexutils returns true");

	TEST_ASSERT_NOT_NULL_MESSAGE(text, "A flex block was allocated");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("ABCDEFGHIJ", text, "The correct buffer contents returned");
	TEST_ASSERT_EQUAL_INT_MESSAGE(11, flex_size((flex_ptr) &text), "The returned block was the correct size");
}

/* Copy an empty string with 1 character wrapper. */

void test_empty_string_and_1char_wrapper_into_null_ptr(void)
{
	char *text = NULL;

	TEST_ASSERT_TRUE_MESSAGE(flexutils_store_string((flex_ptr) &text, "", "1"), "Call to flexutils returns true");

	TEST_ASSERT_NOT_NULL_MESSAGE(text, "A flex block was allocated");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("11", text, "The correct buffer contents returned");
	TEST_ASSERT_EQUAL_INT_MESSAGE(3, flex_size((flex_ptr) &text), "The returned block was the correct size");
}

void test_empty_string_and_1char_wrapper_into_existing_ptr(void)
{
	char *text = NULL;

	TEST_ASSERT_EQUAL_INT_MESSAGE(1, flex_alloc((flex_ptr) &text, 5), "Allocate dummy flex block");
	TEST_ASSERT_NOT_NULL_MESSAGE(text, "Flex block now points to memory");

	TEST_ASSERT_TRUE_MESSAGE(flexutils_store_string((flex_ptr) &text, "", "1"), "Call to flexutils returns true");

	TEST_ASSERT_NOT_NULL_MESSAGE(text, "A flex block was allocated");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("11", text, "The correct buffer contents returned");
	TEST_ASSERT_EQUAL_INT_MESSAGE(3, flex_size((flex_ptr) &text), "The returned block was the correct size");
}

/* Copy a single character string with 1 character wrapper. */

void test_1char_string_and_1char_wrapper_into_null_ptr(void)
{
	char *text = NULL;

	TEST_ASSERT_TRUE_MESSAGE(flexutils_store_string((flex_ptr) &text, "A", "1"), "Call to flexutils returns true");

	TEST_ASSERT_NOT_NULL_MESSAGE(text, "A flex block was allocated");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("1A1", text, "The correct buffer contents returned");
	TEST_ASSERT_EQUAL_INT_MESSAGE(4, flex_size((flex_ptr) &text), "The returned block was the correct size");
}

void test_1char_string_and_1char_wrapper_into_existing_ptr(void)
{
	char *text = NULL;

	TEST_ASSERT_EQUAL_INT_MESSAGE(1, flex_alloc((flex_ptr) &text, 5), "Allocate dummy flex block");
	TEST_ASSERT_NOT_NULL_MESSAGE(text, "Flex block now points to memory");

	TEST_ASSERT_TRUE_MESSAGE(flexutils_store_string((flex_ptr) &text, "A", "1"), "Call to flexutils returns true");

	TEST_ASSERT_NOT_NULL_MESSAGE(text, "A flex block was allocated");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("1A1", text, "The correct buffer contents returned");
	TEST_ASSERT_EQUAL_INT_MESSAGE(4, flex_size((flex_ptr) &text), "The returned block was the correct size");
}

/* Copy a 10 character string with 1 character wrapper. */

void test_10char_string_and_1char_wrapper_into_null_ptr(void)
{
	char *text = NULL;

	TEST_ASSERT_TRUE_MESSAGE(flexutils_store_string((flex_ptr) &text, "ABCDEFGHIJ", "1"), "Call to flexutils returns true");

	TEST_ASSERT_NOT_NULL_MESSAGE(text, "A flex block was allocated");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("1ABCDEFGHIJ1", text, "The correct buffer contents returned");
	TEST_ASSERT_EQUAL_INT_MESSAGE(13, flex_size((flex_ptr) &text), "The returned block was the correct size");
}

void test_10char_string_and_1char_wrapper_into_existing_ptr(void)
{
	char *text = NULL;

	TEST_ASSERT_EQUAL_INT_MESSAGE(1, flex_alloc((flex_ptr) &text, 5), "Allocate dummy flex block");
	TEST_ASSERT_NOT_NULL_MESSAGE(text, "Flex block now points to memory");

	TEST_ASSERT_TRUE_MESSAGE(flexutils_store_string((flex_ptr) &text, "ABCDEFGHIJ", "1"), "Call to flexutils returns true");

	TEST_ASSERT_NOT_NULL_MESSAGE(text, "A flex block was allocated");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("1ABCDEFGHIJ1", text, "The correct buffer contents returned");
	TEST_ASSERT_EQUAL_INT_MESSAGE(13, flex_size((flex_ptr) &text), "The returned block was the correct size");
}

/* Copy an empty string with 10 character wrapper. */

void test_empty_string_and_10char_wrapper_into_null_ptr(void)
{
	char *text = NULL;

	TEST_ASSERT_TRUE_MESSAGE(flexutils_store_string((flex_ptr) &text, "", "1234567890"), "Call to flexutils returns true");

	TEST_ASSERT_NOT_NULL_MESSAGE(text, "A flex block was allocated");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("12345678901234567890", text, "The correct buffer contents returned");
	TEST_ASSERT_EQUAL_INT_MESSAGE(21, flex_size((flex_ptr) &text), "The returned block was the correct size");
}

void test_empty_string_and_10char_wrapper_into_existing_ptr(void)
{
	char *text = NULL;

	TEST_ASSERT_EQUAL_INT_MESSAGE(1, flex_alloc((flex_ptr) &text, 5), "Allocate dummy flex block");
	TEST_ASSERT_NOT_NULL_MESSAGE(text, "Flex block now points to memory");

	TEST_ASSERT_TRUE_MESSAGE(flexutils_store_string((flex_ptr) &text, "", "1234567890"), "Call to flexutils returns true");

	TEST_ASSERT_NOT_NULL_MESSAGE(text, "A flex block was allocated");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("12345678901234567890", text, "The correct buffer contents returned");
	TEST_ASSERT_EQUAL_INT_MESSAGE(21, flex_size((flex_ptr) &text), "The returned block was the correct size");
}

/* Copy a single character string with 10 character wrapper. */

void test_1char_string_and_10char_wrapper_into_null_ptr(void)
{
	char *text = NULL;

	TEST_ASSERT_TRUE_MESSAGE(flexutils_store_string((flex_ptr) &text, "A", "1234567890"), "Call to flexutils returns true");

	TEST_ASSERT_NOT_NULL_MESSAGE(text, "A flex block was allocated");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("1234567890A1234567890", text, "The correct buffer contents returned");
	TEST_ASSERT_EQUAL_INT_MESSAGE(22, flex_size((flex_ptr) &text), "The returned block was the correct size");
}

void test_1char_string_and_10char_wrapper_into_existing_ptr(void)
{
	char *text = NULL;

	TEST_ASSERT_EQUAL_INT_MESSAGE(1, flex_alloc((flex_ptr) &text, 5), "Allocate dummy flex block");
	TEST_ASSERT_NOT_NULL_MESSAGE(text, "Flex block now points to memory");

	TEST_ASSERT_TRUE_MESSAGE(flexutils_store_string((flex_ptr) &text, "A", "1234567890"), "Call to flexutils returns true");

	TEST_ASSERT_NOT_NULL_MESSAGE(text, "A flex block was allocated");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("1234567890A1234567890", text, "The correct buffer contents returned");
	TEST_ASSERT_EQUAL_INT_MESSAGE(22, flex_size((flex_ptr) &text), "The returned block was the correct size");
}

/* Copy a 10 character string with 10 character wrapper. */

void test_10char_string_and_10char_wrapper_into_null_ptr(void)
{
	char *text = NULL;

	TEST_ASSERT_TRUE_MESSAGE(flexutils_store_string((flex_ptr) &text, "ABCDEFGHIJ", "1234567890"), "Call to flexutils returns true");

	TEST_ASSERT_NOT_NULL_MESSAGE(text, "A flex block was allocated");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("1234567890ABCDEFGHIJ1234567890", text, "The correct buffer contents returned");
	TEST_ASSERT_EQUAL_INT_MESSAGE(31, flex_size((flex_ptr) &text), "The returned block was the correct size");
}

void test_10char_string_and_10char_wrapper_into_existing_ptr(void)
{
	char *text = NULL;

	TEST_ASSERT_EQUAL_INT_MESSAGE(1, flex_alloc((flex_ptr) &text, 5), "Allocate dummy flex block");
	TEST_ASSERT_NOT_NULL_MESSAGE(text, "Flex block now points to memory");

	TEST_ASSERT_TRUE_MESSAGE(flexutils_store_string((flex_ptr) &text, "ABCDEFGHIJ", "1234567890"), "Call to flexutils returns true");

	TEST_ASSERT_NOT_NULL_MESSAGE(text, "A flex block was allocated");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("1234567890ABCDEFGHIJ1234567890", text, "The correct buffer contents returned");
	TEST_ASSERT_EQUAL_INT_MESSAGE(31, flex_size((flex_ptr) &text), "The returned block was the correct size");
}

/**
 * The main test runner.
 */

int main(void)
{
	UNITY_BEGIN();
	RUN_TEST(test_no_flex_copy_null_string);
	RUN_TEST(test_no_flex_copy_null_string_with_wrapper);
	RUN_TEST(test_no_flex_copy_valid_string);
	RUN_TEST(test_no_flex_copy_valid_string_with_wrapper);
	RUN_TEST(test_null_flex_copy_null_string);
	RUN_TEST(test_null_flex_copy_null_string_with_wrapper);
	RUN_TEST(test_valid_flex_copy_null_string);
	RUN_TEST(test_valid_flex_copy_null_string_with_wrapper);
	RUN_TEST(test_empty_string_into_null_ptr);
	RUN_TEST(test_empty_string_into_existing_ptr);
	RUN_TEST(test_1char_string_into_null_ptr);
	RUN_TEST(test_1char_string_into_existing_ptr);
	RUN_TEST(test_10char_string_into_null_ptr);
	RUN_TEST(test_10char_string_into_existing_ptr);
	RUN_TEST(test_empty_string_and_empty_wrapper_into_null_ptr);
	RUN_TEST(test_empty_string_and_empty_wrapper_into_existing_ptr);
	RUN_TEST(test_1char_string_and_empty_wrapper_into_null_ptr);
	RUN_TEST(test_1char_string_and_empty_wrapper_into_existing_ptr);
	RUN_TEST(test_10char_string_and_empty_wrapper_into_null_ptr);
	RUN_TEST(test_10char_string_and_empty_wrapper_into_existing_ptr);
	RUN_TEST(test_empty_string_and_1char_wrapper_into_null_ptr);
	RUN_TEST(test_empty_string_and_1char_wrapper_into_existing_ptr);
	RUN_TEST(test_1char_string_and_1char_wrapper_into_null_ptr);
	RUN_TEST(test_1char_string_and_1char_wrapper_into_existing_ptr);
	RUN_TEST(test_10char_string_and_1char_wrapper_into_null_ptr);
	RUN_TEST(test_10char_string_and_1char_wrapper_into_existing_ptr);
	RUN_TEST(test_empty_string_and_10char_wrapper_into_null_ptr);
	RUN_TEST(test_empty_string_and_10char_wrapper_into_existing_ptr);
	RUN_TEST(test_1char_string_and_10char_wrapper_into_null_ptr);
	RUN_TEST(test_1char_string_and_10char_wrapper_into_existing_ptr);
	RUN_TEST(test_10char_string_and_10char_wrapper_into_null_ptr);
	RUN_TEST(test_10char_string_and_10char_wrapper_into_existing_ptr);
	return UNITY_END();
}