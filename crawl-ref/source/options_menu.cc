#include "options_menu.h"
#include <string>
#include <fstream>
#include "branch-type.h"
#include <map>
#include "skill-type.h"
#include "externs.h"
#include <memory>
#include "end.h"
#include <stdlib.h>
#include "unwind.h"
#include "libutil.h"
#include "libconsole.h"
#include "outer-menu.h"
#include "viewchar.h"
#include "startup.h"
#include "initfile.h"
#ifdef USE_TILE
#include "tilepick.h"
#endif
// core program of options_menu

using namespace ui;

#ifndef DGAMELAUNCH

struct options_menu_item
{
    option_identifier id;
    const char* label;
    const char* description;
};

static const options_menu_item entries[] =
{
    {OPTION_TYPE_1, "Show game time", "show_game_time" },
    {OPTION_TYPE_2, "Name", "name" },
    {OPTION_TYPE_3, "Exit", "Quit to Main Menu"},
};


static void _construct_boolean_menu(shared_ptr<OuterMenu>& container, bool exit) {
    auto trueLabel = make_shared<Text>();
    auto falseLabel = make_shared<Text>();

#ifdef USE_TILE_LOCAL
    auto thbox = make_shared<Box>(Box::HORZ);
    thbox->set_cross_alignment(Widget::Align::CENTER);
    auto ttile = make_shared<Image>();
    ttile->set_tile(tile_def(tileidx_boolean(TRUE_BOOL)));
    ttile->set_margin_for_sdl(0, 6, 0, 0);
    thbox->add_child(move(ttile));
    thbox->add_child(trueLabel);
#endif 

    trueLabel->set_text(formatted_string("True", WHITE));
    if(exit){
        trueLabel->set_text(formatted_string("Exit and Save", WHITE));
    }

    auto tBtn = make_shared<MenuButton>();
#ifdef USE_TILE_LOCAL
    thbox->set_margin_for_sdl(2, 10, 2, 2);
    tBtn->set_child(move(thbox));
#else
    tBtn->set_child(move(trueLabel));
#endif
    tBtn->id = TRUE_BOOL;
    tBtn->highlight_colour = LIGHTGREY;
    container->add_button(move(tBtn), 0, 0);

#ifdef USE_TILE_LOCAL
    auto fhbox = make_shared<Box>(Box::HORZ);
    fhbox->set_cross_alignment(Widget::Align::CENTER);
    auto ftile = make_shared<Image>();
    ftile->set_tile(tile_def(tileidx_boolean(FALSE_BOOL)));
    ftile->set_margin_for_sdl(0, 6, 0, 0);
    fhbox->add_child(move(ftile));
    fhbox->add_child(falseLabel);
#endif

    falseLabel->set_text(formatted_string("False", WHITE));
    if(exit){
        falseLabel->set_text(formatted_string("Exit without Saving", WHITE));
    }

    auto fBtn = make_shared<MenuButton>();
#ifdef USE_TILE_LOCAL
    fhbox->set_margin_for_sdl(2, 10, 2, 2);
    fBtn->set_child(move(fhbox));
#else
    fBtn->set_child(move(falseLabel));
#endif
    fBtn->id = FALSE_BOOL;
    fBtn->highlight_colour = LIGHTGREY;
    container->add_button(move(fBtn), 0, 1);
}


class BoolOptionMenu : public Widget {
public: 
    BoolOptionMenu(int *original_option, bool exit) 
        : done(false), selection(*original_option), changed_option(original_option){
        m_root = make_shared<Box>(Box::VERT);
        add_internal_child(m_root);
        m_root->set_cross_alignment(Widget::Align::STRETCH);

        auto grid = make_shared<Grid>();
        grid->set_margin_for_crt(0, 0, 1, 0);
        auto mode_prompt = make_shared<Text>("Choices:");
        if(exit){
            mode_prompt = make_shared<Text>("Exiting:");
        }
        
        mode_prompt->set_margin_for_crt(0, 1, 1, 0);
        mode_prompt->set_margin_for_sdl(0, 0, 10, 0);
        bool_menu = make_shared<OuterMenu>(true, 1, 2);
        bool_menu->set_margin_for_sdl(0, 0, 10, 10);
        bool_menu->set_margin_for_crt(0, 0, 1, 0);
        _construct_boolean_menu(bool_menu, exit);


#ifdef USE_TILE_LOCAL
        bool_menu->min_size().height = TILE_Y * 3;
#else
        bool_menu->min_size().height = 2;
#endif

        grid->add_child(move(mode_prompt), 0, 1);
        grid->add_child(bool_menu, 1, 1);
        
        m_root->on_activate_event([this](const ActivateEvent& event) {
            const auto button = static_pointer_cast<const MenuButton>(event.target());
            this->menu_item_activated(button->id);
            return true;
        });
        grid->column_flex_grow(0) = 1;
        grid->column_flex_grow(1) = 10;

        m_root->add_child(move(grid));
    }
    
