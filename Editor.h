/**
 * $author David Kviloria <david@skystargames.com>
 * $file Editor.h
 */
#pragma once

#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stack>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <unordered_set>

#include "nlohmann/json.hpp"

#include "Math.h"
#include "Tokenizer.h"
#include "backend/2d/Renderer.h"
#include "backend/common.h"

struct WrappedLine
{
  std::string text;
  size_t startPos;
  size_t logicalLineIndex;
  size_t logicalLineStartPos;
};

class SimpleTextEditor
{
private:
  std::string text;
  std::string bufferName;
  std::string bufferExt;
  size_t cursorPosition = 0;
  size_t selectionStart = 0;
  size_t selectionEnd = 0;
  Vector2 position;
  float fontSize;
  Vector4 textColor;
  Vector4 cursorColor;
  Vector4 selectionColor;
  Vector4 lineNumberColor;
  float cursorBlinkTime;
  bool showCursor;
  stbtt_fontinfo* fontInfo;

  Vector2 cursorVisualPosition;
  Vector2 cursorTargetPosition;
  float cursorMoveSpeed = 40.0f;
  int ascent, descent, lineGap;
  float baseline;
  float lineHeight;

  float scrollOffsetY = 0.0f;
  float maxScrollOffsetY = 0.0f;
  float editorHeight;
  float editorWidth;
  float lineNumberWidth = 0.0f;

  bool textChanged = true;
  std::vector<SyntaxToken> tokens;

  nlohmann::json projectConfig;

  std::stack<std::string> undoStack;
  std::stack<std::string> redoStack;
  const size_t MAX_STACK_SIZE = 100;

  std::string currentToken;
  std::unordered_map<std::string, std::string> tagDefinitions;
  
  void generateCtags();
  void parseTagsFile();
  std::string getTokenDeclaration(const std::string& token);
  void updateTokenInfo();
  std::string getHoverInfo(const std::string& token);

  std::string hoverInfo;
  Vector2 hoverPosition;
  bool showHoverInfo;

  std::string getCurrentTokenUnderCursor();
  void displayHoverInfo(const std::string& info, const Vector2& position);
  void renderHoverInfo(BatchRenderer& renderer);
  void hideHoverInfo();

public:
  std::string projectConfigPath;

public:
  SimpleTextEditor(BatchRenderer& renderer,
                   Vector2 pos,
                   float size,
                   Vector4 tColor,
                   Vector4 cColor,
                   Vector4 sColor,
                   Vector4 lnColor);

  void renderBar(BatchRenderer& renderer);

  const std::string& getText() const;

  void handleCommandPaletteSelection(size_t position);

  void resize(uint32_t width, uint32_t height);

  void loadProjectConfig();

  void executeBuildCommand();

  inline bool isSupportedLanguage();

  void formatCodeWithClangFormat();

  // todo (David): move this to utils
  std::string getFileExtension(const std::string& filename);

  void loadTextFromFile(const std::string& filename);

  void handleInput(SDL_Event& event);

  void pushUndoState();

  void undo();

  void redo();

  bool hasSelection() const;

  void copySelectedText();

  void handleMouseWheel(SDL_Event& event);

  void increaseFontSize();

  void decreaseFontSize();

  void insertTab(bool unindent = false);

  void removeTab();

  void toggleComment();

  void jumpToTop();

  void jumpToBottom();

  void jumpToMiddleOfLine();

  void duplicateLine();

  void cutSelectedText();

  void pasteText();

  void deleteSelection();

  void resetSelection();

  void moveCursorLeft(bool shiftPressed);

  void moveCursorRight(bool shiftPressed);

  void moveCursorUp(bool shiftPressed);

  void moveCursorDown(bool shiftPressed);

  void moveCursorToLineStart(bool shiftPressed);

  void moveCursorToLineEnd(bool shiftPressed);

  void updateSelection(bool shiftPressed, size_t oldCursorPosition = 0);

  float measureTextWidth(const std::string& text) const;

  float measureTextHeight() const;

  Vector2 measureText(const std::string& text) const;

  void recalculateFontMetrics();

  void saveBufferToFile();

  void updateCursorTargetPosition();

  void render(BatchRenderer& renderer);

  void update(float deltaTime);

  void autoScrollToCursor();

  std::vector<WrappedLine> wrapText(const std::string& text);

public:
  size_t getLineIndexAtPosition(size_t position,
                                const std::vector<WrappedLine>& lines);

  size_t getLineStartPosition(size_t lineIndex,
                              const std::vector<WrappedLine>& lines);
};
