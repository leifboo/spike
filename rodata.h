
#ifndef __rodata_h__
#define __rodata_h__


#include "obj.h"
#include "oper.h"


/* symbols */
extern struct Symbol *self;


/* selectors */
extern struct Symbol *__add__;
extern struct Symbol *__addr__;
extern struct Symbol *__apply__;
extern struct Symbol *__apply__;
extern struct Symbol *__band__;
extern struct Symbol *__bneg__;
extern struct Symbol *__bor__;
extern struct Symbol *__bxor__;
extern struct Symbol *__div__;
extern struct Symbol *__eq__;
extern struct Symbol *__ge__;
extern struct Symbol *__gt__;
extern struct Symbol *__ind__;
extern struct Symbol *__index__;
extern struct Symbol *__le__;
extern struct Symbol *__lneg__;
extern struct Symbol *__lshift__;
extern struct Symbol *__lt__;
extern struct Symbol *__mod__;
extern struct Symbol *__mul__;
extern struct Symbol *__ne__;
extern struct Symbol *__neg__;
extern struct Symbol *__pos__;
extern struct Symbol *__pred__;
extern struct Symbol *__rshift__;
extern struct Symbol *__str__;
extern struct Symbol *__sub__;
extern struct Symbol *__succ__;
extern struct Symbol *_import;
extern struct Symbol *_xinit;
extern struct Symbol *_predef;
extern struct Symbol *_thunk;
extern struct Symbol *badExpr;
extern struct Symbol *blockCopy;
extern struct Symbol *cannotReenterBlock;
extern struct Symbol *cannotReturn;
extern struct Symbol *compoundExpression;
extern struct Symbol *doesNotUnderstand;
extern struct Symbol *failOnError;
extern struct Symbol *importModule;
extern struct Symbol *importPackage;
extern struct Symbol *xinit;
extern struct Symbol *klass;
extern struct Symbol *xmain;
extern struct Symbol *mustBeBoolean;
extern struct Symbol *mustBeSymbol;
extern struct Symbol *mustBeTuple;
extern struct Symbol *new;
extern struct Symbol *noRunnableFiber;
extern struct Symbol *numArgs;
extern struct Symbol *printString;
extern struct Symbol *recursiveDoesNotUnderstand;
extern struct Symbol *redefinedSymbol;
extern struct Symbol *source;
extern struct Symbol *undefinedSymbol;
extern struct Symbol *unknownOpcode;
extern struct Symbol *wrongNumberOfArguments;

/* selectors -- class names */
extern struct Symbol *Boolean;
extern struct Symbol *False;
extern struct Symbol *True;

/* selectors -- parser */
extern struct Symbol *concat;
extern struct Symbol *declSpecs;
extern struct Symbol *exprAnd;
extern struct Symbol *exprAssign;
extern struct Symbol *exprAttr;
extern struct Symbol *exprAttrVar;
extern struct Symbol *exprBinaryOp;
extern struct Symbol *exprBlock;
extern struct Symbol *exprCall;
extern struct Symbol *exprCompound;
extern struct Symbol *exprCond;
extern struct Symbol *exprId;
extern struct Symbol *exprKeyword;
extern struct Symbol *exprLiteral;
extern struct Symbol *exprName;
extern struct Symbol *exprNI;
extern struct Symbol *exprOr;
extern struct Symbol *exprPostOp;
extern struct Symbol *exprPreOp;
extern struct Symbol *exprUnaryOp;
extern struct Symbol *isSpec;
extern struct Symbol *left;
extern struct Symbol *next;
extern struct Symbol *nextArg;
extern struct Symbol *operAdd;
extern struct Symbol *operAddr;
extern struct Symbol *operApply;
extern struct Symbol *operBAnd;
extern struct Symbol *operBNeg;
extern struct Symbol *operBOr;
extern struct Symbol *operBXOr;
extern struct Symbol *operDiv;
extern struct Symbol *operEq;
extern struct Symbol *operGE;
extern struct Symbol *operGT;
extern struct Symbol *operInd;
extern struct Symbol *operIndex;
extern struct Symbol *operLE;
extern struct Symbol *operLNeg;
extern struct Symbol *operLShift;
extern struct Symbol *operLT;
extern struct Symbol *operMod;
extern struct Symbol *operMul;
extern struct Symbol *operNE;
extern struct Symbol *operNeg;
extern struct Symbol *operPos;
extern struct Symbol *operPred;
extern struct Symbol *operRShift;
extern struct Symbol *operSub;
extern struct Symbol *operSucc;
extern struct Symbol *stmtBreak;
extern struct Symbol *stmtCompound;
extern struct Symbol *stmtContinue;
extern struct Symbol *stmtDefClass;
extern struct Symbol *stmtDefMethod;
extern struct Symbol *stmtDefModule;
extern struct Symbol *stmtDefSpec;
extern struct Symbol *stmtDefVar;
extern struct Symbol *stmtDoWhile;
extern struct Symbol *stmtExpr;
extern struct Symbol *stmtFor;
extern struct Symbol *stmtIfElse;
extern struct Symbol *stmtPragmaSource;
extern struct Symbol *stmtReturn;
extern struct Symbol *stmtWhile;
extern struct Symbol *stmtYield;
extern struct Symbol *symbolNodeForSymbol;

typedef struct SpecialSelector {
    struct Symbol **selector;
    size_t argumentCount;
} SpecialSelector;

extern SpecialSelector operSelectors[NUM_OPER];
extern SpecialSelector operCallSelectors[NUM_CALL_OPER];


/* integers */
extern struct Integer *zero;
extern struct Integer *one;


/* strings */
extern struct String *emptyString;
extern struct String *unknownSelector;
extern struct String *unknownSourcePathname;


/* arguments */
extern struct Array *emptyArgs;


int InitSymbols(void);
int InitReadOnlyData(void);
struct Symbol *ParseSelector(const char *);


#endif /* __rodata_h__ */
