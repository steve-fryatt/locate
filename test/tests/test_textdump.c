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
 * Helper function to add a string to an unhashed textdump.
 *
 * \param *td			Pointer to the textdump instance to be tested.
 * \param *string		Pointer to the string to be added.
 * \param *end_of_string_data	Pointer to a variable holding the offset to the
 *				end of the string data, updated on return.
 * \param end_of_allocation	The offset to the end of the allocation.
 * \return			The offset at which the string was added.
 */

static unsigned add_unhashed_item(struct textdump_block *td, char *string, unsigned *end_of_string_data, unsigned end_of_allocation)
{
	/* Add the next string to the text dump. */

	TEST_ASSERT_EQUAL_UINT_MESSAGE(*end_of_string_data, textdump_store(td, string),
			"Returned offset correct after store");

	unsigned offset_of_string = *end_of_string_data;

	/* The size should now increase by the string length + 1 for the terminator... */

	*end_of_string_data += strlen(string) + 1;

	TEST_ASSERT_EQUAL_UINT_MESSAGE(*end_of_string_data, textdump_get_size(td),
			"Textdump size correct after store");

	/* ...but the extent should not have changed. */

	TEST_ASSERT_EQUAL_UINT_MESSAGE(end_of_allocation, textdump_get_extent(td),
			"Textdump allocation correct after store");

	return offset_of_string;
}

/**
 * Helper function to read a string back from an unhashed textdump.
 *
 * \param *dump_base		Pointer to the base of the text dump.
 * \param *string		Pointer to the string to be read back.
 * \param *offset_to_string	Pointer to a variable holding the offset to the
 *				next string data, updated on return.
 */

static void read_unhashed_item(char *dump_base, char *string, unsigned *offset_to_string)
{
	TEST_ASSERT_EQUAL_STRING(string, dump_base + *offset_to_string);

	*offset_to_string += strlen(string);

	TEST_ASSERT_EQUAL_CHAR('\0', *(dump_base + *offset_to_string));

	*offset_to_string += 1;
}

/**
 * Test that we can add some items to a textdump without name hashing enabled.
 * We don't go anywhere near the size of the initial allocation, so there's
 * no need for the dump to grow.
 *
 * This test adds duplicates, as these should just be put on to the end of the
 * dump each time.
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

	for (int next_string = 0; next_string < strings_to_test; next_string++)
		add_unhashed_item(td, strings[next_string], &end_of_string_data, TEST_TEXTDUMP_ALLOCATION);

	/* After all that, check that the final extent is what we expect. */

	TEST_ASSERT_EQUAL_UINT(522, end_of_string_data);

	/* Now check that the expected data was stored. */

	unsigned int offset_to_string = 0;

	/* The pointer to the textdump base should not be NULL. */

	char *dump_base = textdump_get_base(td);
	TEST_ASSERT_NOT_NULL(dump_base);

	/* Read through the stored strings. */

	for (int next_string = 0; next_string < strings_to_test; next_string++)
		read_unhashed_item(dump_base, strings[next_string], &offset_to_string);

	/* We should now be at the end of the text strings. */

	TEST_ASSERT_EQUAL_UINT_MESSAGE(offset_to_string, textdump_get_size(td),
			"Read to the end of the textdump.");
}

/**
 * Helper function to add a unique string to a hashed textdump. The expectation
 * is that this will be added to the end of the dump as a new string.
 *
 * \param *td			Pointer to the textdump instance to be tested.
 * \param *string		Pointer to the string to be added.
 * \param *end_of_string_data	Pointer to a variable holding the offset to the
 *				end of the string data, updated on return.
 * \param end_of_allocation	The offset to the end of the allocation.
 * \return			The offset at which the string was added.
 */

