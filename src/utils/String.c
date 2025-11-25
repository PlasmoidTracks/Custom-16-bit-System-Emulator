#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "Log.h"
#include "utils/ExtendedTypes.h"

#include "utils/String.h"


int contains(char character, char* matches) {
    if (!matches) {return 1;}
    int index = 0;
    while (matches[index] != '\0') {
        if (character == matches[index]) {
            return 1;
        }
        index ++;
    }
    return 0;
}

// TODO: Add "ignore_string_literals" as an optional parameter
char** split(const char* string, char* separators, char* preserved_separators) {
    int capacity = 10;
    char** splits = calloc(capacity, sizeof(char*));
    int count = 0;

    int i = 0;
    int word_start = -1;
    int inside_quotes = 0;

    while (string[i] != '\0') {
        if (string[i] == '"') {
            if (!inside_quotes) {
                // Close the previous word if it exists
                if (word_start != -1) {
                    int word_len = i - word_start;
                    if (word_len > 0) {
                        char* word = calloc(word_len + 1, sizeof(char));
                        memcpy(word, &string[word_start], word_len);
                        word[word_len] = '\0';
                        if (count >= capacity - 1) {
                            capacity *= 2;
                            splits = realloc(splits, sizeof(char*) * capacity);
                        }
                        splits[count++] = word;
                    }
                    word_start = -1;
                }
        
                inside_quotes = 1;
                word_start = i;  // Start of quoted string
            } else {
                inside_quotes = 0;
                int word_len = i - word_start + 1;
                char* word = calloc(word_len + 1, sizeof(char));
                memcpy(word, &string[word_start], word_len);
                word[word_len] = '\0';
        
                if (count >= capacity - 1) {
                    capacity *= 2;
                    splits = realloc(splits, sizeof(char*) * capacity);
                }
                splits[count++] = word;
                word_start = -1;
            }
            i++;
            continue;
        }

        if (!inside_quotes && (contains(string[i], separators) || contains(string[i], preserved_separators))) {
            if (word_start != -1) {
                int word_len = i - word_start;
                char* word = calloc(word_len + 1, sizeof(char));
                memcpy(word, &string[word_start], word_len);
                word[word_len] = '\0';

                if (count >= capacity - 1) {
                    capacity *= 2;
                    splits = realloc(splits, sizeof(char*) * capacity);
                }
                splits[count++] = word;
                word_start = -1;
            }

            if (contains(string[i], preserved_separators)) {
                char* sep = malloc(2);
                sep[0] = string[i];
                sep[1] = '\0';
                if (count >= capacity - 1) {
                    capacity *= 2;
                    splits = realloc(splits, sizeof(char*) * capacity);
                }
                splits[count++] = sep;
            }
        } else {
            if (word_start == -1) word_start = i;
        }

        i++;
    }

    if (word_start != -1) {
        int word_len = i - word_start;
        char* word = malloc(word_len + 1);
        memcpy(word, &string[word_start], word_len);
        word[word_len] = '\0';
        if (count >= capacity - 1) {
            capacity *= 2;
            splits = realloc(splits, sizeof(char*) * capacity);
        }
        splits[count++] = word;
    }

    splits[count] = NULL;
    return splits;
}

int string_is_numeral(char text[]) {
    int index = 0;
    if (text[0] == '-') {
        index ++;
        while (text[index]) {
            if (text[index] < '0' || text[index] > '9') return 0;
            index ++;
        }
        return index > 1;
    }
    while (text[index]) {
        if (text[index] < '0' || text[index] > '9') return 0;
        index ++;
    }
    return 1;
}

int string_is_hex_numeral(char text[]) {
    int index = 0;
    if (text[0] == '-') {index ++;}
    while (text[index]) {
        if (text[index] < '0' || text[index] > '9') {
            if (text[index] < 'a' || text[index] > 'f') {
                if (text[index] < 'A' || text[index] > 'F') {
                    return 0;
                }
            }
        }
        index ++;
    }
    return 1;
}

int string_is_float(char text[]) {
    int index = 0;
    int point_count = 0;
    int has_number = 0;
    if (!text || !text[0]) return 0;
    if (text[0] == '-') {index ++;}
    if (!text[index]) return 0;
    while (text[index]) {
        if (text[index] == '.') {
            point_count ++;
            if (point_count > 1) return 0;
        }
        else if (text[index] < '0' || text[index] > '9') {
            return 0;
        } else {
            has_number = 1;
        }
        index ++;
    }
    return has_number;
}

int string_is_asm_immediate(char text[]) {
    if (string_is_numeral(&text[0])) return 1;
    if (text[0] == '$' && string_is_hex_numeral(&text[1])) return 1;
    if (text[0] == 'f' && string_is_float(&text[1])) return 1;
    if (text[0] == 'b' && text[1] == 'f' && string_is_float(&text[2])) return 1;
    if (text[0] == '0') {
        if ((text[1] == 'x') && string_is_hex_numeral(&text[2])) return 1;
        if ((text[1] == 'o' || text[1] == 'b') && string_is_numeral(&text[2])) return 1;
    }
    return 0;
}

int string_is_immediate(char text[]) {
    //log_msg(LP_CRITICAL, "TEST %s", text);
    if (string_is_numeral(text)) return 1;
    if (string_is_float(text)) return 1;
    //if (text[0] == '$' && string_is_hex_numeral(&text[1])) return 1;
    if (text[0] == 'f' && string_is_float(&text[1])) return 1;
    if (text[0] == '0') {
        if ((text[1] == 'x') && string_is_hex_numeral(&text[2])) return 1;
        if ((text[1] == 'o' || text[1] == 'b') && string_is_numeral(&text[2])) return 1;
    }
    return 0;
}

int string_is_string(char text[]) {
    if (text[0] == '\"' && text[strlen(text) - 1] == '\"') return 1;
    return 0;
}

uint16_t parse_immediate(const char text[]) {
    if (text[0] == '$') {
        return strtol(&text[1], NULL, 16);
    }
    if (text[0] == 'f') {
        return f16_from_float(strtof(&text[1], NULL));
    }
    if (text[0] == 'b' && text[1] == 'f') {
        return bf16_from_float(strtof(&text[2], NULL));
    }
    if (text[0] == '0') {
        if (text[1] == 'x') {
            return strtol(&text[2], NULL, 16);
        } 
        if (text[1] == 'o') {
            return strtol(&text[2], NULL, 8);
        } 
        if (text[1] == 'b') {
            return strtol(&text[2], NULL, 2);
        }
    }
    return strtol(&text[0], NULL, 10);;
}

char* append_to_output_ext(char* output, long* length, const char* text, int text_len) {
    if (text_len <= 0) text_len = strlen(text);
    
    long current_len = *length;
    long new_len = current_len + text_len;

    output = realloc(output, new_len + 1);  // +1 for null terminator
    memcpy(output + current_len, text, text_len);
    output[new_len] = '\0';

    *length = new_len;
    return output;
}

char* append_filename_ext(char* filename, const char* text, int text_len) {
    if (text_len <= 0) text_len = strlen(text);
    
    long current_len = strlen(filename);
    long new_len = current_len + text_len;

    filename = realloc(filename, new_len + 1);  // +1 for null terminator
    memcpy(filename + current_len, text, text_len);
    filename[new_len] = '\0';

    return filename;
}

