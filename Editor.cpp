#include "Editor.h"
#include "Tokenizer.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
#include <signal.h>
#include <sstream>
#include <stack>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <unordered_set>

extern std::map<SyntaxElementType, SyntaxStyle> syntaxStyles;
extern inline const std::unordered_set<std::string> cKeywords;
extern inline const std::unordered_set<char> cOperators;

constexpr int32_t OFFSET_FROM_BOTTOM = 80;

SimpleTextEditor::SimpleTextEditor(BatchRenderer& renderer,
                                   Vector2 pos,
                                   float size,
                                   Vector4 tColor,
                                   Vector4 cColor,
                                   Vector4 sColor,
                                   Vector4 lnColor)
  : position(pos)
  , fontSize(size)
  , textColor(tColor)
  , cursorColor(cColor)
  , selectionColor(sColor)
  , lineNumberColor(lnColor)
  , cursorPosition(0)
  , selectionStart(0)
  , selectionEnd(0)
  , cursorBlinkTime(0)
  , showCursor(true)
  , cursorVisualPosition(pos)
  , cursorTargetPosition(pos)
{
  fontInfo = &renderer.fontData.fontInfo;
  recalculateFontMetrics();
  lineNumberWidth = measureTextWidth("000") + 20.0f;
  editorWidth = renderer.windowWidth;
  editorHeight = renderer.windowHeight - OFFSET_FROM_BOTTOM;

  text = " ";
  cursorPosition = 0;
  resetSelection();
  updateCursorTargetPosition();

  textChanged = true;
  const std::vector<WrappedLine>& lines = wrapText(text);
  float totalContentHeight = lines.size() * lineHeight;
  maxScrollOffsetY = std::max(0.0f, totalContentHeight - editorHeight);
  scrollOffsetY = 0;
}

// note (David) WIP CTAGS Integration
void
SimpleTextEditor::generateCtags()
{
  std::filesystem::path filePath(bufferName);
  std::string fileName = filePath.filename().string();

  if (!std::filesystem::exists(filePath)) {
    std::cerr << "Error: File does not exist - " << fileName << std::endl;
    return;
  }

  std::string command = "ctags --fields=+l --languages=C,C++ --exclude=.git "
                        "--exclude=build --exclude=external " +
                        fileName;

  int32_t result = system(command.c_str());
  if (result != 0) {
    std::cerr << "Error: Failed to generate ctags, error code: " << result
              << std::endl;
  }
}

void
SimpleTextEditor::parseTagsFile()
{
  std::ifstream tagsFile("tags");
  if (!tagsFile.is_open()) {
    std::cerr << "Error: Unable to open tags file" << std::endl;
    return;
  }

  std::string line;
  while (std::getline(tagsFile, line)) {
    // metadata lines with '!'
    if (line.empty() || line[0] == '!')
      continue;

    std::istringstream iss(line);
    std::string token, file, patternOrLine;

    if (!(iss >> token >> file))
      continue;

    std::getline(iss, patternOrLine, ';');

    std::string pattern;
    std::size_t start = patternOrLine.find('"');
    std::size_t end = patternOrLine.rfind('"');
    if (start != std::string::npos && end != std::string::npos &&
        start != end) {
      pattern = patternOrLine.substr(start + 1, end - start - 1);
    } else {
      pattern = patternOrLine;
    }

    tagDefinitions[token] = pattern;
  }
}

std::string
SimpleTextEditor::getTokenDeclaration(const std::string& token)
{
  auto it = tagDefinitions.find(token);
  if (it != tagDefinitions.end()) {
    return it->second;
  }
  return "";
}

void
SimpleTextEditor::updateTokenInfo()
{
  generateCtags();
  parseTagsFile();
}

std::string
SimpleTextEditor::getHoverInfo(const std::string& token)
{
  std::string declaration = getTokenDeclaration(token);
  if (!declaration.empty()) {
    return declaration;
  }
  return "NOT FOUND";
}

std::string
SimpleTextEditor::getCurrentTokenUnderCursor()
{
  if (cursorPosition >= text.length()) {
    return "";
  }

  size_t start = cursorPosition;
  while (start > 0 &&
         (std::isalnum(text[start - 1]) || text[start - 1] == '_')) {
    start--;
  }

  size_t end = cursorPosition;
  while (end < text.length() && (std::isalnum(text[end]) || text[end] == '_')) {
    end++;
  }

  return text.substr(start, end - start);
}

void
SimpleTextEditor::displayHoverInfo(const std::string& info,
                                   const Vector2& position)
{
  hoverInfo = info;
  hoverPosition = position;
  showHoverInfo = true;
}

// todo (David): wip detailed info on hover
void
SimpleTextEditor::renderHoverInfo(BatchRenderer& renderer)
{
  if (!showHoverInfo || hoverInfo.empty()) {
    return;
  }

  Vector2 textSize = measureText(hoverInfo);
  float padding = 5.0f;
  float boxWidth = textSize.x + 2 * padding;
  float boxHeight = textSize.y + 2 * padding;

  float x = std::max(
    padding, std::min(hoverPosition.x, editorWidth - boxWidth - padding));
  float y = std::max(
    padding, std::min(hoverPosition.y, editorHeight - boxHeight - padding));

  renderer.AddQuad({ x, y },
                   boxWidth,
                   boxHeight,
                   { 0.2f, 0.2f, 0.2f, 0.9f },
                   0.0f,
                   ORIGIN_TOP_LEFT,
                   LAYER_UI + 1);

  renderer.DrawText(hoverInfo.c_str(),
                    { x + padding, y + padding },
                    fontSize,
                    WHITE,
                    LAYER_UI + 2);
}

