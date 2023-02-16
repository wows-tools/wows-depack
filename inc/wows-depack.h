#include <stdint.h>
#include <stdbool.h>

// INDEX file header
typedef struct {
	uint32_t  magic;
	uint32_t unknown_1;
	uint32_t id;
	uint32_t unknown_2;
	uint32_t file_plus_dir_count;
	uint32_t file_count;
	uint64_t unknown_3;
	uint64_t header_size;
	uint64_t offset_idx_data_section;
	uint64_t offset_idx_footer_section;
} WOWS_INDEX_HEADER;

// INDEX file metadata entry
typedef struct {
	uint64_t file_type_1;
	uint64_t offset_idx_file_name;
	uint64_t unknown_4;
	uint64_t file_type_2;
} WOWS_INDEX_METADATA_ENTRY;

// INDEX file pkg data pointer entry
typedef struct {
	uint64_t unknown_5;
	uint64_t unknown_6;
	uint64_t offset_pkg_data_chunk;
	uint32_t type_1;
	uint32_t type_2;
	uint32_t size_pkg_data_chunk;
	uint64_t id_pkg_data_chunk;
	uint32_t padding;
} WOWS_INDEX_DATA_FILE_ENTRY;

// INDEX file footer
typedef struct {
	uint64_t size_pkg_file_name;
	uint64_t unknown_7;
	uint64_t unknown_6;
} WOWS_INDEX_FOOTER;

// PKG file ID + padding
typedef struct {
	uint32_t padding_1;
	uint64_t id_pkg_data_chunk;
	uint32_t padding_2;
} WOWS_PKG_ID_ENTRY;


// Context for the parsing/file extraction
typedef struct {
	bool debug;
} WOWS_CONTEXT;

int wows_inflate(FILE *source, FILE *dest, long *read);
int wows_inflate(FILE *source, FILE *dest, long *read);
void wows_zerr(int ret);
int wows_is_dir(const char *path);
int wows_inflate_all(FILE *in, char *outdir);

int wows_parse_index(char *contents, size_t length, WOWS_CONTEXT *context);
