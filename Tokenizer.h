#pragma once

#include <string.h>

#include "Math.h"
#include <vector>

enum class SyntaxElementType
{
  Keyword,
  Identifier,
  Number,
  String,
  CharLiteral,
  Comment,
  Operator,
  Preprocessor,
  Default
};

struct SyntaxStyle
{
  Vector4 color;
};

struct SyntaxToken
{
  SyntaxElementType type;
  std::string text;
  size_t startPos;
};

std::vector<SyntaxToken>
tokenize(const std::string& text);