void
SimpleTextEditor::hideHoverInfo()
{
  showHoverInfo = false;
}

// [/CTAGS]

const std::string&
SimpleTextEditor::getText() const
{
  return text;
}

void
SimpleTextEditor::handleCommandPaletteSelection(size_t position)
{
  cursorPosition = position;
  resetSelection();
  updateCursorTargetPosition();

  const std::vector<WrappedLine>& lines = wrapText(text);
  size_t lineIndex = getLineIndexAtPosition(cursorPosition, lines);
  scrollOffsetY = lineIndex * lineHeight;
  scrollOffsetY = std::max(0.0f, std::min(scrollOffsetY, maxScrollOffsetY));
}

void
SimpleTextEditor::resize(uint32_t width, uint32_t height)
{
  editorWidth = width;
  editorHeight = height - OFFSET_FROM_BOTTOM;
  recalculateFontMetrics();
}

void
SimpleTextEditor::loadProjectConfig()
{
  std::ifstream configFile(projectConfigPath);
  if (configFile.is_open()) {
    try {
      configFile >> projectConfig;
      std::cout << "Project configuration loaded successfully. "
                << projectConfigPath << std::endl;
    } catch (nlohmann::json::parse_error& e) {
      std::cerr << "Error parsing project configuration: " << e.what()
                << std::endl;
    }
    configFile.close();
  } else {
    std::cerr << "Error: Unable to open project configuration file."
              << projectConfigPath << std::endl;
    exit(-1);
  }
}

static std::string build_command_status = "IDLE";
static pid_t build_process_pid = -1;
static std::chrono::steady_clock::time_point build_start_time;

void
handle_sigchld(int sig)
{
  int status;
  pid_t pid;
  while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
    if (pid == build_process_pid) {
      auto build_end_time = std::chrono::steady_clock::now();
      auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        build_end_time - build_start_time);

      std::string time_info =
        " (took " + std::to_string(duration.count()) + "ms)";

      if (WIFEXITED(status)) {
        int exit_status = WEXITSTATUS(status);
        if (exit_status == 0) {
          build_command_status = "COMPLETED" + time_info;
        } else {
          build_command_status =
            "FAILED (Exit code: " + std::to_string(exit_status) + ")" +
            time_info;
        }
      } else if (WIFSIGNALED(status)) {
        int term_sig = WTERMSIG(status);
        build_command_status =
          "FAILED (Terminated by signal: " + std::to_string(term_sig) + ")" +
          time_info;
      }
      build_process_pid = -1;
    }
  }
}

void
SimpleTextEditor::executeBuildCommand()
{
  if (projectConfig.contains("build_command")) {
    std::string buildCommand = projectConfig["build_command"];
    std::cout << "Executing build command: " << buildCommand << std::endl;
    std::filesystem::path configDir =
      std::filesystem::path(projectConfigPath).parent_path();

    struct sigaction sa;
    sa.sa_handler = &handle_sigchld;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sa, 0) == -1) {
      perror("sigaction");
      return;
    }

    build_start_time = std::chrono::steady_clock::now();
    pid_t pid = fork();
    if (pid == 0) {
      if (!configDir.empty()) {
        if (chdir(configDir.c_str()) != 0) {
          std::cerr << "Error: Failed to change directory to " << configDir
                    << std::endl;
          std::exit(1);
        }
      }
      char* args[] = {
        (char*)"/bin/sh", (char*)"-c", (char*)buildCommand.c_str(), nullptr
      };
      execvp(args[0], args);
      std::cerr << "Error: Failed to execute build command" << std::endl;
      std::exit(1);
    } else if (pid > 0) {
      std::cout << "Build process started (PID: " << pid << ")" << std::endl;
      build_command_status = "STARTED (" + std::to_string(pid) + ")";
      build_process_pid = pid;
    } else {
      std::cerr << "Error: Failed to fork process" << std::endl;
      build_command_status = "FAILED (Fork error)";
    }
  } else {
    std::cerr
      << "Error: No build command specified in the project configuration."
      << std::endl;
    build_command_status = "FAILED (No build command)";
  }
}

inline bool
SimpleTextEditor::isSupportedLanguage()
{
  return (bufferExt == "cpp" || bufferExt == "c" || bufferExt == "h" ||
          bufferExt == "hpp");
}

void
SimpleTextEditor::formatCodeWithClangFormat()
{
  // note (David): for now we only care abou C and C++
  if (!isSupportedLanguage()) {
    std::cout << "CLANG_FORMAT: Skipped (Different FileType).\n";
    return;
  }

  std::string tempInputFile = "temp_program_in.c";
  std::ofstream outFile(tempInputFile);
  if (!outFile) {
    std::cerr << "Error: Unable to create temporary input file.\n";
    return;
  }
  outFile << text;
  outFile.close();

  std::string tempOutputFile = "temp_program_out.c";

  std::string clangFormatCommand;

  if (projectConfig.contains("formatter")) {
    if (projectConfig["formatter"].contains("bin")) {
      std::string bin = projectConfig["formatter"]["bin"];
      clangFormatCommand = bin + " " + tempInputFile + " > " + tempOutputFile;
    }

    if (projectConfig["formatter"].contains("style")) {
      std::string style = projectConfig["formatter"]["style"];
      clangFormatCommand += " --style=" + style;
    }

    std::cout << ": " << clangFormatCommand << "\n";
  } else {
    clangFormatCommand = "clang-format " + tempInputFile + " > " +
                         tempOutputFile + " --style=Mozilla";
  }

  int32_t result = std::system(clangFormatCommand.c_str());
  if (result != 0) {
    std::cerr << "Error: Clang-format command failed.\n";
    std::remove(tempInputFile.c_str());
    return;
  }

  std::ifstream inFile(tempOutputFile);
  if (!inFile) {
    std::cerr << "Error: Unable to read formatted output file.\n";
    std::remove(tempInputFile.c_str());
    std::remove(tempOutputFile.c_str());
    return;
  }
  std::stringstream buffer;
  buffer << inFile.rdbuf();
  inFile.close();

  std::string oldText = text;
  pushUndoState();
  text = buffer.str();

  // cursorPosition = 0;
  resetSelection();
  updateCursorTargetPosition();

  std::remove(tempInputFile.c_str());
  std::remove(tempOutputFile.c_str());

  std::cout << "Code formatted successfully.\n";
}

