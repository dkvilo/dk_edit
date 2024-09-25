#pragma once

#include "backend/2d/Renderer.h"
#include "pch.h"
#include <functional>
#include <regex>

enum class CommandPaletteMode : size_t
{
  FileList = 0,
  FunctionList,
  SystemCommand,
  CommentList,
};

class CommandPalette
{
public:
  struct Item
  {
    std::string displayText;
    size_t data; // can be 0 for files, or position when listing symbols
    size_t visual_line_number;

    Item(const std::string& text, size_t d = 0, size_t vln = 0)
      : displayText(text)
      , data(d)
      , visual_line_number(vln)
    {
    }
  };

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
  Item getSelectedItem() const;
  void handleTextInput(const char* text);
  void handleInput(SDL_Event e);
  float measureTextWidth(const std::string& text) const;
  void render();
  void setEditorText(const std::string& text) { m_editorText = text; }
  inline CommandPaletteMode getMode() const { return m_mode; }

public:
  std::function<void(const Item&)> onItemSelect;
  std::function<void(const std::string& command)> onCommandSelect;
  void executeSystemCommand(const std::string& command);
  void setWorkDir(std::string pWorkDir);

private:
  void updateFileList();
  void updateFunctionList();
  void filterItems();
  void switchMode(CommandPaletteMode newMode);
  void checkAndUpdateMode();
  void updateSystemCommandList();
  void updateCommentList();

  std::vector<std::string> m_systemCommands;
  BatchRenderer& m_renderer;
  uint32_t m_windowWidth;
  uint32_t m_windowHeight;
  bool m_isVisible;
  std::vector<Item> m_items;
  std::vector<Item> m_filteredItems;
  int32_t m_selectedIndex;
  int32_t m_scrollOffset;
  int32_t m_maxVisibleItems;
  std::string m_inputText;
  size_t m_cursorPosition;
  float fontSize;
  CommandPaletteMode m_mode;
  std::string m_editorText;
  std::string m_workDir;
};