#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <stdio.h>
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <pcre.h>
#include <zlib.h>
#include "wows-depack.h" // replace with the name of your header file
#include "../lib/wows-depack-private.h"

#define TEST_DATA_SIZE 2048

void test_print_header() {
    WOWS_INDEX_HEADER header = {0};
    int ret = print_header(&header);
    CU_ASSERT_EQUAL(ret, 0);
}

void test_print_footer() {
    WOWS_INDEX_FOOTER footer = {0};
    int ret = print_footer(&footer);
    CU_ASSERT_EQUAL(ret, 0);
}

void test_print_metadata_entry() {
    WOWS_INDEX_METADATA_ENTRY entry = {0};
    int ret = print_metadata_entry(&entry);
    CU_ASSERT_EQUAL(ret, 0);
}

void test_print_data_file_entry() {
    WOWS_INDEX_DATA_FILE_ENTRY entry = {0};
    int ret = print_data_file_entry(&entry);
    CU_ASSERT_EQUAL(ret, 0);
}

void test_wows_error_string() {
    WOWS_CONTEXT *context = wows_init_context(0);

    char *error_string = wows_error_string(WOWS_ERROR_CORRUPTED_FILE, context);
    CU_ASSERT_STRING_EQUAL(error_string, "The index is corrupted");
    free(error_string);

    error_string = wows_error_string(WOWS_ERROR_BAD_MAGIC, context);
    CU_ASSERT_STRING_EQUAL(error_string, "The index has an invalid magic number");
    free(error_string);

    error_string = wows_error_string(WOWS_ERROR_MISSING_METADATA_ENTRY, context);
    CU_ASSERT_STRING_EQUAL(error_string, "The file is missing a required metadata entry");
    free(error_string);

    error_string = wows_error_string(WOWS_ERROR_MAX_LEVEL_REACHED, context);
    CU_ASSERT_STRING_EQUAL(error_string, "The maximum level has been reached");
    free(error_string);

    error_string = wows_error_string(WOWS_ERROR_NON_ZERO_TERMINATED_STRING, context);
    CU_ASSERT_STRING_EQUAL(error_string, "A string in the index is not null-terminated");
    free(error_string);

    error_string = wows_error_string(WOWS_ERROR_PATH_TOO_LONG, context);
    CU_ASSERT_STRING_EQUAL(error_string, "The file path is too long");
    free(error_string);

    error_string = wows_error_string(WOWS_ERROR_UNKNOWN, context);
    CU_ASSERT_STRING_EQUAL(error_string, "An unknown error occurred");
    free(error_string);

    error_string = wows_error_string(WOWS_ERROR_ID_COLLISION_FILE_DIR, context);
    CU_ASSERT_STRING_EQUAL(error_string, "There is an ID collision between a file and a directory");
    free(error_string);

    error_string = wows_error_string(WOWS_ERROR_FILE_OPEN_FAILURE, context);
    CU_ASSERT_STRING_EQUAL(error_string, "The index could not be opened");
    free(error_string);

    error_string = wows_error_string(WOWS_ERROR_DECOMPOSE_PATH, context);
    CU_ASSERT_STRING_EQUAL(error_string, "failed to decompose path into [dir1, dir2, etc] + file");
    free(error_string);

    error_string = wows_error_string(WOWS_ERROR_INVALID_SEARCH_PATTERN, context);
    CU_ASSERT_STRING_EQUAL(error_string, "failure to compile regex pattern");
    free(error_string);

    error_string = wows_error_string(WOWS_ERROR_NOT_A_FILE, context);
    CU_ASSERT_STRING_EQUAL(error_string, "path is not a file");
    free(error_string);

    error_string = wows_error_string(WOWS_ERROR_NOT_A_DIR, context);
    CU_ASSERT_STRING_EQUAL(error_string, "path is not a directory");
    free(error_string);

    error_string = wows_error_string(WOWS_ERROR_NOT_FOUND, context);
    CU_ASSERT_STRING_EQUAL(error_string, "file or directory not found");
    free(error_string);

    error_string = wows_error_string(WOWS_ERROR_FILE_WRITE, context);
    CU_ASSERT_STRING_EQUAL(error_string, "file write error");
    free(error_string);

    error_string = wows_error_string(9999999, context);
    CU_ASSERT_STRING_EQUAL(error_string, "unrecognized error code");
    free(error_string);

    // Test with a context error message.
    wows_set_error_details(context, "Test error message %s", "variable");
    error_string = wows_error_string(WOWS_ERROR_CORRUPTED_FILE, context);
    CU_ASSERT_STRING_EQUAL(error_string, "The index is corrupted: Test error message variable");
    free(error_string);

    // Test with a context error message.
    wows_set_error_details(context, "Test error message 2");
    error_string = wows_error_string(WOWS_ERROR_CORRUPTED_FILE, context);
    CU_ASSERT_STRING_EQUAL(error_string, "The index is corrupted: Test error message 2");
    free(error_string);

    wows_free_context(context);
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
    WOWS_INDEX_METADATA_ENTRY metadata2 = {.file_name_size = 6, .offset_idx_file_name = 16, .id = 2, .parent_id = 0};
    WOWS_INDEX_DATA_FILE_ENTRY data_file_entry = {.metadata_id = 1,
                                                  .footer_id = 42,
                                                  .offset_pkg_data = 1024,
                                                  .type_1 = 1,
                                                  .type_2 = 2,
                                                  .size_pkg_data = 100,
                                                  .id_pkg_data = 1};
    WOWS_INDEX_DATA_FILE_ENTRY data_file_entry2 = {.metadata_id = 2,
                                                   .footer_id = 42,
                                                   .offset_pkg_data = 1024,
                                                   .type_1 = 1,
                                                   .type_2 = 2,
                                                   .size_pkg_data = 100,
                                                   .id_pkg_data = 1};
    WOWS_INDEX_FOOTER footer = {.pkg_file_name_size = 5, .id = 42};
    memcpy(contents, &header, sizeof(WOWS_INDEX_HEADER));
    memcpy(contents + sizeof(WOWS_INDEX_HEADER), &metadata, sizeof(WOWS_INDEX_METADATA_ENTRY));
    memcpy(contents + sizeof(WOWS_INDEX_HEADER) + sizeof(WOWS_INDEX_METADATA_ENTRY), &metadata2,
           sizeof(WOWS_INDEX_METADATA_ENTRY));
    memcpy(contents + MAGIC_SECTION_OFFSET + 512, &data_file_entry, sizeof(WOWS_INDEX_DATA_FILE_ENTRY));
    memcpy(contents + MAGIC_SECTION_OFFSET + 512 + sizeof(WOWS_INDEX_DATA_FILE_ENTRY), &data_file_entry2,
           sizeof(WOWS_INDEX_DATA_FILE_ENTRY));
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
    CU_ASSERT_EQUAL(index->metadata[1].file_name_size, 6);
    CU_ASSERT_EQUAL(index->metadata[1].offset_idx_file_name, 16);
    CU_ASSERT_EQUAL(index->metadata[1].id, 2);
    CU_ASSERT_EQUAL(index->metadata[1].parent_id, 0);

    CU_ASSERT_PTR_EQUAL(
        index->data_file_entry,
        (WOWS_INDEX_DATA_FILE_ENTRY *)(contents + index->header->offset_idx_data_section + MAGIC_SECTION_OFFSET));
    CU_ASSERT_EQUAL(index->data_file_entry[0].metadata_id, 1);
    CU_ASSERT_EQUAL(index->data_file_entry[0].footer_id, 42);
    CU_ASSERT_EQUAL(index->data_file_entry[0].offset_pkg_data, 1024);
    CU_ASSERT_EQUAL(index->data_file_entry[0].type_1, 1);
    CU_ASSERT_EQUAL(index->data_file_entry[0].type_2, 2);
    CU_ASSERT_EQUAL(index->data_file_entry[0].size_pkg_data, 100);
    CU_ASSERT_EQUAL(index->data_file_entry[0].id_pkg_data, 1);
    CU_ASSERT_EQUAL(index->data_file_entry[1].metadata_id, 2);
    CU_ASSERT_EQUAL(index->data_file_entry[1].footer_id, 42);
    CU_ASSERT_EQUAL(index->data_file_entry[1].offset_pkg_data, 1024);
    CU_ASSERT_EQUAL(index->data_file_entry[1].type_1, 1);
    CU_ASSERT_EQUAL(index->data_file_entry[1].type_2, 2);
    CU_ASSERT_EQUAL(index->data_file_entry[1].size_pkg_data, 100);
    CU_ASSERT_EQUAL(index->data_file_entry[1].id_pkg_data, 1);

    CU_ASSERT_EQUAL(index->footer->pkg_file_name_size, 5);
    CU_ASSERT_EQUAL(index->footer->id, 42);

    free(index);
}

