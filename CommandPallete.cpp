#include "CommandPallete.h"

CommandPalette::CommandPalette(BatchRenderer& renderer,
                               int32_t windowWidth,
                               int32_t windowHeight)
  : m_renderer(renderer)
  , m_windowWidth(windowWidth)
  , m_windowHeight(windowHeight)
  , fontSize(20.0f)
  , m_workDir(".")
{
  m_isVisible = false;
  m_selectedIndex = 0;
  m_scrollOffset = 0;
  m_cursorPosition = 0;
  m_mode = CommandPaletteMode::FileList;
  updateFileList();
}

// TODO this is my todo
void
CommandPalette::toggle()
{
  m_isVisible = !m_isVisible;
  if (m_isVisible) {
    updateFileList();
  }
}

// NOTE this is my note
void
CommandPalette::resize(uint32_t pWidth, uint32_t pHeight)
{
  m_windowWidth = pWidth;
  m_windowHeight = pHeight;
}

void
CommandPalette::show()
{
  m_isVisible = true;
  m_inputText.clear();
  m_cursorPosition = 0;
  updateFileList();
}

void
CommandPalette::hide()
{
  m_isVisible = false;
}

bool
CommandPalette::isVisible() const
{
  return m_isVisible;
}

void
CommandPalette::navigateUp()
{
  if (m_selectedIndex > 0) {
    m_selectedIndex--;
    if (m_selectedIndex < m_scrollOffset) {
      m_scrollOffset = m_selectedIndex;
    }
  }
}

void
CommandPalette::navigateDown()
{
  if ((size_t)m_selectedIndex < m_filteredItems.size() - 1) {
    m_selectedIndex++;
    if (m_selectedIndex >= m_scrollOffset + m_maxVisibleItems) {
      m_scrollOffset = m_selectedIndex - m_maxVisibleItems + 1;
    }
  }
}

CommandPalette::Item
CommandPalette::getSelectedItem() const
{
  if (!m_filteredItems.empty() && m_selectedIndex >= 0 &&
      (size_t)m_selectedIndex < m_filteredItems.size()) {
    return m_filteredItems[m_selectedIndex];
  }
  return Item(""); // nothing is selected
}

void
CommandPalette::handleTextInput(const char* text)
{

  if (m_inputText.empty()) {
    if (text[0] == '@') {
      switchMode(CommandPaletteMode::FunctionList);
    } else if (text[0] == '/') {
      switchMode(CommandPaletteMode::SystemCommand);
    } else if (text[0] == '#') {
      switchMode(CommandPaletteMode::CommentList);
    } else {
      switchMode(CommandPaletteMode::FileList);
    }
  }

  m_inputText.insert(m_cursorPosition, text);
  m_cursorPosition += strlen(text);

  filterItems();
}

void
CommandPalette::switchMode(CommandPaletteMode newMode)
{
  if (m_mode != newMode) {
    m_mode = newMode;
    m_selectedIndex = 0;
    m_scrollOffset = 0;

    switch (m_mode) {
      case CommandPaletteMode::FileList:
        updateFileList();
        break;
      case CommandPaletteMode::FunctionList:
        updateFunctionList();
        break;
      case CommandPaletteMode::SystemCommand:
        updateSystemCommandList();
        break;
      case CommandPaletteMode::CommentList:
        updateCommentList();
        break;
    }
  }
}

void
CommandPalette::checkAndUpdateMode()
{
  if (m_inputText.empty()) {
    switchMode(CommandPaletteMode::FileList);
  } else if (m_inputText[0] == '@') {
    switchMode(CommandPaletteMode::FunctionList);
  } else if (m_inputText[0] == '/') {
    switchMode(CommandPaletteMode::SystemCommand);
  } else if (m_inputText[0] == '#') {
    switchMode(CommandPaletteMode::CommentList);
  } else {
    switchMode(CommandPaletteMode::FileList);
  }
}

void
CommandPalette::updateSystemCommandList()
{
  m_items.clear();
  m_items.reserve(6);

  m_items.emplace_back("/q", 0);
  m_items.emplace_back("/n", 1);
  m_items.emplace_back("/w", 2);
  m_items.emplace_back("/r", 3);
  m_items.emplace_back("/fmt", 4);
  m_items.emplace_back("/wdir", 5);

  filterItems();
}

