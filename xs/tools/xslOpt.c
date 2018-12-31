#include "xsl.h"
#include "xslOpt.xs.h"

#ifdef mxDebug
	#define xsElseThrow(_ASSERTION) \
		((void)((_ASSERTION) || (fxThrowMessage(the,(char *)__FILE__,__LINE__,XS_UNKNOWN_ERROR,"%s",strerror(errno)), 1)))
#else
	#define xsElseThrow(_ASSERTION) \
		((void)((_ASSERTION) || (fxThrowMessage(the,NULL,0,XS_UNKNOWN_ERROR,"%s",strerror(errno)), 1)))
#endif

typedef txS1* (*txCodeReader)(xsMachine* the, txU1 id, txS1* p);
typedef txS1* (*txCodeWriter)(xsMachine* the, txU1 id, txS1* p);

static txS1* fxReadCode(xsMachine* the, txU1 id, txS1* p);
static txS1* fxReadCodeKey(xsMachine* the, txU1 id, txS1* p);
static txS1* fxReadCodeNumber(xsMachine* the, txU1 id, txS1* p);
static txS1* fxReadCodeS1(xsMachine* the, txU1 id, txS1* p);
static txS1* fxReadCodeS2(xsMachine* the, txU1 id, txS1* p);
static txS1* fxReadCodeS4(xsMachine* the, txU1 id, txS1* p);
static txS1* fxReadCodeString1(xsMachine* the, txU1 id, txS1* p);
static txS1* fxReadCodeString2(xsMachine* the, txU1 id, txS1* p);
static txS1* fxReadCodeU1(xsMachine* the, txU1 id, txS1* p);
static txS1* fxReadCodeU2(xsMachine* the, txU1 id, txS1* p);
static void fxReadCodes(xsMachine* the, void* buffer, txSize size);
static void fxReadHosts(xsMachine* the, void* buffer, txSize size);

static txS1* fxWriteCode(xsMachine* the, txU1 id, txS1* p);
static txS1* fxWriteCodeKey(xsMachine* the, txU1 id, txS1* p);
static txS1* fxWriteCodeNumber(xsMachine* the, txU1 id, txS1* p);
static txS1* fxWriteCodeS1(xsMachine* the, txU1 id, txS1* p);
static txS1* fxWriteCodeS2(xsMachine* the, txU1 id, txS1* p);
static txS1* fxWriteCodeS4(xsMachine* the, txU1 id, txS1* p);
static txS1* fxWriteCodeString1(xsMachine* the, txU1 id, txS1* p);
static txS1* fxWriteCodeString2(xsMachine* the, txU1 id, txS1* p);
static txS1* fxWriteCodeU1(xsMachine* the, txU1 id, txS1* p);
static txS1* fxWriteCodeU2(xsMachine* the, txU1 id, txS1* p);
static void fxWriteCodes(xsMachine* the, txS1* p);

txID gxCodeConstructors[XS_CODE_COUNT];

