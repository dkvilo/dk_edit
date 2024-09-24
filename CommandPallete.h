#pragma once

#include "backend/2d/Renderer.h"
#include "pch.h"
#include <functional>

class CommandPalette
{
public:
  CommandPalette(BatchRenderer& renderer,
                 int32_t windowWidth,
                 int32_t windowHeight);
  void toggle();

  void resize(uint32_t pWidth, uint32_t pHeight);

  void show();

  void hide();

  bool isVisible() const;

  void navigateUp();

  void navigateDown();

  std::string getSelectedFile();

  void handleTextInput(const char* text);

  void handleInput(SDL_Event e);

  float measureTextWidth(const std::string& text) const;

  void render();

public:
  std::function<void(const char*)> onFileSelect;

private:
  void updateFileList();

  void filterFiles();

  BatchRenderer& m_renderer;
  uint32_t m_windowWidth;
  uint32_t m_windowHeight;
  bool m_isVisible;
  std::vector<std::string> m_files;
  std::vector<std::string> m_filteredFiles;
  int32_t m_selectedIndex;
  int32_t m_scrollOffset;
  int32_t m_maxVisibleItems;
  std::string m_inputText;
  size_t m_cursorPosition;
  float fontSize;
};