void
CommandPalette::executeSystemCommand(const std::string& command)
{
  if (command.length() <= 1) {
    std::cerr << "Error: Invalid command length" << std::endl;
    return;
  }

  if (command.substr(0, 6) == "/wdir ") {
    std::string dir = command.substr(6);
    dir = std::regex_replace(dir, std::regex("^\\s+|\\s+$"), "");
    if (dir.front() == '"' && dir.back() == '"') {
      dir = dir.substr(1, dir.length() - 2);
    }

    if (dir[0] == '~') {
      const char* home = std::getenv("HOME");
      if (home == nullptr) {
        std::cerr << "Error: Unable to expand '~'. HOME environment variable "
                     "not set.\n";
        return;
      }
      dir.replace(0, 1, home);
    }

    std::filesystem::path path(dir);
    if (path.is_relative()) {
      std::cerr << "Error: Please provide an absolute path.\n";
      return;
    }

    path = path.lexically_normal();
    dir = path.string();

    setWorkDir(dir);
    std::cout << "New Work Dir: " << dir << "\n";
  } else {
    onCommandSelect(command);
  }
}

void
CommandPalette::filterItems()
{
  m_filteredItems.clear();
  std::string filter = m_inputText;
  if ((m_mode == CommandPaletteMode::FunctionList ||
       m_mode == CommandPaletteMode::CommentList) &&
      !m_inputText.empty()) {
    filter = m_inputText.substr(1);
  }

  std::transform(filter.begin(), filter.end(), filter.begin(), ::tolower);

  // special handling for our beloved SystemCommand mode
  if (m_mode == CommandPaletteMode::SystemCommand) {
    if (filter.substr(0, 5) == "/wdir") {
      m_filteredItems.push_back(Item(m_inputText));
    } else {
      for (const auto& item : m_items) {
        std::string lowercaseText = item.displayText;
        std::transform(lowercaseText.begin(),
                       lowercaseText.end(),
                       lowercaseText.begin(),
                       ::tolower);

        if (lowercaseText.find(filter) != std::string::npos) {
          m_filteredItems.push_back(item);
        }
      }
    }
  } else {
    // filtering for other modes
    for (const auto& item : m_items) {
      std::string lowercaseText = item.displayText;
      std::transform(lowercaseText.begin(),
                     lowercaseText.end(),
                     lowercaseText.begin(),
                     ::tolower);

      if (lowercaseText.find(filter) != std::string::npos) {
        m_filteredItems.push_back(item);
      }
    }
  }

  m_selectedIndex = 0;
  m_scrollOffset = 0;
}

void
CommandPalette::handleInput(SDL_Event e)
{
  switch (e.type) {
    case SDL_EVENT_KEY_DOWN: {
      switch (e.key.key) {
        case SDLK_ESCAPE:
          hide();
          break;
        case SDLK_UP:
          navigateUp();
          break;
        case SDLK_DOWN:
          navigateDown();
          break;
        case SDLK_RETURN: {
          if (m_mode == CommandPaletteMode::SystemCommand) {
            if (m_inputText.substr(0, 5) == "/wdir") {
              executeSystemCommand(m_inputText);
            } else if (!m_filteredItems.empty()) {
              executeSystemCommand(
                m_filteredItems[m_selectedIndex].displayText);
            }
          } else if (!m_filteredItems.empty() && onItemSelect) {
            onItemSelect(m_filteredItems[m_selectedIndex]);
          }
          m_mode = CommandPaletteMode::FileList;
          hide();
        } break;
        case SDLK_BACKSPACE:
          if (!m_inputText.empty() && m_cursorPosition > 0) {
            m_inputText.erase(m_cursorPosition - 1, 1);
            m_cursorPosition--;
            checkAndUpdateMode();
            filterItems();
          }
          break;
        case SDLK_LEFT:
          if (m_cursorPosition > 0) {
            m_cursorPosition--;
          }
          break;
        case SDLK_RIGHT:
          if (m_cursorPosition < m_inputText.length()) {
            m_cursorPosition++;
          }
          break;
        case SDLK_HOME:
          m_cursorPosition = 0;
          break;
        case SDLK_END:
          m_cursorPosition = m_inputText.length();
          break;
      }
    } break;

    case SDL_EVENT_TEXT_INPUT: {
      handleTextInput(e.text.text);
    } break;
  }
}

