#include <stdlib.h>
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include "wows-depack.h" // replace with the name of your header file
#include "../lib/wows-depack-private.h"

#define TEST_DATA_SIZE 2048

void test_wows_error_string() {
    WOWS_CONTEXT *context = (WOWS_CONTEXT *)malloc(sizeof(WOWS_CONTEXT));
    context->err_msg = NULL;

    char *error_string = wows_error_string(WOWS_ERROR_CORRUPTED_FILE, context);
    CU_ASSERT_STRING_EQUAL(error_string, "The index is corrupted.");
    free(error_string);

    error_string = wows_error_string(WOWS_ERROR_BAD_MAGIC, context);
    CU_ASSERT_STRING_EQUAL(error_string, "The index has an invalid magic number.");
    free(error_string);

    error_string = wows_error_string(WOWS_ERROR_MISSING_METADATA_ENTRY, context);
    CU_ASSERT_STRING_EQUAL(error_string, "The file is missing a required metadata entry.");
    free(error_string);

    error_string = wows_error_string(WOWS_ERROR_MAX_LEVEL_REACHED, context);
    CU_ASSERT_STRING_EQUAL(error_string, "The maximum level has been reached.");
    free(error_string);

    error_string = wows_error_string(WOWS_ERROR_NON_ZERO_TERMINATED_STRING, context);
    CU_ASSERT_STRING_EQUAL(error_string, "A string in the index is not null-terminated.");
    free(error_string);

    error_string = wows_error_string(WOWS_ERROR_PATH_TOO_LONG, context);
    CU_ASSERT_STRING_EQUAL(error_string, "The file path is too long.");
    free(error_string);

    error_string = wows_error_string(WOWS_ERROR_UNKNOWN, context);
    CU_ASSERT_STRING_EQUAL(error_string, "An unknown error occurred.");
    free(error_string);

    error_string = wows_error_string(WOWS_ERROR_ID_COLLISION_FILE_DIR, context);
    CU_ASSERT_STRING_EQUAL(error_string, "There is an ID collision between a file and a directory.");
    free(error_string);

    error_string = wows_error_string(WOWS_ERROR_FILE_OPEN_FAILURE, context);
    CU_ASSERT_STRING_EQUAL(error_string, "The index could not be opened.");
    free(error_string);

    // Test with a context error message.
    context->err_msg = "Test error message";
    error_string = wows_error_string(WOWS_ERROR_CORRUPTED_FILE, context);
    CU_ASSERT_STRING_EQUAL(error_string, "The index is corrupted.: Test error message");
    free(error_string);

    free(context);
}

void test_checkOutOfIndex_valid(void) {
    char buffer[1024];
    WOWS_INDEX index;
    index.start_address = buffer;
    index.end_address = buffer + sizeof(buffer);

    // Start and end within index boundaries
    char *start = buffer + 10;
    char *end = buffer + 100;
    CU_ASSERT_FALSE(checkOutOfIndex(start, end, &index));
}

void test_checkOutOfIndex_start_outside(void) {
    char buffer[1024];
    WOWS_INDEX index;
    index.start_address = buffer + 100;
    index.end_address = buffer + sizeof(buffer);

    // Start outside index boundaries
    char *start = buffer + 10;
    char *end = buffer + 100;
    CU_ASSERT_TRUE(checkOutOfIndex(start, end, &index));
}

void test_checkOutOfIndex_end_outside(void) {
    char buffer[1024];
    WOWS_INDEX index;
    index.start_address = buffer;
    index.end_address = buffer + 100;

    // End outside index boundaries
    char *start = buffer + 10;
    char *end = buffer + 200;
    CU_ASSERT_TRUE(checkOutOfIndex(start, end, &index));
}

void test_checkOutOfIndex_both_outside(void) {
    char buffer[1024];
    WOWS_INDEX index;
    index.start_address = buffer + 100;
    index.end_address = buffer + 200;

    // Start and end outside index boundaries
    char *start = buffer + 10;
    char *end = buffer + 300;
    CU_ASSERT_TRUE(checkOutOfIndex(start, end, &index));
}

