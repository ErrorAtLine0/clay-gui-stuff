#include "Toolbar.hpp"
#include "Helpers/ConvertVec.hpp"
#include "MainProgram.hpp"
#include "GUIStuff/Elements/Element.hpp"
#include "imgui/imgui.h"
#include "src/GUIStuff/GUIManager.hpp"
#include "src/GridManager.hpp"
#include "src/InputManager.hpp"
#include "src/World.hpp"
#include <algorithm>
#include <filesystem>
#include <optional>

Toolbar::Toolbar(MainProgram& initMain):
    io(std::make_shared<GUIStuff::UpdateInputData>()),
    main(initMain)
{
    io->theme = GUIStuff::get_default_dark_mode();
    io->theme->textTypeface = main.fonts.map["Roboto"];
    io->theme->fontSize = 20;
}

void Toolbar::open_file_selector(const std::string& filePickerName, const std::vector<std::string>& extensionFilters, const std::function<void(const std::filesystem::path&, const std::string& extensionSelected)>& postSelectionFunc) {
    filePicker.isOpen = true;
    filePicker.refreshEntries = true;
    filePicker.extensionFilters = extensionFilters;
    filePicker.extensionSelected = extensionFilters.size() - 1;
    filePicker.filePickerWindowName = filePickerName;
    filePicker.postSelectionFunc = postSelectionFunc;
}

std::filesystem::path& Toolbar::file_selector_path() {
    return filePicker.currentSearchPath;
}

void Toolbar::color_selector_left(Vector4f* color) {
    colorLeft = color;
    justAssignedColorLeft = true;
}

void Toolbar::color_selector_right(Vector4f* color) {
    colorRight = color;
    justAssignedColorRight = true;
}

nlohmann::json Toolbar::get_appearance_json() {
    using json = nlohmann::json;
    json toRet;
    toRet["darkMode"] = darkMode;
    toRet["guiScale"] = guiScale;
    toRet["gridType"] = main.grid.gridType;
    return toRet;
}

void Toolbar::set_appearance_json(const nlohmann::json& j) {
    j.at("darkMode").get_to(newDarkMode);
    j.at("guiScale").get_to(guiScale);
    j.at("gridType").get_to(main.grid.gridType);
}

void Toolbar::update() {
    if(newDarkMode != darkMode) {
        darkMode = newDarkMode;
        if(darkMode) {
            io->theme = GUIStuff::get_default_dark_mode();
            io->theme->textTypeface = main.fonts.map["Roboto"];
            io->theme->fontSize = 20;
        }
        else {
            io->theme = std::make_shared<GUIStuff::Theme>();
            io->theme->textTypeface = main.fonts.map["Roboto"];
            io->theme->fontSize = 20;
        }
    }

    if(main.input.key(InputManager::KEY_SAVE).pressed && !optionsMenuOpen && !filePicker.isOpen)
        save_func();
    if(main.input.key(InputManager::KEY_SAVE_AS).pressed && !optionsMenuOpen && !filePicker.isOpen)
        save_as_func();

    start_gui();

    CLAY({
        .layout = {
            .sizing = {.width = CLAY_SIZING_FIXED(gui.windowSize.x()), .height = CLAY_SIZING_FIXED(gui.windowSize.y())},
            .padding = CLAY_PADDING_ALL(io->theme->padding1),
            .childGap = io->theme->childGap1,
            .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP},
            .layoutDirection = CLAY_TOP_TO_BOTTOM
        }
    }) {
        top_toolbar();
        drawing_program_gui();
        if(filePicker.isOpen)
            file_picker_gui();
        else if(optionsMenuOpen)
            options_menu();
    }

    justAssignedColorLeft = false;
    justAssignedColorRight = false;

    if(!optionsMenuOpen || generalSettingsOptions != GSETTINGS_KEYBINDS)
        keybindWaiting = std::nullopt;

    end_gui();
}

