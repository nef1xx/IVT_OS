#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PAGE_SIZE 256
#define PAGE_COUNT 256
#define FRAME_COUNT 256
#define TLB_SIZE 16
#define MEMORY_SIZE (FRAME_COUNT * PAGE_SIZE)
#define PATH_BUFFER_SIZE 1024

static signed char physical_memory[MEMORY_SIZE];
static int page_table[PAGE_COUNT];
static int tlb_pages[TLB_SIZE];
static int tlb_frames[TLB_SIZE];

static FILE *open_file(const char *path, const char *mode)
{
    FILE *file = NULL;

#ifdef _MSC_VER
    if (fopen_s(&file, path, mode) != 0) {
        return NULL;
    }
#else
    file = fopen(path, mode);
#endif

    return file;
}

static int read_address(FILE *file, int *address)
{
#ifdef _MSC_VER
    return fscanf_s(file, "%d", address);
#else
    return fscanf(file, "%d", address);
#endif
}

static void initialize_array(int values[], int count, int value)
{
    int i;

    for (i = 0; i < count; ++i) {
        values[i] = value;
    }
}

static int find_last_separator(const char *path)
{
    int i;
    int last_separator = -1;

    for (i = 0; path[i] != '\0'; ++i) {
        if (path[i] == '\\' || path[i] == '/') {
            last_separator = i;
        }
    }

    return last_separator;
}

static int build_sibling_path(const char *source_path, const char *file_name, char output[], size_t output_size)
{
    int separator_index = find_last_separator(source_path);
    size_t directory_length = 0;
    size_t file_name_length = strlen(file_name);

    if (separator_index >= 0) {
        directory_length = (size_t)separator_index + 1;
    }

    if (directory_length + file_name_length + 1 > output_size) {
        return 0;
    }

    if (directory_length > 0) {
        memcpy(output, source_path, directory_length);
    }

    memcpy(output + directory_length, file_name, file_name_length + 1);
    return 1;
}

static FILE *open_backing_store(const char *addresses_path, char opened_path[], size_t opened_path_size)
{
    const char *backing_store_names[] = {
        "BACKING_STORE.bin",
        "BACKING STORE.bin"
    };
    int i;

    for (i = 0; i < 2; ++i) {
        char candidate[PATH_BUFFER_SIZE];
        FILE *file;

        if (!build_sibling_path(addresses_path, backing_store_names[i], candidate, sizeof(candidate))) {
            continue;
        }

        file = open_file(candidate, "rb");
        if (file != NULL) {
            size_t candidate_length = strlen(candidate);

            if (candidate_length + 1 > opened_path_size) {
                fclose(file);
                return NULL;
            }

            memcpy(opened_path, candidate, candidate_length + 1);
            return file;
        }
    }

    return NULL;
}

static int search_tlb(int page_number)
{
    int i;

    for (i = 0; i < TLB_SIZE; ++i) {
        if (tlb_pages[i] == page_number) {
            return tlb_frames[i];
        }
    }

    return -1;
}

static void update_tlb(int page_number, int frame_number, int *next_tlb_slot)
{
    tlb_pages[*next_tlb_slot] = page_number;
    tlb_frames[*next_tlb_slot] = frame_number;
    *next_tlb_slot = (*next_tlb_slot + 1) % TLB_SIZE;
}

static int load_page(FILE *backing_store, int page_number, int frame_number)
{
    long backing_store_offset = (long)page_number * PAGE_SIZE;
    size_t bytes_read;

    if (fseek(backing_store, backing_store_offset, SEEK_SET) != 0) {
        return 0;
    }

    bytes_read = fread(&physical_memory[frame_number * PAGE_SIZE], sizeof(signed char), PAGE_SIZE, backing_store);
    return bytes_read == PAGE_SIZE;
}

int main(int argc, char *argv[])
{
    FILE *addresses_file;
    FILE *backing_store;
    char backing_store_path[PATH_BUFFER_SIZE];
    int next_free_frame = 0;
    int next_tlb_slot = 0;
    int logical_address;
    int address_count = 0;
    int page_fault_count = 0;
    int tlb_hit_count = 0;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s addresses.txt\n", argv[0]);
        return EXIT_FAILURE;
    }

    addresses_file = open_file(argv[1], "r");
    if (addresses_file == NULL) {
        fprintf(stderr, "Error: cannot open addresses file '%s'.\n", argv[1]);
        return EXIT_FAILURE;
    }

    backing_store = open_backing_store(argv[1], backing_store_path, sizeof(backing_store_path));
    if (backing_store == NULL) {
        fprintf(stderr, "Error: cannot open BACKING_STORE.bin next to '%s'.\n", argv[1]);
        fclose(addresses_file);
        return EXIT_FAILURE;
    }

    initialize_array(page_table, PAGE_COUNT, -1);
    initialize_array(tlb_pages, TLB_SIZE, -1);
    initialize_array(tlb_frames, TLB_SIZE, -1);

    while (read_address(addresses_file, &logical_address) == 1) {
        int masked_address = logical_address & 0xFFFF;
        int page_number = (masked_address >> 8) & 0xFF;
        int offset = masked_address & 0xFF;
        int frame_number = search_tlb(page_number);
        int physical_address;
        signed char value;

        ++address_count;

        if (frame_number != -1) {
            ++tlb_hit_count;
        } else {
            frame_number = page_table[page_number];

            if (frame_number == -1) {
                if (next_free_frame >= FRAME_COUNT) {
                    fprintf(stderr, "Error: physical memory is full. Page replacement is not part of the main task.\n");
                    fclose(backing_store);
                    fclose(addresses_file);
                    return EXIT_FAILURE;
                }

                frame_number = next_free_frame;
                if (!load_page(backing_store, page_number, frame_number)) {
                    fprintf(stderr, "Error: cannot read page %d from '%s'.\n", page_number, backing_store_path);
                    fclose(backing_store);
                    fclose(addresses_file);
                    return EXIT_FAILURE;
                }

                page_table[page_number] = frame_number;
                ++next_free_frame;
                ++page_fault_count;
            }

            update_tlb(page_number, frame_number, &next_tlb_slot);
        }

        physical_address = frame_number * PAGE_SIZE + offset;
        value = physical_memory[physical_address];

        printf("Virtual address: %d Physical address: %d Value: %d\n",
               logical_address,
               physical_address,
               (int)value);
    }

    if (ferror(addresses_file)) {
        fprintf(stderr, "Error: failed while reading addresses file '%s'.\n", argv[1]);
        fclose(backing_store);
        fclose(addresses_file);
        return EXIT_FAILURE;
    }

    printf("Number of Translated Addresses = %d\n", address_count);
    printf("Page Faults = %d\n", page_fault_count);
    printf("Page Fault Rate = %.3f\n", address_count == 0 ? 0.0 : (double)page_fault_count / address_count);
    printf("TLB Hits = %d\n", tlb_hit_count);
    printf("TLB Hit Rate = %.3f\n", address_count == 0 ? 0.0 : (double)tlb_hit_count / address_count);

    fclose(backing_store);
    fclose(addresses_file);

    return EXIT_SUCCESS;
}