void test_wows_parse_index_buffer() {
    // Initialize WOWS_CONTEXT
    WOWS_CONTEXT *context = wows_init_context(WOWS_DEBUG_FILE_LISTING | WOWS_DEBUG_RAW_RECORD);

    char *contents = calloc(sizeof(char) * TEST_DATA_SIZE, 1);

    // Prepare test data
    WOWS_INDEX_HEADER header = {.magic = {'I', 'S', 'F', 'P'},
                                .file_dir_count = 5,
                                .file_count = 2,
                                .header_size = sizeof(WOWS_INDEX_HEADER),
                                .offset_idx_data_section = 512,
                                .offset_idx_footer_section = 1024};

    memcpy(contents, &header, sizeof(WOWS_INDEX_HEADER));

    WOWS_INDEX_METADATA_ENTRY metadata1 = {.file_name_size = 6, .offset_idx_file_name = 256, .id = 1, .parent_id = 3};
    char *offset_metadata1 = contents + sizeof(WOWS_INDEX_HEADER);
    WOWS_INDEX_METADATA_ENTRY metadata2 = {.file_name_size = 6, .offset_idx_file_name = 256, .id = 2, .parent_id = 3};
    char *offset_metadata2 = contents + sizeof(WOWS_INDEX_HEADER) + sizeof(WOWS_INDEX_METADATA_ENTRY);
    WOWS_INDEX_METADATA_ENTRY metadata3 = {.file_name_size = 6, .offset_idx_file_name = 256, .id = 3, .parent_id = 4};
    char *offset_metadata3 = contents + sizeof(WOWS_INDEX_HEADER) + sizeof(WOWS_INDEX_METADATA_ENTRY) * 2;
    WOWS_INDEX_METADATA_ENTRY metadata4 = {.file_name_size = 6, .offset_idx_file_name = 256, .id = 4, .parent_id = 5};
    char *offset_metadata4 = contents + sizeof(WOWS_INDEX_HEADER) + sizeof(WOWS_INDEX_METADATA_ENTRY) * 3;
    WOWS_INDEX_METADATA_ENTRY metadata5 = {
        .file_name_size = 6, .offset_idx_file_name = 256, .id = 5, .parent_id = 1111};
    char *offset_metadata5 = contents + sizeof(WOWS_INDEX_HEADER) + sizeof(WOWS_INDEX_METADATA_ENTRY) * 3;

    memcpy(offset_metadata1, &metadata1, sizeof(WOWS_INDEX_METADATA_ENTRY));
    memcpy(offset_metadata2, &metadata2, sizeof(WOWS_INDEX_METADATA_ENTRY));
    memcpy(offset_metadata3, &metadata3, sizeof(WOWS_INDEX_METADATA_ENTRY));
    memcpy(offset_metadata4, &metadata4, sizeof(WOWS_INDEX_METADATA_ENTRY));
    memcpy(offset_metadata5, &metadata5, sizeof(WOWS_INDEX_METADATA_ENTRY));

    char *name1 = "a1234";
    char *name2 = "b1234";
    char *name3 = "c1234";
    char *name4 = "d1234";
    char *name5 = "e1234";
    memcpy(offset_metadata1 + 256, name1, strlen(name1));
    memcpy(offset_metadata2 + 256, name2, strlen(name2));
    memcpy(offset_metadata3 + 256, name3, strlen(name3));
    memcpy(offset_metadata4 + 256, name4, strlen(name4));
    memcpy(offset_metadata5 + 256, name5, strlen(name5));

    WOWS_INDEX_DATA_FILE_ENTRY data_file_entry = {.metadata_id = 1,
                                                  .footer_id = 42,
                                                  .offset_pkg_data = 1024,
                                                  .type_1 = 1,
                                                  .type_2 = 2,
                                                  .size_pkg_data = 100,
                                                  .id_pkg_data = 1};
    WOWS_INDEX_DATA_FILE_ENTRY data_file_entry2 = {.metadata_id = 2,
                                                   .footer_id = 42,
                                                   .offset_pkg_data = 1024,
                                                   .type_1 = 1,
                                                   .type_2 = 2,
                                                   .size_pkg_data = 100,
                                                   .id_pkg_data = 1};
    WOWS_INDEX_FOOTER footer = {.pkg_file_name_size = 5, .id = 42};
    memcpy(contents + MAGIC_SECTION_OFFSET + 512, &data_file_entry, sizeof(WOWS_INDEX_DATA_FILE_ENTRY));
    memcpy(contents + MAGIC_SECTION_OFFSET + 512 + sizeof(WOWS_INDEX_DATA_FILE_ENTRY), &data_file_entry2,
           sizeof(WOWS_INDEX_DATA_FILE_ENTRY));
    memcpy(contents + MAGIC_SECTION_OFFSET + 1024, &footer, sizeof(WOWS_INDEX_FOOTER));
    memcpy(contents + MAGIC_SECTION_OFFSET + 1024 + sizeof(WOWS_INDEX_FOOTER), "foot", 5);

    size_t length = TEST_DATA_SIZE;
    char *index_file_path = "index file path";

    // Call the function
    int result = wows_parse_index_buffer(contents, length, index_file_path, 0, context);

    // Check the result and assert on success
    CU_ASSERT_EQUAL(result, 0);
    char *err_msg = wows_error_string(result, context);
    printf("Error: %s\n", err_msg);

    // FILE *fp = fopen("dump.idx", "w");
    // wows_dump_index_to_file(context->indexes[0], fp);
    // fclose(fp);

    wows_print_flat(context);
    wows_print_tree(context);

    // Free the WOWS_CONTEXT
    // TODO add more asserts
    free(err_msg);
    wows_free_context_no_munmap(context);
    free(contents);
}