// todo (David): move this to utils
std::string
SimpleTextEditor::getFileExtension(const std::string& filename)
{
  size_t dotPosition = filename.rfind('.');
  if (dotPosition == std::string::npos || dotPosition == 0) {
    return "";
  }

  return filename.substr(dotPosition + 1);
}

void
SimpleTextEditor::loadTextFromFile(const std::string& filename)
{
  bufferExt = getFileExtension(filename);
  std::ifstream file(filename);
  if (file.is_open()) {
    std::stringstream buffer;
    buffer << file.rdbuf();
    text = buffer.str();
    file.close();

    cursorPosition = 0;
    resetSelection();
    updateCursorTargetPosition();

    textChanged = true;
    const std::vector<WrappedLine>& lines = wrapText(text);
    float totalContentHeight = lines.size() * lineHeight;
    maxScrollOffsetY = std::max(0.0f, totalContentHeight - editorHeight);
    scrollOffsetY = 0;

    std::cout << "File loaded successfully: " << filename << std::endl;

    bufferName = filename;

    updateTokenInfo();
  } else {
    std::cerr << "Error: Unable to open file: " << filename << std::endl;
  }
}

void
SimpleTextEditor::handleInput(SDL_Event& event)
{
  bool shiftPressed = (SDL_GetModState() & SDL_KMOD_SHIFT) != 0;
  bool ctrlPressed = (SDL_GetModState() & SDL_KMOD_CTRL) != 0;
#if 0
  if (event.type == SDL_EVENT_MOUSE_MOTION) {
    std::string token = getCurrentTokenUnderCursor();
    if (!token.empty()) {
      std::string info = getHoverInfo(token);
      displayHoverInfo(info,
                       { static_cast<float>(event.motion.x),
                         static_cast<float>(event.motion.y) });
    } else {
      hideHoverInfo();
    }
  }

  else
#endif
  if (event.type == SDL_EVENT_KEY_DOWN) {
    textChanged = true;
    switch (event.key.key) {
      case SDLK_BACKSPACE:
        if (hasSelection()) {
          pushUndoState();
          deleteSelection();
        } else if (cursorPosition > 0) {
          pushUndoState();
          text.erase(cursorPosition - 1, 1);
          cursorPosition--;
        }
        resetSelection();
        break;
      case SDLK_DELETE:
        if (hasSelection()) {
          pushUndoState();
          deleteSelection();
        } else if (cursorPosition < text.length()) {
          pushUndoState();
          text.erase(cursorPosition, 1);
        }
        resetSelection();

        break;
      case SDLK_TAB:
        if (shiftPressed) {
          removeTab();
        } else {
          insertTab();
        }
        break;

      case SDLK_D:
        if (ctrlPressed) {
          duplicateLine();
        }
        break;
      case SDLK_M:
        if (ctrlPressed) {
          jumpToMiddleOfLine();
          break;
        }

      case SDLK_SLASH:
        if (ctrlPressed) {
          toggleComment();
        }
        break;

      case SDLK_LEFT:
        if (ctrlPressed) {
          moveCursorLeft(shiftPressed);
        } else {
          moveCursorLeft(shiftPressed);
        }
        break;
      case SDLK_RIGHT:
        if (ctrlPressed) {
          moveCursorRight(shiftPressed);
        } else {
          moveCursorRight(shiftPressed);
        }
        break;
      case SDLK_UP:
        if (ctrlPressed) {
          scrollOffsetY -= lineHeight;
          if (scrollOffsetY < 0)
            scrollOffsetY = 0;
        } else {
          moveCursorUp(shiftPressed);
        }
        break;
      case SDLK_DOWN:
        if (ctrlPressed) {
          scrollOffsetY += lineHeight;
          if (scrollOffsetY > maxScrollOffsetY)
            scrollOffsetY = maxScrollOffsetY;
        } else {
          moveCursorDown(shiftPressed);
        }
        break;
      case SDLK_HOME:
        if (ctrlPressed) {
          jumpToTop();
        } else {
          moveCursorToLineStart(shiftPressed);
        }
        break;

      case SDLK_END:
        if (ctrlPressed) {
          jumpToBottom();
        } else {
          moveCursorToLineEnd(shiftPressed);
        }
        break;
      case SDLK_RETURN:
      case SDLK_KP_ENTER:
        pushUndoState();
        if (hasSelection()) {
          deleteSelection();
        }

        text.insert(cursorPosition, "\n");
        cursorPosition++;
        resetSelection();
        updateCursorTargetPosition();
        break;
      case SDLK_A:
        if (ctrlPressed) {
          selectionStart = 0;
          selectionEnd = text.length();
          cursorPosition = selectionEnd;
        }
        break;
      case SDLK_C:
        if (ctrlPressed) {
          copySelectedText();
        }
        break;
      case SDLK_X:
        if (ctrlPressed) {
          cutSelectedText();
        }
        break;
      case SDLK_V:
        if (ctrlPressed) {
          pasteText();
        }
        break;
      case SDLK_S:
        if (ctrlPressed) {
          if (projectConfig.contains("format_on_save")) {
            bool enabled = projectConfig["format_on_save"];
            if (enabled) {
              formatCodeWithClangFormat();
            }
          }
          saveBufferToFile();
        }
        break;
      case SDLK_O:
        if (ctrlPressed) {
          if (!bufferName.empty()) {
            loadTextFromFile(bufferName);
          }
        }
        break;

      case SDLK_B:
        if (ctrlPressed) {
          executeBuildCommand();
        }
        break;

      case SDLK_Z:
        if (ctrlPressed && !shiftPressed) {
          undo();
        } else if (ctrlPressed && shiftPressed) {
          redo();
        }
        break;

      case SDLK_Y:
        if (ctrlPressed) {
          redo();
        }
        break;
    }
  } else if (event.type == SDL_EVENT_TEXT_INPUT) {
    pushUndoState();
    if (hasSelection()) {
      deleteSelection();
    }
    text.insert(cursorPosition, event.text.text);
    cursorPosition += SDL_strlen(event.text.text);
    resetSelection();
  }

  else if (event.type == SDL_EVENT_MOUSE_WHEEL) {
    handleMouseWheel(event);
  }

  updateCursorTargetPosition();
}

