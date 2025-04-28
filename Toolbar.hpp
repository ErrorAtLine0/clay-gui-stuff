#pragma once
#include "GUIStuff/GUIManager.hpp"
#include "DrawData.hpp"
#include <filesystem>
#include <nlohmann/json.hpp>

class MainProgram;

class Toolbar {
    public:
        Toolbar(MainProgram& initMain);
        void update();
        void draw(SkCanvas* canvas);
        void color_selector_left(Vector4f* color);
        void color_selector_right(Vector4f* color);
        void open_file_selector(const std::string& filePickerName, const std::vector<std::string>& extensionFilters, const std::function<void(const std::filesystem::path&, const std::string& extensionSelected)>& postSelectionFunc);
        void save_func();
        void save_as_func();
        std::filesystem::path& file_selector_path();
        std::shared_ptr<GUIStuff::UpdateInputData> io;
        GUIStuff::GUIManager gui;

        nlohmann::json get_appearance_json();
        void set_appearance_json(const nlohmann::json& j);

        Vector4f* colorLeft = nullptr;
        Vector4f* colorRight = nullptr;
    private:
        void top_toolbar();
        void drawing_program_gui();
        void options_menu();
        void file_picker_gui();

        bool justAssignedColorLeft = false;
        bool justAssignedColorRight = false;

        float guiScale = 1.0f;
        bool darkMode = true;

        bool newDarkMode = true;

        bool menuPopUpOpen = false;
        bool optionsMenuOpen = false;
        enum OptionsMenuType {
            CONNECT_IP_MENU = 0,
            GENERAL_SETTINGS_MENU = 1
        } optionsMenuType;
        enum GeneralSettingsOptions {
            GSETTINGS_APPEARANCE = 0,
            GSETTINGS_KEYBINDS = 1
        } generalSettingsOptions = GSETTINGS_APPEARANCE;

        struct FilePicker {
            bool isOpen = false;
            std::string filePickerWindowName;
            std::vector<std::string> extensionFilters;
            std::vector<std::filesystem::path> entries;
            bool refreshEntries = true;
            std::filesystem::path currentSearchPath;
            std::filesystem::path currentSelectedPath;
            std::string fileName;
            size_t extensionSelected;
            std::function<void(const std::filesystem::path&, const std::string& extensionSelected)> postSelectionFunc;
        } filePicker;

        std::optional<size_t> keybindWaiting;

        std::filesystem::path testing;

        void start_gui();
        void end_gui();

        std::string ipToConnectTo;

        MainProgram& main;
};