float
CommandPalette::measureTextWidth(const std::string& text) const
{
  float totalWidth = 0;
  float scale =
    stbtt_ScaleForPixelHeight(&m_renderer.fontData.fontInfo, fontSize);

  for (char c : text) {
    int32_t advance, lsb;
    stbtt_GetCodepointHMetrics(
      &m_renderer.fontData.fontInfo, c, &advance, &lsb);
    totalWidth += advance * scale;
  }

  return totalWidth;
}

void
CommandPalette::render()
{
  if (!m_isVisible)
    return;

  // @background
  float paletteWidth = m_windowWidth * 0.6f;
  float paletteHeight = m_windowHeight * 0.6f;
  Vector2 palettePosition = { (m_windowWidth - paletteWidth) * 0.5f,
                              (m_windowHeight - paletteHeight) * 0.5f };
  m_renderer.AddQuad(palettePosition,
                     paletteWidth,
                     paletteHeight,
                     { 0.2f, 0.2f, 0.2f, 0.9f },
                     0.0f,
                     ORIGIN_TOP_LEFT,
                     LAYER_UI);

  // @text-input
  float inputHeight = 30.0f;
  Vector2 inputPosition = { palettePosition.x + 10.0f,
                            palettePosition.y + 10.0f };

  m_renderer.AddQuad(inputPosition,
                     paletteWidth - 20.0f,
                     inputHeight,
                     { 0.3f, 0.3f, 0.3f, 1.0f },
                     0.0f,
                     ORIGIN_TOP_LEFT,
                     LAYER_UI);

  Vector2 inputTextPosition = {
    inputPosition.x + 5.0f,
    inputPosition.y + m_renderer.MeasureText(m_inputText.c_str(), fontSize).y
  };
  m_renderer.DrawText(
    m_inputText.c_str(), inputTextPosition, fontSize, WHITE, LAYER_UI);

  // @cursor
  Vector2 cursorPosition = { inputPosition.x + 5.0f +
                               measureTextWidth(
                                 m_inputText.substr(0, m_cursorPosition)),
                             inputPosition.y + 5.0f };

  m_renderer.AddQuad(cursorPosition,
                     2.0f,
                     inputHeight - 10.0f,
                     WHITE,
                     0.0f,
                     ORIGIN_TOP_LEFT,
                     LAYER_UI);

  float itemHeight = 30.0f;
  m_maxVisibleItems =
    (int32_t)((paletteHeight - inputHeight - 20.0f) / itemHeight);

  for (int32_t i = 0; i < m_maxVisibleItems &&
                      (size_t)(i + m_scrollOffset) < m_filteredItems.size();
       i++) {
    int32_t index = i + m_scrollOffset;
    Vector2 itemPosition = { palettePosition.x + 10.0f,
                             palettePosition.y + inputHeight + i * itemHeight +
                               20.0f };
    if (index == m_selectedIndex) {
      m_renderer.AddQuad({ itemPosition.x, itemPosition.y },
                         paletteWidth - 20.0f,
                         itemHeight,
                         RGBA32(0x218c74),
                         0.0f,
                         ORIGIN_TOP_LEFT,
                         LAYER_UI);
    }

    const auto& item = m_filteredItems[index];
    Vector4 textColor = WHITE;

    if (m_mode == CommandPaletteMode::CommentList) {
      if (item.displayText.substr(0, 4) == "TODO") {
        textColor = ORANGE;
      } else if (item.displayText.substr(0, 4) == "NOTE") {
        textColor = WHITE;
      }
    }

    m_renderer.DrawText(m_filteredItems[index].displayText.c_str(),
                        { itemPosition.x + 5.0f, itemPosition.y + 20.0f },
                        fontSize,
                        textColor,
                        LAYER_UI);
#if 0
    if (m_mode == CommandPaletteMode::CommentList ||
        m_mode == CommandPaletteMode::FunctionList) {
      std::string lineNumber = "L: " + std::to_string(item.visual_line_number);
      m_renderer.DrawText(
        lineNumber.c_str(),
        { itemPosition.x + paletteWidth - 80.0f, itemPosition.y + 20.0f },
        fontSize * 0.8f,
        GREY,
        LAYER_UI);
    }
#endif
  }

  std::string modeText;
  switch (m_mode) {
    case CommandPaletteMode::FileList:
      modeText = "Files";
      break;
    case CommandPaletteMode::FunctionList:
      modeText = "Symbols";
      break;
    case CommandPaletteMode::SystemCommand:
      modeText = "Commands";
      break;
    case CommandPaletteMode::CommentList:
      modeText = "Tasks";
      break;
  }
  modeText += " (" + std::to_string(m_filteredItems.size()) + ")";

  m_renderer.DrawText(
    modeText.c_str(),
    { palettePosition.x + paletteWidth - measureTextWidth(modeText) - 10.0f,
      palettePosition.y - 15.0f },
    fontSize,
    WHITE,
    LAYER_UI);
}

