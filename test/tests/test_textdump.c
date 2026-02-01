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
 * Unit Tests for the textdump.c module.
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

#include "../src/textdump.h"

/**
 * The default allocation block size.
 * (should be the same as defined in textdump.h)
 */

#define TEST_TEXTDUMP_ALLOCATION 1024

/**
 * Unit Test setup.
 */

void setUp(void) {
	flex_init("Unit Tests", 0, 0);
	heap_initialise();
}

/**
 * Unit Test teardown.
 */

void tearDown(void) {

}

/**
 * Test that we can create a textdump with the default options.
 */

void test_textdump_create_default(void)
{
	/* Create a textdump and confirm that the instance is non-null. */

	struct textdump_block *td = textdump_create(0, 0, '\0');
	TEST_ASSERT_NOT_NULL(td);

	/* The default block should have been allocated. */

	TEST_ASSERT_EQUAL_UINT(TEST_TEXTDUMP_ALLOCATION, textdump_get_extent(td));

	/* The store should be empty. */

	TEST_ASSERT_EQUAL_UINT(0, textdump_get_size(td));
}

/**
 * Test that we can create a textdump when supplying a default allocation.
 */

void test_textdump_create_standard_allocation(void)
{
	unsigned int size = 1024;

	/* Create a textdump and confirm that the instance is non-null. */

	struct textdump_block *td = textdump_create(size, 0, '\0');
	TEST_ASSERT_NOT_NULL(td);

	/* The size of the store should be as reuested. */

	TEST_ASSERT_EQUAL_UINT(size, textdump_get_extent(td));

	/* The store should be empty. */

	TEST_ASSERT_EQUAL_UINT(0, textdump_get_size(td));
}

/**
 * Test that we can create a textdump when supplying a larger allocation.
 */

void test_textdump_create_large_allocation(void)
{
	unsigned int size = 2048;

	/* Create a textdump and confirm that the instance is non-null. */

	struct textdump_block *td = textdump_create(size, 0, '\0');
	TEST_ASSERT_NOT_NULL(td);

	/* The size of the store should be as reuested. */

	TEST_ASSERT_EQUAL_UINT(size, textdump_get_extent(td));

	/* The store should be empty. */

	TEST_ASSERT_EQUAL_UINT(0, textdump_get_size(td));
}

/**
 * Test that we can create a textdump with a hash table.
 */

void test_textdump_create_hash(void)
{
	/* Create a textdump and confirm that the instance is non-null. */

	struct textdump_block *td = textdump_create(0, 100, '\0');
	TEST_ASSERT_NOT_NULL(td);

	/* The default block should have been allocated. */

	TEST_ASSERT_EQUAL_UINT(TEST_TEXTDUMP_ALLOCATION, textdump_get_extent(td));

	/* The store should be empty. */

	TEST_ASSERT_EQUAL_UINT(0, textdump_get_size(td));
}

/**
 * Test that we can create a textdump with a hash table, supplying a
 * default allocation.
 */

void test_textdump_create_hash_standard_allocation(void)
{
	unsigned int size = 1024;

	/* Create a textdump and confirm that the instance is non-null. */

	struct textdump_block *td = textdump_create(size, 100, '\0');
	TEST_ASSERT_NOT_NULL(td);

	/* The size of the store should be as reuested. */

	TEST_ASSERT_EQUAL_UINT(size, textdump_get_extent(td));

	/* The store should be empty. */

	TEST_ASSERT_EQUAL_UINT(0, textdump_get_size(td));
}

/**
 * Test that we can create a textdump with a hash table, supplying a
 * larger allocation.
 */

void test_textdump_create_hash_large_allocation(void)
{
	unsigned int size = 2048;

	/* Create a textdump and confirm that the instance is non-null. */

	struct textdump_block *td = textdump_create(size, 100, '\0');
	TEST_ASSERT_NOT_NULL(td);

	/* The size of the store should be as reuested. */

	TEST_ASSERT_EQUAL_UINT(size, textdump_get_extent(td));

	/* The store should be empty. */

	TEST_ASSERT_EQUAL_UINT(0, textdump_get_size(td));
}

/**
 * Test that we can add some items to a textdump without name hashing enabled.
 * We don't go anywhere near the size of the initial allocation, so there's
 * no need for the dump to grow.
 */