const txInteger gxCodeIDs[XS_CODE_COUNT] = {
	/* XS_NO_CODE */ XS_NO_CODE,
	/* XS_CODE_ADD */ XS_CODE_ADD,
	/* XS_CODE_ARGUMENT */ XS_CODE_ARGUMENT,
	/* XS_CODE_ARGUMENTS */ XS_CODE_ARGUMENTS,
	/* XS_CODE_ARGUMENTS_SLOPPY */ XS_CODE_ARGUMENTS_SLOPPY,
	/* XS_CODE_ARGUMENTS_STRICT */ XS_CODE_ARGUMENTS_STRICT,
	/* XS_CODE_ARRAY */ XS_CODE_ARRAY,
	/* XS_CODE_ASYNC_FUNCTION */ XS_CODE_ASYNC_FUNCTION,
	/* XS_CODE_ASYNC_GENERATOR_FUNCTION */ XS_CODE_ASYNC_GENERATOR_FUNCTION,
	/* XS_CODE_AT */ XS_CODE_AT,
	/* XS_CODE_AWAIT */ XS_CODE_AWAIT,
	/* XS_CODE_BEGIN_SLOPPY */ XS_CODE_BEGIN_SLOPPY,
	/* XS_CODE_BEGIN_STRICT */ XS_CODE_BEGIN_STRICT,
	/* XS_CODE_BEGIN_STRICT_BASE */ XS_CODE_BEGIN_STRICT_BASE,
	/* XS_CODE_BEGIN_STRICT_DERIVED */ XS_CODE_BEGIN_STRICT_DERIVED,
	/* XS_CODE_BIT_AND */ XS_CODE_BIT_AND,
	/* XS_CODE_BIT_NOT */ XS_CODE_BIT_NOT,
	/* XS_CODE_BIT_OR */ XS_CODE_BIT_OR,
	/* XS_CODE_BIT_XOR */ XS_CODE_BIT_XOR,
	/* XS_CODE_BRANCH_1 */ XS_CODE_BRANCH_1,
	/* XS_CODE_BRANCH_2 */ XS_CODE_BRANCH_1,
	/* XS_CODE_BRANCH_4 */ XS_CODE_BRANCH_1,
	/* XS_CODE_BRANCH_ELSE_1 */ XS_CODE_BRANCH_ELSE_1,
	/* XS_CODE_BRANCH_ELSE_2 */ XS_CODE_BRANCH_ELSE_1,
	/* XS_CODE_BRANCH_ELSE_4 */ XS_CODE_BRANCH_ELSE_1,
	/* XS_CODE_BRANCH_IF_1 */ XS_CODE_BRANCH_IF_1,
	/* XS_CODE_BRANCH_IF_2 */ XS_CODE_BRANCH_IF_1,
	/* XS_CODE_BRANCH_IF_4 */ XS_CODE_BRANCH_IF_1,
	/* XS_CODE_BRANCH_STATUS_1 */ XS_CODE_BRANCH_STATUS_1,
	/* XS_CODE_BRANCH_STATUS_2 */ XS_CODE_BRANCH_STATUS_1,
	/* XS_CODE_BRANCH_STATUS_4 */ XS_CODE_BRANCH_STATUS_1,
	/* XS_CODE_CALL */ XS_CODE_CALL,
	/* XS_CODE_CALL_TAIL */ XS_CODE_CALL_TAIL,
	/* XS_CODE_CATCH_1 */ XS_CODE_CATCH_1,
	/* XS_CODE_CATCH_2 */ XS_CODE_CATCH_1,
	/* XS_CODE_CATCH_4 */ XS_CODE_CATCH_1,
	/* XS_CODE_CHECK_INSTANCE */ XS_CODE_CHECK_INSTANCE,
	/* XS_CODE_CLASS */ XS_CODE_CLASS,
	/* XS_CODE_CODE_1 */ XS_CODE_CODE_1,
	/* XS_CODE_CODE_2 */ XS_CODE_CODE_1,
	/* XS_CODE_CODE_4 */ XS_CODE_CODE_1,
	/* XS_CODE_CODE_ARCHIVE_1 */ XS_CODE_CODE_ARCHIVE_1,
	/* XS_CODE_CODE_ARCHIVE_2 */ XS_CODE_CODE_ARCHIVE_1,
	/* XS_CODE_CODE_ARCHIVE_4 */ XS_CODE_CODE_ARCHIVE_1,
	/* XS_CODE_CONST_CLOSURE_1 */ XS_CODE_CONST_CLOSURE_1,
	/* XS_CODE_CONST_CLOSURE_2 */ XS_CODE_CONST_CLOSURE_1,
	/* XS_CODE_CONST_LOCAL_1 */ XS_CODE_CONST_LOCAL_1,
	/* XS_CODE_CONST_LOCAL_2 */ XS_CODE_CONST_LOCAL_1,
	/* XS_CODE_CONSTRUCTOR_FUNCTION */ XS_CODE_CONSTRUCTOR_FUNCTION,
	/* XS_CODE_CURRENT */ XS_CODE_CURRENT,
	/* XS_CODE_DEBUGGER */ XS_CODE_DEBUGGER,
	/* XS_CODE_DECREMENT */ XS_CODE_DECREMENT,
	/* XS_CODE_DELETE_PROPERTY */ XS_CODE_DELETE_PROPERTY,
	/* XS_CODE_DELETE_PROPERTY_AT */ XS_CODE_DELETE_PROPERTY_AT,
	/* XS_CODE_DELETE_SUPER */ XS_CODE_DELETE_SUPER,
	/* XS_CODE_DELETE_SUPER_AT */ XS_CODE_DELETE_SUPER_AT,
	/* XS_CODE_DIVIDE */ XS_CODE_DIVIDE,
	/* XS_CODE_DUB */ XS_CODE_DUB,
	/* XS_CODE_DUB_AT */ XS_CODE_DUB_AT,
	/* XS_CODE_END */ XS_CODE_END,
	/* XS_CODE_END_ARROW */ XS_CODE_END_ARROW,
	/* XS_CODE_END_BASE */ XS_CODE_END_BASE,
	/* XS_CODE_END_DERIVED */ XS_CODE_END_DERIVED,
	/* XS_CODE_ENVIRONMENT */ XS_CODE_ENVIRONMENT,
	/* XS_CODE_EQUAL */ XS_CODE_EQUAL,
	/* XS_CODE_EVAL */ XS_CODE_EVAL,
	/* XS_CODE_EVAL_ENVIRONMENT */ XS_CODE_EVAL_ENVIRONMENT,
	/* XS_CODE_EVAL_REFERENCE */ XS_CODE_EVAL_REFERENCE,
	/* XS_CODE_EXCEPTION */ XS_CODE_EXCEPTION,
	/* XS_CODE_EXPONENTIATION */ XS_CODE_EXPONENTIATION,
	/* XS_CODE_EXTEND */ XS_CODE_EXTEND,
	/* XS_CODE_FALSE */ XS_CODE_FALSE,
	/* XS_CODE_FILE */ XS_CODE_FILE,
	/* XS_CODE_FOR_AWAIT_OF */ XS_CODE_FOR_AWAIT_OF,
	/* XS_CODE_FOR_IN */ XS_CODE_FOR_IN,
	/* XS_CODE_FOR_OF */ XS_CODE_FOR_OF,
	/* XS_CODE_FUNCTION */ XS_CODE_FUNCTION,
	/* XS_CODE_GENERATOR_FUNCTION */ XS_CODE_GENERATOR_FUNCTION,
	/* XS_CODE_GET_CLOSURE_1 */ XS_CODE_GET_CLOSURE_1,
	/* XS_CODE_GET_CLOSURE_2 */ XS_CODE_GET_CLOSURE_1,
	/* XS_CODE_GET_LOCAL_1 */ XS_CODE_GET_LOCAL_1,
	/* XS_CODE_GET_LOCAL_2 */ XS_CODE_GET_LOCAL_1,
	/* XS_CODE_GET_PROPERTY */ XS_CODE_GET_PROPERTY,
	/* XS_CODE_GET_PROPERTY_AT */ XS_CODE_GET_PROPERTY_AT,
	/* XS_CODE_GET_SUPER */ XS_CODE_GET_SUPER,
	/* XS_CODE_GET_SUPER_AT */ XS_CODE_GET_SUPER_AT,
	/* XS_CODE_GET_THIS */ XS_CODE_GET_THIS,
	/* XS_CODE_GET_VARIABLE */ XS_CODE_GET_VARIABLE,
	/* XS_CODE_GLOBAL */ XS_CODE_GLOBAL,
	/* XS_CODE_HOST */ XS_CODE_HOST,
	/* XS_CODE_IN */ XS_CODE_IN,
	/* XS_CODE_INCREMENT */ XS_CODE_INCREMENT,
	/* XS_CODE_INSTANCEOF */ XS_CODE_INSTANCEOF,
	/* XS_CODE_INSTANTIATE */ XS_CODE_INSTANTIATE,
	/* XS_CODE_INTEGER_1 */ XS_CODE_INTEGER_1,
	/* XS_CODE_INTEGER_2 */ XS_CODE_INTEGER_1,
	/* XS_CODE_INTEGER_4 */ XS_CODE_INTEGER_1,
	/* XS_CODE_INTRINSIC */ XS_CODE_INTRINSIC,
	/* XS_CODE_LEFT_SHIFT */ XS_CODE_LEFT_SHIFT,
	/* XS_CODE_LESS */ XS_CODE_LESS,
	/* XS_CODE_LESS_EQUAL */ XS_CODE_LESS_EQUAL,
	/* XS_CODE_LET_CLOSURE_1 */ XS_CODE_LET_CLOSURE_1,
	/* XS_CODE_LET_CLOSURE_2 */ XS_CODE_LET_CLOSURE_1,
	/* XS_CODE_LET_LOCAL_1 */ XS_CODE_LET_LOCAL_1,
	/* XS_CODE_LET_LOCAL_2 */ XS_CODE_LET_LOCAL_1,
	/* XS_CODE_LINE */ XS_CODE_LINE,
	/* XS_CODE_MINUS */ XS_CODE_MINUS,
	/* XS_CODE_MODULE */ XS_CODE_MODULE,
	/* XS_CODE_MODULO */ XS_CODE_MODULO,
	/* XS_CODE_MORE */ XS_CODE_MORE,
	/* XS_CODE_MORE_EQUAL */ XS_CODE_MORE_EQUAL,
	/* XS_CODE_MULTIPLY */ XS_CODE_MULTIPLY,
	/* XS_CODE_NAME */ XS_CODE_NAME,
	/* XS_CODE_NEW */ XS_CODE_NEW,
	/* XS_CODE_NEW_CLOSURE */ XS_CODE_NEW_CLOSURE,
	/* XS_CODE_NEW_LOCAL */ XS_CODE_NEW_LOCAL,
	/* XS_CODE_NEW_PROPERTY */ XS_CODE_NEW_PROPERTY,
	/* XS_CODE_NEW_TEMPORARY */ XS_CODE_NEW_TEMPORARY,
	/* XS_CODE_NOT */ XS_CODE_NOT,
	/* XS_CODE_NOT_EQUAL */ XS_CODE_NOT_EQUAL,
	/* XS_CODE_NULL */ XS_CODE_NULL,
	/* XS_CODE_NUMBER */ XS_CODE_NUMBER,
	/* XS_CODE_OBJECT */ XS_CODE_OBJECT,
	/* XS_CODE_PLUS */ XS_CODE_PLUS,
	/* XS_CODE_POP */ XS_CODE_POP,
	/* XS_CODE_PROGRAM_ENVIRONMENT */ XS_CODE_PROGRAM_ENVIRONMENT,
	/* XS_CODE_PROGRAM_REFERENCE */ XS_CODE_PROGRAM_REFERENCE,
	/* XS_CODE_PULL_CLOSURE_1 */ XS_CODE_PULL_CLOSURE_1,
	/* XS_CODE_PULL_CLOSURE_2 */ XS_CODE_PULL_CLOSURE_1,
	/* XS_CODE_PULL_LOCAL_1 */ XS_CODE_PULL_LOCAL_1,
	/* XS_CODE_PULL_LOCAL_2 */ XS_CODE_PULL_LOCAL_1,
	/* XS_CODE_REFRESH_CLOSURE_1 */ XS_CODE_REFRESH_CLOSURE_1,
	/* XS_CODE_REFRESH_CLOSURE_2 */ XS_CODE_REFRESH_CLOSURE_1,
	/* XS_CODE_REFRESH_LOCAL_1 */ XS_CODE_REFRESH_LOCAL_1,
	/* XS_CODE_REFRESH_LOCAL_2 */ XS_CODE_REFRESH_LOCAL_1,
	/* XS_CODE_RESERVE_1 */ XS_CODE_RESERVE_1,
	/* XS_CODE_RESERVE_2 */ XS_CODE_RESERVE_1,
	/* XS_CODE_RESET_CLOSURE_1 */ XS_CODE_RESET_CLOSURE_1,
	/* XS_CODE_RESET_CLOSURE_2 */ XS_CODE_RESET_CLOSURE_1,
	/* XS_CODE_RESET_LOCAL_1 */ XS_CODE_RESET_LOCAL_1,
	/* XS_CODE_RESET_LOCAL_2 */ XS_CODE_RESET_LOCAL_1,
	/* XS_CODE_RESULT */ XS_CODE_RESULT,
	/* XS_CODE_RETHROW */ XS_CODE_RETHROW,
	/* XS_CODE_RETRIEVE_1 */ XS_CODE_RETRIEVE_1,
	/* XS_CODE_RETRIEVE_2 */ XS_CODE_RETRIEVE_1,
	/* XS_CODE_RETRIEVE_TARGET */ XS_CODE_RETRIEVE_TARGET,
	/* XS_CODE_RETRIEVE_THIS */ XS_CODE_RETRIEVE_THIS,
	/* XS_CODE_RETURN */ XS_CODE_RETURN,
	/* XS_CODE_SET_CLOSURE_1 */ XS_CODE_SET_CLOSURE_1,
	/* XS_CODE_SET_CLOSURE_2 */ XS_CODE_SET_CLOSURE_1,
	/* XS_CODE_SET_LOCAL_1 */ XS_CODE_SET_LOCAL_1,
	/* XS_CODE_SET_LOCAL_2 */ XS_CODE_SET_LOCAL_1,
	/* XS_CODE_SET_PROPERTY */ XS_CODE_SET_PROPERTY,
	/* XS_CODE_SET_PROPERTY_AT */ XS_CODE_SET_PROPERTY_AT,
	/* XS_CODE_SET_SUPER */ XS_CODE_SET_SUPER,
	/* XS_CODE_SET_SUPER_AT */ XS_CODE_SET_SUPER_AT,
	/* XS_CODE_SET_THIS */ XS_CODE_SET_THIS,
	/* XS_CODE_SET_VARIABLE */ XS_CODE_SET_VARIABLE,
	/* XS_CODE_SIGNED_RIGHT_SHIFT */ XS_CODE_SIGNED_RIGHT_SHIFT,
	/* XS_CODE_START_ASYNC */ XS_CODE_START_ASYNC,
	/* XS_CODE_START_ASYNC_GENERATOR */ XS_CODE_START_ASYNC_GENERATOR,
	/* XS_CODE_START_GENERATOR */ XS_CODE_START_GENERATOR,
	/* XS_CODE_STORE_1 */ XS_CODE_STORE_1,
	/* XS_CODE_STORE_2 */ XS_CODE_STORE_1,
	/* XS_CODE_STORE_ARROW */ XS_CODE_STORE_ARROW,
	/* XS_CODE_STRICT_EQUAL */ XS_CODE_STRICT_EQUAL,
	/* XS_CODE_STRICT_NOT_EQUAL */ XS_CODE_STRICT_NOT_EQUAL,
	/* XS_CODE_STRING_1 */ XS_CODE_STRING_1,
	/* XS_CODE_STRING_2 */ XS_CODE_STRING_1,
	/* XS_CODE_STRING_ARCHIVE_1 */ XS_CODE_STRING_ARCHIVE_1,
	/* XS_CODE_STRING_ARCHIVE_2 */ XS_CODE_STRING_ARCHIVE_1,
	/* XS_CODE_SUBTRACT */ XS_CODE_SUBTRACT,
	/* XS_CODE_SUPER */ XS_CODE_SUPER,
	/* XS_CODE_SWAP */ XS_CODE_SWAP,
	/* XS_CODE_SYMBOL */ XS_CODE_SYMBOL,
	/* XS_CODE_TARGET */ XS_CODE_TARGET,
	/* XS_CODE_TEMPLATE */ XS_CODE_TEMPLATE,
	/* XS_CODE_THIS */ XS_CODE_THIS,
	/* XS_CODE_THROW */ XS_CODE_THROW,
	/* XS_CODE_THROW_STATUS */ XS_CODE_THROW_STATUS,
	/* XS_CODE_TO_INSTANCE */ XS_CODE_TO_INSTANCE,
	/* XS_CODE_TRANSFER */ XS_CODE_TRANSFER,
	/* XS_CODE_TRUE */ XS_CODE_TRUE,
	/* XS_CODE_TYPEOF */ XS_CODE_TYPEOF,
	/* XS_CODE_UNCATCH */ XS_CODE_UNCATCH,
	/* XS_CODE_UNDEFINED */ XS_CODE_UNDEFINED,
	/* XS_CODE_UNSIGNED_RIGHT_SHIFT */ XS_CODE_UNSIGNED_RIGHT_SHIFT,
	/* XS_CODE_UNWIND_1 */ XS_CODE_UNWIND_1,
	/* XS_CODE_UNWIND_2 */ XS_CODE_UNWIND_1,
	/* XS_CODE_VAR_CLOSURE_1 */ XS_CODE_VAR_CLOSURE_1,
	/* XS_CODE_VAR_CLOSURE_2 */ XS_CODE_VAR_CLOSURE_1,
	/* XS_CODE_VAR_LOCAL_1 */ XS_CODE_VAR_LOCAL_1,
	/* XS_CODE_VAR_LOCAL_2 */ XS_CODE_VAR_LOCAL_1,
	/* XS_CODE_VOID */ XS_CODE_VOID,
	/* XS_CODE_WITH */ XS_CODE_WITH,
	/* XS_CODE_WITHOUT */ XS_CODE_WITHOUT,
	/* XS_CODE_YIELD */ XS_CODE_YIELD,
};

