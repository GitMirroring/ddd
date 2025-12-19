
#ifndef SOURCETOKENIZER_H
#define SOURCETOKENIZER_H

#include "SourceWidget.h"

void TokenizeCpp_BreezeLight(const char* text, size_t length, std::vector<XmhColorToken>& out);
void TokenizeGdbDisassembly(const char* text, size_t length, std::vector<XmhColorToken>& out);

#endif // SOURCETOKENIZER_H
