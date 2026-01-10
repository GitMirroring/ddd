#include <cctype>
#include <cstdint>
#include <vector>
#include <string>
#include <cstring>
#include <algorithm>

#include "SourceTokenizer.h"


// Breeze Light category indices (map these to actual colors in your renderer)
enum BreezeLightColor {
    BL_Default      = 0,
    BL_Keyword      = 1,
    BL_Type         = 2,
    BL_Number       = 3,
    BL_String       = 4,
    BL_Char         = 5,
    BL_Comment      = 6,
    BL_Preprocessor = 7,
    BL_Includes     = 8,
    BL_Operator     = 9,
    BL_Standard_Classes = 10,
    BL_Boost_Stuff = 11,
    BL_Special_Variables = 12,  // Data Members m_*, Globals g_*, Statics s_*
    BL_Annotation = 13, // Doxygen commands
    BL_Delimiter    = 14,  // , and ;
    BL_Bracket      = 15,   // ( ) { } [ ]
    BL_Address      = 16,  // Hex addresses
    BL_Register     = 17,  // CPU registers
    BL_Instruction  = 18,  // Assembly instructions
    BL_Label        = 19   // Function labels
};

static inline bool isIdentStart(unsigned char c)
{
    return c == '_' || isalpha(c) || (c & 0x80); // allow non-ASCII for UTF-8 identifiers
}

static inline bool isIdentContinue(unsigned char c)
{
    return isIdentStart(c) || (c >= '0' && c <= '9');
}

static inline bool isBin(unsigned char c)
{
    return c == '0' || c == '1';
}

static inline bool isOct(unsigned char c)
{
    return c >= '0' && c <= '7';
}

static bool isKeyword(const std::string& s)
{
    // C++20 keywords
    static const char* kwords[] = {
        "alignas","alignof","asm","auto","break","case","catch","class","consteval",
        "constexpr","constinit","const_cast","continue","co_await","co_return","co_yield",
        "decltype","default","delete","do","else","enum","explicit","export","extern",
        "final","for","friend","goto","if","inline","mutable","namespace",
        "new","noexcept","nullptr","operator","override","private","protected","public",
        "register","reinterpret_cast","requires","return","sizeof",
        "static_assert","static_cast","struct","switch","template","this","thread_local",
        "throw","try","typedef","typeid","typename","union","using","virtual",
        "volatile","while"
    };

    for (auto kw : kwords)
        if (s == kw)
            return true;

    return false;
}

static bool isTypeLike(const std::string& s)
{
    // Built-in and common fundamental types
    static const char* tys[] = {
        "bool","char","char8_t","char16_t","char32_t","wchar_t",
        "short","int","long","float","double","void",
        "signed","unsigned","size_t","ptrdiff_t","int8_t","int16_t","int32_t","int64_t",
        "uint8_t","uint16_t","uint32_t","uint64_t","intptr_t","uintptr_t","static","const"
    };

    for (auto t : tys)
        if (s == t)
        return true;

    return false;
}

static bool isStandardClass(const std::string& s)
{
    static const char* stdClasses[] = {
        "string", "vector", "array", "map", "unordered_map", "set", "unordered_set",
        "list", "deque", "stack", "queue", "priority_queue", "bitset",
        "iostream", "istream", "ostream", "fstream", "stringstream",
        "exception", "runtime_error", "logic_error", "invalid_argument",
        "shared_ptr", "unique_ptr", "weak_ptr", "make_shared", "make_unique",
        "thread", "mutex", "lock_guard", "unique_lock", "future", "promise",
        "regex", "smatch", "cmatch", "function", "bind", "tuple", "pair",
        "optional", "variant", "any", "filesystem", "path"
    };

    for (auto cls : stdClasses)
        if (s == cls)
            return true;

    return false;
}

static bool isBoostStuff(const std::string& s)
{
    // Check if identifier starts with "boost"
    if (s.length() >= 5 && s.substr(0, 5) == "boost") 
        return true;

    static const char* boostItems[] = {
        "asio", "filesystem", "system", "thread", "program_options",
        "property_tree", "date_time", "chrono", "atomic", "container"
    };

    for (auto item : boostItems)
        if (s == item)
            return true;
    return false;
}

