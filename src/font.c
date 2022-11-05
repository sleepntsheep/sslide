#include <fontconfig/fontconfig.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include "font.h"
#include "fmt.h"

int get_best_ttf(char *text, char *out, size_t outsize) {
    FcCharSet *cs = FcCharSetCreate();
    FcPattern *pat = NULL;
    FcFontSet *matches = NULL;
    char *result = NULL;
    if (!cs) goto cleanup;
    size_t textlen = strlen(text);
    for (size_t i = 0; i < textlen; ) {
        FcChar32 cs4;
        int utf8len = FcUtf8ToUcs4((const FcChar8*)text + i, &cs4, textlen - i);
        if (cs4 == 0 || utf8len < 0) {
            ffmt(stderr, "Invalid input, is it UTF-8?");
            return -1;
        }
        i += utf8len;
        FcCharSetAddChar(cs, cs4);
    }

    pat = FcPatternBuild(NULL, FC_CHARSET, FcTypeCharSet, cs, NULL);
    if (!pat) goto cleanup;
    FcResult match_result;
    matches = FcFontSort(NULL, pat, FcFalse, &cs, &match_result);
    if (!matches) goto cleanup;

    for (int i = 0; i < matches->nfont; i++) {
        FcResult get_result = FcPatternGetString(matches->fonts[i], FC_FILE, 0, (FcChar8**)&result);
        if (get_result != 0) continue;
        if (FcStrStr((FcChar8*)result, (const FcChar8*)".ttf") == NULL) {
            result = NULL;
        } else {
            break;
        }
    }

cleanup:
    if (result) {
        size_t result_len = 0;
        if (result_len + 1 > outsize) {
            return 1;
        }
        strcpy(out, result);
    }
    if (cs) FcCharSetDestroy(cs);
    if (pat) FcPatternDestroy(pat);
    if (matches) FcFontSetDestroy(matches);
    return 0;
}