void
CommandPalette::updateFileList()
{
  m_items.clear();
  std::set<std::string> dirsToSkip = {
    ".git", "external", "vendor", "target", "build"
  };

  std::set<std::string> extensionsToSkip = { ".o",   ".a",   ".so", ".png",
                                             ".jpg", ".ttf", ".otf" };

  std::filesystem::path workDirPath(m_workDir);

  for (auto it = std::filesystem::recursive_directory_iterator(workDirPath);
       it != std::filesystem::recursive_directory_iterator();
       ++it) {
    if (it->is_directory()) {
      if (dirsToSkip.find(it->path().filename().string()) != dirsToSkip.end()) {
        it.disable_recursion_pending();
      }
    } else if (it->is_regular_file()) {
      std::string ext = it->path().extension().string();
      if (extensionsToSkip.find(ext) != extensionsToSkip.end()) {
        continue;
      }

      std::filesystem::path relativePath =
        std::filesystem::relative(it->path(), workDirPath);
      std::string displayPath = relativePath.string();

      if (displayPath.length() > 50) {
        displayPath = "..." + displayPath.substr(displayPath.length() - 47);
      }

      m_items.emplace_back(it->path().string()); // add support for displayPath
    }
  }

  std::sort(m_items.begin(), m_items.end(), [](const Item& a, const Item& b) {
    return a.displayText < b.displayText;
  });

  filterItems();
}

void
CommandPalette::setWorkDir(std::string pWorkDir)
{
  m_workDir = pWorkDir;
  updateFileList();
}

void
CommandPalette::updateFunctionList()
{
  m_items.clear();
  std::regex functionRegex(R"(\b(\w+)\s*\([^)]*\)\s*(\{|;))");
  std::sregex_iterator it(
    m_editorText.begin(), m_editorText.end(), functionRegex);
  std::sregex_iterator end;

  size_t lineNumber = 1;
  std::string::const_iterator searchStart(m_editorText.cbegin());

  while (it != end) {
    std::smatch match = *it;
    lineNumber += std::count(searchStart, match[0].first, '\n');
    if (match[1].str() != "if" && match[1].str() != "while" &&
        match[1].str() != "for" && match[1].str() != "switch") {
      m_items.emplace_back(match[1].str(), match.position(), lineNumber);
    }
    searchStart = match[0].second;
    ++it;
  }

  filterItems();
}

void
CommandPalette::updateCommentList()
{
  m_items.clear();
  std::regex todoRegex(R"(//\s*[Tt][Oo][Dd][Oo]\s*:?\s*(.*))",
                       std::regex::icase);
  std::regex noteRegex(R"(//\s*[Nn][Oo][Tt][Ee]\s*:?\s*(.*))",
                       std::regex::icase);

  std::sregex_iterator it(m_editorText.begin(), m_editorText.end(), todoRegex);
  std::sregex_iterator end;

  size_t lineNumber = 1;
  std::string::const_iterator searchStart(m_editorText.cbegin());

  while (it != end) {
    std::smatch match = *it;
    lineNumber += std::count(searchStart, match[0].first, '\n');
    m_items.emplace_back(
      "TODO: " + match[1].str(), match.position(), lineNumber);
    searchStart = match[0].second;
    ++it;
  }

  it =
    std::sregex_iterator(m_editorText.begin(), m_editorText.end(), noteRegex);
  lineNumber = 1;
  searchStart = m_editorText.cbegin();

  while (it != end) {
    std::smatch match = *it;
    lineNumber += std::count(searchStart, match[0].first, '\n');
    m_items.emplace_back(
      "NOTE: " + match[1].str(), match.position(), lineNumber);
    searchStart = match[0].second;
    ++it;
  }

  filterItems();
}