static bool isSpecialVariable(const std::string& s)
{
    // Check for member variables (m_*), globals (g_*), statics (s_*)
    if (s.length() >= 2)
    {
        if (s[0] == 'm' && s[1] == '_')
            return true;  // Member variables
        if (s[0] == 'g' && s[1] == '_')
            return true;  // Global variables
        if (s[0] == 's' && s[1] == '_')
            return true;  // Static variables
    }
    return false;
}

static bool isOperatorChar(unsigned char c)
{
    static const char operators[] = "+-*/%=<>!&|^~?:";
    for (char op : operators)
    {
        if (c == op)
            return true;
    }
    return false;
}

static bool isDoxygenCommand(const char* s, size_t pos, size_t n, size_t& cmdEnd)
{
    // Check if we're at a Doxygen command (starts with @ or \)
    if (pos >= n) return false;

    if ((s[pos] == '@' || s[pos] == '\\') && pos + 1 < n)
    {
        // Find end of command (whitespace, punctuation, or end of comment)
        size_t j = pos + 1;
        while (j < n && isIdentContinue(static_cast<unsigned char>(s[j]))) 
            ++j;

        if (j > pos + 1)
        {  // Found a potential command
            std::string cmd(s + pos + 1, s + j);

            // Whitelist of valid Doxygen commands
            static const char* doxygenCommands[] = {
                // Main commands
                "brief", "short", "class", "struct", "union", "enum", "fn", "var",
                "def", "typedef", "file", "namespace", "package", "interface",
                "exception", "throw", "throws", "see", "sa", "link", "code", "endcode",
                "verbatim", "endverbatim", "copydoc", "copybrief", "copydetails",

                // Documentation sections
                "author", "authors", "version", "since", "date", "copyright",
                "license", "invariant", "note", "warning", "pre", "post",
                "remark", "attention", "par", "paragraph", "param", "tparam",

                // Function documentation
                "return", "returns", "result", "retval", "exception", "throw", "throws",

                // Grouping
                "addtogroup", "ingroup", "weakgroup", "group", "defgroup",
                "addtogroup", "ingroup", "weakgroup",

                // Other common commands
                "deprecated", "todo", "bug", "test", "example", "page",
                "section", "subsection", "subsubsection", "anchor", "ref", "refitem"
            };

            for (const char* validCmd : doxygenCommands) 
            {
                if (cmd == validCmd) 
                {
                    cmdEnd = j;
                    return true;
                }
            }
        }
    }
    return false;
}


static void addToken(std::vector<XmhColorToken>& out, Utf8Pos start, Utf8Pos end, int color, int style = XMH_STYLE_NONE)
{
    if (end <= start)
        return;

    if (color == BL_Default && style == XMH_STYLE_NONE)
        return; // only emit non-default

    XmhColorToken tok;
    tok.start = start;
    tok.len   = static_cast<int>(end - start);
    tok.color = color;
    tok.style = style;
    out.push_back(tok);
}

// Scan a C++ raw string starting at index i at the initial `R"`. Returns end index (one past)
static size_t scanRawString(const char* s, size_t n, size_t i)
{
    // Precondition: s[i] == 'R' and s[i+1] == '"'
    size_t j = i + 2; // after R"
    // delimiter: chars up to '('
    size_t delimStart = j;
    while (j < n && s[j] != '(' && s[j] != '\n' && s[j] != '\r')
        ++j; // per standard, delimiter cannot contain space, ), \, control chars; we'll be permissive except newline
        
    if (j >= n || s[j] != '(') 
        return std::min(n, i + 1); // malformed; bail minimally

    std::string delim(s + delimStart, s + j);
    ++j; // move past '(' -> now at raw content start
    // search for )delim"
    while (j < n)
    {
        if (s[j] == ')') 
        {
            size_t k = j + 1;
            // match delim
            bool match = true;
            for (size_t d = 0; d < delim.size(); ++d) 
            {
                if (k + d >= n || s[k + d] != delim[d]) 
                { 
                    match = false; 
                    break; 
                }
            }

            if (match) 
            {
                k += delim.size();
                if (k < n && s[k] == '"') 
                    return k + 1; // include closing quote
            }
        }
        ++j;
    }
    return n; // unterminated -> till end
}

