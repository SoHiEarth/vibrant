#include "docs.h"
#include <imgui.h>
#include <imgui_stdlib.h>
#include <imgui_markdown/imgui_markdown.h>
#include <pugixml.hpp>
#include <string>
#include <iostream>
#include <vector>

namespace {
  std::vector<std::pair<std::string, std::string>> doc_text;

std::vector<std::pair<std::string, std::string>> LoadDocumentation() {
  pugi::xml_document doc;
  pugi::xml_parse_result result = doc.load_file("documentation.xml");
  if (!result) {
    std::cerr << "Failed to load documentation: " << result.description() << std::endl;
    return {};
  }

  pugi::xml_node root = doc.child("documentation");
  for (pugi::xml_node section : root.children("tab")) {
    std::string content;
    for (pugi::xml_node child : section.children()) {
      if (child.type() == pugi::node_cdata || child.type() == pugi::node_pcdata) {
        content += child.value();
      }
    }
    doc_text.emplace_back(section.attribute("name").as_string(), content);
  }
  return doc_text;
}

void SaveDocumentation() {
  pugi::xml_document doc;
  pugi::xml_node root = doc.append_child("documentation");
  for (const auto& [name, text] : doc_text) {
    pugi::xml_node section = root.append_child("tab");
    section.append_attribute("name") = name.c_str();
    pugi::xml_node text_node = section.append_child(pugi::node_cdata);
    text_node.set_value(text.c_str());
  }
  doc.save_file("documentation.xml");
}

bool show_editor = false;

}

void DocumentationWindow() {
  if (doc_text.empty()) {
    LoadDocumentation();
  }
  ImGui::Begin("Documentation");
#ifndef NDEBUG
  if (ImGui::Button("Reload")) {
    doc_text.clear();
    LoadDocumentation();
  }
  ImGui::SameLine();
  if (ImGui::Button("Edit")) {
    show_editor = !show_editor;
  }
  ImGui::SameLine();
  if (ImGui::Button("Save")) {
    SaveDocumentation();
  }
  ImGui::Separator();
#endif
  ImGui::MarkdownConfig md_config;
  if (ImGui::BeginTabBar("DocumentationTabs")) {
    for (const auto& [tab, line] : doc_text) {
      if (ImGui::BeginTabItem(tab.c_str())) {
        ImGui::Markdown(line.c_str(), line.size(), md_config);
        ImGui::EndTabItem();
      }
    }
    ImGui::EndTabBar();
  }
  ImGui::End();

  if (show_editor) {
    std::vector<std::string> tabs_to_delete;
    ImGui::Begin("Documentation Editor", &show_editor);
    int i = 0;
    for (auto& [tab, line] : doc_text) {
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
      doc_text.emplace_back("New Tab", "New Content");
    }
    for (const auto& tab : tabs_to_delete) {
      doc_text.erase(std::remove_if(doc_text.begin(), doc_text.end(),
        [&tab](const auto& pair) { return pair.first == tab; }), doc_text.end());
    }
    ImGui::End();
  }
}
