#ifndef _IR_SEMANTIC_ANALYZER_H_
#define _IR_SEMANTIC_ANALYZER_H_

#include "compiler/ir/ir_parser.h"

int show_error_in_syntax(IRParserToken_t* root, IRParserToken_t* AST);

int show_error_in_syntax_ext(IRParserToken_t* root, IRParserToken_t* AST, int* last_line_shown, int* last_column, int pad, int newline_on_first_line_change);

int ir_semantic_analysis(IRParserToken_t** parse, int root_count);

#endif // _IR_SEMANTIC_ANALYZER_H_
