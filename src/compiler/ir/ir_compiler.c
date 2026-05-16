
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "utils/Log.h"
#include "utils/String.h"
#include "utils/IO.h"

#include "compiler/asm/assembler.h"

#include "compiler/ir/ir_parser.h"
#include "compiler/ir/ir_token_name_table.h"
#include "compiler/ir/ir_lexer.h"
#include "compiler/ir/ir_compiler.h"
#include "compiler/ir/ir_semantic_analyzer.h"


char* ir_compile(char* source, long source_length, IRCompileOption_t options) {

    long token_count;
    IRLexerToken_t* token = ir_lexer_parse(source, source_length, &token_count);

    for (long i = 0; i < token_count; i++) {
        //log_msg(LP_DEBUG, "LEX: %d %s (l:%d, c:%d)", i, token[i].raw, token[i].line, token[i].column);
    }

    long parser_root_count;
    IRParserToken_t** parse = ir_parser_parse(source, source_length, &parser_root_count);
    if (!parse) {
        //log_msg(LP_ERROR, "parser returned NULL [%s:%d]", __FILE__, __LINE__);
        return NULL;
    }
    int invalid = 0;
    for (long i = 0; i < parser_root_count; i++) {
        if (parse[i]->token.type != IR_PAR_STATEMENT && parse[i]->token.type != IR_PAR_FUNCTION_DEFINITION) {
            log_msg(LP_ERROR, "Invalid syntax (found %s where only STATEMENT and FUNCTION_DEFINITION are allowed)", ir_token_name[parse[i]->token.type]);
            show_error_in_syntax(parse[i], parse[i]);
            invalid = 1;
        }
    }
    if (invalid) {
        return NULL;
    }

    // First comes semantic analysis, operations beyond functions is not allowed for instance. 
    if (!ir_semantic_analysis(parse, parser_root_count)) {
        log_msg(LP_ERROR, "Semantic analysis returned invalid semantics [%s:%d]", __FILE__, __LINE__);
        return NULL;
    }
    
    // Prepass, collecting all variables and their scope (either global, or inside one of the function scopes (scopebegin - scopeend))
    // So all offsets are stored, VLAs are tracked, type modifiers set, statics delegated and indexed, etc. required files, 

    return NULL;
}

char* ir_compile_from_filename(const char* const filename, IRCompileOption_t options) {
    long content_size;
    char* content = read_file(filename, &content_size);
    if (!content) {
        log_msg(LP_ERROR, "IR: read_file failed [%s:%d]", __FILE__, __LINE__);
        return NULL;
    }
    char* asm = ir_compile(content, content_size, options);
    if (!asm) {
        log_msg(LP_ERROR, "IR: Compiler returned NULL [%s:%d]", __FILE__, __LINE__);
        return NULL;
    }
    return asm;
}
