#include <stddef.h>
#include <stdio.h>
#include "wows-depack.h"


int print_header(WOWS_INDEX_HEADER *header) {
	printf("Index Header Content:\n");
	printf("* magic:                     %.4s\n", (char *)&header->magic);
	printf("* unknown_1:                 0x%x\n", header->unknown_1);
	printf("* id:                        0x%x\n", header->id);
	printf("* unknown_2:                 0x%x\n", header->unknown_2);
	printf("* file_plus_dir_count:       %u\n", header->file_plus_dir_count);
	printf("* file_count:                %u\n", header->file_count);
	printf("* unknown_3:                 %lu\n", header->unknown_3);
	printf("* header_size:               %lu\n", header->header_size);
	printf("* offset_idx_data_section:   0x%lx\n", header->offset_idx_data_section);
	printf("* offset_idx_footer_section: 0x%lx\n", header->offset_idx_footer_section);
	return 0;
}

int wows_parse_index(char *contents, size_t length, WOWS_CONTEXT *context) {
	WOWS_INDEX_HEADER *header = (WOWS_INDEX_HEADER *)contents;
	// TODO overflow, check size
	if (context->debug) {
		print_header(header);
	}
	return 0;
}