// Scan a standard string literal, supports escapes and multiline if escaped newline; returns end index (one past)
static size_t scanQuoted(const char* s, size_t n, size_t i, char quote)
{
    // Precondition: s[i] == quote
    size_t j = i + 1;
    while (j < n)
    {
        char c = s[j++];
        if (c == '\\')
        {
            if (j < n) 
                ++j; // skip escaped char
        }
        else if (c == quote)
        {
            return j;
        }
        else if (c == '\n' || c == '\r')
        {
            // Unterminated
            return j - 1;
        }
    }
    return n;
}

// Scan a character literal starting at prefix (possibly L, u, U); returns end index (one past)
static size_t scanCharLiteral(const char* s, size_t n, size_t i)
{
    // i at '\'' (or earlier prefix handled by caller)
    return scanQuoted(s, n, i, '\'');
}

// Scan numeric literal; returns end index (one past)
static size_t scanNumber(const char* s, size_t n, size_t i)
{
    size_t j = i;
    // prefixes
    if (j + 1 < n && s[j] == '0' && (s[j+1] == 'x' || s[j+1] == 'X'))
    {
        j += 2;
        while (j < n && (isxdigit(static_cast<unsigned char>(s[j])) || s[j] == '\'' || s[j] == '_')) 
            ++j;
        // hex float exponent p/P
        if (j < n && (s[j] == '.')) 
        {
            ++j;
            while (j < n && (isxdigit(static_cast<unsigned char>(s[j])) || s[j] == '\'' || s[j] == '_')) 
                ++j;
        }

        if (j < n && (s[j] == 'p' || s[j] == 'P')) 
        {
            ++j;
            if (j < n && (s[j] == '+' || s[j] == '-')) 
                ++j;
            while (j < n && (isdigit(static_cast<unsigned char>(s[j])) || s[j] == '\'' || s[j] == '_')) 
                ++j;
        }
    }
    else if (j + 1 < n && s[j] == '0' && (s[j+1] == 'b' || s[j+1] == 'B'))
    {
        j += 2;
        while (j < n && (isBin(static_cast<unsigned char>(s[j])) || s[j] == '\'' || s[j] == '_')) 
            ++j;
    }
    else if (j + 1 < n && s[j] == '0' && isOct(static_cast<unsigned char>(s[j+1])))
    {
        // octal or just 0
        ++j;
        while (j < n && (isOct(static_cast<unsigned char>(s[j])) || s[j] == '\'' || s[j] == '_')) 
            ++j;
    }
    else
    {
        // decimal or floating
        while (j < n && (isdigit(static_cast<unsigned char>(s[j])) || s[j] == '\'' || s[j] == '_')) 
            ++j;
        if (j < n && s[j] == '.')
        {
            ++j;
            while (j < n && (isdigit(static_cast<unsigned char>(s[j])) || s[j] == '\'' || s[j] == '_')) 
                ++j;
        }
        if (j < n && (s[j] == 'e' || s[j] == 'E'))
        {
            ++j;
            if (j < n && (s[j] == '+' || s[j] == '-'))
                ++j;
            while (j < n && (isdigit(static_cast<unsigned char>(s[j])) || s[j] == '\'' || s[j] == '_')) 
                ++j;
        }
    }
    // suffixes (u, l, f, i, etc.)
    while (j < n)
    {
        unsigned char c = static_cast<unsigned char>(s[j]);
        if (std::isalpha(c) || c == '_') 
            ++j; 
        else 
            break;
    }
    return j;
}

// Scan identifier (UTF-8 friendly by byte rule); returns end index
static size_t scanIdentifier(const char* s, size_t n, size_t i)
{
    size_t j = i;
    if (j < n && isIdentStart(static_cast<unsigned char>(s[j])))
    {
        ++j;
        while (j < n && isIdentContinue(static_cast<unsigned char>(s[j]))) ++j;
    }
    return j;
}