const txCodeReader gxCodeReaders[XS_CODE_COUNT] = {
	/* XS_NO_CODE */ NULL,
	/* XS_CODE_ADD */ fxReadCode,
	/* XS_CODE_ARGUMENT */ fxReadCodeS1,
	/* XS_CODE_ARGUMENTS */ fxReadCodeS1,
	/* XS_CODE_ARGUMENTS_SLOPPY */ fxReadCodeS1,
	/* XS_CODE_ARGUMENTS_STRICT */ fxReadCodeS1,
	/* XS_CODE_ARRAY */ fxReadCode,
	/* XS_CODE_ASYNC_FUNCTION */ fxReadCodeKey,
	/* XS_CODE_ASYNC_GENERATOR_FUNCTION */ fxReadCodeKey,
	/* XS_CODE_AT */ fxReadCode,
	/* XS_CODE_AWAIT */ fxReadCode,
	/* XS_CODE_BEGIN_SLOPPY */ fxReadCodeS1,
	/* XS_CODE_BEGIN_STRICT */ fxReadCodeS1,
	/* XS_CODE_BEGIN_STRICT_BASE */ fxReadCodeS1,
	/* XS_CODE_BEGIN_STRICT_DERIVED */ fxReadCodeS1,
	/* XS_CODE_BIT_AND */ fxReadCode,
	/* XS_CODE_BIT_NOT */ fxReadCode,
	/* XS_CODE_BIT_OR */ fxReadCode,
	/* XS_CODE_BIT_XOR */ fxReadCode,
	/* XS_CODE_BRANCH_1 */ fxReadCodeS1,
	/* XS_CODE_BRANCH_2 */ fxReadCodeS2,
	/* XS_CODE_BRANCH_4 */ fxReadCodeS4,
	/* XS_CODE_BRANCH_ELSE_1 */ fxReadCodeS1,
	/* XS_CODE_BRANCH_ELSE_2 */ fxReadCodeS2,
	/* XS_CODE_BRANCH_ELSE_4 */ fxReadCodeS4,
	/* XS_CODE_BRANCH_IF_1 */ fxReadCodeS1,
	/* XS_CODE_BRANCH_IF_2 */ fxReadCodeS2,
	/* XS_CODE_BRANCH_IF_4 */ fxReadCodeS4,
	/* XS_CODE_BRANCH_STATUS_1 */ fxReadCodeS1,
	/* XS_CODE_BRANCH_STATUS_2 */ fxReadCodeS2,
	/* XS_CODE_BRANCH_STATUS_4 */ fxReadCodeS4,
	/* XS_CODE_CALL */ fxReadCode,
	/* XS_CODE_CALL_TAIL */ fxReadCode,
	/* XS_CODE_CATCH_1 */ fxReadCodeS1,
	/* XS_CODE_CATCH_2 */ fxReadCodeS2,
	/* XS_CODE_CATCH_4 */ fxReadCodeS4,
	/* XS_CODE_CHECK_INSTANCE */ fxReadCode,
	/* XS_CODE_CLASS */ fxReadCode,
	/* XS_CODE_CODE_1 */ fxReadCodeS1,
	/* XS_CODE_CODE_2 */ fxReadCodeS2,
	/* XS_CODE_CODE_4 */ fxReadCodeS4,
	/* XS_CODE_CODE_ARCHIVE_1 */ fxReadCodeS1,
	/* XS_CODE_CODE_ARCHIVE_2 */ fxReadCodeS2,
	/* XS_CODE_CODE_ARCHIVE_4 */ fxReadCodeS4,
	/* XS_CODE_CONST_CLOSURE_1 */ fxReadCodeU1,
	/* XS_CODE_CONST_CLOSURE_2 */ fxReadCodeU2,
	/* XS_CODE_CONST_LOCAL_1 */ fxReadCodeU1,
	/* XS_CODE_CONST_LOCAL_2 */ fxReadCodeU2,
	/* XS_CODE_CONSTRUCTOR_FUNCTION */ fxReadCodeKey,
	/* XS_CODE_CURRENT */ fxReadCode,
	/* XS_CODE_DEBUGGER */ fxReadCode,
	/* XS_CODE_DECREMENT */ fxReadCode,
	/* XS_CODE_DELETE_PROPERTY */ fxReadCodeKey,
	/* XS_CODE_DELETE_PROPERTY_AT */ fxReadCode,
	/* XS_CODE_DELETE_SUPER */ fxReadCodeKey,
	/* XS_CODE_DELETE_SUPER_AT */ fxReadCode,
	/* XS_CODE_DIVIDE */ fxReadCode,
	/* XS_CODE_DUB */ fxReadCode,
	/* XS_CODE_DUB_AT */ fxReadCode,
	/* XS_CODE_END */ fxReadCode,
	/* XS_CODE_END_ARROW */ fxReadCode,
	/* XS_CODE_END_BASE */ fxReadCode,
	/* XS_CODE_END_DERIVED */ fxReadCode,
	/* XS_CODE_ENVIRONMENT */ fxReadCode,
	/* XS_CODE_EQUAL */ fxReadCode,
	/* XS_CODE_EVAL */ fxReadCode,
	/* XS_CODE_EVAL_ENVIRONMENT */ fxReadCode,
	/* XS_CODE_EVAL_REFERENCE */ fxReadCodeKey,
	/* XS_CODE_EXCEPTION */ fxReadCode,
	/* XS_CODE_EXPONENTIATION */ fxReadCode,
	/* XS_CODE_EXTEND */ fxReadCode,
	/* XS_CODE_FALSE */ fxReadCode,
	/* XS_CODE_FILE */ fxReadCodeKey,
	/* XS_CODE_FOR_AWAIT_OF */ fxReadCode,
	/* XS_CODE_FOR_IN */ fxReadCode,
	/* XS_CODE_FOR_OF */ fxReadCode,
	/* XS_CODE_FUNCTION */ fxReadCodeKey,
	/* XS_CODE_GENERATOR_FUNCTION */ fxReadCodeKey,
	/* XS_CODE_GET_CLOSURE_1 */ fxReadCodeU1,
	/* XS_CODE_GET_CLOSURE_2 */ fxReadCodeU2,
	/* XS_CODE_GET_LOCAL_1 */ fxReadCodeU1,
	/* XS_CODE_GET_LOCAL_2 */ fxReadCodeU2,
	/* XS_CODE_GET_PROPERTY */ fxReadCodeKey,
	/* XS_CODE_GET_PROPERTY_AT */ fxReadCode,
	/* XS_CODE_GET_SUPER */ fxReadCodeKey,
	/* XS_CODE_GET_SUPER_AT */ fxReadCode,
	/* XS_CODE_GET_THIS */ fxReadCode,
	/* XS_CODE_GET_VARIABLE */ fxReadCodeKey,
	/* XS_CODE_GLOBAL */ fxReadCode,
	/* XS_CODE_HOST */ fxReadCodeU2,
	/* XS_CODE_IN */ fxReadCode,
	/* XS_CODE_INCREMENT */ fxReadCode,
	/* XS_CODE_INSTANCEOF */ fxReadCode,
	/* XS_CODE_INSTANTIATE */ fxReadCode,
	/* XS_CODE_INTEGER_1 */ fxReadCodeS1,
	/* XS_CODE_INTEGER_2 */ fxReadCodeS2,
	/* XS_CODE_INTEGER_4 */ fxReadCodeS4,
	/* XS_CODE_INTRINSIC */ fxReadCodeU2,
	/* XS_CODE_LEFT_SHIFT */ fxReadCode,
	/* XS_CODE_LESS */ fxReadCode,
	/* XS_CODE_LESS_EQUAL */ fxReadCode,
	/* XS_CODE_LET_CLOSURE_1 */ fxReadCodeU1,
	/* XS_CODE_LET_CLOSURE_2 */ fxReadCodeU2,
	/* XS_CODE_LET_LOCAL_1 */ fxReadCodeU1,
	/* XS_CODE_LET_LOCAL_2 */ fxReadCodeU2,
	/* XS_CODE_LINE */ fxReadCodeU2,
	/* XS_CODE_MINUS */ fxReadCode,
	/* XS_CODE_MODULE */ fxReadCode,
	/* XS_CODE_MODULO */ fxReadCode,
	/* XS_CODE_MORE */ fxReadCode,
	/* XS_CODE_MORE_EQUAL */ fxReadCode,
	/* XS_CODE_MULTIPLY */ fxReadCode,
	/* XS_CODE_NAME */ fxReadCodeKey,
	/* XS_CODE_NEW */ fxReadCode,
	/* XS_CODE_NEW_CLOSURE */ fxReadCodeKey,
	/* XS_CODE_NEW_LOCAL */ fxReadCodeKey,
	/* XS_CODE_NEW_PROPERTY */ fxReadCodeU1,
	/* XS_CODE_NEW_TEMPORARY */ fxReadCode,
	/* XS_CODE_NOT */ fxReadCode,
	/* XS_CODE_NOT_EQUAL */ fxReadCode,
	/* XS_CODE_NULL */ fxReadCode,
	/* XS_CODE_NUMBER */ fxReadCodeNumber,
	/* XS_CODE_OBJECT */ fxReadCode,
	/* XS_CODE_PLUS */ fxReadCode,
	/* XS_CODE_POP */ fxReadCode,
	/* XS_CODE_PROGRAM_ENVIRONMENT */ fxReadCode,
	/* XS_CODE_PROGRAM_REFERENCE */ fxReadCodeKey,
	/* XS_CODE_PULL_CLOSURE_1 */ fxReadCodeU1,
	/* XS_CODE_PULL_CLOSURE_2 */ fxReadCodeU2,
	/* XS_CODE_PULL_LOCAL_1 */ fxReadCodeU1,
	/* XS_CODE_PULL_LOCAL_2 */ fxReadCodeU2,
	/* XS_CODE_REFRESH_CLOSURE_1 */ fxReadCodeU1,
	/* XS_CODE_REFRESH_CLOSURE_2 */ fxReadCodeU2,
	/* XS_CODE_REFRESH_LOCAL_1 */ fxReadCodeU1,
	/* XS_CODE_REFRESH_LOCAL_2 */ fxReadCodeU2,
	/* XS_CODE_RESERVE_1 */ fxReadCodeU1,
	/* XS_CODE_RESERVE_2 */ fxReadCodeU2,
	/* XS_CODE_RESET_CLOSURE_1 */ fxReadCodeU1,
	/* XS_CODE_RESET_CLOSURE_2 */ fxReadCodeU2,
	/* XS_CODE_RESET_LOCAL_1 */ fxReadCodeU1,
	/* XS_CODE_RESET_LOCAL_2 */ fxReadCodeU2,
	/* XS_CODE_RESULT */ fxReadCode,
	/* XS_CODE_RETHROW */ fxReadCode,
	/* XS_CODE_RETRIEVE_1 */ fxReadCodeU1,
	/* XS_CODE_RETRIEVE_2 */ fxReadCodeU2,
	/* XS_CODE_RETRIEVE_TARGET */ fxReadCode,
	/* XS_CODE_RETRIEVE_THIS */ fxReadCode,
	/* XS_CODE_RETURN */ fxReadCode,
	/* XS_CODE_SET_CLOSURE_1 */ fxReadCodeU1,
	/* XS_CODE_SET_CLOSURE_2 */ fxReadCodeU2,
	/* XS_CODE_SET_LOCAL_1 */ fxReadCodeU1,
	/* XS_CODE_SET_LOCAL_2 */ fxReadCodeU2,
	/* XS_CODE_SET_PROPERTY */ fxReadCodeKey,
	/* XS_CODE_SET_PROPERTY_AT */ fxReadCode,
	/* XS_CODE_SET_SUPER */ fxReadCodeKey,
	/* XS_CODE_SET_SUPER_AT */ fxReadCode,
	/* XS_CODE_SET_THIS */ fxReadCode,
	/* XS_CODE_SET_VARIABLE */ fxReadCodeKey,
	/* XS_CODE_SIGNED_RIGHT_SHIFT */ fxReadCode,
	/* XS_CODE_START_ASYNC */ fxReadCode,
	/* XS_CODE_START_ASYNC_GENERATOR */ fxReadCode,
	/* XS_CODE_START_GENERATOR */ fxReadCode,
	/* XS_CODE_STORE_1 */ fxReadCodeU1,
	/* XS_CODE_STORE_2 */ fxReadCodeU2,
	/* XS_CODE_STORE_ARROW */ fxReadCode,
	/* XS_CODE_STRICT_EQUAL */ fxReadCode,
	/* XS_CODE_STRICT_NOT_EQUAL */ fxReadCode,
	/* XS_CODE_STRING_1 */ fxReadCodeString1,
	/* XS_CODE_STRING_2 */ fxReadCodeString2,
	/* XS_CODE_STRING_ARCHIVE_1 */ fxReadCodeString1,
	/* XS_CODE_STRING_ARCHIVE_2 */ fxReadCodeString2,
	/* XS_CODE_SUBTRACT */ fxReadCode,
	/* XS_CODE_SUPER */ fxReadCode,
	/* XS_CODE_SWAP */ fxReadCode,
	/* XS_CODE_SYMBOL */ fxReadCodeKey,
	/* XS_CODE_TARGET */ fxReadCode,
	/* XS_CODE_TEMPLATE */ fxReadCode,
	/* XS_CODE_THIS */ fxReadCode,
	/* XS_CODE_THROW */ fxReadCode,
	/* XS_CODE_THROW_STATUS */ fxReadCode,
	/* XS_CODE_TO_INSTANCE */ fxReadCode,
	/* XS_CODE_TRANSFER */ fxReadCode,
	/* XS_CODE_TRUE */ fxReadCode,
	/* XS_CODE_TYPEOF */ fxReadCode,
	/* XS_CODE_UNCATCH */ fxReadCode,
	/* XS_CODE_UNDEFINED */ fxReadCode,
	/* XS_CODE_UNSIGNED_RIGHT_SHIFT */ fxReadCode,
	/* XS_CODE_UNWIND_1 */ fxReadCodeU1,
	/* XS_CODE_UNWIND_2 */ fxReadCodeU2,
	/* XS_CODE_VAR_CLOSURE_1 */ fxReadCodeU1,
	/* XS_CODE_VAR_CLOSURE_2 */ fxReadCodeU2,
	/* XS_CODE_VAR_LOCAL_1 */ fxReadCodeU1,
	/* XS_CODE_VAR_LOCAL_2 */ fxReadCodeU2,
	/* XS_CODE_VOID */ fxReadCode,
	/* XS_CODE_WITH */ fxReadCode,
	/* XS_CODE_WITHOUT */ fxReadCode,
	/* XS_CODE_YIELD */ fxReadCode,
};