void
SimpleTextEditor::pushUndoState()
{
  if (undoStack.size() >= MAX_STACK_SIZE) {
    std::stack<std::string> tempStack;
    while (undoStack.size() > 1) {
      tempStack.push(undoStack.top());
      undoStack.pop();
    }
    undoStack.pop();
    while (!tempStack.empty()) {
      undoStack.push(tempStack.top());
      tempStack.pop();
    }
  }
  undoStack.push(text);
  while (!redoStack.empty()) {
    redoStack.pop();
  }
}

void
SimpleTextEditor::undo()
{
  if (!undoStack.empty()) {
    redoStack.push(text);
    text = undoStack.top();
    undoStack.pop();

    cursorPosition = std::min(cursorPosition, text.length() - 1);
    resetSelection();
    updateCursorTargetPosition();
    textChanged = true;
  }
}

void
SimpleTextEditor::redo()
{
  if (!redoStack.empty()) {
    undoStack.push(text);
    text = redoStack.top();
    redoStack.pop();

    cursorPosition = std::min(cursorPosition, text.length() - 1);
    resetSelection();
    updateCursorTargetPosition();
    textChanged = true;
  }
}

bool
SimpleTextEditor::hasSelection() const
{
  return selectionStart != selectionEnd;
}

void
SimpleTextEditor::copySelectedText()
{
  if (hasSelection()) {
    size_t start = std::min(selectionStart, selectionEnd);
    size_t end = std::max(selectionStart, selectionEnd);
    std::string selectedText = text.substr(start, end - start);
    SDL_SetClipboardText(selectedText.c_str());
  }
}

void
SimpleTextEditor::handleMouseWheel(SDL_Event& event)
{
  int wheelDelta = event.wheel.y;

  if (wheelDelta > 0) {
    increaseFontSize();
  } else if (wheelDelta < 0) {
    decreaseFontSize();
  }
}

void
SimpleTextEditor::increaseFontSize()
{
  float maxFontSize = 72.0f;
  fontSize += 2.0f;
  if (fontSize > maxFontSize) {
    fontSize = maxFontSize;
  }

  recalculateFontMetrics();
  updateCursorTargetPosition();
}

void
SimpleTextEditor::decreaseFontSize()
{
  float minFontSize = 23.0f;
  fontSize -= 2.0f;
  if (fontSize < minFontSize) {
    fontSize = minFontSize;
  }

  recalculateFontMetrics();
  updateCursorTargetPosition();
}

void
SimpleTextEditor::insertTab(bool unindent)
{
  const size_t space_size = 2;

  if (selectionStart == selectionEnd) {
    if (!unindent) {
      text.insert(cursorPosition, space_size, ' ');
      cursorPosition += space_size;
    }
  } else {
    // with selection
    size_t start = std::min(selectionStart, selectionEnd);
    size_t end = std::max(selectionStart, selectionEnd);

    const std::vector<WrappedLine>& lines = wrapText(text);
    size_t startLine = getLineIndexAtPosition(start, lines);
    size_t endLine = getLineIndexAtPosition(end, lines);

    size_t offset = 0;
    for (size_t i = startLine; i <= endLine; ++i) {
      size_t lineStart = lines[i].startPos + offset;

      if (unindent) {
        size_t spaces = 0;
        while (spaces < space_size && text[lineStart + spaces] == ' ') {
          spaces++;
        }
        if (spaces > 0) {
          text.erase(lineStart, spaces);
          offset -= spaces;
        }
      } else {
        text.insert(lineStart, space_size, ' ');
        offset += space_size;
      }
    }

    if (selectionStart < selectionEnd) {
      selectionStart += (startLine == endLine) ? space_size : 0;
      selectionEnd += offset;
    } else {
      selectionEnd += (startLine == endLine) ? space_size : 0;
      selectionStart += offset;
    }
    cursorPosition = selectionEnd;
  }

  updateCursorTargetPosition();
}