// Tokenize a single preprocessor line; returns end-of-line index
static size_t tokenizePreprocessor(const char* s, size_t n, size_t i, std::vector<XmhColorToken>& out)
{
    size_t j = i;
    // consume leading whitespace
    while (j < n && (s[j] == ' ' || s[j] == '\t')) 
        ++j;

    size_t hashPos = j;
    if (j < n && s[j] == '#')
    {
        // mark directive '#...'
        size_t k = j + 1;
        while (k < n && (s[k] == ' ' || s[k] == '\t')) 
            ++k;
        size_t dirStart = k;
        while (k < n && isIdentContinue(static_cast<unsigned char>(s[k]))) 
           ++k;
        addToken(out, hashPos, k, BL_Preprocessor, XMH_STYLE_BOLD);
        std::string dir(s + dirStart, s + k);
        // special-case include-like path tokens: <...> or "..."
        size_t p = k;
        while (p < n && s[p] != '\n' && s[p] != '\r')
        {
            if (s[p] == '/' && p + 1 < n && s[p+1] == '/')
            {
                // comment till EOL
                size_t lineEnd = p;
                while (lineEnd < n && s[lineEnd] != '\n' && s[lineEnd] != '\r')
                    ++lineEnd;
                addToken(out, p, lineEnd, BL_Comment, XMH_STYLE_ITALIC);
                break;
            }
            else if ((s[p] == '<' || s[p] == '"') && (dir == "include" || dir == "import"))
            {
                // scan until '>' (basic)
                size_t q = p + 1;
                while (q < n && s[q] != '>' && s[q] != '"' && s[q] != '\n')
                    q++;
                if (q < n && (s[q] == '>' || s[q] == '"'))
                    q++;
                addToken(out, p, q, BL_Includes);
                p = q;
            }
            else if (s[p] == '"' )
            {
                size_t q = scanQuoted(s, n, p, '"');
                addToken(out, p, q, BL_String);
                p = q;
            }
            else if (s[p] == '\'' )
            {
                size_t q = scanQuoted(s, n, p, '\'');
                addToken(out, p, q, BL_Char);
                p = q;
            }
            else if (s[p] == '/' && p + 1 < n && s[p+1] == '*')
            {
                size_t q = p + 2;
                while (q + 1 < n && !(s[q] == '*' && s[q+1] == '/')) 
                    ++q;
                if (q + 1 < n) 
                    q += 2;
                addToken(out, p, q, BL_Comment, XMH_STYLE_ITALIC);
                p = q;
            }
            else
            {
                p++;
            }
        }
        // move to EOL
        while (j < n && s[j] != '\n' && s[j] != '\r') 
            ++j;
        return j;
    }
    // not a preprocessor; just go to EOL
    while (j < n && s[j] != '\n' && s[j] != '\r') 
        ++j;
    return j;
}