void test_textdump_add_items_without_hash(void)
{
	int strings_to_test = 12;

	char *strings[] = {
		"0123456789",
		"ABCDEFGHIJKLMNO",
		"PQRSTUVWXYZabcdefghi",
		"jklmnopqrstuvwxyz0123456789ABC",
		"DEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrsetuvwzyz0123467890",
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrsetuvwzyz0123467890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrsetuvwzyz0123",
		"0123456789",
		"ABCDEFGHIJKLMNO",
		"PQRSTUVWXYZabcdefghi",
		"jklmnopqrstuvwxyz0123456789ABC",
		"DEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrsetuvwzyz0123467890",
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrsetuvwzyz0123467890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrsetuvwzyz0123"
	};

	/* Create a textdump and confirm that the instance is non-null. */

	struct textdump_block *td = textdump_create(0, 0, '\0');
	TEST_ASSERT_NOT_NULL(td);

	/* The default block should have been allocated. */

	TEST_ASSERT_EQUAL_UINT(TEST_TEXTDUMP_ALLOCATION, textdump_get_extent(td));

	/* The store should be empty. */

	TEST_ASSERT_EQUAL_UINT(0, textdump_get_size(td));

	/* Add a collection of strings to the dump. */

	unsigned int end_of_string_data = 0;

	for (int next_string = 0; next_string < strings_to_test; next_string++) {
		/* Add the next string to the text dump. */

		TEST_ASSERT_EQUAL_UINT(end_of_string_data, textdump_store(td, strings[next_string]));

		/* The size should now increase by the string length + 1 for the terminator... */

		end_of_string_data += strlen(strings[next_string]) + 1;

		TEST_ASSERT_EQUAL_UINT(end_of_string_data, textdump_get_size(td));

		/* ...but the extent should not have changed. */

		TEST_ASSERT_EQUAL_UINT(TEST_TEXTDUMP_ALLOCATION, textdump_get_extent(td));
	}

	/* After all that, check that the final extent is what we expect. */

	TEST_ASSERT_EQUAL_UINT(522, end_of_string_data);

	/* Now check that the expected data was stored. */

	unsigned int offset_to_string = 0;

	/* The pointer to the textdump base should not be NULL. */

	char *dump_base = textdump_get_base(td);
	TEST_ASSERT_NOT_NULL(dump_base);

	for (int next_string = 0; next_string < strings_to_test; next_string++) {
		TEST_ASSERT_EQUAL_STRING(strings[next_string], dump_base + offset_to_string);

		offset_to_string += strlen(strings[next_string]);

		TEST_ASSERT_EQUAL_CHAR('\0', *(dump_base + offset_to_string));

		offset_to_string += 1;
	}
}

/**
 * Test that we can add some items to a textdump with name hashing enabled.
 * We don't go anywhere near the size of the initial allocation, so there's
 * no need for the dump to grow.
 */

void test_textdump_add_items_with_hash(void)
{
	int strings_to_test = 12;

	char *strings[] = {
		"0123456789",
		"ABCDEFGHIJKLMNO",
		"PQRSTUVWXYZabcdefghi",
		"jklmnopqrstuvwxyz0123456789ABC",
		"DEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrsetuvwzyz0123467890",
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrsetuvwzyz0123467890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrsetuvwzyz0123",
		"0123456789",
		"ABCDEFGHIJKLMNO",
		"PQRSTUVWXYZabcdefghi",
		"jklmnopqrstuvwxyz0123456789ABC",
		"DEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrsetuvwzyz0123467890",
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrsetuvwzyz0123467890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrsetuvwzyz0123"
	};

	/* Create a textdump and confirm that the instance is non-null. */

	struct textdump_block *td = textdump_create(0, 10, '\0');
	TEST_ASSERT_NOT_NULL(td);

	/* The default block should have been allocated. */

	TEST_ASSERT_EQUAL_UINT(TEST_TEXTDUMP_ALLOCATION, textdump_get_extent(td));

	/* The store should be empty. */

	TEST_ASSERT_EQUAL_UINT(0, textdump_get_size(td));

	/* Add a collection of strings to the dump. */

	unsigned int end_of_string_data = 0;

	for (int next_string = 0; next_string < strings_to_test; next_string++) {
		/* Add the next string to the text dump. */

		TEST_ASSERT_EQUAL_UINT(end_of_string_data, textdump_store(td, strings[next_string]));

		/* The size should now increase by the string length + 1 for the terminator... */

		end_of_string_data += strlen(strings[next_string]) + 1;

		TEST_ASSERT_EQUAL_UINT(end_of_string_data, textdump_get_size(td));

		/* ...but the extent should not have changed. */

		TEST_ASSERT_EQUAL_UINT(TEST_TEXTDUMP_ALLOCATION, textdump_get_extent(td));
	}

	/* After all that, check that the final extent is what we expect. */

	TEST_ASSERT_EQUAL_UINT(522, end_of_string_data);

	/* Now check that the expected data was stored. */

	unsigned int offset_to_string = 0;

	/* The pointer to the textdump base should not be NULL. */

	char *dump_base = textdump_get_base(td);
	TEST_ASSERT_NOT_NULL(dump_base);

	for (int next_string = 0; next_string < strings_to_test; next_string++) {
		TEST_ASSERT_EQUAL_STRING(strings[next_string], dump_base + offset_to_string);

		offset_to_string += strlen(strings[next_string]);

		TEST_ASSERT_EQUAL_CHAR('\0', *(dump_base + offset_to_string));

		offset_to_string += 1;
	}
}

/**
 * The test runner.
 */

int main(void) {
	UNITY_BEGIN();
	RUN_TEST(test_textdump_create_default);
	RUN_TEST(test_textdump_create_standard_allocation);
	RUN_TEST(test_textdump_create_large_allocation);
	RUN_TEST(test_textdump_create_hash);
	RUN_TEST(test_textdump_create_hash_standard_allocation);
	RUN_TEST(test_textdump_create_hash_large_allocation);
	RUN_TEST(test_textdump_add_items_without_hash);
	RUN_TEST(test_textdump_add_items_with_hash);

	return UNITY_END();
}