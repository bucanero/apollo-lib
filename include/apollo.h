#ifndef _APOLLO_LIB_H_
#define _APOLLO_LIB_H_

#include <stdint.h>
#include <stdlib.h>

#define APOLLO_LIB_VERSION         "0.4.0"

#define APOLLO_CODE_GAMEGENIE      1
#define APOLLO_CODE_BSD            2

#define APOLLO_CODE_FLAG_PARENT    1
#define APOLLO_CODE_FLAG_CHILD     2
#define APOLLO_CODE_FLAG_REQUIRED  4
#define APOLLO_CODE_FLAG_ALERT     8
#define APOLLO_CODE_FLAG_EMPTY     16
#define APOLLO_CODE_FLAG_ENABLED   32

#ifdef __cplusplus
extern "C" {
#endif

typedef struct list_node_s
{
	void *value;
	struct list_node_s *next;
} list_node_t;

typedef struct list_s
{
	list_node_t *head;
	size_t count;
} list_t;

typedef struct option_entry
{
    char * line;
    char * * name;
    char * * value;
    int id;
    int size;
    int sel;
} option_entry_t;

typedef struct code_entry
{
    uint8_t type;
    uint8_t flags;
    char * name;
    char * file;
    uint8_t activated;
    int options_count;
    char * codes;
    option_entry_t * options;
} code_entry_t;


//---  Generic list functions ---

list_t * list_alloc(void);
void list_free(list_t *list);

list_node_t * list_append(list_t *list, void *value);

list_node_t * list_head(list_t *list);
list_node_t * list_tail(list_t *list);
size_t list_count(list_t *list);

list_node_t * list_next(list_node_t *node);
void * list_get(list_node_t *node);
void * list_get_item(list_t *list, size_t item);

void list_bubbleSort(list_t *list, int (*compar)(const void *, const void *));


//---  Generic utility functions ---

uint64_t x_to_u64(const char *hex);
uint8_t * x_to_u8_buffer(const char *hex);

int wildcard_match(const char *data, const char *mask);
int wildcard_match_icase(const char *data, const char *mask);

int read_buffer(const char *file_path, uint8_t **buf, size_t *size);
int write_buffer(const char *file_path, uint8_t *buf, size_t size);


//---  Apollo patch functions ---

void free_patch_var_list();
int apply_bsd_patch_code(const char* file_path, code_entry_t* code);
int apply_ggenie_patch_code(const char* file_path, code_entry_t* code);
int apply_cheat_patch_code(const char* file_path, const char* title_id, code_entry_t* code, const char* tmp_dir);
void load_patch_code_list(char* buffer, list_t* list_codes, option_entry_t* (*get_files_opt)(const char*, const char*), const char* save_path);

#ifdef __cplusplus
}
#endif

#endif /* !_APOLLO_LIB_H_ */
