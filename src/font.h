#pragma once
#include <fontconfig/fontconfig.h>

typedef struct {
    FcConfig *fc_config;
    FcFontSet *all_font;
} FontManager;

char *FontManager_get_best_font(FontManager *manager, char **text, size_t nstr);

int FontManager_init(FontManager *manager);
int FontManager_cleanup(FontManager *manager);