void Toolbar::save_func() {
    if(main.world->filePath == std::filesystem::path()) {
        open_file_selector("Save", {".", World::FILE_EXTENSION}, [&](const std::filesystem::path& p, const std::string& e) {
            main.world->save_to_file(p);
        });
    }
    else
        main.world->save_to_file(main.world->filePath.string());
}

void Toolbar::save_as_func() {
    open_file_selector("Save As", {".", World::FILE_EXTENSION}, [&](const std::filesystem::path& p, const std::string& e) {
        main.world->save_to_file(p);
    });
}

void Toolbar::top_toolbar() {
    CLAY({
        .layout = {
            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) },
            .padding = CLAY_PADDING_ALL(io->theme->padding1),
            .childGap = io->theme->childGap1,
            .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER},
            .layoutDirection = CLAY_LEFT_TO_RIGHT
        },
        .backgroundColor = convert_vec4<Clay_Color>(io->theme->backColor1),
        .cornerRadius = CLAY_CORNER_RADIUS(io->theme->windowCorners1),
        .border = {.color = convert_vec4<Clay_Color>(io->theme->backColor2), .width = CLAY_BORDER_OUTSIDE(io->theme->windowBorders1)}
    }) {
        gui.obstructing_window();
        bool menuPopUpJustOpen = false;
        if(gui.svg_icon_button("Main Menu", "icons/menu.svg", menuPopUpOpen)) {
            menuPopUpOpen = true;
            menuPopUpJustOpen = true;
        }
        std::vector<std::pair<std::string, std::string>> tabNames;
        for(size_t i = 0; i < main.worlds.size(); i++)
            tabNames.emplace_back(main.worlds[i]->network_being_used() ? "icons/network.svg" : "", main.worlds[i]->name);
        std::optional<size_t> closedTab;
        gui.tab_list("file tab list", tabNames, main.worldIndex, closedTab);
        if(closedTab)
            main.set_tab_to_close(closedTab.value());
        if(menuPopUpOpen) {
            CLAY({
                .layout = {
                    .sizing = {.width = CLAY_SIZING_FIT(300), .height = CLAY_SIZING_FIT(0) },
                    .padding = CLAY_PADDING_ALL(io->theme->padding1),
                    .childGap = io->theme->childGap1,
                    .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP},
                    .layoutDirection = CLAY_TOP_TO_BOTTOM
                },
                .backgroundColor = convert_vec4<Clay_Color>(io->theme->backColor1),
                .cornerRadius = CLAY_CORNER_RADIUS(io->theme->windowCorners1),
                .floating = {.attachPoints = {.element = CLAY_ATTACH_POINT_LEFT_TOP, .parent = CLAY_ATTACH_POINT_LEFT_BOTTOM}, .attachTo = CLAY_ATTACH_TO_PARENT},
                .border = {.color = convert_vec4<Clay_Color>(io->theme->backColor2), .width = CLAY_BORDER_OUTSIDE(io->theme->windowBorders1)}
            }) {
                gui.obstructing_window();
                if(gui.text_button_wide("new file local", "New File"))
                    main.new_tab(World::CONNECTIONTYPE_LOCAL, "");
                if(gui.text_button_wide("save file", "Save"))
                    save_func();
                if(gui.text_button_wide("save as file", "Save As"))
                    save_as_func();
                if(gui.text_button_wide("open file", "Open")) {
                    open_file_selector("Open", {".", World::FILE_EXTENSION}, [&](const std::filesystem::path& p, const std::string& e) {
                        main.new_tab(World::CONNECTIONTYPE_LOCAL, p.string());
                    });
                }
                if(!main.net_server_hosted()) {
                    if(gui.text_button_wide("start hosting", "Host New Server"))
                        main.new_tab(World::CONNECTIONTYPE_SERVER, "");
                    if(gui.text_button_wide("start hosting file", "Host Server From File")) {
                        open_file_selector("Open", {".", World::FILE_EXTENSION}, [&](const std::filesystem::path& p, const std::string& e) {
                            main.new_tab(World::CONNECTIONTYPE_SERVER, p.string());
                        });
                    }
                }
                if(gui.text_button_wide("connect to ip new file", "Connect to IP")) {
                    optionsMenuOpen = true;
                    optionsMenuType = CONNECT_IP_MENU;
                }
                if(gui.text_button_wide("open options", "Settings")) {
                    optionsMenuOpen = true;
                    optionsMenuType = GENERAL_SETTINGS_MENU;
                }
                if(gui.text_button_wide("quit button", "Quit"))
                    main.setToQuit = true;
                if(io->mouse.leftClick && !menuPopUpJustOpen)
                    menuPopUpOpen = false;
            }
        }
    }
}