void
SimpleTextEditor::removeTab()
{
  const size_t space_size = 2;

  if (selectionStart == selectionEnd) {
    size_t lineStart = text.rfind('\n', cursorPosition);
    if (lineStart == std::string::npos)
      lineStart = 0;
    else
      lineStart++;

    size_t spacesToRemove = std::min(cursorPosition - lineStart, space_size);
    size_t actualSpaces = 0;
    for (size_t i = 0; i < spacesToRemove; ++i) {
      if (text[lineStart + i] == ' ')
        actualSpaces++;
      else
        break;
    }

    if (actualSpaces > 0) {
      text.erase(lineStart, actualSpaces);
      cursorPosition -= actualSpaces;
    }
  } else {
    size_t start = std::min(selectionStart, selectionEnd);
    size_t end = std::max(selectionStart, selectionEnd);

    const std::vector<WrappedLine>& lines = wrapText(text);
    size_t startLine = getLineIndexAtPosition(start, lines);
    size_t endLine = getLineIndexAtPosition(end, lines);

    size_t totalRemoved = 0;
    for (size_t i = startLine; i <= endLine; ++i) {
      size_t lineStart = lines[i].startPos - totalRemoved;
      size_t spacesToRemove = 0;

      for (size_t j = 0; j < space_size && lineStart + j < text.length(); ++j) {
        if (text[lineStart + j] == ' ')
          spacesToRemove++;
        else
          break;
      }

      if (spacesToRemove > 0) {
        text.erase(lineStart, spacesToRemove);
        totalRemoved += spacesToRemove;
      }
    }

    if (selectionStart < selectionEnd) {
      selectionEnd -= totalRemoved;
    } else {
      selectionStart -= totalRemoved;
    }
    cursorPosition = std::max(selectionStart, selectionEnd);
  }

  updateCursorTargetPosition();
}

void
SimpleTextEditor::toggleComment()
{
  if (selectionStart == selectionEnd) {
    const std::vector<WrappedLine>& lines = wrapText(text);
    size_t lineIndex = getLineIndexAtPosition(cursorPosition, lines);
    size_t lineStart = lines[lineIndex].startPos;
    const std::string& lineText = lines[lineIndex].text;

    if (lineText.substr(0, 2) == "//") {
      text.erase(lineStart, 2);
      cursorPosition = std::max(cursorPosition - 2, lineStart);
    } else {
      text.insert(lineStart, "//");
      cursorPosition += 2;
    }
  } else {
    size_t start = std::min(selectionStart, selectionEnd);
    size_t end = std::max(selectionStart, selectionEnd);
    std::string selectedText = text.substr(start, end - start);

    if (selectedText.substr(0, 2) == "/*" &&
        selectedText.substr(selectedText.length() - 2) == "*/") {
      text.erase(end - 2, 2);
      text.erase(start, 2);
      selectionEnd -= 4;
    } else {
      text.insert(end, "*/");
      text.insert(start, "/*");
      selectionEnd += 4;
    }

    cursorPosition = selectionEnd;
  }

  updateCursorTargetPosition();
}

void
SimpleTextEditor::jumpToTop()
{
  cursorPosition = 0;
  resetSelection();
  updateCursorTargetPosition();
}

void
SimpleTextEditor::jumpToBottom()
{
  cursorPosition = text.length();
  resetSelection();
  updateCursorTargetPosition();
}

void
SimpleTextEditor::jumpToMiddleOfLine()
{
  const std::vector<WrappedLine>& lines = wrapText(text);
  size_t currentLineIndex = getLineIndexAtPosition(cursorPosition, lines);
  const WrappedLine& currentLine = lines[currentLineIndex];

  size_t lineStart = currentLine.startPos;
  size_t lineLength = currentLine.text.length();

  cursorPosition = lineStart + lineLength / 2;
  resetSelection();
  updateCursorTargetPosition();
}

void
SimpleTextEditor::duplicateLine()
{
  if (!text.size())
    return;
  const std::vector<WrappedLine>& lines = wrapText(text);

  if (selectionStart == selectionEnd) {
    size_t lineIndex = getLineIndexAtPosition(cursorPosition, lines);
    size_t lineStart = lines[lineIndex].startPos;
    size_t lineEnd = (lineIndex < lines.size() - 1)
                       ? lines[lineIndex + 1].startPos
                       : text.length();

    std::string lineToDuplicate = text.substr(lineStart, lineEnd - lineStart);

    text.insert(lineEnd, lineToDuplicate);

    cursorPosition = lineEnd + (cursorPosition - lineStart);
  } else {
    size_t start = std::min(selectionStart, selectionEnd);
    size_t end = std::max(selectionStart, selectionEnd);

    size_t startLine = getLineIndexAtPosition(start, lines);
    size_t endLine = getLineIndexAtPosition(end, lines);

    size_t selectionStart = lines[startLine].startPos;
    size_t selectionEnd = (endLine < lines.size() - 1)
                            ? lines[endLine + 1].startPos
                            : text.length();

    std::string textToDuplicate =
      text.substr(selectionStart, selectionEnd - selectionStart);

    text.insert(selectionEnd, textToDuplicate);

    size_t insertedLength = selectionEnd - selectionStart;
    selectionStart = selectionEnd;
    selectionEnd = selectionStart + insertedLength;
    cursorPosition = selectionEnd;
  }

  updateCursorTargetPosition();
}

void
SimpleTextEditor::cutSelectedText()
{
  if (hasSelection()) {
    pushUndoState();
    copySelectedText();
    deleteSelection();
    updateCursorTargetPosition();
  }
}

void
SimpleTextEditor::pasteText()
{
  if (SDL_HasClipboardText()) {
    char* clipboardText = SDL_GetClipboardText();
    if (clipboardText) {
      pushUndoState();
      if (hasSelection()) {
        deleteSelection();
      }
      text.insert(cursorPosition, clipboardText);
      cursorPosition += strlen(clipboardText);
      resetSelection();
      updateCursorTargetPosition();
      SDL_free(clipboardText);
    }
  }
}

void
SimpleTextEditor::deleteSelection()
{
  size_t start = std::min(selectionStart, selectionEnd);
  size_t end = std::max(selectionStart, selectionEnd);
  text.erase(start, end - start);
  cursorPosition = start;
  resetSelection();
}