const txCodeWriter gxCodeWriters[XS_CODE_COUNT] = {
	/* XS_NO_CODE */ NULL,
	/* XS_CODE_ADD */ fxWriteCode,
	/* XS_CODE_ARGUMENT */ fxWriteCodeS1,
	/* XS_CODE_ARGUMENTS */ fxWriteCodeS1,
	/* XS_CODE_ARGUMENTS_SLOPPY */ fxWriteCodeS1,
	/* XS_CODE_ARGUMENTS_STRICT */ fxWriteCodeS1,
	/* XS_CODE_ARRAY */ fxWriteCode,
	/* XS_CODE_ASYNC_FUNCTION */ fxWriteCodeKey,
	/* XS_CODE_ASYNC_GENERATOR_FUNCTION */ fxWriteCodeKey,
	/* XS_CODE_AT */ fxWriteCode,
	/* XS_CODE_AWAIT */ fxWriteCode,
	/* XS_CODE_BEGIN_SLOPPY */ fxWriteCodeS1,
	/* XS_CODE_BEGIN_STRICT */ fxWriteCodeS1,
	/* XS_CODE_BEGIN_STRICT_BASE */ fxWriteCodeS1,
	/* XS_CODE_BEGIN_STRICT_DERIVED */ fxWriteCodeS1,
	/* XS_CODE_BIT_AND */ fxWriteCode,
	/* XS_CODE_BIT_NOT */ fxWriteCode,
	/* XS_CODE_BIT_OR */ fxWriteCode,
	/* XS_CODE_BIT_XOR */ fxWriteCode,
	/* XS_CODE_BRANCH_1 */ fxWriteCodeS1,
	/* XS_CODE_BRANCH_2 */ fxWriteCodeS2,
	/* XS_CODE_BRANCH_4 */ fxWriteCodeS4,
	/* XS_CODE_BRANCH_ELSE_1 */ fxWriteCodeS1,
	/* XS_CODE_BRANCH_ELSE_2 */ fxWriteCodeS2,
	/* XS_CODE_BRANCH_ELSE_4 */ fxWriteCodeS4,
	/* XS_CODE_BRANCH_IF_1 */ fxWriteCodeS1,
	/* XS_CODE_BRANCH_IF_2 */ fxWriteCodeS2,
	/* XS_CODE_BRANCH_IF_4 */ fxWriteCodeS4,
	/* XS_CODE_BRANCH_STATUS_1 */ fxWriteCodeS1,
	/* XS_CODE_BRANCH_STATUS_2 */ fxWriteCodeS2,
	/* XS_CODE_BRANCH_STATUS_4 */ fxWriteCodeS4,
	/* XS_CODE_CALL */ fxWriteCode,
	/* XS_CODE_CALL_TAIL */ fxWriteCode,
	/* XS_CODE_CATCH_1 */ fxWriteCodeS1,
	/* XS_CODE_CATCH_2 */ fxWriteCodeS2,
	/* XS_CODE_CATCH_4 */ fxWriteCodeS4,
	/* XS_CODE_CHECK_INSTANCE */ fxWriteCode,
	/* XS_CODE_CLASS */ fxWriteCode,
	/* XS_CODE_CODE_1 */ fxWriteCodeS1,
	/* XS_CODE_CODE_2 */ fxWriteCodeS2,
	/* XS_CODE_CODE_4 */ fxWriteCodeS4,
	/* XS_CODE_CODE_ARCHIVE_1 */ fxWriteCodeS1,
	/* XS_CODE_CODE_ARCHIVE_2 */ fxWriteCodeS2,
	/* XS_CODE_CODE_ARCHIVE_4 */ fxWriteCodeS4,
	/* XS_CODE_CONST_CLOSURE_1 */ fxWriteCodeU1,
	/* XS_CODE_CONST_CLOSURE_2 */ fxWriteCodeU2,
	/* XS_CODE_CONST_LOCAL_1 */ fxWriteCodeU1,
	/* XS_CODE_CONST_LOCAL_2 */ fxWriteCodeU2,
	/* XS_CODE_CONSTRUCTOR_FUNCTION */ fxWriteCodeKey,
	/* XS_CODE_CURRENT */ fxWriteCode,
	/* XS_CODE_DEBUGGER */ fxWriteCode,
	/* XS_CODE_DECREMENT */ fxWriteCode,
	/* XS_CODE_DELETE_PROPERTY */ fxWriteCodeKey,
	/* XS_CODE_DELETE_PROPERTY_AT */ fxWriteCode,
	/* XS_CODE_DELETE_SUPER */ fxWriteCodeKey,
	/* XS_CODE_DELETE_SUPER_AT */ fxWriteCode,
	/* XS_CODE_DIVIDE */ fxWriteCode,
	/* XS_CODE_DUB */ fxWriteCode,
	/* XS_CODE_DUB_AT */ fxWriteCode,
	/* XS_CODE_END */ fxWriteCode,
	/* XS_CODE_END_ARROW */ fxWriteCode,
	/* XS_CODE_END_BASE */ fxWriteCode,
	/* XS_CODE_END_DERIVED */ fxWriteCode,
	/* XS_CODE_ENVIRONMENT */ fxWriteCode,
	/* XS_CODE_EQUAL */ fxWriteCode,
	/* XS_CODE_EVAL */ fxWriteCode,
	/* XS_CODE_EVAL_ENVIRONMENT */ fxWriteCode,
	/* XS_CODE_EVAL_REFERENCE */ fxWriteCodeKey,
	/* XS_CODE_EXCEPTION */ fxWriteCode,
	/* XS_CODE_EXPONENTIATION */ fxWriteCode,
	/* XS_CODE_EXTEND */ fxWriteCode,
	/* XS_CODE_FALSE */ fxWriteCode,
	/* XS_CODE_FILE */ fxWriteCodeKey,
	/* XS_CODE_FOR_AWAIT_OF */ fxWriteCode,
	/* XS_CODE_FOR_IN */ fxWriteCode,
	/* XS_CODE_FOR_OF */ fxWriteCode,
	/* XS_CODE_FUNCTION */ fxWriteCodeKey,
	/* XS_CODE_GENERATOR_FUNCTION */ fxWriteCodeKey,
	/* XS_CODE_GET_CLOSURE_1 */ fxWriteCodeU1,
	/* XS_CODE_GET_CLOSURE_2 */ fxWriteCodeU2,
	/* XS_CODE_GET_LOCAL_1 */ fxWriteCodeU1,
	/* XS_CODE_GET_LOCAL_2 */ fxWriteCodeU2,
	/* XS_CODE_GET_PROPERTY */ fxWriteCodeKey,
	/* XS_CODE_GET_PROPERTY_AT */ fxWriteCode,
	/* XS_CODE_GET_SUPER */ fxWriteCodeKey,
	/* XS_CODE_GET_SUPER_AT */ fxWriteCode,
	/* XS_CODE_GET_THIS */ fxWriteCode,
	/* XS_CODE_GET_VARIABLE */ fxWriteCodeKey,
	/* XS_CODE_GLOBAL */ fxWriteCode,
	/* XS_CODE_HOST */ fxWriteCodeU2,
	/* XS_CODE_IN */ fxWriteCode,
	/* XS_CODE_INCREMENT */ fxWriteCode,
	/* XS_CODE_INSTANCEOF */ fxWriteCode,
	/* XS_CODE_INSTANTIATE */ fxWriteCode,
	/* XS_CODE_INTEGER_1 */ fxWriteCodeS1,
	/* XS_CODE_INTEGER_2 */ fxWriteCodeS2,
	/* XS_CODE_INTEGER_4 */ fxWriteCodeS4,
	/* XS_CODE_INTRINSIC */ fxWriteCodeU2,
	/* XS_CODE_LEFT_SHIFT */ fxWriteCode,
	/* XS_CODE_LESS */ fxWriteCode,
	/* XS_CODE_LESS_EQUAL */ fxWriteCode,
	/* XS_CODE_LET_CLOSURE_1 */ fxWriteCodeU1,
	/* XS_CODE_LET_CLOSURE_2 */ fxWriteCodeU2,
	/* XS_CODE_LET_LOCAL_1 */ fxWriteCodeU1,
	/* XS_CODE_LET_LOCAL_2 */ fxWriteCodeU2,
	/* XS_CODE_LINE */ fxWriteCodeU2,
	/* XS_CODE_MINUS */ fxWriteCode,
	/* XS_CODE_MODULE */ fxWriteCode,
	/* XS_CODE_MODULO */ fxWriteCode,
	/* XS_CODE_MORE */ fxWriteCode,
	/* XS_CODE_MORE_EQUAL */ fxWriteCode,
	/* XS_CODE_MULTIPLY */ fxWriteCode,
	/* XS_CODE_NAME */ fxWriteCodeKey,
	/* XS_CODE_NEW */ fxWriteCode,
	/* XS_CODE_NEW_CLOSURE */ fxWriteCodeKey,
	/* XS_CODE_NEW_LOCAL */ fxWriteCodeKey,
	/* XS_CODE_NEW_PROPERTY */ fxWriteCodeU1,
	/* XS_CODE_NEW_TEMPORARY */ fxWriteCode,
	/* XS_CODE_NOT */ fxWriteCode,
	/* XS_CODE_NOT_EQUAL */ fxWriteCode,
	/* XS_CODE_NULL */ fxWriteCode,
	/* XS_CODE_NUMBER */ fxWriteCodeNumber,
	/* XS_CODE_OBJECT */ fxWriteCode,
	/* XS_CODE_PLUS */ fxWriteCode,
	/* XS_CODE_POP */ fxWriteCode,
	/* XS_CODE_PROGRAM_ENVIRONMENT */ fxWriteCode,
	/* XS_CODE_PROGRAM_REFERENCE */ fxWriteCodeKey,
	/* XS_CODE_PULL_CLOSURE_1 */ fxWriteCodeU1,
	/* XS_CODE_PULL_CLOSURE_2 */ fxWriteCodeU2,
	/* XS_CODE_PULL_LOCAL_1 */ fxWriteCodeU1,
	/* XS_CODE_PULL_LOCAL_2 */ fxWriteCodeU2,
	/* XS_CODE_REFRESH_CLOSURE_1 */ fxWriteCodeU1,
	/* XS_CODE_REFRESH_CLOSURE_2 */ fxWriteCodeU2,
	/* XS_CODE_REFRESH_LOCAL_1 */ fxWriteCodeU1,
	/* XS_CODE_REFRESH_LOCAL_2 */ fxWriteCodeU2,
	/* XS_CODE_RESERVE_1 */ fxWriteCodeU1,
	/* XS_CODE_RESERVE_2 */ fxWriteCodeU2,
	/* XS_CODE_RESET_CLOSURE_1 */ fxWriteCodeU1,
	/* XS_CODE_RESET_CLOSURE_2 */ fxWriteCodeU2,
	/* XS_CODE_RESET_LOCAL_1 */ fxWriteCodeU1,
	/* XS_CODE_RESET_LOCAL_2 */ fxWriteCodeU2,
	/* XS_CODE_RESULT */ fxWriteCode,
	/* XS_CODE_RETHROW */ fxWriteCode,
	/* XS_CODE_RETRIEVE_1 */ fxWriteCodeU1,
	/* XS_CODE_RETRIEVE_2 */ fxWriteCodeU2,
	/* XS_CODE_RETRIEVE_TARGET */ fxWriteCode,
	/* XS_CODE_RETRIEVE_THIS */ fxWriteCode,
	/* XS_CODE_RETURN */ fxWriteCode,
	/* XS_CODE_SET_CLOSURE_1 */ fxWriteCodeU1,
	/* XS_CODE_SET_CLOSURE_2 */ fxWriteCodeU2,
	/* XS_CODE_SET_LOCAL_1 */ fxWriteCodeU1,
	/* XS_CODE_SET_LOCAL_2 */ fxWriteCodeU2,
	/* XS_CODE_SET_PROPERTY */ fxWriteCodeKey,
	/* XS_CODE_SET_PROPERTY_AT */ fxWriteCode,
	/* XS_CODE_SET_SUPER */ fxWriteCodeKey,
	/* XS_CODE_SET_SUPER_AT */ fxWriteCode,
	/* XS_CODE_SET_THIS */ fxWriteCode,
	/* XS_CODE_SET_VARIABLE */ fxWriteCodeKey,
	/* XS_CODE_SIGNED_RIGHT_SHIFT */ fxWriteCode,
	/* XS_CODE_START_ASYNC */ fxWriteCode,
	/* XS_CODE_START_ASYNC_GENERATOR */ fxWriteCode,
	/* XS_CODE_START_GENERATOR */ fxWriteCode,
	/* XS_CODE_STORE_1 */ fxWriteCodeU1,
	/* XS_CODE_STORE_2 */ fxWriteCodeU2,
	/* XS_CODE_STORE_ARROW */ fxWriteCode,
	/* XS_CODE_STRICT_EQUAL */ fxWriteCode,
	/* XS_CODE_STRICT_NOT_EQUAL */ fxWriteCode,
	/* XS_CODE_STRING_1 */ fxWriteCodeString1,
	/* XS_CODE_STRING_2 */ fxWriteCodeString2,
	/* XS_CODE_STRING_ARCHIVE_1 */ fxWriteCodeString1,
	/* XS_CODE_STRING_ARCHIVE_2 */ fxWriteCodeString2,
	/* XS_CODE_SUBTRACT */ fxWriteCode,
	/* XS_CODE_SUPER */ fxWriteCode,
	/* XS_CODE_SWAP */ fxWriteCode,
	/* XS_CODE_SYMBOL */ fxWriteCodeKey,
	/* XS_CODE_TARGET */ fxWriteCode,
	/* XS_CODE_TEMPLATE */ fxWriteCode,
	/* XS_CODE_THIS */ fxWriteCode,
	/* XS_CODE_THROW */ fxWriteCode,
	/* XS_CODE_THROW_STATUS */ fxWriteCode,
	/* XS_CODE_TO_INSTANCE */ fxWriteCode,
	/* XS_CODE_TRANSFER */ fxWriteCode,
	/* XS_CODE_TRUE */ fxWriteCode,
	/* XS_CODE_TYPEOF */ fxWriteCode,
	/* XS_CODE_UNCATCH */ fxWriteCode,
	/* XS_CODE_UNDEFINED */ fxWriteCode,
	/* XS_CODE_UNSIGNED_RIGHT_SHIFT */ fxWriteCode,
	/* XS_CODE_UNWIND_1 */ fxWriteCodeU1,
	/* XS_CODE_UNWIND_2 */ fxWriteCodeU2,
	/* XS_CODE_VAR_CLOSURE_1 */ fxWriteCodeU1,
	/* XS_CODE_VAR_CLOSURE_2 */ fxWriteCodeU2,
	/* XS_CODE_VAR_LOCAL_1 */ fxWriteCodeU1,
	/* XS_CODE_VAR_LOCAL_2 */ fxWriteCodeU2,
	/* XS_CODE_VOID */ fxWriteCode,
	/* XS_CODE_WITH */ fxWriteCode,
	/* XS_CODE_WITHOUT */ fxWriteCode,
	/* XS_CODE_YIELD */ fxWriteCode,
};