static unsigned add_unique_hashed_item(struct textdump_block *td, char *string, unsigned *end_of_string_data, unsigned end_of_allocation)
{
	*end_of_string_data += sizeof(unsigned);

	/* Add the next string to the text dump. */

	TEST_ASSERT_EQUAL_UINT_MESSAGE(*end_of_string_data, textdump_store(td, string),
			"Returned offset correct after store");

	unsigned offset_of_string = *end_of_string_data;

	/* The size should now increase by the string length + 1 for the terminator... */

	*end_of_string_data += (strlen(string) + 4) & 0xfffffffc;

	TEST_ASSERT_EQUAL_UINT_MESSAGE(*end_of_string_data, textdump_get_size(td),
			"Textdump size correct after store");

	/* ...but the extent should not have changed. */

	TEST_ASSERT_EQUAL_UINT_MESSAGE(end_of_allocation, textdump_get_extent(td),
			"Textdump allocation correct after store");

	return offset_of_string;
}

/**
 * Helper function to add a duplicate string to a hashed textdump. The expectation
 * is that this will be detected as duplicate and the original offset will be
 * returned again.
 *
 * \param *td			Pointer to the textdump instance to be tested.
 * \param *string		Pointer to the string to be added.
 * \param *end_of_string_data	Pointer to a variable holding the offset to the
 *				end of the string data, updated on return.
 * \param end_of_allocation	The offset to the end of the allocation.
 * \param expected_offset	The offset from which the string is expected to
 *				be returned.
 */

static void add_duplicate_hashed_item(
		struct textdump_block *td,
		char *string,
		unsigned *end_of_string_data,
		unsigned end_of_allocation,
		unsigned expected_offset
)
{
	/* Add the next string to the text dump. */

	TEST_ASSERT_EQUAL_UINT_MESSAGE(expected_offset, textdump_store(td, string),
			"Returned offset correct after store");

	/* The size should not change... */

	TEST_ASSERT_EQUAL_UINT_MESSAGE(*end_of_string_data, textdump_get_size(td),
			"Textdump size correct after store");

	/* ...and nor should the extent. */

	TEST_ASSERT_EQUAL_UINT_MESSAGE(end_of_allocation, textdump_get_extent(td),
			"Textdump allocation correct after store");
}

/**
 * Helper function to read a string back from a hashed textdump.
 *
 * \param *dump_base		Pointer to the base of the text dump.
 * \param *string		Pointer to the string to be read back.
 * \param *offset_to_string	Pointer to a variable holding the offset to the
 *				next string data, updated on return.
 */

static void read_hashed_item(char *dump_base, char *string, unsigned *offset_to_string)
{
	*offset_to_string += sizeof(unsigned);

	TEST_ASSERT_EQUAL_STRING_MESSAGE(string, dump_base + *offset_to_string,
			"Stored string is correct");

	*offset_to_string += strlen(string);

	TEST_ASSERT_EQUAL_CHAR_MESSAGE('\0', *(dump_base + *offset_to_string),
			"Stored terminator is correct");

	*offset_to_string = (*offset_to_string + 4) & 0xfffffffc;
}

/**
 * Test that we can add some items to a textdump with name hashing enabled.
 * We don't go anywhere near the size of the initial allocation, so there's
 * no need for the dump to grow.
 *
 * This test does not add duplicates, as it assumes that all strings will be
 * added to the end of the dump in their own right,
 */

