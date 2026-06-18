#include "menu_controller.h"

void MenuController::enterMenu() {
    if (_active) return;
    _active = true;
    _selectedItem = 0;
    _dirty = true;
    ESP_LOGI(tag, "Entering menu");
}

bool MenuController::previousMenuItem() {
    if (!_active) return false;
    _selectedItem = _selectedItem == 0 ? MENU_ITEM_COUNT - 1 : _selectedItem - 1;
    _dirty = true;
    return true;
}

bool MenuController::nextMenuItem() {
    if (!_active) return false;
    _selectedItem = (_selectedItem + 1) % MENU_ITEM_COUNT;
    _dirty = true;
    return true;
}

MenuController::SelectResult MenuController::selectMenuItem() {
    if (!_active) return SelectResult::Ignored;

    if (_selectedItem == EXIT_MENU_ITEM) {
        ESP_LOGI(tag, "Selected menu item: %s", MENU_ITEMS[_selectedItem]);
        exitMenu();
        return SelectResult::ExitMenu;
    }

    switch (_selectedItem) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
            ESP_LOGI(tag, "Selected menu item: %s", MENU_ITEMS[_selectedItem]);
            return SelectResult::Handled;
        default:
            ESP_LOGW(tag, "Unhandled menu item: %u", _selectedItem);
            exitMenu();
            return SelectResult::ExitMenu;
    }
}

void MenuController::exitMenu() {
    if (!_active) return;
    _active = false;
    _dirty = true;
}

MenuSnapshot MenuController::snapshot() const {
    MenuSnapshot s = {};
    s.active = _active;
    s.dirty = _dirty;
    s.selectedItem = _selectedItem;
    s.itemCount = MENU_ITEM_COUNT;
    s.items = MENU_ITEMS;
    return s;
}

void MenuController::markRendered() {
    _dirty = false;
}