txS1* fxReadCode(xsMachine* the, txU1 id, txS1* p)
{
	xsVar(2) = xsNew0(xsArg(1), gxCodeConstructors[id]);
	return p;
}

txS1* fxReadCodeKey(xsMachine* the, txU1 id, txS1* p)
{
	txID param;
	mxDecode2(p, param);
	xsVar(2) = xsNew1(xsArg(1), gxCodeConstructors[id], xsInteger(param));
	return p;
}

txS1* fxReadCodeNumber(xsMachine* the, txU1 id, txS1* p)
{
	txNumber param;
	mxDecode8(p, param);
	xsVar(2) = xsNew1(xsArg(1), gxCodeConstructors[id], xsNumber(param));
	return p;
}

txS1* fxReadCodeS1(xsMachine* the, txU1 id, txS1* p)
{
	txS1 param = *p++;
	xsVar(2) = xsNew1(xsArg(1), gxCodeConstructors[id], xsInteger(param));
	return p;
}

txS1* fxReadCodeS2(xsMachine* the, txU1 id, txS1* p)
{
	txS2 param;
	mxDecode2(p, param);
	xsVar(2) = xsNew1(xsArg(1), gxCodeConstructors[id], xsInteger(param));
	return p;
}

txS1* fxReadCodeS4(xsMachine* the, txU1 id, txS1* p)
{
	txS4 param;
	mxDecode4(p, param);
	xsVar(2) = xsNew1(xsArg(1), gxCodeConstructors[id], xsInteger(param));
	return p;
}

txS1* fxReadCodeString1(xsMachine* the, txU1 id, txS1* p)
{
	txU1 param = *((txU1*)p++);
	xsVar(2) = xsNew1(xsArg(1), gxCodeConstructors[id], xsString(p));
	return p + param;
}

