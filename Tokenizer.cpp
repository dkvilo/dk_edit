#include "Tokenizer.h"
#include <map>
#include <unordered_set>

std::map<SyntaxElementType, SyntaxStyle> syntaxStyles = {
  { SyntaxElementType::Keyword, { { 0.93f, 0.79f, 0.22f, 1.0f } } },
  { SyntaxElementType::Identifier, { { 0.78f, 0.78f, 0.78f, 1.0f } } },
  { SyntaxElementType::Number, { { 0.40f, 0.85f, 0.94f, 1.0f } } },
  { SyntaxElementType::String, { { 0.54f, 0.96f, 0.54f, 1.0f } } },
  { SyntaxElementType::CharLiteral, { { 0.54f, 0.96f, 0.54f, 1.0f } } },
  { SyntaxElementType::Comment, { { 0.47f, 0.53f, 0.60f, 1.0f } } },
  { SyntaxElementType::Operator, { { 0.78f, 0.78f, 0.78f, 1.0f } } },
  { SyntaxElementType::Preprocessor, { { 0.40f, 0.85f, 0.94f, 1.0f } } },
  { SyntaxElementType::Default, { { 0.78f, 0.78f, 0.78f, 1.0f } } }
};

inline const std::unordered_set<std::string> cKeywords = {
  "auto",       "break",     "case",           "char",
  "const",      "continue",  "default",        "do",
  "double",     "else",      "enum",           "extern",
  "float",      "for",       "goto",           "if",
  "inline",     "int",       "long",           "register",
  "restrict",   "return",    "short",          "signed",
  "sizeof",     "static",    "struct",         "switch",
  "typedef",    "union",     "unsigned",       "void",
  "volatile",   "while",     "_Alignas",       "_Alignof",
  "_Atomic",    "_Bool",     "_Complex",       "_Generic",
  "_Imaginary", "_Noreturn", "_Static_assert", "_Thread_local",
};

inline const std::unordered_set<char> cOperators = {
  '+', '-', '*', '/', '%', '=', '<', '>', '!', '&', '|', '^',
  '~', '?', ':', '.', ',', ';', '(', ')', '{', '}', '[', ']'
};

std::vector<SyntaxToken>
tokenize(const std::string& text)
{
  std::vector<SyntaxToken> tokens;
  size_t length = text.length();
  size_t pos = 0;

  while (pos < length) {
    char c = text[pos];

    // skip whitespace
    if (isspace(c)) {
      size_t start = pos;
      while (pos < length && isspace(text[pos])) {
        pos++;
      }
      tokens.push_back(
        { SyntaxElementType::Default, text.substr(start, pos - start), start });
      continue;
    }

    // comments
    if (c == '/') {
      if (pos + 1 < length) {
        if (text[pos + 1] == '/') {
          // single-line comment
          size_t start = pos;
          pos += 2;
          while (pos < length && text[pos] != '\n') {
            pos++;
          }
          tokens.push_back({ SyntaxElementType::Comment,
                             text.substr(start, pos - start),
                             start });
          continue;
        } else if (text[pos + 1] == '*') {
          // multi-line comment
          size_t start = pos;
          pos += 2;
          while (pos + 1 < length &&
                 !(text[pos] == '*' && text[pos + 1] == '/')) {
            pos++;
          }
          pos += 2; // skip '*/'
          tokens.push_back({ SyntaxElementType::Comment,
                             text.substr(start, pos - start),
                             start });
          continue;
        }
      }
    }

    // strings
    if (c == '"') {
      size_t start = pos;
      pos++;
      while (pos < length) {
        if (text[pos] == '\\' && pos + 1 < length) {
          pos += 2;
        } else if (text[pos] == '"') {
          pos++;
          break;
        } else {
          pos++;
        }
      }
      tokens.push_back(
        { SyntaxElementType::String, text.substr(start, pos - start), start });
      continue;
    }

    // characters
    if (c == '\'') {
      size_t start = pos;
      pos++;
      while (pos < length) {
        if (text[pos] == '\\' && pos + 1 < length) {
          pos += 2;
        } else if (text[pos] == '\'') {
          pos++;
          break;
        } else {
          pos++;
        }
      }
      tokens.push_back({ SyntaxElementType::CharLiteral,
                         text.substr(start, pos - start),
                         start });
      continue;
    }

    // preprocessor
    if (c == '#' && (pos == 0 || text[pos - 1] == '\n')) {
      size_t start = pos;
      pos++;
      while (pos < length && text[pos] != '\n') {
        pos++;
      }
      tokens.push_back({ SyntaxElementType::Preprocessor,
                         text.substr(start, pos - start),
                         start });
      continue;
    }

    // identifiers and keywords
    if (isalpha(c) || c == '_') {
      size_t start = pos;
      pos++;
      while (pos < length && (isalnum(text[pos]) || text[pos] == '_')) {
        pos++;
      }
      std::string word = text.substr(start, pos - start);
      if (cKeywords.find(word) != cKeywords.end()) {
        tokens.push_back({ SyntaxElementType::Keyword, word, start });
      } else {
        tokens.push_back({ SyntaxElementType::Identifier, word, start });
      }
      continue;
    }

    // number
    if (isdigit(c)) {
      size_t start = pos;
      pos++;
      bool isFloat = false;
      while (pos < length && (isdigit(text[pos]) || text[pos] == '.')) {
        if (text[pos] == '.')
          isFloat = true;
        pos++;
      }
      (void*)isFloat;
      tokens.push_back(
        { SyntaxElementType::Number, text.substr(start, pos - start), start });
      continue;
    }

    if (cOperators.find(c) != cOperators.end()) {
      size_t start = pos;
      pos++;
      if (pos < length && cOperators.find(text[pos]) != cOperators.end()) {
        pos++;
      }
      tokens.push_back({ SyntaxElementType::Operator,
                         text.substr(start, pos - start),
                         start });
      continue;
    }

    // Any other character
    tokens.push_back({ SyntaxElementType::Default, std::string(1, c), pos });
    pos++;
  }
  return tokens;
}