void Toolbar::drawing_program_gui() {
    CLAY({
        .layout = {
            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) },
            .childGap = io->theme->childGap1,
            .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER},
            .layoutDirection = CLAY_LEFT_TO_RIGHT
        },
    }) {
        main.world->drawProg.toolbar_gui();
        if(colorLeft) {
            CLAY({
                .layout = {
                    .sizing = {.width = CLAY_SIZING_FIXED(300), .height = CLAY_SIZING_FIT(0)},
                    .padding = CLAY_PADDING_ALL(io->theme->padding1),
                    .childGap = io->theme->childGap1,
                    .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP},
                    .layoutDirection = CLAY_TOP_TO_BOTTOM
                },
                .backgroundColor = convert_vec4<Clay_Color>(io->theme->backColor1),
                .cornerRadius = CLAY_CORNER_RADIUS(io->theme->windowCorners1),
                .border = {.color = convert_vec4<Clay_Color>(io->theme->backColor2), .width = CLAY_BORDER_OUTSIDE(io->theme->windowBorders1)}
            }) {
                gui.obstructing_window();
                gui.color_picker_items("colorpickerleft", colorLeft, true);
                if(!Clay_Hovered() && !justAssignedColorLeft && io->mouse.leftClick)
                    colorLeft = nullptr;
            }
        }
        CLAY({
            .layout = {
                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)}
            }
        }) {}
        if(colorRight) {
            CLAY({
                .layout = {
                    .sizing = {.width = CLAY_SIZING_FIXED(300), .height = CLAY_SIZING_FIT(0)},
                    .padding = CLAY_PADDING_ALL(io->theme->padding1),
                    .childGap = io->theme->childGap1,
                    .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP},
                    .layoutDirection = CLAY_TOP_TO_BOTTOM
                },
                .backgroundColor = convert_vec4<Clay_Color>(io->theme->backColor1),
                .cornerRadius = CLAY_CORNER_RADIUS(io->theme->windowCorners1),
                .border = {.color = convert_vec4<Clay_Color>(io->theme->backColor2), .width = CLAY_BORDER_OUTSIDE(io->theme->windowBorders1)}
            }) {
                gui.obstructing_window();
                gui.color_picker_items("colorpickerright", colorRight, true);
                if(!Clay_Hovered() && !justAssignedColorRight && io->mouse.leftClick)
                    colorRight = nullptr;
            }
        }
        main.world->drawProg.tool_options_gui();
    }
}