txS1* fxReadCodeString2(xsMachine* the, txU1 id, txS1* p)
{
	txU2 param;
	mxDecode2(p, param);
	xsVar(2) = xsNew1(xsArg(1), gxCodeConstructors[id], xsString(p));
	return p + param;
}

txS1* fxReadCodeU1(xsMachine* the, txU1 id, txS1* p)
{
	txU1 param = *((txU1*)p++);
	xsVar(2) = xsNew1(xsArg(1), gxCodeConstructors[id], xsInteger(param));
	return p;
}

txS1* fxReadCodeU2(xsMachine* the, txU1 id, txS1* p)
{
	txU2 param;
	mxDecode2(p, param);
	xsVar(2) = xsNew1(xsArg(1), gxCodeConstructors[id], xsInteger(param));
	return p;
}

void fxReadCodes(xsMachine* the, void* buffer, txSize size)
{
	txS1* p = buffer;
	txS1* q = p + size;
	txInteger offset = 0;
	while (p < q) {
		txS1* r = p;
		txU1 id = *((txU1*)p++);
		p = (*gxCodeReaders[id])(the, id, p);
		//fprintf(stderr, "%s %d\n", gxCodeNames[id], r - p);
		xsSet(xsVar(2), xsID_offset, xsInteger(offset));
		xsCall1(xsThis, xsID_append, xsVar(2));
		offset += p - r;
	}
}

void fxReadHosts(xsMachine* the, void* buffer, txSize size)
{
	txS1* p = buffer;
	txID c, i, index;
	mxDecode2(p, c);
	xsVar(1) = xsNewArray(c);
	for (i = 0; i < c; i++) {
		xsVar(2) = xsInteger(*p++);
		mxDecode2(p, index);
		xsVar(3) = xsInteger(index);
		xsVar(4) = xsString(p);
		p += c_strlen((char*)p) + 1;
		xsVar(5) = xsNew3(xsArg(1), xsID_Host, xsVar(2), xsVar(3), xsVar(4));
		xsSetAt(xsVar(1), xsInteger(i), xsVar(5));
	}
	xsSet(xsThis, xsID_hosts, xsVar(1));
}

txS1* fxWriteCode(xsMachine* the, txU1 id, txS1* p)
{
	*((txU1*)p++) = id;
	return p;
}

txS1* fxWriteCodeKey(xsMachine* the, txU1 id, txS1* p)
{
	txID index = (txID)xsToInteger(xsGet(xsVar(0), xsID_param));
	*((txU1*)p++) = id;
	mxEncode2(p, index);
	return p;
}

txS1* fxWriteCodeNumber(xsMachine* the, txU1 id, txS1* p)
{
	txNumber n = xsToNumber(xsGet(xsVar(0), xsID_param));
	*((txU1*)p++) = id;
	mxEncode8(p, n);
	return p;
}

txS1* fxWriteCodeS1(xsMachine* the, txU1 id, txS1* p)
{
	txS1 s1 = (txS1)xsToInteger(xsGet(xsVar(0), xsID_param));
	*((txU1*)p++) = id;
	*((txS1*)p++) = s1;
	return p;
}

txS1* fxWriteCodeS2(xsMachine* the, txU1 id, txS1* p)
{
	txS2 s2 = (txS2)xsToInteger(xsGet(xsVar(0), xsID_param));
	*((txU1*)p++) = id;
	mxEncode2(p, s2);
	return p;
}

txS1* fxWriteCodeS4(xsMachine* the, txU1 id, txS1* p)
{
	txS4 s4 = (txS4)xsToInteger(xsGet(xsVar(0), xsID_param));
	*((txU1*)p++) = id;
	mxEncode4(p, s4);
	return p;
}

txS1* fxWriteCodeString1(xsMachine* the, txU1 id, txS1* p)

{
	txU1 u1 = (txU1)(xsToInteger(xsGet(xsVar(0), xsID_size)) - 2);
	*((txU1*)p++) = id;
	*((txU1*)p++) = u1;
	c_memcpy(p, xsToString(xsGet(xsVar(0), xsID_param)), u1);
	p += u1;
	return p;
}

txS1* fxWriteCodeString2(xsMachine* the, txU1 id, txS1* p)
{
	txU2 u2 = (txU2)(xsToInteger(xsGet(xsVar(0), xsID_size)) - 3);
	*((txU1*)p++) = id;
	mxEncode2(p, u2);
	c_memcpy(p, xsToString(xsGet(xsVar(0), xsID_param)), u2);
	p += u2;
	return p;
}

txS1* fxWriteCodeU1(xsMachine* the, txU1 id, txS1* p)
{
	txU1 u1 = (txU1)xsToInteger(xsGet(xsVar(0), xsID_param));
	*((txU1*)p++) = id;
	*((txU1*)p++) = u1;
	return p;
}

txS1* fxWriteCodeU2(xsMachine* the, txU1 id, txS1* p)
{
	txU2 u2 = (txU2)xsToInteger(xsGet(xsVar(0), xsID_param));
	*((txU1*)p++) = id;
	mxEncode2(p, u2);
	return p;
}

void fxWriteCodes(xsMachine* the, txS1* p)
{
	xsVar(0) = xsGet(xsThis, xsID_first);
	while (xsTest(xsVar(0))) {
		//txS1* r = p;
		txU1 id = (txU1)xsToInteger(xsGet(xsVar(0), xsID_id));
		p = (*gxCodeWriters[id])(the, id, p);
		//fprintf(stderr, "%s %d\n", gxCodeNames[id], p - r);
		xsVar(0) = xsGet(xsVar(0), xsID_next);
	}
}

void Tool_prototype_getModule(xsMachine* the)
{
	txLinker* linker = xsGetContext(the);
	txLinkerScript* script = linker->currentScript;
	char buffer[1024];
	c_strcpy(buffer, linker->base);
	c_strcat(buffer, script->path);
	txSlot* module = fxRequireModule(the, xsID(buffer), mxArgv(0));
	if (module) {
		mxResult->kind = module->kind;
		mxResult->value = module->value;
	}
}

void Tool_prototype_getPath(xsMachine* the)
{
	txLinker* linker = xsGetContext(the);
	txLinkerScript* script = linker->currentScript;
	xsResult = xsString(script->path);
}

void Tool_prototype_keyString(xsMachine* the)
{
	txLinker* linker = xsGetContext(the);
	xsIntegerValue key = xsToInteger(xsArg(0));
	if (key != XS_NO_ID) {
		txLinkerSymbol* linkerSymbol = linker->symbolArray[key & 0x7FFF];
		xsResult = xsString(linkerSymbol->string);
	}
	else
		xsResult = xsString("?");
}