void TokenizeCpp_BreezeLight(const char* text, size_t length, std::vector<XmhColorToken>& out)
{
    if (!text)
        return;
    const char* s = text;
    size_t n = length ? length : std::strlen(text);

    size_t i = 0;
    while (i < n)
    {
        unsigned char c = static_cast<unsigned char>(s[i]);

        // Newlines / whitespace: skip
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
        {
            i++;
            continue;
        }

        // Preprocessor: at line start or after only whitespace until '#'
        if (c == '#') 
        {
            size_t lineEnd = tokenizePreprocessor(s, n, i, out);
            i = lineEnd;
            continue;
        }
        else
        {
            // If at line start with leading spaces then '#'
            // find beginning of line
            size_t back = i;
            while (back > 0 && s[back-1] != '\n' && s[back-1] != '\r') 
                --back;
            size_t look = back;
            while (look < i && (s[look] == ' ' || s[look] == '\t')) 
                ++look;
            if (look < n && s[look] == '#')
            {
                size_t lineEnd = tokenizePreprocessor(s, n, look, out);
                // skip to end of that line
                i = lineEnd;
                continue;
            }
        }

        // Comments
        if (c == '/' && i + 1 < n)
        {
            if (s[i+1] == '/')
            {
                size_t j = i + 2;
                while (j < n && s[j] != '\n' && s[j] != '\r')
                    j++;

                // Process Doxygen commands in line comments
                size_t k = i + 2;  // Start after //
                while (k < j)
                {
                    size_t cmdEnd;
                    if (isDoxygenCommand(s, k, j, cmdEnd))
                    {
                        // Add normal comment part before command (if any)
                        if (k > i + 2)
                            addToken(out, i, k, BL_Comment, XMH_STYLE_ITALIC);

                        // Add Doxygen command
                        addToken(out, k, cmdEnd, BL_Annotation, XMH_STYLE_BOLD);
                        k = cmdEnd;
                        i = cmdEnd;  // Update i to continue after command
                    }
                    else
                    {
                        ++k;
                    }
                }
                // Add remaining comment part (if any)
                if (i < j)
                    addToken(out, i, j, BL_Comment, XMH_STYLE_ITALIC);

                i = j;
                continue;
            }
            else if (s[i+1] == '*')
            {
                size_t j = i + 2;
                while (j + 1 < n && !(s[j] == '*' && s[j+1] == '/'))
                    ++j;
                if (j + 1 < n)
                    j += 2;

                // Process Doxygen commands in block comments
                size_t k = i + 2;  // Start after /*
                while (k < j - 2)
                {  // Stop before
                    size_t cmdEnd;
                    if (isDoxygenCommand(s, k, j - 2, cmdEnd))
                    {
                        // Add normal comment part before command (if any)
                        if (k > i + 2)
                            addToken(out, i, k, BL_Comment, XMH_STYLE_ITALIC);

                        // Add Doxygen command
                        addToken(out, k, cmdEnd, BL_Annotation, XMH_STYLE_BOLD);
                        k = cmdEnd;
                        i = cmdEnd;  // Update i to continue after command
                    }
                    else
                    {
                        ++k;
                    }
                }
                // Add remaining comment part (if any)
                if (i < j)
                    addToken(out, i, j, BL_Comment, XMH_STYLE_ITALIC);

                i = j;
                continue;
            }
        }

        // String / char literals (with prefixes)
        if (c == 'R' && i + 1 < n && s[i+1] == '"')
        {
            size_t j = scanRawString(s, n, i);
            addToken(out, i, j, BL_String);
            i = j;
            continue;
        }

        // Possible prefixes: u8, u, U, L, R (handled), and combinations like u8R"..."
        if ((c == 'u' || c == 'U' || c == 'L') && i + 1 < n)
        {
            if (s[i+1] == '8' && i + 2 < n)
            {
                if (s[i+2] == 'R' && i + 3 < n && s[i+3] == '"')
                {
                    size_t j = scanRawString(s, n, i + 2);
                    addToken(out, i, j, BL_String);
                    i = j;
                    continue;
                }
                else if (s[i+2] == '"')
                {
                    size_t j = scanQuoted(s, n, i + 2, '"');
                    addToken(out, i, j, BL_String);
                    i = j;
                    continue;
                }
            }
            else if (s[i+1] == 'R' && i + 2 < n && s[i+2] == '"')
            {
                size_t j = scanRawString(s, n, i + 1);
                addToken(out, i, j, BL_String);
                i = j;
                continue;
            }
            else if (s[i+1] == '"')
            {
                size_t j = scanQuoted(s, n, i + 1, '"');
                addToken(out, i, j, BL_String);
                i = j;
                continue;
            }
            else if (s[i+1] == '\'')
            {
                size_t j = scanCharLiteral(s, n, i + 1);
                addToken(out, i, j, BL_Char);
                i = j;
                continue;
            }
        }

        if (c == '"')
        {
            size_t j = scanQuoted(s, n, i, '"');
            addToken(out, i, j, BL_String);
            i = j;
            continue;
        }

        if (c == '\'')
        {
            size_t j = scanCharLiteral(s, n, i);
            addToken(out, i, j, BL_Char);
            i = j;
            continue;
        }

        // Numbers (including .123)
        if (isdigit(c) || (c == '.' && i + 1 < n && isdigit(static_cast<unsigned char>(s[i+1]))))
        {
            size_t j = scanNumber(s, n, i);
            addToken(out, i, j, BL_Number);
            i = j;
            continue;
        }


        // Delimiters and brackets
        if (c == ',' || c == ';')
        {
            addToken(out, i, i + 1, BL_Delimiter);
            ++i;
            continue;
        }

        if (c == '(' || c == ')' || c == '{' || c == '}' || c == '[' || c == ']')
        {
            addToken(out, i, i + 1, BL_Bracket);
            ++i;
            continue;
        }

        // Operators / punctuation: default
        if (isOperatorChar(c))
        {
            // Handle multi-character operators
            size_t j = i + 1;
            if (i + 1 < n)
            {
                // Check for 2-character operators
                char op2[] = {s[i], s[i+1], '\0'};
                if (strcmp(op2, "==") == 0 || strcmp(op2, "!=") == 0 ||
                    strcmp(op2, "<=") == 0 || strcmp(op2, ">=") == 0 ||
                    strcmp(op2, "&&") == 0 || strcmp(op2, "||") == 0 ||
                    strcmp(op2, "<<") == 0 || strcmp(op2, ">>") == 0 ||
                    strcmp(op2, "++") == 0 || strcmp(op2, "--") == 0 ||
                    strcmp(op2, "+=") == 0 || strcmp(op2, "-=") == 0 ||
                    strcmp(op2, "*=") == 0 || strcmp(op2, "/=") == 0 ||
                    strcmp(op2, "%=") == 0 || strcmp(op2, "&=") == 0 ||
                    strcmp(op2, "|=") == 0 || strcmp(op2, "^=") == 0) 
                {
                    j = i + 2;
                }
                else if (i + 2 < n)  // Check for 3-character operators
                {
                    char op3[] = {s[i], s[i+1], s[i+2], '\0'};
                    if (strcmp(op3, "<<=") == 0 || strcmp(op3, ">>=") == 0) 
                        j = i + 3;
                }
            }
            addToken(out, i, j, BL_Operator);
            i = j;
            continue;
        }

        // Identifier / keyword / type / special cases
        if (isIdentStart(c))
        {
            size_t j = scanIdentifier(s, n, i);
            std::string ident(s + i, s + j);
            if (isKeyword(ident)) 
                addToken(out, i, j, BL_Keyword, XMH_STYLE_BOLD);
            else if (isTypeLike(ident)) 
                addToken(out, i, j, BL_Type);
            else if (isStandardClass(ident)) 
                addToken(out, i, j, BL_Standard_Classes);
            else if (isBoostStuff(ident)) 
                addToken(out, i, j, BL_Boost_Stuff);
            else if (isSpecialVariable(ident)) 
                addToken(out, i, j, BL_Special_Variables);
            
            i = j;
            continue;
        }

        // Skip unrecognized characters
        ++i;
    }
}

