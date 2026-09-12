#include "tutorial.h"
#include <imgui.h>
#include <imgui_stdlib.h>
#include <imgui_markdown/imgui_markdown.h>
#include <fstream>
#include <vector>
#include <pugixml.hpp>
#include <iostream>

namespace {
  std::vector<std::pair<std::string, std::string>> tutorial_text;

std::vector<std::pair<std::string, std::string>> LoadTutorial() {
  pugi::xml_document doc;
  pugi::xml_parse_result result = doc.load_file("tutorial.xml");
  if (!result) {
    std::cerr << "Failed to load tutorial: " << result.description() << std::endl;
    return {};
  }

  pugi::xml_node root = doc.child("tutorial");
  for (pugi::xml_node step : root.children("step")) {
    std::string content;
    for (pugi::xml_node child : step.children()) {
      if (child.type() == pugi::node_cdata || child.type() == pugi::node_pcdata) {
        content += child.value();
      }
    }
    tutorial_text.emplace_back(step.attribute("label").as_string(), content);
  }
  return tutorial_text;
}

void SaveTutorial() {
  pugi::xml_document doc;
  pugi::xml_node root = doc.append_child("tutorial");
  for (const auto& [name, text] : tutorial_text) {
    pugi::xml_node step = root.append_child("step");
    step.append_attribute("label") = name.c_str();
    pugi::xml_node text_node = step.append_child(pugi::node_cdata);
    text_node.set_value(text.c_str());
  }
  doc.save_file("tutorial.xml");
}

bool show_editor = false;

}


void TutorialWindow(bool& show, Scene& scene) {
  if (!show) {
    return;
  }
  if (tutorial_text.empty()) {
    LoadTutorial();
  }
  ImGui::Begin("Tutorial");
  #ifndef NDEBUG
  if (ImGui::Button("Reload")) {
    tutorial_text.clear();
    LoadTutorial();
  }
  ImGui::SameLine();
  if (ImGui::Button("Edit")) {
    show_editor = !show_editor;
  }
  ImGui::SameLine();
  if (ImGui::Button("Save")) {
    SaveTutorial();
  }
  ImGui::Separator();
#endif
  ImGui::MarkdownConfig md_config;
  if (ImGui::BeginTabBar("TutorialTabs")) {
    for (const auto& [tab, line] : tutorial_text) {
      if (ImGui::BeginTabItem(tab.c_str())) {
        ImGui::Markdown(line.c_str(), line.size(), md_config);
        ImGui::EndTabItem();
      }
    }

    if (ImGui::BeginTabItem("Finish")) {
      ImGui::TextWrapped("You have finished the tutorial! Click the button below to finish the tutorial.");
      if (ImGui::Button("Finish Tutorial")) {
        // make a file called tutorial.txt
        std::ofstream tutorial_file("tutorial.txt");
        if (tutorial_file.is_open()) {
          tutorial_file << "Congratulations! You have completed the tutorial.\n";
          tutorial_file.close();
        }
        show = false;  // close the tutorial window
      }
      ImGui::TextWrapped("Or, try a test scene! Click the button below to load it.");
      if (ImGui::Button("Load Test Scene")) {
        scene = LoadScene("tutorial_scene.xml");
      }
      ImGui::TextWrapped("You can see that the textures aren't loaded correct."
          "Try to fix that issue by loading the textures from the assets/ folder."
          "texture.color should load assets/color.png, and texture.normal should load assets/normal.png"
          "Try it out! If you get stuck, check the documentation for how to load textures.");
      ImGui::EndTabItem();
    }
    
    ImGui::EndTabBar();
  }
  ImGui::End();

  if (show_editor) {
    std::vector<std::string> tabs_to_delete;
    ImGui::Begin("Tutorial Editor", &show_editor);
    int i = 0;
    for (auto& [tab, line] : tutorial_text) {
      ImGui::PushID(i++);
      ImGui::InputText("Tab Name", &tab);
      ImGui::SameLine();
      if (ImGui::Button(("Delete##" + tab).c_str())) {
        tabs_to_delete.push_back(tab);
      }
      ImGui::InputTextMultiline("Contents", &line);
      ImGui::Separator();
      ImGui::PopID();
    }
    if (ImGui::Button("Add New Tab")) {
      tutorial_text.emplace_back("New Tab", "New Content");
    }
    for (const auto& tab : tabs_to_delete) {
      tutorial_text.erase(std::remove_if(tutorial_text.begin(), tutorial_text.end(),
        [&tab](const auto& pair) { return pair.first == tab; }), tutorial_text.end());
    }
    ImGui::End();
  }
}