void test_wows_dump_index_to_file(void) {
    WOWS_INDEX_HEADER header = {.magic = "WoWS",
                                .unknown_1 = 0x10000,
                                .id = 0x12345678,
                                .unknown_2 = 0x20000,
                                .file_dir_count = 2,
                                .file_count = 3,
                                .unknown_3 = 0,
                                .header_size = 0,
                                .offset_idx_data_section = 0,
                                .offset_idx_footer_section = 0};
    WOWS_INDEX_METADATA_ENTRY metadata[] = {{.file_name_size = 0, .offset_idx_file_name = 0, .id = 1, .parent_id = 0},
                                            {.file_name_size = 0, .offset_idx_file_name = 5, .id = 2, .parent_id = 0}};
    WOWS_INDEX_DATA_FILE_ENTRY data_files[] = {{.metadata_id = 1,
                                                .footer_id = 1,
                                                .offset_pkg_data = 0,
                                                .type_1 = 1,
                                                .type_2 = 2,
                                                .size_pkg_data = 100,
                                                .id_pkg_data = 1,
                                                .padding = 0},
                                               {.metadata_id = 2,
                                                .footer_id = 1,
                                                .offset_pkg_data = 100,
                                                .type_1 = 2,
                                                .type_2 = 3,
                                                .size_pkg_data = 200,
                                                .id_pkg_data = 2,
                                                .padding = 0},
                                               {.metadata_id = 2,
                                                .footer_id = 1,
                                                .offset_pkg_data = 300,
                                                .type_1 = 3,
                                                .type_2 = 4,
                                                .size_pkg_data = 300,
                                                .id_pkg_data = 3,
                                                .padding = 0}};
    WOWS_INDEX_FOOTER footer = {.pkg_file_name_size = 9, .unknown_7 = 0, .id = 1};

    WOWS_INDEX index = {.header = &header, .metadata = metadata, .data_file_entry = data_files, .footer = &footer};

    // Open a file for writing
    char *buf = NULL;
    size_t buf_size = 0;
    FILE *f = open_memstream(&buf, &buf_size);
    CU_ASSERT_PTR_NOT_NULL_FATAL(f);

    // Call the function being tested
    int ret = wows_dump_index_to_file(&index, f);
    fclose(f);

    CU_ASSERT_EQUAL(ret, 0);

    WOWS_INDEX_HEADER *buf_header = (WOWS_INDEX_HEADER *)buf;
    CU_ASSERT_STRING_EQUAL(buf_header->magic, "WoWS");
    CU_ASSERT_EQUAL(buf_header->file_dir_count, 2);
    CU_ASSERT_EQUAL(buf_header->file_count, 3);

    // Close the file
    free(buf);
}