void
SimpleTextEditor::resetSelection()
{
  selectionStart = cursorPosition;
  selectionEnd = cursorPosition;
}

void
SimpleTextEditor::moveCursorLeft(bool shiftPressed)
{
  size_t oldCursorPosition = cursorPosition;
  if (cursorPosition > 0) {
    cursorPosition--;
  }
  updateSelection(shiftPressed, oldCursorPosition);
}

void
SimpleTextEditor::moveCursorRight(bool shiftPressed)
{
  size_t oldCursorPosition = cursorPosition;
  if (cursorPosition < text.length()) {
    cursorPosition++;
  }
  updateSelection(shiftPressed, oldCursorPosition);
}

void
SimpleTextEditor::moveCursorUp(bool shiftPressed)
{
  size_t oldCursorPosition = cursorPosition;
  const std::vector<WrappedLine>& lines = wrapText(text);
  size_t lineIndex = getLineIndexAtPosition(cursorPosition, lines);
  if (lineIndex == 0)
    return;

  size_t prevLineStartPos = getLineStartPosition(lineIndex - 1, lines);
  size_t prevLineLength = lines[lineIndex - 1].text.length();

  size_t cursorInLine = cursorPosition - getLineStartPosition(lineIndex, lines);
  cursorPosition = prevLineStartPos + std::min(prevLineLength, cursorInLine);

  updateSelection(shiftPressed, oldCursorPosition);
}

void
SimpleTextEditor::moveCursorDown(bool shiftPressed)
{
  size_t oldCursorPosition = cursorPosition;
  const std::vector<WrappedLine>& lines = wrapText(text);
  size_t lineIndex = getLineIndexAtPosition(cursorPosition, lines);
  if (lineIndex >= lines.size() - 1)
    return;

  size_t nextLineStartPos = getLineStartPosition(lineIndex + 1, lines);
  size_t nextLineLength = lines[lineIndex + 1].text.length();

  size_t cursorInLine = cursorPosition - getLineStartPosition(lineIndex, lines);
  cursorPosition = nextLineStartPos + std::min(nextLineLength, cursorInLine);

  updateSelection(shiftPressed, oldCursorPosition);
}

void
SimpleTextEditor::moveCursorToLineStart(bool shiftPressed)
{
  size_t oldCursorPosition = cursorPosition;
  const std::vector<WrappedLine>& lines = wrapText(text);
  size_t lineIndex = getLineIndexAtPosition(cursorPosition, lines);
  cursorPosition = getLineStartPosition(lineIndex, lines);

  updateSelection(shiftPressed, oldCursorPosition);
}

void
SimpleTextEditor::moveCursorToLineEnd(bool shiftPressed)
{
  size_t oldCursorPosition = cursorPosition;
  const std::vector<WrappedLine>& lines = wrapText(text);
  size_t lineIndex = getLineIndexAtPosition(cursorPosition, lines);
  cursorPosition =
    getLineStartPosition(lineIndex, lines) + lines[lineIndex].text.length();

  updateSelection(shiftPressed, oldCursorPosition);
}

void
SimpleTextEditor::updateSelection(bool shiftPressed, size_t oldCursorPosition)
{
  if (shiftPressed) {
    if (!hasSelection())
      selectionStart = oldCursorPosition;
    selectionEnd = cursorPosition;
  } else {
    resetSelection();
  }
}

float
SimpleTextEditor::measureTextWidth(const std::string& text) const
{
  float totalWidth = 0;
  float scale = stbtt_ScaleForPixelHeight(fontInfo, fontSize);

  for (char c : text) {
    int advance, lsb;
    stbtt_GetCodepointHMetrics(fontInfo, c, &advance, &lsb);
    totalWidth += advance * scale;
  }

  return totalWidth;
}

float
SimpleTextEditor::measureTextHeight() const
{
  float scale = stbtt_ScaleForPixelHeight(fontInfo, fontSize);
  int ascent, descent, lineGap;
  stbtt_GetFontVMetrics(fontInfo, &ascent, &descent, &lineGap);

  float height = (ascent - descent + lineGap) * scale;
  return height;
}

Vector2
SimpleTextEditor::measureText(const std::string& text) const
{
  return { measureTextWidth(text), measureTextHeight() };
}

void
SimpleTextEditor::recalculateFontMetrics()
{
  float scale = stbtt_ScaleForPixelHeight(fontInfo, fontSize);
  stbtt_GetFontVMetrics(fontInfo, &ascent, &descent, &lineGap);
  baseline = ascent * scale;
  lineHeight = (ascent - descent + lineGap) * scale;
  lineNumberWidth = measureTextWidth("000") + 20.0f;
}

void
SimpleTextEditor::saveBufferToFile()
{
  std::ofstream outFile(bufferName);
  if (outFile.is_open()) {
    outFile << text;
    outFile.close();
    std::cout << "File saved successfully.\n";
  } else {
    std::cerr << "Error: Unable to open file for writing.\n";
  }
}

void
SimpleTextEditor::updateCursorTargetPosition()
{
  const std::vector<WrappedLine>& lines = wrapText(text);
  float y = position.y;

  for (size_t i = 0; i < lines.size(); ++i) {
    const WrappedLine& line = lines[i];
    Vector2 linePosition = { position.x + lineNumberWidth, y };

    size_t lineStartPos = line.startPos;
    size_t lineEndPos = lineStartPos + line.text.length();

    if (cursorPosition >= lineStartPos && cursorPosition <= lineEndPos) {
      size_t cursorIndexInLine = cursorPosition - lineStartPos;
      float cursorX = linePosition.x +
                      measureTextWidth(line.text.substr(0, cursorIndexInLine));
      cursorTargetPosition = { cursorX, y + (fontSize - baseline) };
      return;
    }

    y += lineHeight;
  }

  cursorTargetPosition = { position.x + lineNumberWidth,
                           y + (fontSize - baseline) };
}