void test_textdump_add_items_with_hash(void)
{
	int strings_to_test = 6;

	char *strings[] = {
		"0123456789",
		"ABCDEFGHIJKLMNO",
		"PQRSTUVWXYZabcdefghi",
		"jklmnopqrstuvwxyz0123456789ABC",
		"DEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrsetuvwzyz0123467890",
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrsetuvwzyz0123467890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrsetuvwzyz0123"
	};

	/* Create a textdump and confirm that the instance is non-null. */

	struct textdump_block *td = textdump_create(0, 10, '\0');
	TEST_ASSERT_NOT_NULL_MESSAGE(td, "Textdump instance not NULL");

	/* The default block should have been allocated. */

	TEST_ASSERT_EQUAL_UINT_MESSAGE(TEST_TEXTDUMP_ALLOCATION, textdump_get_extent(td),
			"Initial extent correct");

	/* The store should be empty. */

	TEST_ASSERT_EQUAL_UINT_MESSAGE(0, textdump_get_size(td), "Initial store empty");

	/* Add a collection of strings to the dump. */

	unsigned int end_of_string_data = 0;

	for (int next_string = 0; next_string < strings_to_test; next_string++)
		add_unique_hashed_item(td, strings[next_string], &end_of_string_data, TEST_TEXTDUMP_ALLOCATION);

	/* After all that, check that the final extent is what we expect. */

	TEST_ASSERT_EQUAL_UINT_MESSAGE(296, end_of_string_data, "Final offset correct");

	/* Now check that the expected data was stored. */

	unsigned int offset_to_string = 0;

	/* The pointer to the textdump base should not be NULL. */

	char *dump_base = textdump_get_base(td);
	TEST_ASSERT_NOT_NULL_MESSAGE(dump_base, "The dump base is not NULL");

	/* Read through the stored strings. */

	for (int next_string = 0; next_string < strings_to_test; next_string++)
		read_hashed_item(dump_base, strings[next_string], &offset_to_string);

	/* We should now be at the end of the text strings. */

	TEST_ASSERT_EQUAL_UINT_MESSAGE(offset_to_string, textdump_get_size(td),
			"Read to the end of the textdump.");
}

/**
 * Test that we can add some duplicate items to the hashed textdump and have
 * the duplicates detected and merged into a single entry.
 */

void test_textdump_add_duplicates_with_hash(void)
{
	/* Create a textdump and confirm that the instance is non-null. */

	struct textdump_block *td = textdump_create(0, 10, '\0');
	TEST_ASSERT_NOT_NULL_MESSAGE(td, "Textdump instance not NULL");

	/* The default block should have been allocated. */

	TEST_ASSERT_EQUAL_UINT_MESSAGE(TEST_TEXTDUMP_ALLOCATION, textdump_get_extent(td),
			"Initial extent correct");

	/* The store should be empty. */

	TEST_ASSERT_EQUAL_UINT_MESSAGE(0, textdump_get_size(td),
			"Initial store empty");

	unsigned int end_of_string_data = 0;

	/* Add a couple of unique strings. */

	unsigned string1 = add_unique_hashed_item(td, "ABCDEFGHIJ", &end_of_string_data, TEST_TEXTDUMP_ALLOCATION);
	add_unique_hashed_item(td, "1234567890", &end_of_string_data, TEST_TEXTDUMP_ALLOCATION);

	/* Add a duplicate string. */

	add_duplicate_hashed_item(td, "ABCDEFGHIJ", &end_of_string_data, TEST_TEXTDUMP_ALLOCATION, string1);

	/* Now add a couple of strings which aren't quite duplicates. */

	unsigned string3 = add_unique_hashed_item(td, "ABCDEFGHI", &end_of_string_data, TEST_TEXTDUMP_ALLOCATION);
	unsigned string4 = add_unique_hashed_item(td, "ABCDEFGHIJK", &end_of_string_data, TEST_TEXTDUMP_ALLOCATION);

	/* And another couple of duplicates. */

	add_duplicate_hashed_item(td, "ABCDEFGHIJ", &end_of_string_data, TEST_TEXTDUMP_ALLOCATION, string1);
	add_duplicate_hashed_item(td, "ABCDEFGHI", &end_of_string_data, TEST_TEXTDUMP_ALLOCATION, string3);
	add_duplicate_hashed_item(td, "ABCDEFGHIJK", &end_of_string_data, TEST_TEXTDUMP_ALLOCATION, string4);

	/* After all that, check that the final extent is what we expect. */

	TEST_ASSERT_EQUAL_UINT_MESSAGE(64, end_of_string_data, "Final offset correct");

	/* Now check that the expected data was stored. */

	unsigned int offset_to_string = 0;

	/* The pointer to the textdump base should not be NULL. */

	char *dump_base = textdump_get_base(td);
	TEST_ASSERT_NOT_NULL_MESSAGE(dump_base, "The dump base is not NULL");

	read_hashed_item(dump_base, "ABCDEFGHIJ", &offset_to_string);
	read_hashed_item(dump_base, "1234567890", &offset_to_string);
	read_hashed_item(dump_base, "ABCDEFGHI", &offset_to_string);
	read_hashed_item(dump_base, "ABCDEFGHIJK", &offset_to_string);

	/* We should now be at the end of the text strings. */

	TEST_ASSERT_EQUAL_UINT_MESSAGE(offset_to_string, textdump_get_size(td),
			"Read to the end of the textdump.");
}

