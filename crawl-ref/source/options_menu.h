//header file of options_menu
#pragma once
void options_menu();

// this should contain all possible option names
enum option_identifier {
    OPTION_TYPE_UNSPECIFIED,
    // boolean options
    OPTION_TYPE_1,
    // string options
    OPTION_TYPE_2,
    NUM_OPTIONS
};

enum bool_option {
    BOOL_UNSPECIFIED,
    FALSE_BOOL,
    TRUE_BOOL,
    NUM_BOOL
};