void
SimpleTextEditor::render(BatchRenderer& renderer)
{
  const std::vector<WrappedLine>& lines = wrapText(text);
  float y = position.y - scrollOffsetY;
  if (textChanged) {
    tokens = tokenize(text);
    textChanged = false;
  }

  size_t tokenIndex = 0;

  for (size_t i = 0; i < lines.size(); ++i) {
    const WrappedLine& line = lines[i];
    Vector2 lineNumberPosition = { position.x, y };
    Vector2 linePosition = { position.x + lineNumberWidth, y };
    if (y + lineHeight + 30 < position.y) {
      y += lineHeight;
      continue;
    } else if (y > position.y + editorHeight) {
      break;
    }

    char lineNumberText[16];
    sprintf(lineNumberText, "%3zu", i + 1);
    renderer.DrawText(
      lineNumberText, lineNumberPosition, fontSize, lineNumberColor, LAYER_UI);

    if (hasSelection()) {
      size_t lineStartPos = line.startPos;
      size_t lineEndPos = lineStartPos + line.text.length();

      size_t selStart = selectionStart;
      size_t selEnd = selectionEnd;

      if (selStart > selEnd) {
        std::swap(selStart, selEnd);
      }

      if (selEnd > lineStartPos && selStart < lineEndPos) {
        size_t selectionStartInLine =
          std::max(selStart, lineStartPos) - lineStartPos;
        size_t selectionEndInLine = std::min(selEnd, lineEndPos) - lineStartPos;

        float selectionXStart =
          linePosition.x +
          measureTextWidth(line.text.substr(0, selectionStartInLine));
        float selectionXEnd =
          linePosition.x +
          measureTextWidth(line.text.substr(0, selectionEndInLine));

        if (selectionXEnd < position.x ||
            selectionXStart > position.x + editorWidth) {
          goto RenderText;
        }

        selectionXStart = std::max(selectionXStart, position.x);
        selectionXEnd = std::min(selectionXEnd, position.x + editorWidth);

        float underlineThickness = 2.0f;
        Vector2 underlineStart = {
          selectionXStart, y + (fontSize - baseline) + lineHeight * 0.05f
        };
        float selectionWidth = selectionXEnd - selectionXStart;
        renderer.AddQuad(underlineStart,
                         selectionWidth,
                         underlineThickness,
                         cursorColor,
                         0.0f,
                         ORIGIN_TOP_LEFT,
                         LAYER_UI);

        renderer.AddQuad(
          { underlineStart.x, underlineStart.y - lineHeight },
          selectionWidth,
          lineHeight,
          { selectionColor.x, selectionColor.y, selectionColor.z, 0.2f },
          0.0f,
          ORIGIN_TOP_LEFT,
          LAYER_UI);
      }
    }

  RenderText:
    float textEndX = linePosition.x + measureTextWidth(line.text);
    if (textEndX < position.x || linePosition.x > position.x + editorWidth) {
      y += lineHeight;
      continue;
    }

    size_t lineStartPos = line.startPos;
    size_t lineEndPos = lineStartPos + line.text.length();

    float x = linePosition.x;
    while (tokenIndex < tokens.size()) {
      const SyntaxToken& token = tokens[tokenIndex];

      size_t tokenStartPos = token.startPos;
      size_t tokenEndPos = token.startPos + token.text.length();
      if (tokenEndPos <= lineStartPos) {
        tokenIndex++;
        continue;
      }

      if (tokenStartPos >= lineEndPos) {
        break;
      }

      size_t overlapStart = std::max(tokenStartPos, lineStartPos);
      size_t overlapEnd = std::min(tokenEndPos, lineEndPos);
      size_t overlapLength = overlapEnd - overlapStart;

      if (overlapLength > 0) {
        size_t substringStart = overlapStart - tokenStartPos;
        size_t substringLength = overlapLength;

        std::string tokenSubstring =
          token.text.substr(substringStart, substringLength);
        float tokenWidth = measureTextWidth(tokenSubstring);
        Vector4 color = syntaxStyles[token.type].color;

        renderer.DrawText(
          tokenSubstring.c_str(), { x, y }, fontSize, color, LAYER_UI);

        x += tokenWidth;
      }

      if (tokenEndPos <= lineEndPos) {
        tokenIndex++;
      } else {
        break;
      }
    }

    if (hasSelection() && selectionStart != cursorPosition) {
      size_t selPos = selectionStart;
      size_t lineStartPos = line.startPos;
      size_t lineEndPos = lineStartPos + line.text.length();

      if (selPos >= lineStartPos && selPos <= lineEndPos) {
        size_t selIndexInLine = selPos - lineStartPos;
        float selCursorX =
          linePosition.x +
          measureTextWidth(line.text.substr(0, selIndexInLine));
        Vector2 selCursorPos = { selCursorX, y + (fontSize - baseline) };

        if (selCursorPos.x >= position.x &&
            selCursorPos.x <= position.x + editorWidth) {
          renderer.AddQuad(selCursorPos,
                           4.0,
                           lineHeight,
                           cursorColor,
                           0.0f,
                           ORIGIN_BOTTOM_RIGHT,
                           LAYER_UI);
        }
      }
    }

    y += lineHeight;
  }

  if (showCursor) {

    float cursorRenderY = cursorVisualPosition.y - scrollOffsetY;
    if (cursorRenderY >= position.y &&
        cursorRenderY <= position.y + editorHeight) {
      renderer.AddQuad({ cursorVisualPosition.x + 2, cursorRenderY },
                       4.0f,
                       lineHeight,
                       cursorColor,
                       0.0f,
                       ORIGIN_BOTTOM_RIGHT,
                       LAYER_UI);
    }
  }
}