/**
 * Test that we can add items to an unhashed textdump until the dump is full,
 * without triggering any more memory being claimed.
 */

void test_textdump_add_unhashed_up_to_allocation(void)
{
	const unsigned small_allocation = 128;

	/* Create a textdump and confirm that the instance is non-null. */

	struct textdump_block *td = textdump_create(small_allocation, 0, '\0');
	TEST_ASSERT_NOT_NULL_MESSAGE(td, "Textdump instance not NULL");

	/* The default block should have been allocated. */

	TEST_ASSERT_EQUAL_UINT_MESSAGE(small_allocation, textdump_get_extent(td),
			"Initial extent correct");

	/* The store should be empty. */

	TEST_ASSERT_EQUAL_UINT_MESSAGE(0, textdump_get_size(td),
			"Initial store empty");

	unsigned int end_of_string_data = 0;

	/* Add four strings. */

	add_unhashed_item(td, "1234567890123456789012345678901", &end_of_string_data, small_allocation);
	add_unhashed_item(td, "1234567890123456789012345678901", &end_of_string_data, small_allocation);
	add_unhashed_item(td, "1234567890123456789012345678901", &end_of_string_data, small_allocation);
	add_unhashed_item(td, "1234567890123456789012345678901", &end_of_string_data, small_allocation);

	/* After all that, check that the final extent is what we expect. */

	TEST_ASSERT_EQUAL_UINT_MESSAGE(small_allocation, end_of_string_data,
			"Final offset correct");

	/* Now check that the expected data was stored. */

	unsigned int offset_to_string = 0;

	/* The pointer to the textdump base should not be NULL. */

	char *dump_base = textdump_get_base(td);
	TEST_ASSERT_NOT_NULL_MESSAGE(dump_base, "The dump base is not NULL");

	read_unhashed_item(dump_base, "1234567890123456789012345678901", &offset_to_string);
	read_unhashed_item(dump_base, "1234567890123456789012345678901", &offset_to_string);
	read_unhashed_item(dump_base, "1234567890123456789012345678901", &offset_to_string);
	read_unhashed_item(dump_base, "1234567890123456789012345678901", &offset_to_string);

	/* We should now be at the end of the text strings. */

	TEST_ASSERT_EQUAL_UINT_MESSAGE(offset_to_string, textdump_get_size(td),
			"Read to the end of the textdump.");
}

/**
 * Test that we can add items to an unhashed textdump until the dump is full,
 * writing one over the boundary to trigger an allocation extension.
 */

void test_textdump_add_unhashed_to_one_over_allocation(void)
{
	const unsigned small_allocation = 128;

	/* Create a textdump and confirm that the instance is non-null. */

	struct textdump_block *td = textdump_create(small_allocation, 0, '\0');
	TEST_ASSERT_NOT_NULL_MESSAGE(td, "Textdump instance not NULL");

	/* The default block should have been allocated. */

	TEST_ASSERT_EQUAL_UINT_MESSAGE(small_allocation, textdump_get_extent(td),
			"Initial extent correct");

	/* The store should be empty. */

	TEST_ASSERT_EQUAL_UINT_MESSAGE(0, textdump_get_size(td),
			"Initial store empty");

	unsigned int end_of_string_data = 0;

	/* Add four strings. */

	add_unhashed_item(td, "1234567890123456789012345678901", &end_of_string_data, small_allocation);
	add_unhashed_item(td, "1234567890123456789012345678901", &end_of_string_data, small_allocation);
	add_unhashed_item(td, "1234567890123456789012345678901", &end_of_string_data, small_allocation);
	add_unhashed_item(td, "12345678901234567890123456789012", &end_of_string_data, 2 * small_allocation);

	/* After all that, check that the final extent is what we expect. */

	TEST_ASSERT_EQUAL_UINT_MESSAGE(small_allocation + 1, end_of_string_data,
			"Final offset correct");

	/* Now check that the expected data was stored. */

	unsigned int offset_to_string = 0;

	/* The pointer to the textdump base should not be NULL. */

	char *dump_base = textdump_get_base(td);
	TEST_ASSERT_NOT_NULL_MESSAGE(dump_base, "The dump base is not NULL");

	read_unhashed_item(dump_base, "1234567890123456789012345678901", &offset_to_string);
	read_unhashed_item(dump_base, "1234567890123456789012345678901", &offset_to_string);
	read_unhashed_item(dump_base, "1234567890123456789012345678901", &offset_to_string);
	read_unhashed_item(dump_base, "12345678901234567890123456789012", &offset_to_string);

	/* We should now be at the end of the text strings. */

	TEST_ASSERT_EQUAL_UINT_MESSAGE(offset_to_string, textdump_get_size(td),
			"Read to the end of the textdump.");
}