void Tool_prototype_prepare(xsMachine* the)
{
	txInteger i, j, k;
	xsVars(1);
	gxCodeConstructors[XS_CODE_ADD] = XS_NO_ID;
	gxCodeConstructors[XS_CODE_ADD] = xsID_ADD;
	gxCodeConstructors[XS_CODE_ARGUMENT] = xsID_ARGUMENT;
	gxCodeConstructors[XS_CODE_ARGUMENTS] = xsID_ARGUMENTS;
	gxCodeConstructors[XS_CODE_ARGUMENTS_SLOPPY] = xsID_ARGUMENTS_SLOPPY;
	gxCodeConstructors[XS_CODE_ARGUMENTS_STRICT] = xsID_ARGUMENTS_STRICT;
	gxCodeConstructors[XS_CODE_ARRAY] = xsID_ARRAY;
	gxCodeConstructors[XS_CODE_ASYNC_FUNCTION] = xsID_ASYNC_FUNCTION;
	gxCodeConstructors[XS_CODE_ASYNC_GENERATOR_FUNCTION] = xsID_ASYNC_GENERATOR_FUNCTION;
	gxCodeConstructors[XS_CODE_AT] = xsID_AT;
	gxCodeConstructors[XS_CODE_AWAIT] = xsID_AWAIT;
	gxCodeConstructors[XS_CODE_BEGIN_SLOPPY] = xsID_BEGIN_SLOPPY;
	gxCodeConstructors[XS_CODE_BEGIN_STRICT] = xsID_BEGIN_STRICT;
	gxCodeConstructors[XS_CODE_BEGIN_STRICT_BASE] = xsID_BEGIN_STRICT_BASE;
	gxCodeConstructors[XS_CODE_BEGIN_STRICT_DERIVED] = xsID_BEGIN_STRICT_DERIVED;
	gxCodeConstructors[XS_CODE_BIT_AND] = xsID_BIT_AND;
	gxCodeConstructors[XS_CODE_BIT_NOT] = xsID_BIT_NOT;
	gxCodeConstructors[XS_CODE_BIT_OR] = xsID_BIT_OR;
	gxCodeConstructors[XS_CODE_BIT_XOR] = xsID_BIT_XOR;
	gxCodeConstructors[XS_CODE_BRANCH_1] = xsID_BRANCH;
	gxCodeConstructors[XS_CODE_BRANCH_2] = xsID_BRANCH;
	gxCodeConstructors[XS_CODE_BRANCH_4] = xsID_BRANCH;
	gxCodeConstructors[XS_CODE_BRANCH_ELSE_1] = xsID_BRANCH_ELSE;
	gxCodeConstructors[XS_CODE_BRANCH_ELSE_2] = xsID_BRANCH_ELSE;
	gxCodeConstructors[XS_CODE_BRANCH_ELSE_4] = xsID_BRANCH_ELSE;
	gxCodeConstructors[XS_CODE_BRANCH_IF_1] = xsID_BRANCH_IF;
	gxCodeConstructors[XS_CODE_BRANCH_IF_2] = xsID_BRANCH_IF;
	gxCodeConstructors[XS_CODE_BRANCH_IF_4] = xsID_BRANCH_IF;
	gxCodeConstructors[XS_CODE_BRANCH_STATUS_1] = xsID_BRANCH_STATUS;
	gxCodeConstructors[XS_CODE_BRANCH_STATUS_2] = xsID_BRANCH_STATUS;
	gxCodeConstructors[XS_CODE_BRANCH_STATUS_4] = xsID_BRANCH_STATUS;
	gxCodeConstructors[XS_CODE_CALL] = xsID_CALL;
	gxCodeConstructors[XS_CODE_CALL_TAIL] = xsID_CALL_TAIL;
	gxCodeConstructors[XS_CODE_CATCH_1] = xsID_CATCH;
	gxCodeConstructors[XS_CODE_CATCH_2] = xsID_CATCH;
	gxCodeConstructors[XS_CODE_CATCH_4] = xsID_CATCH;
	gxCodeConstructors[XS_CODE_CHECK_INSTANCE] = xsID_CHECK_INSTANCE;
	gxCodeConstructors[XS_CODE_CLASS] = xsID_CLASS;
	gxCodeConstructors[XS_CODE_CODE_1] = xsID_CODE;
	gxCodeConstructors[XS_CODE_CODE_2] = xsID_CODE;
	gxCodeConstructors[XS_CODE_CODE_4] = xsID_CODE;
	gxCodeConstructors[XS_CODE_CODE_ARCHIVE_1] = xsID_CODE_ARCHIVE;
	gxCodeConstructors[XS_CODE_CODE_ARCHIVE_2] = xsID_CODE_ARCHIVE;
	gxCodeConstructors[XS_CODE_CODE_ARCHIVE_4] = xsID_CODE_ARCHIVE;
	gxCodeConstructors[XS_CODE_CONST_CLOSURE_1] = xsID_CONST_CLOSURE;
	gxCodeConstructors[XS_CODE_CONST_CLOSURE_2] = xsID_CONST_CLOSURE;
	gxCodeConstructors[XS_CODE_CONST_LOCAL_1] = xsID_CONST_LOCAL;
	gxCodeConstructors[XS_CODE_CONST_LOCAL_2] = xsID_CONST_LOCAL;
	gxCodeConstructors[XS_CODE_CONSTRUCTOR_FUNCTION] = xsID_CONSTRUCTOR_FUNCTION;
	gxCodeConstructors[XS_CODE_CURRENT] = xsID_CURRENT;
	gxCodeConstructors[XS_CODE_DEBUGGER] = xsID_DEBUGGER;
	gxCodeConstructors[XS_CODE_DECREMENT] = xsID_DECREMENT;
	gxCodeConstructors[XS_CODE_DELETE_PROPERTY] = xsID_DELETE_PROPERTY;
	gxCodeConstructors[XS_CODE_DELETE_PROPERTY_AT] = xsID_DELETE_PROPERTY_AT;
	gxCodeConstructors[XS_CODE_DELETE_SUPER] = xsID_DELETE_SUPER;
	gxCodeConstructors[XS_CODE_DELETE_SUPER_AT] = xsID_DELETE_SUPER_AT;
	gxCodeConstructors[XS_CODE_DIVIDE] = xsID_DIVIDE;
	gxCodeConstructors[XS_CODE_DUB] = xsID_DUB;
	gxCodeConstructors[XS_CODE_DUB_AT] = xsID_DUB_AT;
	gxCodeConstructors[XS_CODE_END] = xsID_END;
	gxCodeConstructors[XS_CODE_END_ARROW] = xsID_END_ARROW;
	gxCodeConstructors[XS_CODE_END_BASE] = xsID_END_BASE;
	gxCodeConstructors[XS_CODE_END_DERIVED] = xsID_END_DERIVED;
	gxCodeConstructors[XS_CODE_ENVIRONMENT] = xsID_ENVIRONMENT;
	gxCodeConstructors[XS_CODE_EQUAL] = xsID_EQUAL;
	gxCodeConstructors[XS_CODE_EVAL] = xsID_EVAL;
	gxCodeConstructors[XS_CODE_EVAL_ENVIRONMENT] = xsID_EVAL_ENVIRONMENT;
	gxCodeConstructors[XS_CODE_EVAL_REFERENCE] = xsID_EVAL_REFERENCE;
	gxCodeConstructors[XS_CODE_EXCEPTION] = xsID_EXCEPTION;
	gxCodeConstructors[XS_CODE_EXPONENTIATION] = xsID_EXPONENTIATION;
	gxCodeConstructors[XS_CODE_EXTEND] = xsID_EXTEND;
	gxCodeConstructors[XS_CODE_FALSE] = xsID_FALSE;
	gxCodeConstructors[XS_CODE_FILE] = xsID_FILE;
	gxCodeConstructors[XS_CODE_FOR_AWAIT_OF] = xsID_FOR_AWAIT_OF;
	gxCodeConstructors[XS_CODE_FOR_IN] = xsID_FOR_IN;
	gxCodeConstructors[XS_CODE_FOR_OF] = xsID_FOR_OF;
	gxCodeConstructors[XS_CODE_FUNCTION] = xsID_FUNCTION;
	gxCodeConstructors[XS_CODE_GENERATOR_FUNCTION] = xsID_GENERATOR_FUNCTION;
	gxCodeConstructors[XS_CODE_GET_CLOSURE_1] = xsID_GET_CLOSURE;
	gxCodeConstructors[XS_CODE_GET_CLOSURE_2] = xsID_GET_CLOSURE;
	gxCodeConstructors[XS_CODE_GET_LOCAL_1] = xsID_GET_LOCAL;
	gxCodeConstructors[XS_CODE_GET_LOCAL_2] = xsID_GET_LOCAL;
	gxCodeConstructors[XS_CODE_GET_PROPERTY] = xsID_GET_PROPERTY;
	gxCodeConstructors[XS_CODE_GET_PROPERTY_AT] = xsID_GET_PROPERTY_AT;
	gxCodeConstructors[XS_CODE_GET_SUPER] = xsID_GET_SUPER;
	gxCodeConstructors[XS_CODE_GET_SUPER_AT] = xsID_GET_SUPER_AT;
	gxCodeConstructors[XS_CODE_GET_THIS] = xsID_GET_THIS;
	gxCodeConstructors[XS_CODE_GET_VARIABLE] = xsID_GET_VARIABLE;
	gxCodeConstructors[XS_CODE_GLOBAL] = xsID_GLOBAL;
	gxCodeConstructors[XS_CODE_HOST] = xsID_HOST;
	gxCodeConstructors[XS_CODE_IN] = xsID_IN;
	gxCodeConstructors[XS_CODE_INCREMENT] = xsID_INCREMENT;
	gxCodeConstructors[XS_CODE_INSTANCEOF] = xsID_INSTANCEOF;
	gxCodeConstructors[XS_CODE_INSTANTIATE] = xsID_INSTANTIATE;
	gxCodeConstructors[XS_CODE_INTEGER_1] = xsID_INTEGER;
	gxCodeConstructors[XS_CODE_INTEGER_2] = xsID_INTEGER;
	gxCodeConstructors[XS_CODE_INTEGER_4] = xsID_INTEGER;
	gxCodeConstructors[XS_CODE_INTRINSIC] = xsID_INTRINSIC;
	gxCodeConstructors[XS_CODE_LEFT_SHIFT] = xsID_LEFT_SHIFT;
	gxCodeConstructors[XS_CODE_LESS] = xsID_LESS;
	gxCodeConstructors[XS_CODE_LESS_EQUAL] = xsID_LESS_EQUAL;
	gxCodeConstructors[XS_CODE_LET_CLOSURE_1] = xsID_LET_CLOSURE;
	gxCodeConstructors[XS_CODE_LET_CLOSURE_2] = xsID_LET_CLOSURE;
	gxCodeConstructors[XS_CODE_LET_LOCAL_1] = xsID_LET_LOCAL;
	gxCodeConstructors[XS_CODE_LET_LOCAL_2] = xsID_LET_LOCAL;
	gxCodeConstructors[XS_CODE_LINE] = xsID_LINE;
	gxCodeConstructors[XS_CODE_MINUS] = xsID_MINUS;
	gxCodeConstructors[XS_CODE_MODULE] = xsID_MODULE;
	gxCodeConstructors[XS_CODE_MODULO] = xsID_MODULO;
	gxCodeConstructors[XS_CODE_MORE] = xsID_MORE;
	gxCodeConstructors[XS_CODE_MORE_EQUAL] = xsID_MORE_EQUAL;
	gxCodeConstructors[XS_CODE_MULTIPLY] = xsID_MULTIPLY;
	gxCodeConstructors[XS_CODE_NAME] = xsID_NAME;
	gxCodeConstructors[XS_CODE_NEW] = xsID_NEW;
	gxCodeConstructors[XS_CODE_NEW_CLOSURE] = xsID_NEW_CLOSURE;
	gxCodeConstructors[XS_CODE_NEW_LOCAL] = xsID_NEW_LOCAL;
	gxCodeConstructors[XS_CODE_NEW_PROPERTY] = xsID_NEW_PROPERTY;
	gxCodeConstructors[XS_CODE_NEW_TEMPORARY] = xsID_NEW_TEMPORARY;
	gxCodeConstructors[XS_CODE_NOT] = xsID_NOT;
	gxCodeConstructors[XS_CODE_NOT_EQUAL] = xsID_NOT_EQUAL;
	gxCodeConstructors[XS_CODE_NULL] = xsID_NULL;
	gxCodeConstructors[XS_CODE_NUMBER] = xsID_NUMBER;
	gxCodeConstructors[XS_CODE_OBJECT] = xsID_OBJECT;
	gxCodeConstructors[XS_CODE_PLUS] = xsID_PLUS;
	gxCodeConstructors[XS_CODE_POP] = xsID_POP;
	gxCodeConstructors[XS_CODE_PROGRAM_ENVIRONMENT] = xsID_PROGRAM_ENVIRONMENT;
	gxCodeConstructors[XS_CODE_PROGRAM_REFERENCE] = xsID_PROGRAM_REFERENCE;
	gxCodeConstructors[XS_CODE_PULL_CLOSURE_1] = xsID_PULL_CLOSURE;
	gxCodeConstructors[XS_CODE_PULL_CLOSURE_2] = xsID_PULL_CLOSURE;
	gxCodeConstructors[XS_CODE_PULL_LOCAL_1] = xsID_PULL_LOCAL;
	gxCodeConstructors[XS_CODE_PULL_LOCAL_2] = xsID_PULL_LOCAL;
	gxCodeConstructors[XS_CODE_REFRESH_CLOSURE_1] = xsID_REFRESH_CLOSURE;
	gxCodeConstructors[XS_CODE_REFRESH_CLOSURE_2] = xsID_REFRESH_CLOSURE;
	gxCodeConstructors[XS_CODE_REFRESH_LOCAL_1] = xsID_REFRESH_LOCAL;
	gxCodeConstructors[XS_CODE_REFRESH_LOCAL_2] = xsID_REFRESH_LOCAL;
	gxCodeConstructors[XS_CODE_RESERVE_1] = xsID_RESERVE;
	gxCodeConstructors[XS_CODE_RESERVE_2] = xsID_RESERVE;
	gxCodeConstructors[XS_CODE_RESET_CLOSURE_1] = xsID_RESET_CLOSURE;
	gxCodeConstructors[XS_CODE_RESET_CLOSURE_2] = xsID_RESET_CLOSURE;
	gxCodeConstructors[XS_CODE_RESET_LOCAL_1] = xsID_RESET_LOCAL;
	gxCodeConstructors[XS_CODE_RESET_LOCAL_2] = xsID_RESET_LOCAL;
	gxCodeConstructors[XS_CODE_RESULT] = xsID_RESULT;
	gxCodeConstructors[XS_CODE_RETHROW] = xsID_RETHROW;
	gxCodeConstructors[XS_CODE_RETRIEVE_1] = xsID_RETRIEVE;
	gxCodeConstructors[XS_CODE_RETRIEVE_2] = xsID_RETRIEVE;
	gxCodeConstructors[XS_CODE_RETRIEVE_TARGET] = xsID_RETRIEVE_TARGET;
	gxCodeConstructors[XS_CODE_RETRIEVE_THIS] = xsID_RETRIEVE_THIS;
	gxCodeConstructors[XS_CODE_RETURN] = xsID_RETURN;
	gxCodeConstructors[XS_CODE_SET_CLOSURE_1] = xsID_SET_CLOSURE;
	gxCodeConstructors[XS_CODE_SET_CLOSURE_2] = xsID_SET_CLOSURE;
	gxCodeConstructors[XS_CODE_SET_LOCAL_1] = xsID_SET_LOCAL;
	gxCodeConstructors[XS_CODE_SET_LOCAL_2] = xsID_SET_LOCAL;
	gxCodeConstructors[XS_CODE_SET_PROPERTY] = xsID_SET_PROPERTY;
	gxCodeConstructors[XS_CODE_SET_PROPERTY_AT] = xsID_SET_PROPERTY_AT;
	gxCodeConstructors[XS_CODE_SET_SUPER] = xsID_SET_SUPER;
	gxCodeConstructors[XS_CODE_SET_SUPER_AT] = xsID_SET_SUPER_AT;
	gxCodeConstructors[XS_CODE_SET_THIS] = xsID_SET_THIS;
	gxCodeConstructors[XS_CODE_SET_VARIABLE] = xsID_SET_VARIABLE;
	gxCodeConstructors[XS_CODE_SIGNED_RIGHT_SHIFT] = xsID_SIGNED_RIGHT_SHIFT;
	gxCodeConstructors[XS_CODE_START_ASYNC] = xsID_START_ASYNC;
	gxCodeConstructors[XS_CODE_START_ASYNC_GENERATOR] = xsID_START_ASYNC_GENERATOR;
	gxCodeConstructors[XS_CODE_START_GENERATOR] = xsID_START_GENERATOR;
	gxCodeConstructors[XS_CODE_STORE_1] = xsID_STORE;
	gxCodeConstructors[XS_CODE_STORE_2] = xsID_STORE;
	gxCodeConstructors[XS_CODE_STORE_ARROW] = xsID_STORE_ARROW;
	gxCodeConstructors[XS_CODE_STRICT_EQUAL] = xsID_STRICT_EQUAL;
	gxCodeConstructors[XS_CODE_STRICT_NOT_EQUAL] = xsID_STRICT_NOT_EQUAL;
	gxCodeConstructors[XS_CODE_STRING_1] = xsID_STRING;
	gxCodeConstructors[XS_CODE_STRING_2] = xsID_STRING;
	gxCodeConstructors[XS_CODE_STRING_ARCHIVE_1] = xsID_STRING_ARCHIVE;
	gxCodeConstructors[XS_CODE_STRING_ARCHIVE_2] = xsID_STRING_ARCHIVE;
	gxCodeConstructors[XS_CODE_SUBTRACT] = xsID_SUBTRACT;
	gxCodeConstructors[XS_CODE_SUPER] = xsID_SUPER;
	gxCodeConstructors[XS_CODE_SWAP] = xsID_SWAP;
	gxCodeConstructors[XS_CODE_SYMBOL] = xsID_SYMBOL;
	gxCodeConstructors[XS_CODE_TARGET] = xsID_TARGET;
	gxCodeConstructors[XS_CODE_TEMPLATE] = xsID_TEMPLATE;
	gxCodeConstructors[XS_CODE_THIS] = xsID_THIS;
	gxCodeConstructors[XS_CODE_THROW] = xsID_THROW;
	gxCodeConstructors[XS_CODE_THROW_STATUS] = xsID_THROW_STATUS;
	gxCodeConstructors[XS_CODE_TO_INSTANCE] = xsID_TO_INSTANCE;
	gxCodeConstructors[XS_CODE_TRANSFER] = xsID_TRANSFER;
	gxCodeConstructors[XS_CODE_TRUE] = xsID_TRUE;
	gxCodeConstructors[XS_CODE_TYPEOF] = xsID_TYPEOF;
	gxCodeConstructors[XS_CODE_UNCATCH] = xsID_UNCATCH;
	gxCodeConstructors[XS_CODE_UNDEFINED] = xsID_UNDEFINED;
	gxCodeConstructors[XS_CODE_UNSIGNED_RIGHT_SHIFT] = xsID_UNSIGNED_RIGHT_SHIFT;
	gxCodeConstructors[XS_CODE_UNWIND_1] = xsID_UNWIND;
	gxCodeConstructors[XS_CODE_UNWIND_2] = xsID_UNWIND;
	gxCodeConstructors[XS_CODE_VAR_CLOSURE_1] = xsID_VAR_CLOSURE;
	gxCodeConstructors[XS_CODE_VAR_CLOSURE_2] = xsID_VAR_CLOSURE;
	gxCodeConstructors[XS_CODE_VAR_LOCAL_1] = xsID_VAR_LOCAL;
	gxCodeConstructors[XS_CODE_VAR_LOCAL_2] = xsID_VAR_LOCAL;
	gxCodeConstructors[XS_CODE_VOID] = xsID_VOID;
	gxCodeConstructors[XS_CODE_WITH] = xsID_WITH;
	gxCodeConstructors[XS_CODE_WITHOUT] = xsID_WITHOUT;
	gxCodeConstructors[XS_CODE_YIELD] = xsID_YIELD;
	
	for (i = 1; i < XS_CODE_COUNT; i++) {
		xsVar(0) = xsGet(xsArg(0), gxCodeConstructors[i]);
		xsVar(0) = xsGet(xsVar(0), xsID_prototype);
		j = gxCodeIDs[i];
		xsSet(xsVar(0), xsID_id, xsInteger(j));
		xsSet(xsVar(0), xsID_name, xsString(gxCodeNames[j]));
		k = gxCodeSizes[j];
		if (k == 0) // key
			k = 3;
		xsSet(xsVar(0), xsID_size, xsInteger(k));
	}
}