    virtual void _render() override;
    virtual SizeReq _get_preferred_size(Direction dim, int prosp_width) override;
    virtual void _allocate_region() override;

    bool has_allocated = false;

    bool done;
    virtual shared_ptr<Widget> get_child_at_offset(int, int) override {
        return m_root;
    }
private:
    void on_show();
    void menu_item_activated(int id);

    int selection;
    int* changed_option;
    shared_ptr<Box> m_root;
    shared_ptr<OuterMenu> bool_menu;
};

SizeReq BoolOptionMenu::_get_preferred_size(Direction dim, int prosp_width)
{
    return m_root->get_preferred_size(dim, prosp_width);
}

void BoolOptionMenu::_render()
{
    m_root->render();
}

void BoolOptionMenu::_allocate_region()
{
    m_root->allocate_region(m_region);

    if (!has_allocated)
    {
        has_allocated = true;
        on_show();
    }
}

void BoolOptionMenu::on_show()
{
    if (selection >= NUM_BOOL)
        selection = BOOL_UNSPECIFIED;

    int id;
    if (selection != BOOL_UNSPECIFIED)
        id = selection;
    else
        id = 0;

    if (auto focus = bool_menu->get_button_by_id(id)) {
        bool_menu->scroll_button_into_view(focus);
    }

    on_hotkey_event([this](const KeyEvent& ev) {
        const auto keyn = ev.key();
        if (key_is_escape(keyn) || keyn == CK_MOUSE_CMD)
        {
            return done = true;
        }
        return false;
    });
}

void BoolOptionMenu::menu_item_activated(int id)
{
    switch (id)
    {
    case FALSE_BOOL:
        // set false
        *changed_option = FALSE_BOOL;
        break;
    case TRUE_BOOL:
        // set true
        *changed_option = TRUE_BOOL;
        break;
    default:
        break;
    }
    done = true;
}

static void _show_bool_menu(int *initial_selection, bool exit)
{
    unwind_bool no_more(crawl_state.show_more_prompt, false);

#if defined(USE_TILE_LOCAL) && defined(TOUCH_UI)
    wm->show_keyboard();
#elif defined(USE_TILE_WEB)
    tiles_crt_popup show_as_popup;
#endif

    auto bool_ui = make_shared<BoolOptionMenu>(initial_selection, exit);
    auto popup = make_shared<ui::Popup>(bool_ui);

    ui::run_layout(move(popup), bool_ui->done);
}

class StringOptionMenu : public Widget {
public:
    StringOptionMenu(string* original_string) 
    : done(false), selection(*original_string), changed_string(original_string)
    {
        m_root = make_shared<Box>(Box::VERT);
        add_internal_child(m_root);
        m_root->set_cross_alignment(Widget::Align::STRETCH);

        auto grid = make_shared<Grid>();
        grid->set_margin_for_crt(0, 0, 1, 0);

        auto name_prompt = make_shared<Text>("Enter option text:");
        name_prompt->set_margin_for_crt(0, 1, 1, 0);
        name_prompt->set_margin_for_sdl(0, 0, 10, 0);

        input_text = make_shared<Text>(formatted_string(selection, WHITE));
        input_text->set_margin_for_crt(0, 0, 1, 0);
        input_text->set_margin_for_sdl(0, 0, 10, 10);

        grid->add_child(move(name_prompt), 0, 0);
        grid->add_child(input_text, 1, 0);

        grid->column_flex_grow(0) = 1;
        grid->column_flex_grow(1) = 10;

        m_root->add_child(move(grid));
    }
    virtual void _render() override;
    virtual SizeReq _get_preferred_size(Direction dim, int prosp_width) override;
    virtual void _allocate_region() override;

    bool has_allocated = false;

    bool done;
    virtual shared_ptr<Widget> get_child_at_offset(int, int) override {
        return m_root;
    }

private:
    void on_show();
    bool first_action = true;
    string selection;
    string* changed_string;
    shared_ptr<Box> m_root;
    shared_ptr<Text> input_text;
};

SizeReq StringOptionMenu::_get_preferred_size(Direction dim, int prosp_width)
{
    return m_root->get_preferred_size(dim, prosp_width);
}

void StringOptionMenu::_render()
{
    m_root->render();
}

