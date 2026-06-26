#include "menu_controller.h"

#include "tasks/api.h"

extern Api api;

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
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
            ESP_LOGI(tag, "Selected menu item: %s", MENU_ITEMS[_selectedItem]);
            return SelectResult::Handled;
        case 14:  // Restart
            ESP_LOGI(tag, "Selected menu item: %s", MENU_ITEMS[_selectedItem]);
            delay(200);
            if (!api.queueCommand("restart")) ESP_LOGE(tag, "Failed to queue restart command");
            return SelectResult::Handled;
        case EXIT_MENU_ITEM:  // Exit
            ESP_LOGE(tag, "We should never get here");
            return SelectResult::ExitMenu;
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
    ESP_LOGD(tag, "Exiting menu");
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