void Tool_prototype_read(xsMachine* the)
{
	txLinker* linker = xsGetContext(the);
	txLinkerScript* script = linker->currentScript;
	xsVars(6);
	fxReadCodes(the, script->codeBuffer, script->codeSize);
	if (script->hostsBuffer)
		fxReadHosts(the, script->hostsBuffer, script->hostsSize);
}

void Tool_prototype_report(xsMachine* the)
{
	fprintf(stderr, "%s\n", xsToString(xsArg(0)));
}

void Tool_prototype_strlen(xsMachine* the)
{
	xsIntegerValue result = strlen(xsToString(xsArg(0)));
	xsResult = xsInteger(result);
}

int gxCodeSize = 0;
int gxOptCodeSize = 0;
int gxOptBranches[3] = { 0, 0, 0 };

void Tool_prototype_write(xsMachine* the)
{
	txLinker* linker = xsGetContext(the);
	txLinkerScript* script = linker->currentScript;
	txSize size;
	void* buffer;
	xsVars(3);
	size = xsToInteger(xsGet(xsThis, xsID_codesSize));
	buffer = fxNewLinkerChunk(linker, size);
	xsElseThrow(buffer);
	fxWriteCodes(the, buffer);
	gxCodeSize += script->codeSize;
	gxOptCodeSize += size;
	fprintf(stderr, "%d/%d\n", script->codeSize - size, script->codeSize);
	script->codeBuffer = buffer;
	script->codeSize = size;
}

void Tool_prototype_count(xsMachine* the)
{
	int index = xsToInteger(xsArg(0));
	gxOptBranches[index]++;
}

static void fxOptimizeScript(txLinker* linker, txLinkerScript* linkerScript)
{
	xsCreation _creation = {
		128 * 1024 * 1024, 	/* initialChunkSize */
		16 * 1024 * 1024, 	/* incrementalChunkSize */
		4 * 1024 * 1024, 	/* initialHeapCount */
		1 * 1024 * 1024,	/* incrementalHeapCount */
		1024,				/* stackCount */
		2048+2048,			/* keyCount */
		1993,				/* nameModulo */
		127,				/* symbolModulo */
		32 * 1024,			/* parserBufferSize */
		1993,				/* parserTableModulo */
	};
	xsCreation* creation = &_creation;
	xsMachine* the = NULL;
	
	fprintf(stderr, "# OPT: %s ", linkerScript->path);
	
	_creation.nameModulo = linker->realm->nameModulo;
	_creation.symbolModulo = linker->realm->symbolModulo;
	the = xsCloneMachine(creation, linker->realm, "xsopt", linker);
	mxThrowElse(the);
	xsBeginHost(the);
	{
		xsVars(2);
		{
			xsTry {
				txScript* script = c_malloc(sizeof(txScript));
				xsAssert(script != NULL);
				c_memcpy(script, &xsScript, sizeof(txScript));
				script->symbolsBuffer = c_malloc(xsScript.symbolsSize);
				xsAssert(script->symbolsBuffer != NULL);
				c_memcpy(script->symbolsBuffer, xsScript.symbolsBuffer, xsScript.symbolsSize);
				script->codeBuffer = c_malloc(xsScript.codeSize);
				xsAssert(script->codeBuffer != NULL);
				c_memcpy(script->codeBuffer, xsScript.codeBuffer, xsScript.codeSize);
				
				linker->currentScript = linkerScript;
				fxRunScript(the, script, &mxGlobal, C_NULL, mxClosures.value.reference, C_NULL, C_NULL);
				mxPullSlot(mxResult);
				linker->currentScript = NULL;
			}
			xsCatch {
				xsStringValue message = xsToString(xsException);
				fxReportLinkerError(linker, "%s", message);
			}
		}
	}
	xsEndHost(the);
	xsDeleteMachine(the);
}

void fxOptimize(txLinker* linker)
{
	txLinkerScript* script = linker->firstScript;
	while (script) {
		fxOptimizeScript(linker, script);
		script = script->nextScript;
	}
	fprintf(stderr, "### OPT %d/%d BRANCH %d BRANCH_ELSE %d BRANCH_IF %d\n", 
		gxCodeSize - gxOptCodeSize, 
		gxCodeSize,
		gxOptBranches[0],
		gxOptBranches[1],
		gxOptBranches[2]
	);
}
