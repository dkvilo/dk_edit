#include "CommandPallete.h"

CommandPalette::CommandPalette(BatchRenderer& renderer,
                               int32_t windowWidth,
                               int32_t windowHeight)
  : m_renderer(renderer)
  , m_windowWidth(windowWidth)
  , m_windowHeight(windowHeight)
  , fontSize(20.0f)
{
  m_isVisible = false;
  m_selectedIndex = 0;
  m_scrollOffset = 0;
  m_cursorPosition = 0;
  updateFileList();
}

void
CommandPalette::toggle()
{
  m_isVisible = !m_isVisible;
  if (m_isVisible) {
    updateFileList();
  }
}

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
  if (m_selectedIndex < m_filteredFiles.size() - 1) {
    m_selectedIndex++;
    if (m_selectedIndex >= m_scrollOffset + m_maxVisibleItems) {
      m_scrollOffset = m_selectedIndex - m_maxVisibleItems + 1;
    }
  }
}

std::string
CommandPalette::getSelectedFile()
{
  if (m_selectedIndex >= 0 && m_selectedIndex < m_filteredFiles.size()) {
    return m_filteredFiles[m_selectedIndex];
  }
  return "";
}

void
CommandPalette::handleTextInput(const char* text)
{
  m_inputText.insert(m_cursorPosition, text);
  m_cursorPosition += strlen(text);
  filterFiles();
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
          std::string selectedFile = getSelectedFile();
          if (!selectedFile.empty()) {
            if (onFileSelect) {
              onFileSelect(selectedFile.c_str());
              hide();
            }
          }
        } break;
        case SDLK_BACKSPACE:
          if (m_cursorPosition > 0) {
            m_inputText.erase(m_cursorPosition - 1, 1);
            m_cursorPosition--;
            filterFiles();
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
      };
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
  if (m_isVisible) {
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

    for (int32_t i = 0;
         i < m_maxVisibleItems && (i + m_scrollOffset) < m_filteredFiles.size();
         i++) {
      int32_t index = i + m_scrollOffset;
      Vector2 itemPosition = { palettePosition.x + 10.0f,
                               palettePosition.y + inputHeight +
                                 i * itemHeight + 20.0f };
      if (index == m_selectedIndex) {
        m_renderer.AddQuad({ itemPosition.x, itemPosition.y },
                           paletteWidth - 20.0f,
                           itemHeight,
                           RGBA32(0x218c74),
                           0.0f,
                           ORIGIN_TOP_LEFT,
                           LAYER_UI);
      }
      m_renderer.DrawText(m_filteredFiles[index].c_str(),
                          { itemPosition.x + 5.0f, itemPosition.y + 20.0f },
                          fontSize,
                          WHITE,
                          LAYER_UI);
    }
  }
}

void
CommandPalette::updateFileList()
{
  m_files.clear();
  for (const auto& entry : std::filesystem::directory_iterator(".")) {
    m_files.push_back(entry.path().filename().string());
  }
  std::sort(m_files.begin(), m_files.end());
  filterFiles();
}

void
CommandPalette::filterFiles()
{
  m_filteredFiles.clear();
  std::string lowercaseInput = m_inputText;
  std::transform(lowercaseInput.begin(),
                 lowercaseInput.end(),
                 lowercaseInput.begin(),
                 [](uint8_t c) { return std::tolower(c); });

  for (const auto& file : m_files) {
    std::string lowercaseFile = file;
    std::transform(lowercaseFile.begin(),
                   lowercaseFile.end(),
                   lowercaseFile.begin(),
                   [](uint8_t c) { return std::tolower(c); });

    if (lowercaseFile.find(lowercaseInput) != std::string::npos) {
      m_filteredFiles.push_back(file);
    }
  }
  m_selectedIndex = 0;
  m_scrollOffset = 0;
}
