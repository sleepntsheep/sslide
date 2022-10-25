#include <fontconfig/fontconfig.h>
#include <stddef.h>
#include <string.h>
#include "font.h"
#include "log.h"

int FontManager_init(FontManager *manager)
{
    if (!FcInit()) {
        panic("FcInit Failed");
        return 1;
    }
    manager->fc_config = FcConfigGetCurrent();
    FcPattern *pat = FcPatternCreate();
    FcObjectSet *os = FcObjectSetBuild(FC_FAMILY, FC_STYLE, FC_FILE, NULL);
    manager->all_font = FcFontList(manager->fc_config, pat, os);
    FcPatternDestroy(pat);
    FcObjectSetDestroy(os);
    return 0;
}

int FontManager_cleanup(FontManager *manager)
{
    FcFontSetDestroy(manager->all_font);
    FcConfigDestroy(manager->fc_config);
    return 0;
}

char *FontManager_get_best_font(FontManager *manager, char **text, size_t nstr) {
    FcCharSet *cs = FcCharSetCreate();
    if (!cs) goto cleanup;
    FcPattern *pat = NULL;
    FcFontSet *matches = NULL;
    char *result = NULL;
    for (size_t i = 0; i < nstr; i++) {
        size_t len = strlen(text[i]);
        for (size_t j = 0; j < len && text[i][j];) {
            FcChar32 cs4;
            int utf8len = FcUtf8ToUcs4((const FcChar8*)text[i] + j, &cs4, len - j);
            j += utf8len;
            FcCharSetAddChar(cs, cs4);
        }
    }
    pat = FcPatternBuild(NULL, FC_CHARSET, FcTypeCharSet, cs, NULL);
    if (!pat) goto cleanup;
    FcResult match_result = 0;
    matches = FcFontSort(manager->fc_config, pat, FcTrue, &cs, &match_result);
    if (!matches) goto cleanup;

    for (int i = 0; i < matches->nfont; i++) {
        FcResult get_result = FcPatternGetString(matches->fonts[i], FC_FILE, 0, (FcChar8**)&result);
        if (get_result != 0) continue;
        if (FcStrStr((FcChar8*)result, (const FcChar8*)".ttf") == NULL) {
            result = NULL;
            continue;
        }
        break;
    }

cleanup:
    if (cs) FcCharSetDestroy(cs);
    if (pat) FcPatternDestroy(pat);
    if (matches) FcFontSetDestroy(matches);
    return result;
}