void Toolbar::options_menu() {
    switch(optionsMenuType) {
        case CONNECT_IP_MENU: {
            CLAY({
                .layout = {
                    .sizing = {.width = CLAY_SIZING_FIT(300), .height = CLAY_SIZING_FIT(0) },
                    .padding = CLAY_PADDING_ALL(io->theme->padding1),
                    .childGap = io->theme->childGap1,
                    .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP},
                    .layoutDirection = CLAY_TOP_TO_BOTTOM
                },
                .backgroundColor = convert_vec4<Clay_Color>(io->theme->backColor1),
                .cornerRadius = CLAY_CORNER_RADIUS(io->theme->windowCorners1),
                .floating = {.attachPoints = {.element = CLAY_ATTACH_POINT_CENTER_CENTER, .parent = CLAY_ATTACH_POINT_CENTER_CENTER}, .attachTo = CLAY_ATTACH_TO_PARENT},
                .border = {.color = convert_vec4<Clay_Color>(io->theme->backColor2), .width = CLAY_BORDER_OUTSIDE(io->theme->windowBorders1)}
            }) {
                gui.obstructing_window();
                gui.input_text_field("connect IP input field", "IP", &ipToConnectTo);
                if(gui.text_button_wide("do connect ip", "Connect IP")) {
                    main.new_tab(World::CONNECTIONTYPE_CLIENT, ipToConnectTo);
                    optionsMenuOpen = false;
                }
                if(gui.text_button_wide("cancel connect ip", "Cancel"))
                    optionsMenuOpen = false;
            }
            break;
        }
        case GENERAL_SETTINGS_MENU: {
            CLAY({
                .layout = {
                    .sizing = {.width = CLAY_SIZING_FIT(500), .height = CLAY_SIZING_FIXED(500) },
                    .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP},
                    .layoutDirection = CLAY_TOP_TO_BOTTOM
                },
                .backgroundColor = convert_vec4<Clay_Color>(io->theme->backColor1),
                .cornerRadius = CLAY_CORNER_RADIUS(io->theme->windowCorners1),
                .floating = {.attachPoints = {.element = CLAY_ATTACH_POINT_CENTER_CENTER, .parent = CLAY_ATTACH_POINT_CENTER_CENTER}, .attachTo = CLAY_ATTACH_TO_PARENT},
                .border = {.color = convert_vec4<Clay_Color>(io->theme->backColor2), .width = CLAY_BORDER_OUTSIDE(io->theme->windowBorders1)}
            }) {
                gui.push_id("gsettings");
                CLAY({
                    .layout = {
                        .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                        .padding = CLAY_PADDING_ALL(io->theme->padding1),
                        .childGap = io->theme->childGap1,
                        .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_TOP},
                        .layoutDirection = CLAY_LEFT_TO_RIGHT
                    }
                }) {
                    gui.obstructing_window();
                    CLAY({
                        .layout = {
                            .sizing = {.width = CLAY_SIZING_FIXED(150), .height = CLAY_SIZING_GROW(0) },
                            .padding = CLAY_PADDING_ALL(io->theme->padding1),
                            .childGap = io->theme->childGap1,
                            .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_TOP},
                            .layoutDirection = CLAY_TOP_TO_BOTTOM
                        }
                    }) {
                        if(gui.text_button_wide("Appearancebutton", "Appearance", generalSettingsOptions == GSETTINGS_APPEARANCE)) generalSettingsOptions = GSETTINGS_APPEARANCE;
                        if(gui.text_button_wide("Keybindsbutton", "Keybinds", generalSettingsOptions == GSETTINGS_KEYBINDS)) generalSettingsOptions = GSETTINGS_KEYBINDS;
                    }
                    CLAY({
                        .layout = {
                            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) },
                            .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_TOP},
                            .layoutDirection = CLAY_TOP_TO_BOTTOM
                        }
                    }) {
                        switch(generalSettingsOptions) {
                            case GSETTINGS_APPEARANCE: {
                                gui.scroll_bar_area("general settings appearance", [&](float, float, float) {
                                    CLAY({
                                        .layout = {
                                            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) },
                                            .padding = CLAY_PADDING_ALL(io->theme->padding1),
                                            .childGap = io->theme->childGap1,
                                            .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_TOP},
                                            .layoutDirection = CLAY_TOP_TO_BOTTOM
                                        }
                                    }) {
                                        if(gui.radio_button_field("grid type no grid", "No Grid", (main.grid.gridType == GridManager::GRIDTYPE_NONE))) main.grid.gridType = GridManager::GRIDTYPE_NONE;
                                        if(gui.radio_button_field("grid type circle grid", "Circle Dot Grid", (main.grid.gridType == GridManager::GRIDTYPE_CIRCLEDOT))) main.grid.gridType = GridManager::GRIDTYPE_CIRCLEDOT;
                                        if(gui.radio_button_field("grid type square grid", "Square Dot Grid", (main.grid.gridType == GridManager::GRIDTYPE_SQUAREDOT))) main.grid.gridType = GridManager::GRIDTYPE_SQUAREDOT;
                                        if(gui.radio_button_field("grid type line grid", "Line Grid", (main.grid.gridType == GridManager::GRIDTYPE_LINE))) main.grid.gridType = GridManager::GRIDTYPE_LINE;
                                        gui.input_scalar_field("GUI Scale", "GUI Scale", &guiScale, 0.5f, 3.0f);
                                        gui.checkbox_field("dark mode gui check", "Dark Mode GUI", &newDarkMode);
                                    }
                                });
                                break;
                            }
                            case GSETTINGS_KEYBINDS: {
                                float entryHeight = 40.0f;

                                if(keybindWaiting.has_value()) {
                                    main.input.stop_key_input();
                                    if(main.input.lastPressedKeybind) {
                                        size_t v = keybindWaiting.value();

                                        Vector2ui32 newKey = main.input.lastPressedKeybind.value();
                                        main.input.keyAssignments.erase(newKey);
                                        auto f = std::find_if(main.input.keyAssignments.begin(), main.input.keyAssignments.end(), [&](auto& p) {
                                            return p.second == v;
                                        });
                                        if(f != main.input.keyAssignments.end())
                                            main.input.keyAssignments.erase(f);
                                        main.input.keyAssignments.emplace(newKey, v);
                                        keybindWaiting = std::nullopt;
                                    }
                                }

                                gui.scroll_bar_area("general settings keybinds", [&](float, float, float) {
                                    for(size_t i = 0; i < InputManager::KEY_ASSIGNABLE_COUNT; i++) {
                                        gui.push_id(i);
                                        CLAY({
                                            .layout = {
                                                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(entryHeight) },
                                                .padding = CLAY_PADDING_ALL(4),
                                                .childGap = io->theme->childGap1,
                                                .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER},
                                                .layoutDirection = CLAY_LEFT_TO_RIGHT 
                                            }
                                        }) {
                                            gui.text_label(nlohmann::json(static_cast<InputManager::KeyCodeEnum>(i)));
                                            auto f = std::find_if(main.input.keyAssignments.begin(), main.input.keyAssignments.end(), [&](auto& p) {
                                                return p.second == i;
                                            });
                                            std::string assignedKeystrokeStr = f != main.input.keyAssignments.end() ? main.input.key_assignment_to_str(f->first) : "";
                                            if(gui.text_button_wide("keybind button", assignedKeystrokeStr, keybindWaiting.has_value() && keybindWaiting.value() == i))
                                                keybindWaiting = i;
                                        }
                                        gui.pop_id();
                                    }
                                });
                                break;
                            }
                        }
                        if(gui.text_button_wide("done menu", "Done")) {
                            main.save_config();
                            optionsMenuOpen = false;
                        }
                    }
                }
                gui.pop_id();
            }
            break;
        }
    }
}