void StringOptionMenu::_allocate_region()
{
    m_root->allocate_region(m_region);

    if (!has_allocated)
    {
        has_allocated = true;
        on_show();
    }
}

void StringOptionMenu::on_show() {
    on_hotkey_event([this](const KeyEvent& ev) {
        const auto keyn = ev.key();
        bool changed_option = false;
        if (key_is_escape(keyn) || keyn == CK_MOUSE_CMD || keyn == CK_ENTER)
        {
            return done = true;
        }

        if (keyn == ' ' && changed_string->empty())
        {
            first_action = false;
            *changed_string = "";
            changed_option = true;
        }
        else if (iswalnum(keyn) || keyn == '-' || keyn == '.'
            || keyn == '_' || keyn == ' ')
        {
            first_action = false;
            *changed_string += stringize_glyph(keyn);
            changed_option = true;
        }
        else if (keyn == CK_BKSP)
        {
            if (changed_string->empty())
            {
                first_action = false;
                *changed_string = "";
                changed_option = true;
            }
            else
            {
                changed_string->erase(changed_string->size() - 1);
                changed_option = true;
            }
        }

        if (!changed_option)
            return false;

        input_text->set_text(formatted_string(*changed_string, WHITE));

        return true;
    });
}

static void _show_string_menu(string *original_string) {
    unwind_bool no_more(crawl_state.show_more_prompt, false);

#if defined(USE_TILE_LOCAL) && defined(TOUCH_UI)
    wm->show_keyboard();
#elif defined(USE_TILE_WEB)
    tiles_crt_popup show_as_popup;
#endif

    auto string_ui = make_shared<StringOptionMenu>(original_string);
    auto popup = make_shared<ui::Popup>(string_ui);

    ui::run_layout(move(popup), string_ui->done);
}

static void _construct_options_menu(shared_ptr<OuterMenu>& container)
{
    for (unsigned int i = 0; i < ARRAYSZ(entries); ++i)
    {
        const auto& entry = entries[i];
        auto label = make_shared<Text>();

#ifdef USE_TILE_LOCAL
        auto hbox = make_shared<Box>(Box::HORZ);
        hbox->set_cross_alignment(Widget::Align::CENTER);
        auto tile = make_shared<Image>();
        tile->set_tile(tile_def(tileidx_options(entry.id)));
        tile->set_margin_for_sdl(0, 6, 0, 0);
        hbox->add_child(move(tile));
        hbox->add_child(label);
#endif

        label->set_text(formatted_string(entry.label, WHITE));

        auto btn = make_shared<MenuButton>();
#ifdef USE_TILE_LOCAL
        hbox->set_margin_for_sdl(2, 10, 2, 2);
        btn->set_child(move(hbox));
#else
        btn->set_child(move(label));
#endif
        btn->id = entry.id;
        btn->description = entry.description;
        btn->highlight_colour = LIGHTGREY;
        container->add_button(move(btn), 0, i);
    }
}

class UIOptionsMenu : public Widget
{
public:
    UIOptionsMenu() : done(false), selected_option(1)
    {
        m_root = make_shared<Box>(Box::VERT);
        add_internal_child(m_root);
        m_root->set_cross_alignment(Widget::Align::STRETCH);

        auto grid = make_shared<Grid>();
        grid->set_margin_for_crt(0, 0, 1, 0);

        descriptions = make_shared<Switcher>();

        auto mode_prompt = make_shared<Text>("Choices:");
        mode_prompt->set_margin_for_crt(0, 1, 1, 0);
        mode_prompt->set_margin_for_sdl(0, 0, 10, 0);
        options_menu = make_shared<OuterMenu>(true, 1, ARRAYSZ(entries));
        options_menu->set_margin_for_sdl(0, 0, 10, 10);
        options_menu->set_margin_for_crt(0, 0, 1, 0);
        options_menu->descriptions = descriptions;
        _construct_options_menu(options_menu);

#ifdef USE_TILE_LOCAL
        options_menu->min_size().height = TILE_Y * 3;
#else
        options_menu->min_size().height = 2;
#endif

        grid->add_child(move(mode_prompt), 0, 1);
        grid->add_child(options_menu, 1, 1);

        m_root->on_activate_event([this](const ActivateEvent& event) {
            const auto button = static_pointer_cast<const MenuButton>(event.target());
            this->menu_item_activated(button->id);
            return true;
        });

        grid->column_flex_grow(0) = 1;
        grid->column_flex_grow(1) = 10;

        m_root->add_child(move(grid));

        descriptions->set_margin_for_crt(1, 0, 0, 0);
        descriptions->set_margin_for_sdl(10, 0, 0, 0);
        m_root->add_child(descriptions);
    };