// Scan identifier for assembly
static size_t scanAsmIdentifier(const char* s, size_t n, size_t i)
{
    size_t j = i;
    if (j < n && (isIdentStart(static_cast<unsigned char>(s[j])) || s[j] == '%'))
    {
        if (s[j] == '%')
            ++j;  // Skip register prefix

        while (j < n && isIdentContinue(static_cast<unsigned char>(s[j])))
            ++j;
    }
    return j;
}

static bool isRegister(const std::string& s)
{
    static const char* baseRegs[] = {
        "rax","rbx","rcx","rdx","rsi","rdi","rsp","rbp",
        "eax","ebx","ecx","edx","esi","edi","esp","ebp",
        "ax","bx","cx","dx","si","di","sp","bp",
        "al","bl","cl","dl","ah","bh","ch","dh",
        "cs","ds","es","fs","gs","ss",
        "rip","eip","ip"
    };
    for (auto r : baseRegs)
        if (s == r)
            return true;

    // r8..r15 with optional b/w/d suffix
    if (s.size() >= 2 && s[0] == 'r')
    {
        // r8..r15
        size_t pos = 1;
        if (s[pos] >= '8' && s[pos] <= '9')
            ++pos;
        else if (s[pos] == '1' && pos + 1 < s.size() && s[pos+1] >= '0' && s[pos+1] <= '5')
            pos += 2;
        else
            return false;

        // Optional size suffix
        if (pos < s.size())
        {
            char c = s[pos];
            if (c == 'b' || c == 'w' || c == 'd')
                ++pos;
        }
        return pos == s.size();
    }

    // xmm/ymm/zmm0..31
    auto isVec = [&](const char* pfx)
    {
        size_t plen = std::strlen(pfx);
        if (s.rfind(pfx, 0) != 0)
            return false;

        if (s.size() == plen)
            return false;

        size_t i = plen;
        int val = 0;
        while (i < s.size() && std::isdigit((unsigned char)s[i]))
        {
            val = val * 10 + (s[i] - '0');
            ++i;
        }
        return (i == s.size() && val >= 0 && val <= 31);
    };
    if (isVec("xmm") || isVec("ymm") || isVec("zmm"))
        return true;

    return false;
}