void
SimpleTextEditor::renderBar(BatchRenderer& renderer)
{
  Vector4 bgColor = GREY;
  if (build_command_status.find("STARTED") != std::string::npos) {
    bgColor = ORANGE;
  } else if (build_command_status.find("FAILED") != std::string::npos) {
    bgColor = RED;
  } else if (build_command_status.find("FAILED") != std::string::npos ||
             build_command_status == "IDLE") {
    bgColor = GREY;
  }

  renderer.AddQuad({ 10.0f, editorHeight + 50.0f },
                   editorWidth - 20.0f,
                   20.0f,
                   bgColor,
                   0.0f,
                   ORIGIN_TOP_LEFT,
                   LAYER_UI);

  std::string tokenInfo = "NOT FOUND";
  std::string currentToken = getCurrentTokenUnderCursor();
  if (!currentToken.empty()) {
    std::string info = getHoverInfo(currentToken);
    tokenInfo = info;
  }

  char buffer[255];
  const char* name = bufferName.empty() ? "Untitled" : bufferName.c_str();
  sprintf(buffer,
          "Buffer: %s | Build: %s | CTAGS: %s",
          name,
          build_command_status.c_str(),
          tokenInfo.c_str());

  renderer.DrawText(
    buffer, { 20.0f, editorHeight + (50.0f + 15.0f) }, 20.0f, WHITE, LAYER_UI);
}

void
SimpleTextEditor::update(float deltaTime)
{
  cursorBlinkTime += deltaTime;
  if (cursorBlinkTime >= 0.1f) {
    showCursor = !showCursor;
    cursorBlinkTime = 0;
  }

  cursorVisualPosition = vector2_lerp(
    cursorVisualPosition, cursorTargetPosition, deltaTime * cursorMoveSpeed);

  if (vector2_distance(cursorVisualPosition, cursorTargetPosition) < 0.5f) {
    cursorVisualPosition = cursorTargetPosition;
  }

  updateCursorTargetPosition();
  autoScrollToCursor();

  if (scrollOffsetY < 0)
    scrollOffsetY = 0;
  if (scrollOffsetY > maxScrollOffsetY)
    scrollOffsetY = maxScrollOffsetY;
}

void
SimpleTextEditor::autoScrollToCursor()
{
  float cursorY = cursorTargetPosition.y - position.y;
  float viewportTop = scrollOffsetY;
  float viewportBottom = scrollOffsetY + editorHeight;
  float margin = lineHeight;
  if (cursorY < viewportTop + margin) {
    scrollOffsetY = cursorY - margin;
  }

  else if (cursorY + lineHeight > viewportBottom - margin) {
    scrollOffsetY = cursorY + lineHeight - editorHeight + margin;
  }

  scrollOffsetY = std::max(0.0f, std::min(scrollOffsetY, maxScrollOffsetY));
}

std::vector<WrappedLine>
SimpleTextEditor::wrapText(const std::string& text)
{
  std::vector<WrappedLine> wrappedLines;
  wrappedLines.reserve(1024 << 1);

  float availableWidth = editorWidth - lineNumberWidth - 20.0f;

  size_t textPos = 0;
  size_t length = text.length();

  size_t logicalLineIndex = 0;
  size_t logicalLineStartPos = 0;

  while (textPos < length) {
    if (text[textPos] == '\n') {
      WrappedLine line;
      line.text = "";
      line.startPos = textPos;
      line.logicalLineIndex = logicalLineIndex;
      line.logicalLineStartPos = logicalLineStartPos;
      wrappedLines.emplace_back(line);

      textPos++;

      logicalLineIndex++;
      logicalLineStartPos = textPos;
      continue;
    }

    size_t lineStartPos = textPos;
    float width = 0.0f;
    size_t lineLength = 0;

    while (textPos < length && text[textPos] != '\n') {
      char c = text[textPos];
      float charWidth = measureTextWidth(std::string(1, c));
      if (width + charWidth > availableWidth && lineLength > 0) {
        break;
      }
      width += charWidth;
      textPos++;
      lineLength++;
    }

    WrappedLine line;
    line.text = text.substr(lineStartPos, lineLength);
    line.startPos = lineStartPos;
    line.logicalLineIndex = logicalLineIndex;
    line.logicalLineStartPos = logicalLineStartPos;
    wrappedLines.emplace_back(line);

    if (textPos < length && text[textPos] == '\n') {
      textPos++;
      logicalLineIndex++;
      logicalLineStartPos = textPos;
    }
  }

  float totalContentHeight = wrappedLines.size() * lineHeight;
  maxScrollOffsetY = std::max(0.0f, totalContentHeight - editorHeight);

  return wrappedLines;
}

size_t
SimpleTextEditor::getLineIndexAtPosition(size_t position,
                                         const std::vector<WrappedLine>& lines)
{
  for (size_t i = 0; i < lines.size(); ++i) {
    size_t lineStartPos = lines[i].startPos;
    size_t lineEndPos = lineStartPos + lines[i].text.length();
    if (position >= lineStartPos && position <= lineEndPos) {
      return i;
    }
  }
  return lines.size() - 1;
}

size_t
SimpleTextEditor::getLineStartPosition(size_t lineIndex,
                                       const std::vector<WrappedLine>& lines)
{
  return lines[lineIndex].startPos;
}
