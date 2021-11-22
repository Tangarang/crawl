#include "options_menu.h"
#include <string>
#include "branch-type.h"
#include <map>
#include "skill-type.h"
#include "externs.h"
#include <memory>
#include "end.h"
#include <stdlib.h>
// core program of options_menu

void options_menu() {
    //show_options_menu();
    end(0);
}

enum optType {
    // list of available options goes here
    TEST_OPT0,  
    TEST_OPT1
};

struct option_menu_item {
    optType id;
    const char* label;
    const char* description;
};

static const option_menu_item entries[] =
{
    //{optionName, option label, option desciption},etc
    {TEST_OPT0, "test option 0", "does this work?"},
    {TEST_OPT1, "test option 1", "does this also work"},
};


// should be called by the UI class
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
        tile->set_tile(tile_def(tileidx_gametype(entry.id)));
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

// this needs alot
class UIOptionsMenu : public Widget {
public:
    UIOptionsMenu() {
        
    }
};

// this will need some changes 
static void _show_options_menu()
{
    unwind_bool no_more(crawl_state.show_more_prompt, false);

#if defined(USE_TILE_LOCAL) && defined(TOUCH_UI)
    wm->show_keyboard();
#elif defined(USE_TILE_WEB)
    tiles_crt_popup show_as_popup;
#endif


    auto startup_ui = make_shared<UIStartupMenu>(ng_choice, defaults);
    auto popup = make_shared<ui::Popup>(startup_ui);

    ui::run_layout(move(popup), startup_ui->done);

    if (startup_ui->end_game || crawl_state.seen_hups)
    {
#ifdef USE_TILE_WEB
        tiles.send_exit_reason("cancel");
#endif
        end(0);
    }
}
