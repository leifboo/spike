
#ifndef __rodata_h__
#define __rodata_h__


#include "obj.h"
#include "oper.h"


/* symbols */
extern Unknown *self;


/* selectors */
extern Unknown *__add__;
extern Unknown *__addr__;
extern Unknown *__apply__;
extern Unknown *__apply__;
extern Unknown *__band__;
extern Unknown *__bneg__;
extern Unknown *__bor__;
extern Unknown *__bxor__;
extern Unknown *__div__;
extern Unknown *__eq__;
extern Unknown *__ge__;
extern Unknown *__gt__;
extern Unknown *__ind__;
extern Unknown *__index__;
extern Unknown *__le__;
extern Unknown *__lneg__;
extern Unknown *__lshift__;
extern Unknown *__lt__;
extern Unknown *__mod__;
extern Unknown *__mul__;
extern Unknown *__ne__;
extern Unknown *__neg__;
extern Unknown *__pos__;
extern Unknown *__pred__;
extern Unknown *__rshift__;
extern Unknown *__str__;
extern Unknown *__sub__;
extern Unknown *__succ__;
extern Unknown *_import;
extern Unknown *_init;
extern Unknown *_predef;
extern Unknown *_thunk;
extern Unknown *badExpr;
extern Unknown *blockCopy;
extern Unknown *cannotReenterBlock;
extern Unknown *cannotReturn;
extern Unknown *compoundExpression;
extern Unknown *doesNotUnderstand;
extern Unknown *failOnError;
extern Unknown *importModule;
extern Unknown *importPackage;
extern Unknown *init;
extern Unknown *_main;
extern Unknown *mustBeBoolean;
extern Unknown *mustBeSymbol;
extern Unknown *mustBeTuple;
extern Unknown *new;
extern Unknown *noRunnableFiber;
extern Unknown *numArgs;
extern Unknown *printString;
extern Unknown *recursiveDoesNotUnderstand;
extern Unknown *redefinedSymbol;
extern Unknown *source;
extern Unknown *undefinedSymbol;
extern Unknown *unknownOpcode;
extern Unknown *wrongNumberOfArguments;

/* selectors -- class names */
extern Unknown *Boolean;
extern Unknown *False;
extern Unknown *True;

/* selectors -- parser */
extern Unknown *concat;
extern Unknown *declSpecs;
extern Unknown *exprAnd;
extern Unknown *exprAssign;
extern Unknown *exprAttr;
extern Unknown *exprAttrVar;
extern Unknown *exprBinaryOp;
extern Unknown *exprBlock;
extern Unknown *exprCall;
extern Unknown *exprCompound;
extern Unknown *exprCond;
extern Unknown *exprId;
extern Unknown *exprKeyword;
extern Unknown *exprLiteral;
extern Unknown *exprName;
extern Unknown *exprNI;
extern Unknown *exprOr;
extern Unknown *exprPostOp;
extern Unknown *exprPreOp;
extern Unknown *exprUnaryOp;
extern Unknown *isSpec;
extern Unknown *left;
extern Unknown *next;
extern Unknown *nextArg;
extern Unknown *operAdd;
extern Unknown *operAddr;
extern Unknown *operApply;
extern Unknown *operBAnd;
extern Unknown *operBNeg;
extern Unknown *operBOr;
extern Unknown *operBXOr;
extern Unknown *operDiv;
extern Unknown *operEq;
extern Unknown *operGE;
extern Unknown *operGT;
extern Unknown *operInd;
extern Unknown *operIndex;
extern Unknown *operLE;
extern Unknown *operLNeg;
extern Unknown *operLShift;
extern Unknown *operLT;
extern Unknown *operMod;
extern Unknown *operMul;
extern Unknown *operNE;
extern Unknown *operNeg;
extern Unknown *operPos;
extern Unknown *operPred;
extern Unknown *operRShift;
extern Unknown *operSub;
extern Unknown *operSucc;
extern Unknown *stmtBreak;
extern Unknown *stmtCompound;
extern Unknown *stmtContinue;
extern Unknown *stmtDefClass;
extern Unknown *stmtDefMethod;
extern Unknown *stmtDefModule;
extern Unknown *stmtDefSpec;
extern Unknown *stmtDefVar;
extern Unknown *stmtDoWhile;
extern Unknown *stmtExpr;
extern Unknown *stmtFor;
extern Unknown *stmtIfElse;
extern Unknown *stmtPragmaSource;
extern Unknown *stmtReturn;
extern Unknown *stmtWhile;
extern Unknown *stmtYield;
extern Unknown *symbolNodeForSymbol;

typedef struct SpecialSelector {
    Unknown **selector;
    size_t argumentCount;
} SpecialSelector;

extern SpecialSelector operSelectors[NUM_OPER];
extern SpecialSelector operCallSelectors[NUM_CALL_OPER];


/* integers */
extern Unknown *zero;
extern Unknown *one;


/* strings */
extern Unknown *emptyString;
extern Unknown *unknownSelector;
extern Unknown *unknownSourcePathname;


/* arguments */
extern Unknown *emptyArgs;


int InitSymbols(void);
int InitReadOnlyData(void);
Unknown *ParseSelector(const char *);
void ReleaseSymbols(void);
void ReleaseReadOnlyData(void);


#endif /* __rodata_h__ */