pcre *re;

void test_decompose_path_no_sep() {
    const char *path = "filename.txt";
    int dir_count;
    char **dirs;
    char *file;
    int result = decompose_path(path, &dir_count, &dirs, &file);

    CU_ASSERT_EQUAL(result, 0);
    CU_ASSERT_EQUAL(dir_count, 0);
    CU_ASSERT_PTR_NULL(dirs);
    CU_ASSERT_STRING_EQUAL(file, "filename.txt");

    free(file);
}

// Initialize test suite
int regex_init_suite(void) {
    const char *pattern = "quick\\s(brown|red) fox";
    int erroffset;
    const char *error;

    re = pcre_compile(pattern, 0, &error, &erroffset, NULL);
    if (!re) {
        return -1;
    }

    return 0;
}

// Cleanup test suite
int regex_clean_suite(void) {
    pcre_free(re);
    return 0;
}

// Test compile_regex function
void test_compile_regex(void) {
    const char *pattern = "quick\\s(brown|red) fox";
    pcre *compiled = compile_regex(pattern);
    CU_ASSERT_PTR_NOT_NULL_FATAL(compiled);
    int result = free_regex(compiled);
    CU_ASSERT_EQUAL(result, 0);
}

// Test match_regex function with matching input
void test_match_regex_match(void) {
    const char *subject = "The quick brown fox jumped over the lazy dog.";
    bool result = match_regex(re, subject);
    CU_ASSERT_TRUE(result);
}

// Test match_regex function with non-matching input
void test_match_regex_no_match(void) {
    const char *subject = "The quick blue fox jumped over the lazy dog.";
    bool result = match_regex(re, subject);
    CU_ASSERT_FALSE(result);
}

void test_decompose_path_with_sep() {
    const char *path = "/home/user///docs/filename.txt";
    int dir_count;
    char **dirs;
    char *file;
    int result = decompose_path(path, &dir_count, &dirs, &file);

    CU_ASSERT_EQUAL(result, 0);
    CU_ASSERT_EQUAL(dir_count, 3);
    CU_ASSERT_STRING_EQUAL(dirs[0], "home");
    CU_ASSERT_STRING_EQUAL(dirs[1], "user");
    CU_ASSERT_STRING_EQUAL(dirs[2], "docs");
    CU_ASSERT_STRING_EQUAL(file, "filename.txt");

    for (int i = 0; i < dir_count; i++) {
        free(dirs[i]);
    }
    free(dirs);
    free(file);
}