/**
 * Test that we can add items to an unhashed textdump until the dump is full,
 * writing up to the boundary and then adding another text to trigger an
 * allocation extension.
 */

void test_textdump_add_unhashed_to_and_then_over_allocation(void)
{
	const unsigned small_allocation = 128;

	/* Create a textdump and confirm that the instance is non-null. */

	struct textdump_block *td = textdump_create(small_allocation, 0, '\0');
	TEST_ASSERT_NOT_NULL_MESSAGE(td, "Textdump instance not NULL");

	/* The default block should have been allocated. */

	TEST_ASSERT_EQUAL_UINT_MESSAGE(small_allocation, textdump_get_extent(td),
			"Initial extent correct");

	/* The store should be empty. */

	TEST_ASSERT_EQUAL_UINT_MESSAGE(0, textdump_get_size(td),
			"Initial store empty");

	unsigned int end_of_string_data = 0;

	/* Add four strings. */

	add_unhashed_item(td, "1234567890123456789012345678901", &end_of_string_data, small_allocation);
	add_unhashed_item(td, "1234567890123456789012345678901", &end_of_string_data, small_allocation);
	add_unhashed_item(td, "1234567890123456789012345678901", &end_of_string_data, small_allocation);
	add_unhashed_item(td, "1234567890123456789012345678901", &end_of_string_data, small_allocation);

	/* After all that, check that the final extent is what we expect. */

	TEST_ASSERT_EQUAL_UINT_MESSAGE(small_allocation, end_of_string_data,
			"Intermediate offset correct");

	/* Now add another string, to force an extension to the allocation. */

	add_unhashed_item(td, "1234567890123456789012345678901", &end_of_string_data, 2 * small_allocation);

	/* Check again that the final extent is what we expect. */

	TEST_ASSERT_EQUAL_UINT_MESSAGE(5 * 32, end_of_string_data,
			"Final offset correct");

	/* Now check that the expected data was stored. */

	unsigned int offset_to_string = 0;

	/* The pointer to the textdump base should not be NULL. */

	char *dump_base = textdump_get_base(td);
	TEST_ASSERT_NOT_NULL_MESSAGE(dump_base, "The dump base is not NULL");

	read_unhashed_item(dump_base, "1234567890123456789012345678901", &offset_to_string);
	read_unhashed_item(dump_base, "1234567890123456789012345678901", &offset_to_string);
	read_unhashed_item(dump_base, "1234567890123456789012345678901", &offset_to_string);
	read_unhashed_item(dump_base, "1234567890123456789012345678901", &offset_to_string);
	read_unhashed_item(dump_base, "1234567890123456789012345678901", &offset_to_string);

	/* We should now be at the end of the text strings. */

	TEST_ASSERT_EQUAL_UINT_MESSAGE(offset_to_string, textdump_get_size(td),
			"Read to the end of the textdump.");
}

/**
 * Test that we can add items to a hashed textdump until the dump is full,
 * without triggering any more memory being claimed.
 */

