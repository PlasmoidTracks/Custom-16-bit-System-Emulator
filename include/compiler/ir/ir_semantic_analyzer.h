#ifndef _IR_SEMANTIC_ANALYZER_H_
#define _IR_SEMANTIC_ANALYZER_H_

#include "compiler/ir/ir_parser.h"

void show_error_in_syntax(IRParserToken_t* root, IRParserToken_t* AST);

int ir_semantic_analysis(IRParserToken_t** parse, int root_count);

#endif // _IR_SEMANTIC_ANALYZER_H_