    virtual void _render() override;
    virtual SizeReq _get_preferred_size(Direction dim, int prosp_width) override;
    virtual void _allocate_region() override;

    bool has_allocated = false;

    bool done;
    virtual shared_ptr<Widget> get_child_at_offset(int, int) override {
        return m_root;
    }

private:
    void on_show();
    void menu_item_activated(int id);
    
    shared_ptr<Box> m_root;
    shared_ptr<Switcher> descriptions;
    shared_ptr<OuterMenu> options_menu;
    int selected_option;
};

SizeReq UIOptionsMenu::_get_preferred_size(Direction dim, int prosp_width)
{
    return m_root->get_preferred_size(dim, prosp_width);
}

void UIOptionsMenu::_render()
{
    m_root->render();
}

void UIOptionsMenu::_allocate_region()
{
    m_root->allocate_region(m_region);

    if (!has_allocated)
    {
        has_allocated = true;
        on_show();
    }
}

void UIOptionsMenu::on_show()
{
    if (selected_option >= NUM_OPTIONS)
        selected_option = OPTION_TYPE_UNSPECIFIED;

    int id;
    if (selected_option != OPTION_TYPE_UNSPECIFIED)
        id = selected_option;
    else
        id = 0;

    if (auto focus = options_menu->get_button_by_id(id)) {
        options_menu->scroll_button_into_view(focus);
    }

    on_hotkey_event([this](const KeyEvent& ev) {
        const auto keyn = ev.key();
        if (key_is_escape(keyn) || keyn == CK_MOUSE_CMD)
        {   
            int fileOpt = TRUE_BOOL;
            int* ptr = &fileOpt;
            _show_bool_menu(ptr, true);
            if (fileOpt == TRUE_BOOL) { // save on exit
                std::ifstream src("NewOptions.txt", std::ios::binary);
                std::ofstream dst("options.txt", std::ios::binary);
                dst << src.rdbuf();
                std::ofstream clear("NewOptions.txt", std::ofstream::out | std::ofstream::trunc); // clear NewOptions
            }
            done = true;
        }
        return false;
    });
}

void UIOptionsMenu::menu_item_activated(int id)
{
    switch (id)
    {
    case OPTION_TYPE_1: // boolean options
    {
        int fileOpt = TRUE_BOOL; // should pass initial value from options file
        int* ptr = &fileOpt;
        _show_bool_menu(ptr, false);
        std::string optionName = std::string(entries[id-1].description);
        std::ofstream outFile("NewOptions.txt", std::ios_base::app | std::ios_base::out);
        if (fileOpt == FALSE_BOOL) { // opposite of initial
            // change option in file
            optionName += " = false\n";
        } else{
            optionName += " = true\n";
        }
        outFile << optionName;
    }
        break;
    case OPTION_TYPE_2: // string options
    {
        string fileOpt = ""; // should be set to initial value from file
        string oldOpt = fileOpt;
        string* ptr = &fileOpt;
        _show_string_menu(ptr);
        if (fileOpt != oldOpt) {
            std::string optionName = std::string(entries[id - 1].description);
            std::ofstream outFile("NewOptions.txt", std::ios_base::app | std::ios_base::out);
            optionName += " = " + fileOpt + "\n";
            outFile << optionName;
        }
    }
        break;
    case OPTION_TYPE_3: // exit
    {
        int fileOpt = TRUE_BOOL; // should pass initial value from options file
        int* ptr = &fileOpt;
        _show_bool_menu(ptr, true);
        if (fileOpt == TRUE_BOOL) { // opposite of initial
            std::ifstream src("NewOptions.txt", std::ios::binary);
            std::ofstream dst("options.txt", std::ios::binary);
            dst << src.rdbuf();
            std::ofstream clear("NewOptions.txt", std::ofstream::out | std::ofstream::trunc); // clear NewOptions
        }
        done = true;
    }
    case OPTION_TYPE_4: // Exit options
    {
        
    }
        break;
    default:
        done = true;
        break;
    }
}

static void _show_options_menu()
{
    unwind_bool no_more(crawl_state.show_more_prompt, false);

#if defined(USE_TILE_LOCAL) && defined(TOUCH_UI)
    wm->show_keyboard();
#elif defined(USE_TILE_WEB)
    tiles_crt_popup show_as_popup;
#endif

    auto options_ui = make_shared<UIOptionsMenu>();
    auto popup = make_shared<ui::Popup>(options_ui);

    ui::run_layout(move(popup), options_ui->done);
}

void options_menu() {  
    _show_options_menu();
    std::ifstream input("options.txt");
    for (std::string line; getline(input, line); ) {
        read_options(line, false, false);
    }
}

#endif