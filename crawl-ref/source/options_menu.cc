#include "options_menu.h"
#include <string>
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

// core program of options_menu

using namespace ui;

// this should contain all possible option names
enum option_identifier {
    OPTION_TYPE_UNSPECIFIED,
    OPTION_TYPE_1, 
    OPTION_TYPE_2,
    NUM_OPTIONS
};

#ifndef DGAMELAUNCH // does not work with dgamelaunch?

struct options_menu_item
{
    option_identifier id;
    const char* label;
    const char* description;
};

static const options_menu_item entries[] =
{
    {OPTION_TYPE_1, "Test 1", "testing one"},
    {OPTION_TYPE_2, "Test 2", "testing two" },
};

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
        tile->set_tile(tile_def(tileidx_gametype(entry.id))); // need to define options tiles
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

        for (auto& w : options_menu->get_buttons())
        {
            w->on_focusin_event([w, this](const FocusEvent&) {
                return this->on_button_focusin(*w);
            });
        }

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

    bool on_button_focusin(const MenuButton& btn)
    {
        selected_option = btn.id;
        switch (selected_option)
        {
        case OPTION_TYPE_1:
            break;
        case OPTION_TYPE_2:
            break;
        default:
            break;
        }
        return false;
    }

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
            // TODO: check for unsaved options here 
            return done = true;
        }
        // TODO: detect ctrl-s to save
        return false;
    });
}

void UIOptionsMenu::menu_item_activated(int id)
{
    switch (id)
    {
    case OPTION_TYPE_1:
        break;
    case OPTION_TYPE_2:
        // TODO: should enter some method to modify diff option types
        // one for bool, one for string, etc
        break;
    default:
        /// unknown option, bug if this happens, just quit to main menu
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
#endif

void options_menu() {  
    _show_options_menu();
}