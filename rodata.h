
#ifndef __spk_rodata_h__
#define __spk_rodata_h__


#include "obj.h"
#include "oper.h"


/* symbols */
extern SpkUnknown *Spk_self;


/* selectors */
extern SpkUnknown *Spk___add__;
extern SpkUnknown *Spk___addr__;
extern SpkUnknown *Spk___apply__;
extern SpkUnknown *Spk___apply__;
extern SpkUnknown *Spk___band__;
extern SpkUnknown *Spk___bneg__;
extern SpkUnknown *Spk___bor__;
extern SpkUnknown *Spk___bxor__;
extern SpkUnknown *Spk___div__;
extern SpkUnknown *Spk___eq__;
extern SpkUnknown *Spk___ge__;
extern SpkUnknown *Spk___gt__;
extern SpkUnknown *Spk___ind__;
extern SpkUnknown *Spk___index__;
extern SpkUnknown *Spk___le__;
extern SpkUnknown *Spk___lneg__;
extern SpkUnknown *Spk___lshift__;
extern SpkUnknown *Spk___lt__;
extern SpkUnknown *Spk___mod__;
extern SpkUnknown *Spk___mul__;
extern SpkUnknown *Spk___ne__;
extern SpkUnknown *Spk___neg__;
extern SpkUnknown *Spk___pos__;
extern SpkUnknown *Spk___pred__;
extern SpkUnknown *Spk___rshift__;
extern SpkUnknown *Spk___str__;
extern SpkUnknown *Spk___sub__;
extern SpkUnknown *Spk___succ__;
extern SpkUnknown *Spk__import;
extern SpkUnknown *Spk__init;
extern SpkUnknown *Spk__predef;
extern SpkUnknown *Spk__thunk;
extern SpkUnknown *Spk_badExpr;
extern SpkUnknown *Spk_blockCopy;
extern SpkUnknown *Spk_cannotReenterBlock;
extern SpkUnknown *Spk_cannotReturn;
extern SpkUnknown *Spk_compoundExpression;
extern SpkUnknown *Spk_doesNotUnderstand;
extern SpkUnknown *Spk_failOnError;
extern SpkUnknown *Spk_importModule;
extern SpkUnknown *Spk_importPackage;
extern SpkUnknown *Spk_init;
extern SpkUnknown *Spk_main;
extern SpkUnknown *Spk_mustBeBoolean;
extern SpkUnknown *Spk_mustBeSymbol;
extern SpkUnknown *Spk_mustBeTuple;
extern SpkUnknown *Spk_new;
extern SpkUnknown *Spk_noRunnableFiber;
extern SpkUnknown *Spk_numArgs;
extern SpkUnknown *Spk_obj;
extern SpkUnknown *Spk_printString;
extern SpkUnknown *Spk_recursiveDoesNotUnderstand;
extern SpkUnknown *Spk_redefinedSymbol;
extern SpkUnknown *Spk_source;
extern SpkUnknown *Spk_undefinedSymbol;
extern SpkUnknown *Spk_unknownOpcode;
extern SpkUnknown *Spk_wrongNumberOfArguments;

extern SpkUnknown *Spk_Boolean;
extern SpkUnknown *Spk_False;
extern SpkUnknown *Spk_True;

typedef struct SpkSpecialSelector {
    SpkUnknown **selector;
    size_t argumentCount;
} SpkSpecialSelector;

extern SpkSpecialSelector Spk_operSelectors[Spk_NUM_OPER];
extern SpkSpecialSelector Spk_operCallSelectors[Spk_NUM_CALL_OPER];


/* integers */
extern SpkUnknown *Spk_1;


/* strings */
extern SpkUnknown *Spk_emptyString;
extern SpkUnknown *Spk_unknownSelector;
extern SpkUnknown *Spk_unknownSourcePathname;


/* arguments */
extern SpkUnknown *Spk_emptyArgs;


int Spk_InitSymbols(void);
int Spk_InitReadOnlyData(void);
SpkUnknown *Spk_ParseSelector(const char *);


#endif /* __spk_rodata_h__ */