void test_decompose_path_only_dir() {
    const char *path = "/home/user///docs/";
    int dir_count;
    char **dirs;
    char *file;
    int result = decompose_path(path, &dir_count, &dirs, &file);

    CU_ASSERT_EQUAL(result, 0);
    CU_ASSERT_EQUAL(dir_count, 3);
    CU_ASSERT_STRING_EQUAL(dirs[0], "home");
    CU_ASSERT_STRING_EQUAL(dirs[1], "user");
    CU_ASSERT_STRING_EQUAL(dirs[2], "docs");
    CU_ASSERT_PTR_NULL(file);

    for (int i = 0; i < dir_count; i++) {
        free(dirs[i]);
    }
    free(dirs);
    free(file);
}

void test_compress() {
    WOWS_CONTEXT *context = wows_init_context(0);
    char *buf_idx = NULL;
    size_t buf_idx_size = 0;
    FILE *nfd_idx = open_memstream(&buf_idx, &buf_idx_size);

    char *buf_pkg = NULL;
    size_t buf_pkg_size = 0;
    FILE *nfd_pkg = open_memstream(&buf_pkg, &buf_pkg_size);

    int result = wows_write_pkg(context, "./tests", "stuff.pkg", nfd_pkg, nfd_idx);
    fclose(nfd_pkg);
    fclose(nfd_idx);

    // TODO add more asserts
    CU_ASSERT_EQUAL(result, 0);
    CU_ASSERT_NOT_EQUAL(buf_idx_size, 0);
    CU_ASSERT_NOT_EQUAL(buf_pkg_size, 0);
    free(buf_idx);
    free(buf_pkg);
    wows_free_context(context);
}

void test_extract() {
    WOWS_CONTEXT *context = wows_init_context(0);
    int ret;

    ret = wows_parse_index_dir("wows_sim_dir/bin/2234567/idx/", context);
    if (ret != 0) {
        char *err_msg = wows_error_string(ret, context);
        printf("Error: %s\n", err_msg);
    }

    char *buf_pkg = NULL;
    size_t buf_pkg_size = 0;
    FILE *fd_pkg = open_memstream(&buf_pkg, &buf_pkg_size);

    ret = wows_extract_file_fp(context, "tests.c", fd_pkg);
    CU_ASSERT_EQUAL(ret, 0);
    if (ret != 0) {
        char *err_msg = wows_error_string(ret, context);
        printf("Error: %s\n", err_msg);
    }

    fclose(fd_pkg);
    uint64_t crc = crc32(0, NULL, 0);
    crc = crc32(crc, (const Bytef *)buf_pkg, strlen(buf_pkg)); // update CRC-64 with string data

    CU_ASSERT_NOT_EQUAL(buf_pkg_size, 0);
    CU_ASSERT_EQUAL(buf_pkg_size, 32393);
    CU_ASSERT_EQUAL(crc, 0xFF3F3136);

    free(buf_pkg);

    buf_pkg_size = 0;
    fd_pkg = open_memstream(&buf_pkg, &buf_pkg_size);
    ret = wows_extract_file_fp(context, "doesnotexist.c", fd_pkg);
    fclose(fd_pkg);
    free(buf_pkg);
    CU_ASSERT_EQUAL(ret, WOWS_ERROR_NOT_FOUND);

    buf_pkg_size = 0;
    fd_pkg = open_memstream(&buf_pkg, &buf_pkg_size);
    ret = wows_extract_file_fp(context, "data/", fd_pkg);
    fclose(fd_pkg);
    free(buf_pkg);
    CU_ASSERT_EQUAL(ret, WOWS_ERROR_NOT_A_FILE);

    buf_pkg_size = 0;
    fd_pkg = open_memstream(&buf_pkg, &buf_pkg_size);
    ret = wows_extract_file_fp(context, "data", fd_pkg);
    fclose(fd_pkg);
    free(buf_pkg);
    CU_ASSERT_EQUAL(ret, WOWS_ERROR_NOT_A_FILE);

    buf_pkg_size = 0;
    fd_pkg = open_memstream(&buf_pkg, &buf_pkg_size);
    ret = wows_extract_file_fp(context, "/data/fake2.idx/test", fd_pkg);
    fclose(fd_pkg);
    free(buf_pkg);
    CU_ASSERT_EQUAL(ret, WOWS_ERROR_NOT_A_DIR);

    wows_free_context(context);
}