void Toolbar::file_picker_gui() {
    gui.push_id("filepicker");
    bool isDoneByDoubleClick = false;
    CLAY({
        .layout = {
            .sizing = {.width = CLAY_SIZING_FIXED(700), .height = CLAY_SIZING_FIXED(500) },
            .padding = CLAY_PADDING_ALL(io->theme->padding1),
            .childGap = io->theme->childGap1,
            .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP},
            .layoutDirection = CLAY_TOP_TO_BOTTOM
        },
        .backgroundColor = convert_vec4<Clay_Color>(io->theme->backColor1),
        .cornerRadius = CLAY_CORNER_RADIUS(io->theme->windowCorners1),
        .floating = {.attachPoints = {.element = CLAY_ATTACH_POINT_CENTER_CENTER, .parent = CLAY_ATTACH_POINT_CENTER_CENTER}, .attachTo = CLAY_ATTACH_TO_PARENT},
        .border = {.color = convert_vec4<Clay_Color>(io->theme->backColor2), .width = CLAY_BORDER_OUTSIDE(io->theme->windowBorders1)}
    }) {
        gui.obstructing_window();
        gui.text_label_centered(filePicker.filePickerWindowName);
        gui.left_to_right_line_layout([&]() {
            if(gui.svg_icon_button("file picker back button", "icons/backarrow.svg", false, 30.0f)) {
                filePicker.currentSearchPath = filePicker.currentSearchPath.parent_path();
                filePicker.refreshEntries = true;
            }
            std::filesystem::path pathDiff = filePicker.currentSearchPath;
            gui.input_path_field("file picker path", "Path", &filePicker.currentSearchPath, std::filesystem::file_type::directory);
            if(pathDiff != filePicker.currentSearchPath)
                filePicker.refreshEntries = true;
        });
        CLAY({
            .layout = {
                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP},
                .layoutDirection = CLAY_TOP_TO_BOTTOM
            },
            .backgroundColor = convert_vec4<Clay_Color>(io->theme->backColor2)
        }) {
            if(filePicker.refreshEntries) {
                filePicker.entries.clear();
                for(const std::filesystem::path& entry : std::filesystem::directory_iterator(filePicker.currentSearchPath))
                    filePicker.entries.emplace_back(entry);
                std::erase_if(filePicker.entries, [&](const std::filesystem::path& a) {
                    if(filePicker.extensionFilters[filePicker.extensionSelected] == ".")
                        return false;
                    return std::filesystem::is_regular_file(a) && (!a.has_extension() || a.extension() != filePicker.extensionFilters[filePicker.extensionSelected]);
                });
                std::sort(filePicker.entries.begin(), filePicker.entries.end(), [&](const std::filesystem::path& a, const std::filesystem::path& b) {
                    bool aDir = std::filesystem::is_directory(a);
                    bool bDir = std::filesystem::is_directory(b);
                    const std::string& aStr = a.string();
                    const std::string& bStr = b.string();
                    if(aDir && !bDir)
                        return true;
                    if(!aDir && bDir)
                        return false;
                    return std::lexicographical_compare(aStr.begin(), aStr.end(), bStr.begin(), bStr.end());
                });
                filePicker.refreshEntries = false;
            }
            float entryHeight = 25.0f;
            gui.scroll_bar_many_entries_area("file picker entries", entryHeight, filePicker.entries.size(), [&](size_t i, bool isListHovered) {
                const std::filesystem::path& entry = filePicker.entries[i];
                bool selectedEntry = filePicker.currentSelectedPath == entry;
                CLAY({
                    .layout = {
                        .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(entryHeight)},
                        .childGap = 2,
                        .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER},
                        .layoutDirection = CLAY_LEFT_TO_RIGHT 
                    },
                    .backgroundColor = selectedEntry ? convert_vec4<Clay_Color>(io->theme->backColor1) : convert_vec4<Clay_Color>(io->theme->backColor2)
                }) {
                    CLAY({
                        .layout = {
                            .sizing = {.width = CLAY_SIZING_FIXED(20), .height = CLAY_SIZING_FIXED(20)}
                        },
                    }) {
                        if(std::filesystem::is_directory(entry))
                            gui.svg_icon("folder icon", "icons/folder.svg", selectedEntry);
                        else
                            gui.svg_icon("file icon", "icons/file.svg", selectedEntry);
                    }
                    gui.text_label(entry.filename().string());
                    if(Clay_Hovered() && io->mouse.leftClick && isListHovered) {
                        if(selectedEntry && io->mouse.leftClick >= 2) {
                            if(std::filesystem::is_directory(entry)) {
                                filePicker.currentSearchPath = entry;
                                filePicker.refreshEntries = true;
                            }
                            else if(std::filesystem::is_regular_file(entry))
                                isDoneByDoubleClick = true;
                        }
                        else {
                            filePicker.currentSelectedPath = entry;
                            if(std::filesystem::is_regular_file(entry))
                                filePicker.fileName = entry.filename().string();
                        }
                    }
                }
            });
        }
        gui.left_to_right_line_layout([&]() {
            gui.input_text("filepicker filename", &filePicker.fileName);
            size_t oldExtensionSelected = filePicker.extensionSelected;
            gui.dropdown_select("filepicker select type", &filePicker.extensionSelected, filePicker.extensionFilters);
            if(oldExtensionSelected != filePicker.extensionSelected)
                filePicker.refreshEntries = true;
        });
        gui.left_to_right_line_layout([&]() {
            std::filesystem::path pathToRet = std::filesystem::path();
            if(gui.text_button_wide("filepicker done", "Done") || isDoneByDoubleClick) {
                if(!filePicker.fileName.empty()) {
                    pathToRet = filePicker.currentSearchPath / filePicker.fileName;
                    filePicker.postSelectionFunc(pathToRet, filePicker.extensionFilters[filePicker.extensionSelected]);
                }
                filePicker.isOpen = false;
            }
            if(gui.text_button_wide("filepicker cancel", "Cancel")) {
                filePicker.isOpen = false;
            }
        });
    }
    gui.pop_id();
}

