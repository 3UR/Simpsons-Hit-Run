#ifndef GFETRANSLATE
#define GFETRANSLATE

//#include "FeTypes.h"
#include "GTxlate.h"
#include "..\..\..\common\strings\unicodeString.h"

class GfeTranslate 
{
public:
    GfeTranslate();
    ~GfeTranslate();
    
    UnicodeString Translate(unsigned long hash_value);
    void SetLanguage(const char* path, int lang);
    void SetLanguage(int lang);
    void SetLanguageDatapath(const char* path);

    void* LoadLanguageDatafile(const char* fileName);
    void UnloadLanguageDatafile(void* data);

private:
    char pathName[255];
    void* languageData;
    int languageFileIndex;
    XLTranslator translator;

};

extern GfeTranslate *gfeTrans;

#endif