void test_extract_file() {
    WOWS_CONTEXT *context = wows_init_context(0);
    int ret;
    ret = wows_parse_index_dir("wows_sim_dir/bin/2234567/idx/", context);
    CU_ASSERT_EQUAL(ret, 0);
    ret = wows_extract_file(context, "tests.c", "tests-extract");
    CU_ASSERT_EQUAL(ret, 0);
    remove("tests-extract");
    wows_free_context(context);
}

void test_decompose_path_root() {
    const char *path = "/";
    int dir_count;
    char **dirs;
    char *file;
    int result = decompose_path(path, &dir_count, &dirs, &file);

    CU_ASSERT_EQUAL(result, 0);
    CU_ASSERT_EQUAL(dir_count, 0);
    CU_ASSERT_PTR_NULL(file);
    free(dirs);
    free(file);
}

void test_wows_parse_index_file(void) {
    // Initialize the context
    WOWS_CONTEXT *context = wows_init_context(0);

    // Parse the index file
    int ret = wows_parse_index_file("./tests/data/fake.idx", context);
    // Assert that the return value is 0 (success)
    CU_ASSERT_EQUAL(ret, 0);

    char *err_msg = wows_error_string(ret, context);
    printf("Error: %s\n", err_msg);
    free(err_msg);

    // Free the context
    wows_free_context(context);
}

void test_wows_parse_index_dir(void) {
    // Initialize the context
    WOWS_CONTEXT *context = wows_init_context(0);

    // Parse the index file
    int ret = wows_parse_index_dir("./tests/data/", context);
    // Assert that the return value is 0 (success)
    CU_ASSERT_EQUAL(ret, 0);

    char *err_msg = wows_error_string(ret, context);
    printf("Error: %s\n", err_msg);
    free(err_msg);

    // Free the context
    wows_free_context(context);
}

void test_get_pkg_filepath() {
    // Prepare test data
    char *contents = calloc(sizeof(char) * TEST_DATA_SIZE, 1);
    WOWS_INDEX_HEADER header = {.magic = {'I', 'S', 'F', 'P'},
                                .file_dir_count = 5,
                                .file_count = 2,
                                .header_size = sizeof(WOWS_INDEX_HEADER),
                                .offset_idx_data_section = 512,
                                .offset_idx_footer_section = 1024};

    memcpy(contents, &header, sizeof(WOWS_INDEX_HEADER));

    WOWS_INDEX_METADATA_ENTRY metadata1 = {.file_name_size = 6, .offset_idx_file_name = 256, .id = 1, .parent_id = 3};
    char *offset_metadata1 = contents + sizeof(WOWS_INDEX_HEADER);
    WOWS_INDEX_METADATA_ENTRY metadata2 = {.file_name_size = 6, .offset_idx_file_name = 256, .id = 2, .parent_id = 3};
    char *offset_metadata2 = contents + sizeof(WOWS_INDEX_HEADER) + sizeof(WOWS_INDEX_METADATA_ENTRY);
    WOWS_INDEX_METADATA_ENTRY metadata3 = {.file_name_size = 6, .offset_idx_file_name = 256, .id = 3, .parent_id = 4};
    char *offset_metadata3 = contents + sizeof(WOWS_INDEX_HEADER) + sizeof(WOWS_INDEX_METADATA_ENTRY) * 2;
    WOWS_INDEX_METADATA_ENTRY metadata4 = {.file_name_size = 6, .offset_idx_file_name = 256, .id = 4, .parent_id = 5};
    char *offset_metadata4 = contents + sizeof(WOWS_INDEX_HEADER) + sizeof(WOWS_INDEX_METADATA_ENTRY) * 3;
    WOWS_INDEX_METADATA_ENTRY metadata5 = {
        .file_name_size = 6, .offset_idx_file_name = 256, .id = 5, .parent_id = 1111};
    char *offset_metadata5 = contents + sizeof(WOWS_INDEX_HEADER) + sizeof(WOWS_INDEX_METADATA_ENTRY) * 3;

    memcpy(offset_metadata1, &metadata1, sizeof(WOWS_INDEX_METADATA_ENTRY));
    memcpy(offset_metadata2, &metadata2, sizeof(WOWS_INDEX_METADATA_ENTRY));
    memcpy(offset_metadata3, &metadata3, sizeof(WOWS_INDEX_METADATA_ENTRY));
    memcpy(offset_metadata4, &metadata4, sizeof(WOWS_INDEX_METADATA_ENTRY));
    memcpy(offset_metadata5, &metadata5, sizeof(WOWS_INDEX_METADATA_ENTRY));

    char *name1 = "a1234";
    char *name2 = "b1234";
    char *name3 = "c1234";
    char *name4 = "d1234";
    char *name5 = "e1234";
    memcpy(offset_metadata1 + 256, name1, strlen(name1));
    memcpy(offset_metadata2 + 256, name2, strlen(name2));
    memcpy(offset_metadata3 + 256, name3, strlen(name3));
    memcpy(offset_metadata4 + 256, name4, strlen(name4));
    memcpy(offset_metadata5 + 256, name5, strlen(name5));

    WOWS_INDEX_DATA_FILE_ENTRY data_file_entry = {.metadata_id = 1,
                                                  .footer_id = 42,
                                                  .offset_pkg_data = 1024,
                                                  .type_1 = 1,
                                                  .type_2 = 2,
                                                  .size_pkg_data = 100,
                                                  .id_pkg_data = 1};
    WOWS_INDEX_DATA_FILE_ENTRY data_file_entry2 = {.metadata_id = 2,
                                                   .footer_id = 42,
                                                   .offset_pkg_data = 1024,
                                                   .type_1 = 1,
                                                   .type_2 = 2,
                                                   .size_pkg_data = 100,
                                                   .id_pkg_data = 1};
    WOWS_INDEX_FOOTER footer = {.pkg_file_name_size = 5, .id = 42};
    memcpy(contents + MAGIC_SECTION_OFFSET + 512, &data_file_entry, sizeof(WOWS_INDEX_DATA_FILE_ENTRY));
    memcpy(contents + MAGIC_SECTION_OFFSET + 512 + sizeof(WOWS_INDEX_DATA_FILE_ENTRY), &data_file_entry2,
           sizeof(WOWS_INDEX_DATA_FILE_ENTRY));
    memcpy(contents + MAGIC_SECTION_OFFSET + 1024, &footer, sizeof(WOWS_INDEX_FOOTER));
    memcpy(contents + MAGIC_SECTION_OFFSET + 1024 + sizeof(WOWS_INDEX_FOOTER), "foot", 5);

    WOWS_INDEX *index;
    map_index_file(contents, 2048, &index);
    index->index_file_path = "/path/to/wows/bin/6831266/idx/basecontent.idx";
    char *out;

    int result = get_pkg_filepath(index, &out);

    CU_ASSERT_EQUAL(0, result);
    CU_ASSERT_PTR_NOT_NULL(out);
    // Ensure the output path is correct
    CU_ASSERT_STRING_EQUAL("/path/to/wows/res_packages/foot", out);

    free(out);
    free(contents);
    free(index);
}