void test_textdump_add_hashed_up_to_allocation(void)
{
	const unsigned small_allocation = 128;

	/* Create a textdump and confirm that the instance is non-null. */

	struct textdump_block *td = textdump_create(small_allocation, 10, '\0');
	TEST_ASSERT_NOT_NULL_MESSAGE(td, "Textdump instance not NULL");

	/* The default block should have been allocated. */

	TEST_ASSERT_EQUAL_UINT_MESSAGE(small_allocation, textdump_get_extent(td),
			"Initial extent correct");

	/* The store should be empty. */

	TEST_ASSERT_EQUAL_UINT_MESSAGE(0, textdump_get_size(td),
			"Initial store empty");

	unsigned int end_of_string_data = 0;

	/* Add four unique strings. These should try all of the alignment padding options. */

	add_unique_hashed_item(td, "123456789012345678901234567", &end_of_string_data, small_allocation);
	add_unique_hashed_item(td, "12345678901234567890123456", &end_of_string_data, small_allocation);
	add_unique_hashed_item(td, "1234567890123456789012345", &end_of_string_data, small_allocation);
	add_unique_hashed_item(td, "123456789012345678901234", &end_of_string_data, small_allocation);

	/* After all that, check that the final extent is what we expect. */

	TEST_ASSERT_EQUAL_UINT_MESSAGE(small_allocation, end_of_string_data,
			"Final offset correct");

	/* Now check that the expected data was stored. */

	unsigned int offset_to_string = 0;

	/* The pointer to the textdump base should not be NULL. */

	char *dump_base = textdump_get_base(td);
	TEST_ASSERT_NOT_NULL_MESSAGE(dump_base, "The dump base is not NULL");

	read_hashed_item(dump_base, "123456789012345678901234567", &offset_to_string);
	read_hashed_item(dump_base, "12345678901234567890123456", &offset_to_string);
	read_hashed_item(dump_base, "1234567890123456789012345", &offset_to_string);
	read_hashed_item(dump_base, "123456789012345678901234", &offset_to_string);

	/* We should now be at the end of the text strings. */

	TEST_ASSERT_EQUAL_UINT_MESSAGE(offset_to_string, textdump_get_size(td),
			"Read to the end of the textdump.");
}

/**
 * Test that we can add items to a hashed textdump until the dump is full,
 * writing one over the boundary to trigger an allocation extension.
 */

void test_textdump_add_hashed_to_one_over_allocation(void)
{
	const unsigned small_allocation = 128;

	/* Create a textdump and confirm that the instance is non-null. */

	struct textdump_block *td = textdump_create(small_allocation, 10, '\0');
	TEST_ASSERT_NOT_NULL_MESSAGE(td, "Textdump instance not NULL");

	/* The default block should have been allocated. */

	TEST_ASSERT_EQUAL_UINT_MESSAGE(small_allocation, textdump_get_extent(td),
			"Initial extent correct");

	/* The store should be empty. */

	TEST_ASSERT_EQUAL_UINT_MESSAGE(0, textdump_get_size(td),
			"Initial store empty");

	unsigned int end_of_string_data = 0;

	/* Add four unique strings. These should try all of the alignment padding options. */

	add_unique_hashed_item(td, "123456789012345678901234567", &end_of_string_data, small_allocation);
	add_unique_hashed_item(td, "12345678901234567890123456", &end_of_string_data, small_allocation);
	add_unique_hashed_item(td, "1234567890123456789012345", &end_of_string_data, small_allocation);
	add_unique_hashed_item(td, "1234567890123456789012345678", &end_of_string_data, 2 * small_allocation);

	/* After all that, check that the final extent is what we expect. */

	TEST_ASSERT_EQUAL_UINT_MESSAGE(small_allocation + 4, end_of_string_data,
			"Final offset correct");

	/* Now check that the expected data was stored. */

	unsigned int offset_to_string = 0;

	/* The pointer to the textdump base should not be NULL. */

	char *dump_base = textdump_get_base(td);
	TEST_ASSERT_NOT_NULL_MESSAGE(dump_base, "The dump base is not NULL");

	read_hashed_item(dump_base, "123456789012345678901234567", &offset_to_string);
	read_hashed_item(dump_base, "12345678901234567890123456", &offset_to_string);
	read_hashed_item(dump_base, "1234567890123456789012345", &offset_to_string);
	read_hashed_item(dump_base, "1234567890123456789012345678", &offset_to_string);

	/* We should now be at the end of the text strings. */

	TEST_ASSERT_EQUAL_UINT_MESSAGE(offset_to_string, textdump_get_size(td),
			"Read to the end of the textdump.");
}

