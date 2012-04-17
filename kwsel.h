
#ifndef __kwsel_h__
#define __kwsel_h__


struct Symbol;
struct Unknown;


struct Unknown *NewKeywordSelectorBuilder(void);
void AppendKeyword(struct Unknown **builder, struct Symbol *kw);
struct Symbol *GetKeywordSelector(struct Unknown *builder, struct Symbol *kw);


extern char *kwSep[2];


#endif /* __kwsel_h__ */