void test_wows_search_file(void) {
    // Initialize the context
    WOWS_CONTEXT *context = wows_init_context(0);

    // Parse the index file
    int ret = wows_parse_index_dir("./tests/data/", context);
    // Assert that the return value is 0 (success)
    CU_ASSERT_EQUAL(ret, 0);

    char *err_msg = wows_error_string(ret, context);
    printf("Error: %s\n", err_msg);
    free(err_msg);

    // Set up search parameters
    char *pattern = "[ab]123.*";
    int mode = WOWS_SEARCH_FILE_ONLY;
    int expected_result_count = 2;

    // Perform search
    int result_count = 0;
    char **results = NULL;
    int result = wows_search(context, pattern, mode, &result_count, &results);

    // Check result code
    CU_ASSERT_EQUAL(result, 0);

    // Check number of matching files
    CU_ASSERT_EQUAL(result_count, expected_result_count);

    // Check file names
    CU_ASSERT_STRING_EQUAL(results[0], "d1234/c1234/b1234");
    CU_ASSERT_STRING_EQUAL(results[1], "d1234/c1234/a1234");

    // Free memory
    for (int i = 0; i < result_count; i++) {
        free(results[i]);
    }
    free(results);

    // Set up search parameters
    pattern = "[cg]123.*";
    mode = WOWS_SEARCH_DIR_ONLY;
    expected_result_count = 2;

    // Perform search
    result = wows_search(context, pattern, mode, &result_count, &results);

    // Check result code
    CU_ASSERT_EQUAL(result, 0);

    // Check number of matching files
    CU_ASSERT_EQUAL(result_count, expected_result_count);

    // Check file names
    CU_ASSERT_STRING_EQUAL(results[0], "d1234/c1234");
    CU_ASSERT_STRING_EQUAL(results[1], "h1234/g1234");

    // Free memory
    for (int i = 0; i < result_count; i++) {
        free(results[i]);
    }
    free(results);

    // Set up search parameters
    pattern = ".*[ab]123.*";
    mode = WOWS_SEARCH_FULL_PATH;
    expected_result_count = 2;

    // Perform search
    result = wows_search(context, pattern, mode, &result_count, &results);

    // Check result code
    CU_ASSERT_EQUAL(result, 0);

    // Check number of matching files
    CU_ASSERT_EQUAL(result_count, expected_result_count);

    // Check file names
    CU_ASSERT_STRING_EQUAL(results[0], "d1234/c1234/b1234");
    CU_ASSERT_STRING_EQUAL(results[1], "d1234/c1234/a1234");

    // Free memory
    for (int i = 0; i < result_count; i++) {
        free(results[i]);
    }
    free(results);

    // Set up search parameters
    pattern = ".*[abd]123.*";
    mode = WOWS_SEARCH_FILE_PLUS_DIR;
    expected_result_count = 3;

    // Perform search
    result = wows_search(context, pattern, mode, &result_count, &results);

    // Check result code
    CU_ASSERT_EQUAL(result, 0);

    // Check number of matching files
    CU_ASSERT_EQUAL(result_count, expected_result_count);

    // Check file names
    CU_ASSERT_STRING_EQUAL(results[0], "d1234");
    CU_ASSERT_STRING_EQUAL(results[1], "d1234/c1234/b1234");
    CU_ASSERT_STRING_EQUAL(results[2], "d1234/c1234/a1234");

    // Free memory
    for (int i = 0; i < result_count; i++) {
        free(results[i]);
    }
    free(results);

    // Free the context
    wows_free_context(context);
}