static void test_map_index_file() {
    char contents[TEST_DATA_SIZE] = {0};
    WOWS_INDEX *index;

    // Prepare test data
    WOWS_INDEX_HEADER header = {.magic = {'I', 'S', 'F', 'P'},
                                .file_dir_count = 1,
                                .file_count = 1,
                                .header_size = sizeof(WOWS_INDEX_HEADER),
                                .offset_idx_data_section = 512,
                                .offset_idx_footer_section = 1024};
    WOWS_INDEX_METADATA_ENTRY metadata = {.file_name_size = 6, .offset_idx_file_name = 16, .id = 1, .parent_id = 0};
    WOWS_INDEX_DATA_FILE_ENTRY data_file_entry = {.metadata_id = 1,
                                                  .footer_id = 1,
                                                  .offset_pkg_data = 1024,
                                                  .type_1 = 1,
                                                  .type_2 = 2,
                                                  .size_pkg_data = 100,
                                                  .id_pkg_data = 1};
    WOWS_INDEX_FOOTER footer = {.pkg_file_name_size = 5, .id = 1};
    memcpy(contents, &header, sizeof(WOWS_INDEX_HEADER));
    memcpy(contents + sizeof(WOWS_INDEX_HEADER), &metadata, sizeof(WOWS_INDEX_METADATA_ENTRY));
    memcpy(contents + MAGIC_SECTION_OFFSET + 512, &data_file_entry, sizeof(WOWS_INDEX_DATA_FILE_ENTRY));
    memcpy(contents + MAGIC_SECTION_OFFSET + 1024, &footer, sizeof(WOWS_INDEX_FOOTER));

    // Test map_index_file function
    int result = map_index_file(contents, TEST_DATA_SIZE, &index);

    CU_ASSERT_EQUAL(result, 0);
    CU_ASSERT_PTR_NOT_NULL(index);
    CU_ASSERT_PTR_EQUAL(index->start_address, contents);
    CU_ASSERT_PTR_EQUAL(index->end_address, contents + TEST_DATA_SIZE);
    CU_ASSERT_EQUAL(index->length, TEST_DATA_SIZE);
    CU_ASSERT_PTR_EQUAL(index->header, (WOWS_INDEX_HEADER *)contents);
    CU_ASSERT_EQUAL(index->header->file_dir_count, 1);
    CU_ASSERT_EQUAL(index->header->file_count, 1);
    CU_ASSERT_EQUAL(index->header->header_size, sizeof(WOWS_INDEX_HEADER));
    CU_ASSERT_EQUAL(index->header->offset_idx_data_section, 512);
    CU_ASSERT_EQUAL(index->header->offset_idx_footer_section, 1024);
    CU_ASSERT_PTR_EQUAL(index->metadata, (WOWS_INDEX_METADATA_ENTRY *)(contents + sizeof(WOWS_INDEX_HEADER)));
    CU_ASSERT_EQUAL(index->metadata[0].file_name_size, 6);
    CU_ASSERT_EQUAL(index->metadata[0].offset_idx_file_name, 16);
    CU_ASSERT_EQUAL(index->metadata[0].id, 1);
    CU_ASSERT_EQUAL(index->metadata[0].parent_id, 0);
    CU_ASSERT_PTR_EQUAL(
        index->data_file_entry,
        (WOWS_INDEX_DATA_FILE_ENTRY *)(contents + index->header->offset_idx_data_section + MAGIC_SECTION_OFFSET));
    CU_ASSERT_EQUAL(index->data_file_entry[0].metadata_id, 1);
    CU_ASSERT_EQUAL(index->data_file_entry[0].footer_id, 1);
    CU_ASSERT_EQUAL(index->data_file_entry[0].offset_pkg_data, 1024);
    CU_ASSERT_EQUAL(index->data_file_entry[0].type_1, 1);
    CU_ASSERT_EQUAL(index->data_file_entry[0].type_2, 2);
    CU_ASSERT_EQUAL(index->data_file_entry[0].size_pkg_data, 100);
    CU_ASSERT_EQUAL(index->data_file_entry[0].id_pkg_data, 1);
    CU_ASSERT_EQUAL(index->footer->pkg_file_name_size, 5);
    CU_ASSERT_EQUAL(index->footer->id, 1);
}

int main() {
    CU_initialize_registry();
    CU_pSuite suite = CU_add_suite("test_map_index_file", NULL, NULL);
    CU_add_test(suite, "test_map_index_file", test_map_index_file);

    suite = CU_add_suite("checkOutOfIndex", NULL, NULL);
    CU_add_test(suite, "Valid arguments", test_checkOutOfIndex_valid);
    CU_add_test(suite, "Start outside index boundaries", test_checkOutOfIndex_start_outside);
    CU_add_test(suite, "End outside index boundaries", test_checkOutOfIndex_end_outside);
    CU_add_test(suite, "Start and end outside index boundaries", test_checkOutOfIndex_both_outside);

    suite = CU_add_suite("Error code convert", NULL, NULL);
    CU_add_test(suite, "Check Conversion", test_wows_error_string);

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    int ret = CU_get_number_of_failures();
    CU_cleanup_registry();
    return ret;
}