void Toolbar::start_gui() {
    io->mouse.leftClick = main.input.mouse.leftClicks;
    io->mouse.leftHeld = main.input.mouse.leftDown;
    io->mouse.globalPos = main.input.mouse.pos / guiScale;
    io->mouse.scroll = main.input.mouse.scrollAmount;
    io->deltaTime = main.deltaTime;

    io->key.left = main.input.key(InputManager::KEY_TEXT_LEFT).repeat;
    io->key.right = main.input.key(InputManager::KEY_TEXT_RIGHT).repeat;
    io->key.up = main.input.key(InputManager::KEY_TEXT_UP).repeat;
    io->key.down = main.input.key(InputManager::KEY_TEXT_DOWN).repeat;
    io->key.leftShift = main.input.key(InputManager::KEY_TEXT_SHIFT).held;
    io->key.leftCtrl = main.input.key(InputManager::KEY_TEXT_CTRL).held;
    io->key.home = main.input.key(InputManager::KEY_TEXT_HOME).repeat;
    io->key.del = main.input.key(InputManager::KEY_TEXT_DELETE).repeat;
    io->key.backspace = main.input.key(InputManager::KEY_TEXT_BACKSPACE).repeat;
    io->key.enter = main.input.key(InputManager::KEY_TEXT_ENTER).repeat;
    io->key.selectAll = main.input.key(InputManager::KEY_TEXT_SELECTALL).repeat;
    io->key.copy = main.input.key(InputManager::KEY_TEXT_COPY).repeat;
    io->key.paste = main.input.key(InputManager::KEY_TEXT_PASTE).repeat;
    io->key.cut = main.input.key(InputManager::KEY_TEXT_CUT).repeat;

    io->textInput = main.input.text.newInput;
    io->clipboard.textIn = main.input.get_clipboard_str();

    gui.windowPos = Vector2f{0.0f, 0.0f};
    gui.windowSize = main.window.size.cast<float>() / guiScale;
    io->hoverObstructed = false;
    io->acceptingTextInput = false;
    gui.io = io;

    gui.begin();
}

void Toolbar::end_gui() {
    gui.end();
    if(io->acceptingTextInput)
        main.input.text_input_silence_everything();
    if(io->clipboard.textOut)
        main.input.set_clipboard_str(*io->clipboard.textOut);
}

void Toolbar::draw(SkCanvas* canvas) {
    canvas->save();
    canvas->scale(guiScale, guiScale);
    gui.draw(canvas);
    canvas->restore();
}
