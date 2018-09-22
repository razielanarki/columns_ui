#include "stdafx.h"

#define VERSION "1.0.0-beta.1"

#ifndef __clang__
#define DATE ", Date "__DATE__
#else
#define DATE
#endif

VALIDATE_COMPONENT_FILENAME("foo_ui_columns.dll");

DECLARE_COMPONENT_VERSION("Columns UI",

    VERSION,

    "Columns UI\n"
    "Version " VERSION DATE "\n"
    "Copyright (C) 2003-2018 musicmusic and contributors\n"
    "Current version at: yuo.be\n\n"

    "Columns UI SDK version: " UI_EXTENSION_VERSION

);