/**
 * Test that we can add items to a hashed textdump until the dump is full,
 * writing up to the boundary and then adding another text to trigger an
 * allocation extension.
 */

void test_textdump_add_hashed_to_and_then_over_allocation(void)
{
	const unsigned small_allocation = 128;

	/* Create a textdump and confirm that the instance is non-null. */

	struct textdump_block *td = textdump_create(small_allocation, 10, '\0');
	TEST_ASSERT_NOT_NULL_MESSAGE(td, "Textdump instance not NULL");

	/* The default block should have been allocated. */

	TEST_ASSERT_EQUAL_UINT_MESSAGE(small_allocation, textdump_get_extent(td),
			"Initial extent correct");

	/* The store should be empty. */

	TEST_ASSERT_EQUAL_UINT_MESSAGE(0, textdump_get_size(td),
			"Initial store empty");

	unsigned int end_of_string_data = 0;

	/* Add four unique strings. These should try all of the alignment padding options. */

	add_unique_hashed_item(td, "123456789012345678901234567", &end_of_string_data, small_allocation);
	add_unique_hashed_item(td, "12345678901234567890123456", &end_of_string_data, small_allocation);
	add_unique_hashed_item(td, "1234567890123456789012345", &end_of_string_data, small_allocation);
	add_unique_hashed_item(td, "123456789012345678901234", &end_of_string_data, small_allocation);

	/* After all that, check that the final extent is what we expect. */

	TEST_ASSERT_EQUAL_UINT_MESSAGE(small_allocation, end_of_string_data,
			"Internediate offset correct");

	/* Now add another string, to force an extension to the allocation. */

	add_unique_hashed_item(td, "12345678901234567890123", &end_of_string_data, 2 * small_allocation);

	/* Check again that the final extent is what we expect. */

	TEST_ASSERT_EQUAL_UINT_MESSAGE(4 * 32 + 28, end_of_string_data,
			"Final offset correct");

	/* Now check that the expected data was stored. */

	unsigned int offset_to_string = 0;

	/* The pointer to the textdump base should not be NULL. */

	char *dump_base = textdump_get_base(td);
	TEST_ASSERT_NOT_NULL_MESSAGE(dump_base, "The dump base is not NULL");

	read_hashed_item(dump_base, "123456789012345678901234567", &offset_to_string);
	read_hashed_item(dump_base, "12345678901234567890123456", &offset_to_string);
	read_hashed_item(dump_base, "1234567890123456789012345", &offset_to_string);
	read_hashed_item(dump_base, "123456789012345678901234", &offset_to_string);
	read_hashed_item(dump_base, "12345678901234567890123", &offset_to_string);

	/* We should now be at the end of the text strings. */

	TEST_ASSERT_EQUAL_UINT_MESSAGE(offset_to_string, textdump_get_size(td),
			"Read to the end of the textdump.");
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
	RUN_TEST(test_textdump_add_duplicates_with_hash);
	RUN_TEST(test_textdump_add_unhashed_up_to_allocation);
	RUN_TEST(test_textdump_add_unhashed_to_one_over_allocation);
	RUN_TEST(test_textdump_add_unhashed_to_and_then_over_allocation);
	RUN_TEST(test_textdump_add_hashed_up_to_allocation);
	RUN_TEST(test_textdump_add_hashed_to_one_over_allocation);
	RUN_TEST(test_textdump_add_hashed_to_and_then_over_allocation);
	return UNITY_END();
}