void test_get_latest_idx_dir(void) {
    char *idx_dir = NULL;
    int ret = get_latest_idx_dir("./wows_sim_dir", &idx_dir);
    CU_ASSERT_EQUAL(ret, 0);         // function should return 0 on success
    CU_ASSERT_PTR_NOT_NULL(idx_dir); // function should allocate memory for idx_dir
    CU_ASSERT_STRING_EQUAL(idx_dir, "./wows_sim_dir/bin/2234567/idx/");
    free(idx_dir); // free memory allocated by function
}

void test_get_latest_idx_dir_errors(void) {
    char *idx_dir = NULL;
    int ret = get_latest_idx_dir("./tests", &idx_dir);
    CU_ASSERT_EQUAL(ret, WOWS_ERROR_FILE_OPEN_FAILURE);
    CU_ASSERT_PTR_NULL(idx_dir);
    free(idx_dir);
}

int main() {
    CU_initialize_registry();
    CU_pSuite suite = CU_add_suite("test_index_file_read_write", NULL, NULL);
    CU_add_test(suite, "test_map_index_file", test_map_index_file);
    CU_add_test(suite, "test_wows_parse_index_buffer", test_wows_parse_index_buffer);
    CU_add_test(suite, "test_wows_parse_index_file", test_wows_parse_index_file);
    CU_add_test(suite, "test_wows_parse_index_dir", test_wows_parse_index_dir);
    CU_add_test(suite, "test_wows_dump_index_to_file", test_wows_dump_index_to_file);

    suite = CU_add_suite("checkOutOfIndex", NULL, NULL);
    CU_add_test(suite, "Valid arguments", test_checkOutOfIndex_valid);
    CU_add_test(suite, "Start outside index boundaries", test_checkOutOfIndex_start_outside);
    CU_add_test(suite, "End outside index boundaries", test_checkOutOfIndex_end_outside);
    CU_add_test(suite, "Start and end outside index boundaries", test_checkOutOfIndex_both_outside);

    suite = CU_add_suite("check_printers", NULL, NULL);
    CU_add_test(suite, "test_print_header", test_print_header);
    CU_add_test(suite, "test_print_footer", test_print_footer);
    CU_add_test(suite, "test_print_metadata_entry", test_print_metadata_entry);
    CU_add_test(suite, "test_print_data_file_entry", test_print_data_file_entry);

    suite = CU_add_suite("Error code convert", NULL, NULL);
    CU_add_test(suite, "Check Conversion", test_wows_error_string);

    suite = CU_add_suite("wows_search", NULL, NULL);
    CU_add_test(suite, "test_wows_search", test_wows_search_file);

    suite = CU_add_suite("regex_suite", regex_init_suite, regex_clean_suite);
    CU_add_test(suite, "test_compile_regex", test_compile_regex);
    CU_add_test(suite, "test_match_regex_match", test_match_regex_match);
    CU_add_test(suite, "test_match_regex_no_match", test_match_regex_no_match);

    suite = CU_add_suite("Compress/Decompress Suite", NULL, NULL);
    CU_add_test(suite, "test_compress", test_compress);
    CU_add_test(suite, "test_extract", test_extract);
    CU_add_test(suite, "test_extract_file", test_extract_file);

    suite = CU_add_suite("Utils Suite", NULL, NULL);
    CU_add_test(suite, "test_decompose_path_no_sep", test_decompose_path_no_sep);
    CU_add_test(suite, "test_decompose_path_with_sep", test_decompose_path_with_sep);
    CU_add_test(suite, "test_decompose_path_only_dir", test_decompose_path_only_dir);
    CU_add_test(suite, "test_decompose_path_root", test_decompose_path_root);
    CU_add_test(suite, "test_get_pkg_filepath", test_get_pkg_filepath);
    CU_add_test(suite, "test_get_latest_idx_dir", test_get_latest_idx_dir);
    CU_add_test(suite, "test_get_latest_idx_dir_errors", test_get_latest_idx_dir_errors);

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    int ret = CU_get_number_of_failures();
    CU_cleanup_registry();
    return ret;
}