void TokenizeGdbDisassembly(const char* text, size_t length, std::vector<XmhColorToken>& out)
{
    if (!text) return;
    const char* s = text;
    size_t n = length ? length : std::strlen(text);

    size_t i = 0;
    while (i < n)
    {
        // Line bounds
        size_t lineStart = i;
        size_t eol = i;
        while (eol < n && s[eol] != '\n' && s[eol] != '\r')
            ++eol;

        size_t p = lineStart;
        while (p < eol && isblank(s[p]))
            ++p;

        // Optional current-instruction marker =>
        if (p + 1 < eol && s[p] == '=' && s[p+1] == '>')
        {
            addToken(out, p, p + 2, BL_Operator);
            p += 2;
            while (p < eol && isblank(s[p])) ++p;
        }

        // Leading address 0x....
        if (p + 2 <= eol && s[p] == '0' && (p + 1 < eol) && (s[p+1] == 'x' || s[p+1] == 'X'))
        {
            size_t q = p + 2;
            while (q < eol && isxdigit((unsigned char)s[q]))
                ++q;
            addToken(out, p, q, BL_Address);
            p = q;
        }

        while (p < eol && isblank(s[p])) ++p;

        // Optional <label+offset> (label only)
        if (p < eol && s[p] == '<')
        {
            size_t q = p + 1;
            while (q < eol && s[q] != '>' && s[q] != '+')
                ++q;
            if (q <= eol)
                addToken(out, p + 1, q, BL_Label);
            while (q < eol && s[q] != '>')
                ++q;
            if (q < eol && s[q] == '>')
                ++q;
            p = q;
        }

        // Expect ':' after the address/label
        while (p < eol && s[p] != ':')
            ++p;

        if (p < eol && s[p] == ':')
            ++p;

        // Spaces then mnemonic
        while (p < eol && isblank(s[p]))
            ++p;

        size_t mStart = p;
        while (p < eol && isalnum((unsigned char)s[p])) ++p;
        if (p > mStart)
            addToken(out, mStart, p, BL_Instruction, XMH_STYLE_BOLD);

        // Operands and trailing comment
        while (p < eol)
        {
            char c = s[p];

            if (c == '#')
            {
                addToken(out, p, eol, BL_Comment, XMH_STYLE_ITALIC);
                break;
            }

            if (c == '%')
            {
                size_t q = scanAsmIdentifier(s, eol, p);
                if (q > p + 1) {
                    std::string reg(s + p + 1, s + q);
                    if (isRegister(reg)) addToken(out, p, q, BL_Register);
                    p = q; continue;
                }
            }

            // Immediates: $0x... or $dec...
            if (c == '$')
            {
                size_t q = p + 1;
                if (q + 1 < eol && s[q] == '0' && (s[q+1] == 'x' || s[q+1] == 'X'))
                {
                    q += 2;
                    while (q < eol && isxdigit((unsigned char)s[q]))
                        ++q;
                }
                else
                {
                    while (q < eol && isdigit((unsigned char)s[q])) ++q;
                }
                addToken(out, p, q, BL_Number);
                p = q; continue;
            }

            // Hex literals in operands: 0x...
            if (c == '0' && p + 1 < eol && (s[p+1] == 'x' || s[p+1] == 'X'))
            {
                size_t q = p + 2;
                while (q < eol && isxdigit((unsigned char)s[q])) ++q;
                addToken(out, p, q, BL_Number);
                p = q; continue;
            }

            ++p;
        }

        // Advance to next line
        i = (eol < n && s[eol] == '\r' && eol + 1 < n && s[eol+1] == '\n') ? eol + 2
        : (eol < n ? eol + 1 : eol);
    }
}

