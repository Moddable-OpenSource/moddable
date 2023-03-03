/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Runtime.
 * 
 *   The Moddable SDK Runtime is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   The Moddable SDK Runtime is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 * 
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with the Moddable SDK Runtime.  If not, see <http://www.gnu.org/licenses/>.
 *
 * This file incorporates work covered by the following copyright and  
 * permission notice:  
 *
 *       Copyright (C) 2010-2016 Marvell International Ltd.
 *       Copyright (C) 2002-2010 Kinoma, Inc.
 *
 *       Licensed under the Apache License, Version 2.0 (the "License");
 *       you may not use this file except in compliance with the License.
 *       You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *       Unless required by applicable law or agreed to in writing, software
 *       distributed under the License is distributed on an "AS IS" BASIS,
 *       WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *       See the License for the specific language governing permissions and
 *       limitations under the License.
 */

#define _GNU_SOURCE
#include "xsCommon.h"
#ifndef mxUseDefaultCStackLimit
	#define mxUseDefaultCStackLimit 1
#endif

const txString gxCodeNames[XS_CODE_COUNT] = {
	"",
	/* XS_CODE_ADD */ "add",
	/* XS_CODE_ARGUMENT */ "argument",
	/* XS_CODE_ARGUMENTS */ "arguments",
	/* XS_CODE_ARGUMENTS_SLOPPY */ "arguments_sloppy",
	/* XS_CODE_ARGUMENTS_STRICT */ "arguments_strict",
	/* XS_CODE_ARRAY */ "array",
	/* XS_CODE_ASYNC_FUNCTION */ "async_function",
	/* XS_CODE_ASYNC_GENERATOR_FUNCTION */ "async_generator_function",
	/* XS_CODE_AT */ "at",
	/* XS_CODE_AWAIT */ "await",
	/* XS_CODE_BEGIN_SLOPPY */ "begin_sloppy",
	/* XS_CODE_BEGIN_STRICT */ "begin_strict",
	/* XS_CODE_BEGIN_STRICT_BASE */ "begin_strict_base",
	/* XS_CODE_BEGIN_STRICT_DERIVED */ "begin_strict_derived",
	/* XS_CODE_BEGIN_STRICT_FIELD */ "begin_strict_field",
	/* XS_CODE_BIGINT_1 */ "bigint",
	/* XS_CODE_BIGINT_2 */ "bigint_2",
	/* XS_CODE_BIT_AND */ "bit_and",
	/* XS_CODE_BIT_NOT */ "bit_not",
	/* XS_CODE_BIT_OR */ "bit_or",
	/* XS_CODE_BIT_XOR */ "bit_xor",
	/* XS_CODE_BRANCH_1 */ "branch",
	/* XS_CODE_BRANCH_2 */ "branch_2",
	/* XS_CODE_BRANCH_4 */ "branch_4",
	/* XS_CODE_BRANCH_CHAIN_1 */ "branch_chain",
	/* XS_CODE_BRANCH_CHAIN_2 */ "branch_chain_2",
	/* XS_CODE_BRANCH_CHAIN_4 */ "branch_chain_4",
	/* XS_CODE_BRANCH_COALESCE_1 */ "branch_coalesce",
	/* XS_CODE_BRANCH_COALESCE_2 */ "branch_coalesce_2",
	/* XS_CODE_BRANCH_COALESCE_4 */ "branch_coalesce_4",
	/* XS_CODE_BRANCH_ELSE_1 */ "branch_else",
	/* XS_CODE_BRANCH_ELSE_2 */ "branch_else_2",
	/* XS_CODE_BRANCH_ELSE_4 */ "branch_else_4",
	/* XS_CODE_BRANCH_IF_1 */ "branch_if",
	/* XS_CODE_BRANCH_IF_2 */ "branch_if_2",
	/* XS_CODE_BRANCH_IF_4 */ "branch_if_4",
	/* XS_CODE_BRANCH_STATUS_1 */ "branch_status",
	/* XS_CODE_BRANCH_STATUS_2 */ "branch_status_2",
	/* XS_CODE_BRANCH_STATUS_4 */ "branch_status_4",
	/* XS_CODE_CALL */ "call",
	/* XS_CODE_CATCH_1 */ "catch",
	/* XS_CODE_CATCH_2 */ "catch_2",
	/* XS_CODE_CATCH_4 */ "catch_4",
	/* XS_CODE_CHECK_INSTANCE */ "check_instance",
	/* XS_CODE_CLASS */ "class",
	/* XS_CODE_CODE_1 */ "code",
	/* XS_CODE_CODE_2 */ "code_2",
	/* XS_CODE_CODE_4 */ "code_4",
	/* XS_CODE_CODE_ARCHIVE_1 */ "code_archive",
	/* XS_CODE_CODE_ARCHIVE_2 */ "code_archive_2",
	/* XS_CODE_CODE_ARCHIVE_4 */ "code_archive_4",
	/* XS_CODE_CONST_CLOSURE_1 */ "const_closure",
	/* XS_CODE_CONST_CLOSURE_2 */ "const_closure_2",
	/* XS_CODE_CONST_LOCAL_1 */ "const_local",
	/* XS_CODE_CONST_LOCAL_2 */ "const_local_2",
	/* XS_CODE_CONSTRUCTOR_FUNCTION */ "constructor_function",
	/* XS_CODE_COPY_OBJECT */ "copy_object",
	/* XS_CODE_CURRENT */ "current",
	/* XS_CODE_DEBUGGER */ "debugger",
	/* XS_CODE_DECREMENT */ "decrement",
	/* XS_CODE_DELETE_PROPERTY */ "delete_property",
	/* XS_CODE_DELETE_PROPERTY_AT */ "delete_property_at",
	/* XS_CODE_DELETE_SUPER */ "delete_super",
	/* XS_CODE_DELETE_SUPER_AT */ "delete_super_at",
	/* XS_CODE_DIVIDE */ "divide",
	/* XS_CODE_DUB */ "dub",
	/* XS_CODE_DUB_AT */ "dub_at",
	/* XS_CODE_END */ "end",
	/* XS_CODE_END_ARROW */ "end_arrow",
	/* XS_CODE_END_BASE */ "end_base",
	/* XS_CODE_END_DERIVED */ "end_derived",
	/* XS_CODE_ENVIRONMENT */ "environment",
	/* XS_CODE_EQUAL */ "equal",
	/* XS_CODE_EVAL */ "eval",
	/* XS_CODE_EVAL_ENVIRONMENT */ "eval_environment",
	/* XS_CODE_EVAL_PRIVATE */ "eval_private",
	/* XS_CODE_EVAL_REFERENCE */ "eval_reference",
	/* XS_CODE_EVAL_TAIL */ "eval_tail",
	/* XS_CODE_EXCEPTION */ "exception",
	/* XS_CODE_EXPONENTIATION */ "exponentiation",
	/* XS_CODE_EXTEND */ "extend",
	/* XS_CODE_FALSE */ "false",
	/* XS_CODE_FILE */ "file",
	/* XS_CODE_FOR_AWAIT_OF */ "for_await_of",
	/* XS_CODE_FOR_IN */ "for_in",
	/* XS_CODE_FOR_OF */ "for_of",
	/* XS_CODE_FUNCTION */ "function",
	/* XS_CODE_FUNCTION_ENVIRONMENT */ "function_environment",
	/* XS_CODE_GENERATOR_FUNCTION */ "generator",
	/* XS_CODE_GET_CLOSURE_1 */ "get_closure",
	/* XS_CODE_GET_CLOSURE_2 */ "get_closure_2",
	/* XS_CODE_GET_LOCAL_1 */ "get_local",
	/* XS_CODE_GET_LOCAL_2 */ "get_local_2",
	/* XS_CODE_GET_PRIVATE_1 */ "get_private",
	/* XS_CODE_GET_PRIVATE_2 */ "get_private_2",
	/* XS_CODE_GET_PROPERTY */ "get_property",
	/* XS_CODE_GET_PROPERTY_AT */ "get_property_at",
	/* XS_CODE_GET_RESULT */ "get_result",
	/* XS_CODE_GET_SUPER */ "get_super",
	/* XS_CODE_GET_SUPER_AT */ "get_super_at",
	/* XS_CODE_GET_THIS */ "get_this",
	/* XS_CODE_GET_THIS_VARIABLE */ "get_this_variable",
	/* XS_CODE_GET_VARIABLE */ "get_variable",
	/* XS_CODE_GLOBAL */ "global",
	/* XS_CODE_HAS_PRIVATE_1 */ "has_private",
	/* XS_CODE_HAS_PRIVATE_2 */ "has_private_2",
	/* XS_CODE_HOST */ "host",
	/* XS_CODE_IMPORT */ "import",
	/* XS_CODE_IMPORT_META */ "import.meta",
	/* XS_CODE_IN */ "in",
	/* XS_CODE_INCREMENT */ "increment",
	/* XS_CODE_INSTANCEOF */ "instanceof",
	/* XS_CODE_INSTANTIATE */ "instantiate",
	/* XS_CODE_INTEGER_1 */ "integer",
	/* XS_CODE_INTEGER_2 */ "integer_2",
	/* XS_CODE_INTEGER_4 */ "integer_4",
	/* XS_CODE_LEFT_SHIFT */ "left_shift",
	/* XS_CODE_LESS */ "less",
	/* XS_CODE_LESS_EQUAL */ "less_equal",
	/* XS_CODE_LET_CLOSURE_1 */ "let_closure",
	/* XS_CODE_LET_CLOSURE_2 */ "let_closure_2",
	/* XS_CODE_LET_LOCAL_1 */ "let_local",
	/* XS_CODE_LET_LOCAL_2 */ "let_local_2",
	/* XS_CODE_LINE */ "line",
	/* XS_CODE_MINUS */ "minus",
	/* XS_CODE_MODULE */ "module",
	/* XS_CODE_MODULO */ "modulo",
	/* XS_CODE_MORE */ "more",
	/* XS_CODE_MORE_EQUAL */ "more_equal",
	/* XS_CODE_MULTIPLY */ "multiply",
	/* XS_CODE_NAME */ "name",
	/* XS_CODE_NEW */ "new",
	/* XS_CODE_NEW_CLOSURE */ "new_closure",
	/* XS_CODE_NEW_LOCAL */ "new_local",
	/* XS_CODE_NEW_PRIVATE_1 */ "new_private",
	/* XS_CODE_NEW_PRIVATE_2 */ "new_private_2",
	/* XS_CODE_NEW_PROPERTY */ "new_property",
	/* XS_CODE_NEW_PROPERTY_AT */ "new_property_at",
	/* XS_CODE_NEW_TEMPORARY */ "new_temporary",
	/* XS_CODE_NOT */ "not",
	/* XS_CODE_NOT_EQUAL */ "not_equal",
	/* XS_CODE_NULL */ "null",
	/* XS_CODE_NUMBER */ "number",
	/* XS_CODE_OBJECT */ "object",
	/* XS_CODE_PLUS */ "plus",
	/* XS_CODE_POP */ "pop",
	/* XS_CODE_PROGRAM_ENVIRONMENT */ "program_environment",
	/* XS_CODE_PROGRAM_REFERENCE */ "program_reference",
	/* XS_CODE_PULL_CLOSURE_1 */ "pull_closure",
	/* XS_CODE_PULL_CLOSURE_2 */ "pull_closure_2",
	/* XS_CODE_PULL_LOCAL_1 */ "pull_local",
	/* XS_CODE_PULL_LOCAL_2 */ "pull_local_2",
	/* XS_CODE_REFRESH_CLOSURE_1 */ "refresh_closure",
	/* XS_CODE_REFRESH_CLOSURE_2 */ "refresh_closure_2",
	/* XS_CODE_REFRESH_LOCAL_1 */ "refresh_local",
	/* XS_CODE_REFRESH_LOCAL_2 */ "refresh_local_2",
	/* XS_CODE_REGEXP */ "regexp",
	/* XS_CODE_RESERVE_1 */ "reserve",
	/* XS_CODE_RESERVE_2 */ "reserve_2",
	/* XS_CODE_RESET_CLOSURE_1 */ "reset_closure",
	/* XS_CODE_RESET_CLOSURE_2 */ "reset_closure_2",
	/* XS_CODE_RESET_LOCAL_1 */ "reset_local",
	/* XS_CODE_RESET_LOCAL_2 */ "reset_local_2",
	/* XS_CODE_RETHROW */ "rethrow",
	/* XS_CODE_RETRIEVE_1 */ "retrieve",
	/* XS_CODE_RETRIEVE_2 */ "retrieve_2",
	/* XS_CODE_RETRIEVE_TARGET */ "retrieve_target",
	/* XS_CODE_RETRIEVE_THIS */ "retrieve_this",
	/* XS_CODE_RETURN */ "return",
	/* XS_CODE_RUN */ "run",
	/* XS_CODE_RUN_1 */ "run_1",
	/* XS_CODE_RUN_2 */ "run_2",
	/* XS_CODE_RUN_4 */ "run_4",
	/* XS_CODE_RUN_TAIL */ "run_tail",
	/* XS_CODE_RUN_TAIL_1 */ "run_tail_1",
	/* XS_CODE_RUN_TAIL_2 */ "run_tail_2",
	/* XS_CODE_RUN_TAIL_4 */ "run_tail_4",
	/* XS_CODE_SET_CLOSURE_1 */ "set_closure",
	/* XS_CODE_SET_CLOSURE_2 */ "set_closure_2",
	/* XS_CODE_SET_HOME */ "set_home",
	/* XS_CODE_SET_LOCAL_1 */ "set_local",
	/* XS_CODE_SET_LOCAL_2 */ "set_local_2",
	/* XS_CODE_SET_PRIVATE_1 */ "set_private",
	/* XS_CODE_SET_PRIVATE_2 */ "set_private_2",
	/* XS_CODE_SET_PROPERTY */ "set_property",
	/* XS_CODE_SET_PROPERTY_AT */ "set_property_at",
	/* XS_CODE_SET_RESULT */ "set_result",
	/* XS_CODE_SET_SUPER */ "set_super",
	/* XS_CODE_SET_SUPER_AT */ "set_super_at",
	/* XS_CODE_SET_THIS */ "set_this",
	/* XS_CODE_SET_VARIABLE */ "set_variable",
	/* XS_CODE_SIGNED_RIGHT_SHIFT */ "signed_right_shift",
	/* XS_CODE_START_ASYNC */ "start_async",
	/* XS_CODE_START_ASYNC_GENERATOR */ "start_async_generator",
	/* XS_CODE_START_GENERATOR */ "start_generator",
	/* XS_CODE_STORE_1 */ "store_1",
	/* XS_CODE_STORE_2 */ "store_2",
	/* XS_CODE_STORE_ARROW */ "store_arrow",
	/* XS_CODE_STRICT_EQUAL */ "strict_equal",
	/* XS_CODE_STRICT_NOT_EQUAL */ "strict_not_equal",
	/* XS_CODE_STRING_1 */ "string",
	/* XS_CODE_STRING_2 */ "string_2",
	/* XS_CODE_STRING_4 */ "string_4",
	/* XS_CODE_STRING_ARCHIVE_1 */ "string_archive",
	/* XS_CODE_STRING_ARCHIVE_2 */ "string_archive_2",
	/* XS_CODE_STRING_ARCHIVE_4 */ "string_archive_4",
	/* XS_CODE_SUBTRACT */ "subtract",
	/* XS_CODE_SUPER */ "super",
	/* XS_CODE_SWAP */ "swap",
	/* XS_CODE_SYMBOL */ "symbol",
	/* XS_CODE_TARGET */ "target",
	/* XS_CODE_TEMPLATE */ "template",
	/* XS_CODE_TEMPLATE_CACHE */ "template_cache",
	/* XS_CODE_THIS */ "this",
	/* XS_CODE_THROW */ "throw",
	/* XS_CODE_THROW_STATUS */ "throw_status",
	/* XS_CODE_TO_INSTANCE */ "to_instance",
	/* XS_CODE_TO_NUMERIC */ "to_numeric",
	/* XS_CODE_TO_STRING */ "to_string",
	/* XS_CODE_TRANSFER */ "transfer",
	/* XS_CODE_TRUE */ "true",
	/* XS_CODE_TYPEOF */ "typeof",
	/* XS_CODE_UNCATCH */ "uncatch",
	/* XS_CODE_UNDEFINED */ "undefined",
	/* XS_CODE_UNSIGNED_RIGHT_SHIFT */ "unsigned_right_shift",
	/* XS_CODE_UNWIND_1 */ "unwind",
	/* XS_CODE_UNWIND_2 */ "unwind_2",
	/* XS_CODE_VAR_CLOSURE_1 */ "var_closure_1",
	/* XS_CODE_VAR_CLOSURE_2 */ "var_closure_2",
	/* XS_CODE_VAR_LOCAL_1 */ "var_local_1",
	/* XS_CODE_VAR_LOCAL_2 */ "var_local_2",
	/* XS_CODE_VOID */ "void",
	/* XS_CODE_WITH */ "with",
	/* XS_CODE_WITHOUT */ "without",
	/* XS_CODE_YIELD */ "yield",
	/* XS_CODE_PROFILE */ "profile"
};

const txS1 gxCodeSizes[XS_CODE_COUNT] ICACHE_FLASH_ATTR = {
	0 /* XS_NO_CODE */,
	1 /* XS_CODE_ADD */,
	2 /* XS_CODE_ARGUMENT */,
	2 /* XS_CODE_ARGUMENTS */,
	2 /* XS_CODE_ARGUMENTS_SLOPPY */,
	2 /* XS_CODE_ARGUMENTS_STRICT */,
	1 /* XS_CODE_ARRAY */,
	0 /* XS_CODE_ASYNC_FUNCTION */,
	0 /* XS_CODE_ASYNC_GENERATOR_FUNCTION */,
	1 /* XS_CODE_AT */,
	1 /* XS_CODE_AWAIT */,
	2 /* XS_CODE_BEGIN_SLOPPY */,
	2 /* XS_CODE_BEGIN_STRICT */,
	2 /* XS_CODE_BEGIN_STRICT_BASE */,
	2 /* XS_CODE_BEGIN_STRICT_DERIVED */,
	2 /* XS_CODE_BEGIN_STRICT_FIELD */,
	-1 /* XS_CODE_BIGINT_1 */,
	-2 /* XS_CODE_BIGINT_2 */,
	1 /* XS_CODE_BIT_AND */,
	1 /* XS_CODE_BIT_NOT */,
	1 /* XS_CODE_BIT_OR */,
	1 /* XS_CODE_BIT_XOR */,
	2 /* XS_CODE_BRANCH_1 */,
	3 /* XS_CODE_BRANCH_2 */,
	5 /* XS_CODE_BRANCH_4 */,
	2 /* XS_CODE_BRANCH_CHAIN_1 */,
	3 /* XS_CODE_BRANCH_CHAIN_2 */,
	5 /* XS_CODE_BRANCH_CHAIN_4 */,
	2 /* XS_CODE_BRANCH_COALESCE_1 */,
	3 /* XS_CODE_BRANCH_COALESCE_2 */,
	5 /* XS_CODE_BRANCH_COALESCE_4 */,
	2 /* XS_CODE_BRANCH_ELSE_1 */,
	3 /* XS_CODE_BRANCH_ELSE_2 */,
	5 /* XS_CODE_BRANCH_ELSE_4 */,
	2 /* XS_CODE_BRANCH_IF_1 */,
	3 /* XS_CODE_BRANCH_IF_2 */,
	5 /* XS_CODE_BRANCH_IF_4 */,
	2 /* XS_CODE_BRANCH_STATUS_1 */,
	3 /* XS_CODE_BRANCH_STATUS_2 */,
	5 /* XS_CODE_BRANCH_STATUS_4 */,
	1 /* XS_CODE_CALL */,
	2 /* XS_CODE_CATCH_1 */,
	3 /* XS_CODE_CATCH_2 */,
	5 /* XS_CODE_CATCH_4 */,
	1 /* XS_CODE_CLASS */,
	1 /* XS_CODE_CHECK_INSTANCE */,
	2 /* XS_CODE_CODE_1 */,
	3 /* XS_CODE_CODE_2 */,
	5 /* XS_CODE_CODE_4 */,
	2 /* XS_CODE_CODE_ARCHIVE_1 */,
	3 /* XS_CODE_CODE_ARCHIVE_2 */,
	5 /* XS_CODE_CODE_ARCHIVE_4 */,
	2 /* XS_CODE_CONST_CLOSURE_1 */,
	3 /* XS_CODE_CONST_CLOSURE_2 */,
	2 /* XS_CODE_CONST_LOCAL_1 */,
	3 /* XS_CODE_CONST_LOCAL_2 */,
	0 /* XS_CODE_CONSTRUCTOR_FUNCTION */,
	1 /* XS_CODE_COPY_OBJECT */,
	1 /* XS_CODE_CURRENT */,
	1 /* XS_CODE_DEBUGGER */,
	1 /* XS_CODE_DECREMENT */,
	0 /* XS_CODE_DELETE_PROPERTY */,
	1 /* XS_CODE_DELETE_PROPERTY_AT */,
	0 /* XS_CODE_DELETE_SUPER */,
	1 /* XS_CODE_DELETE_SUPER_AT */,
	1 /* XS_CODE_DIVIDE */,
	1 /* XS_CODE_DUB */,
	1 /* XS_CODE_DUB_AT */,
	1 /* XS_CODE_END */,
	1 /* XS_CODE_END_ARROW */,
	1 /* XS_CODE_END_BASE */,
	1 /* XS_CODE_END_DERIVED */,
	1 /* XS_CODE_ENVIRONMENT */,
	1 /* XS_CODE_EQUAL */,
	1 /* XS_CODE_EVAL */,
	1 /* XS_CODE_EVAL_ENVIRONMENT */,
	0 /* XS_CODE_EVAL_PRIVATE */,
	0 /* XS_CODE_EVAL_REFERENCE */,
	1 /* XS_CODE_EVAL_TAIL */,
	1 /* XS_CODE_EXCEPTION */,
	1 /* XS_CODE_EXPONENTIATION */,
	1 /* XS_CODE_EXTEND */,
	1 /* XS_CODE_FALSE */,
	0 /* XS_CODE_FILE */,
	1 /* XS_CODE_FOR_AWAIT_OF */,
	1 /* XS_CODE_FOR_IN */,
	1 /* XS_CODE_FOR_OF */,
	0 /* XS_CODE_FUNCTION */,
	1 /* XS_CODE_FUNCTION_ENVIRONMENT */,
	0 /* XS_CODE_GENERATOR_FUNCTION */,
	2 /* XS_CODE_GET_CLOSURE_1 */,
	3 /* XS_CODE_GET_CLOSURE_2 */,
	2 /* XS_CODE_GET_LOCAL_1 */,
	3 /* XS_CODE_GET_LOCAL_2 */,
	2 /* XS_CODE_GET_PRIVATE_1 */,
	3 /* XS_CODE_GET_PRIVATE_2 */,
	0 /* XS_CODE_GET_PROPERTY */,
	1 /* XS_CODE_GET_PROPERTY_AT */,
	1 /* XS_CODE_GET_RESULT */,
	0 /* XS_CODE_GET_SUPER */,
	1 /* XS_CODE_GET_SUPER_AT */,
	1 /* XS_CODE_GET_THIS */,
	0 /* XS_CODE_GET_THIS_VARIABLE */,
	0 /* XS_CODE_GET_VARIABLE */,
	1 /* XS_CODE_GLOBAL */,
	2 /* XS_CODE_HAS_PRIVATE_1 */,
	3 /* XS_CODE_HAS_PRIVATE_2 */,
	3 /* XS_CODE_HOST */,
	1 /* XS_CODE_IMPORT */,
	1 /* XS_CODE_IMPORT_META */,
	1 /* XS_CODE_IN */,
	1 /* XS_CODE_INCREMENT */,
	1 /* XS_CODE_INSTANCEOF */,
	1 /* XS_CODE_INSTANTIATE */,
	2 /* XS_CODE_INTEGER_1 */,
	3 /* XS_CODE_INTEGER_2 */,
	5 /* XS_CODE_INTEGER_4 */,
	1 /* XS_CODE_LEFT_SHIFT */,
	1 /* XS_CODE_LESS */,
	1 /* XS_CODE_LESS_EQUAL */,
	2 /* XS_CODE_LET_CLOSURE_1 */,
	3 /* XS_CODE_LET_CLOSURE_2 */,
	2 /* XS_CODE_LET_LOCAL_1 */,
	3 /* XS_CODE_LET_LOCAL_2 */,
	3 /* XS_CODE_LINE */,
	1 /* XS_CODE_MINUS */,
	2 /* XS_CODE_MODULE */,
	1 /* XS_CODE_MODULO */,
	1 /* XS_CODE_MORE */,
	1 /* XS_CODE_MORE_EQUAL */,
	1 /* XS_CODE_MULTIPLY */,
	0 /* XS_CODE_NAME */,
	1 /* XS_CODE_NEW */,
	0 /* XS_CODE_NEW_CLOSURE */,
	0 /* XS_CODE_NEW_LOCAL */,
	2 /* XS_CODE_NEW_PRIVATE_1 */,
	3 /* XS_CODE_NEW_PRIVATE_2 */,
	0 /* XS_CODE_NEW_PROPERTY */,
	1 /* XS_CODE_NEW_PROPERTY_AT */,
	1 /* XS_CODE_NEW_TEMPORARY */,
	1 /* XS_CODE_NOT */,
	1 /* XS_CODE_NOT_EQUAL */,
	1 /* XS_CODE_NULL */,
	9 /* XS_CODE_NUMBER */,
	1 /* XS_CODE_OBJECT */,
	1 /* XS_CODE_PLUS */,
	1 /* XS_CODE_POP */,
	1 /* XS_CODE_PROGRAM_ENVIRONMENT */,
	0 /* XS_CODE_PROGRAM_REFERENCE */,
	2 /* XS_CODE_PULL_CLOSURE_1 */,
	3 /* XS_CODE_PULL_CLOSURE_2 */,
	2 /* XS_CODE_PULL_LOCAL_1 */,
	3 /* XS_CODE_PULL_LOCAL_2 */,
	2 /* XS_CODE_REFRESH_CLOSURE_1 */,
	3 /* XS_CODE_REFRESH_CLOSURE_2 */,
	2 /* XS_CODE_REFRESH_LOCAL_1 */,
	3 /* XS_CODE_REFRESH_LOCAL_2 */,
	1 /* XS_CODE_REGEXP */,
	2 /* XS_CODE_RESERVE_1 */,
	3 /* XS_CODE_RESERVE_2 */,
	2 /* XS_CODE_RESET_CLOSURE_1 */,
	3 /* XS_CODE_RESET_CLOSURE_2 */,
	2 /* XS_CODE_RESET_LOCAL_1 */,
	3 /* XS_CODE_RESET_LOCAL_2 */,
	1 /* XS_CODE_RETHROW */,
	2 /* XS_CODE_RETRIEVE_1 */,
	3 /* XS_CODE_RETRIEVE_2 */,
	1 /* XS_CODE_RETRIEVE_TARGET */,
	1 /* XS_CODE_RETRIEVE_THIS */,
	1 /* XS_CODE_RETURN */,
	1 /* XS_CODE_RUN */,
	2 /* XS_CODE_RUN_1 */,
	3 /* XS_CODE_RUN_2 */,
	5 /* XS_CODE_RUN_4 */,
	1 /* XS_CODE_RUN_TAIL */,
	2 /* XS_CODE_RUN_TAIL_1 */,
	3 /* XS_CODE_RUN_TAIL_2 */,
	5 /* XS_CODE_RUN_TAIL_4 */,
	2 /* XS_CODE_SET_CLOSURE_1 */,
	3 /* XS_CODE_SET_CLOSURE_2 */,
	1 /* XS_CODE_SET_HOME */,
	2 /* XS_CODE_SET_LOCAL_1 */,
	3 /* XS_CODE_SET_LOCAL_2 */,
	2 /* XS_CODE_SET_PRIVATE_1 */,
	3 /* XS_CODE_SET_PRIVATE_2 */,
	0 /* XS_CODE_SET_PROPERTY */,
	1 /* XS_CODE_SET_PROPERTY_AT */,
	1 /* XS_CODE_SET_RESULT */,
	0 /* XS_CODE_SET_SUPER */,
	1 /* XS_CODE_SET_SUPER_AT */,
	1 /* XS_CODE_SET_THIS */,
	0 /* XS_CODE_SET_VARIABLE */,
	1 /* XS_CODE_SIGNED_RIGHT_SHIFT */,
	1 /* XS_CODE_START_ASYNC */,
	1 /* XS_CODE_START_ASYNC_GENERATOR */,
	1 /* XS_CODE_START_GENERATOR */,
	2 /* XS_CODE_STORE_1 */,
	3 /* XS_CODE_STORE_2 */,
	1 /* XS_CODE_STORE_ARROW */,
	1 /* XS_CODE_STRICT_EQUAL */,
	1 /* XS_CODE_STRICT_NOT_EQUAL */,
	-1 /* XS_CODE_STRING_1 */,
	-2 /* XS_CODE_STRING_2 */,
	-4 /* XS_CODE_STRING_4 */,
	-1 /* XS_CODE_STRING_ARCHIVE_1 */,
	-2 /* XS_CODE_STRING_ARCHIVE_2 */,
	-4 /* XS_CODE_STRING_ARCHIVE_2 */,
	1 /* XS_CODE_SUBTRACT */,
	1 /* XS_CODE_SUPER */,
	1 /* XS_CODE_SWAP */,
	0 /* XS_CODE_SYMBOL */,
	1 /* XS_CODE_TARGET */,
	1 /* XS_CODE_TEMPLATE */,
	1 /* XS_CODE_TEMPLATE_CACHE */,
	1 /* XS_CODE_THIS */,
	1 /* XS_CODE_THROW */,
	1 /* XS_CODE_THROW_STATUS */,
	1 /* XS_CODE_TO_INSTANCE */,
	1 /* XS_CODE_TO_NUMERIC */,
	1 /* XS_CODE_TO_STRING */,
	1 /* XS_CODE_TRANSFER */,
	1 /* XS_CODE_TRUE */,
	1 /* XS_CODE_TYPEOF */,
	1 /* XS_CODE_UNCATCH */,
	1 /* XS_CODE_UNDEFINED */,
	1 /* XS_CODE_UNSIGNED_RIGHT_SHIFT */,
	2 /* XS_CODE_UNWIND_1 */,
	3 /* XS_CODE_UNWIND_2 */,
	2 /* XS_CODE_VAR_CLOSURE_1 */,
	3 /* XS_CODE_VAR_CLOSURE_2 */,
	2 /* XS_CODE_VAR_LOCAL_1 */,
	3 /* XS_CODE_VAR_LOCAL_2 */,
	1 /* XS_CODE_VOID */,
	1 /* XS_CODE_WITH */,
	1 /* XS_CODE_WITHOUT */,
	1 /* XS_CODE_YIELD */,
#ifdef mx32bitID
	5 /* XS_CODE_PROFILE */
#else
	3 /* XS_CODE_PROFILE */
#endif
};

#if mxUseDefaultCStackLimit

#ifdef __ets__
	#if ESP32
		#include "freertos/task.h"
	#else
		#include "cont.h"
	#endif
#endif

#ifndef mxASANStackMargin
	#define mxASANStackMargin 0
#endif

char* fxCStackLimit()
{
	#if mxWindows
		ULONG_PTR low, high;
		GetCurrentThreadStackLimits(&low, &high);
		return (char*)low + (16 * 1024) + mxASANStackMargin;
	#elif mxMacOSX
		pthread_t self = pthread_self();
    	void* stackAddr = pthread_get_stackaddr_np(self);
   		size_t stackSize = pthread_get_stacksize_np(self);
		return (char*)stackAddr - stackSize + (128 * 1024) + mxASANStackMargin;
	#elif mxLinux
		char* result = C_NULL;
		pthread_attr_t attrs;
		pthread_attr_init(&attrs);
		if (pthread_getattr_np(pthread_self(), &attrs) == 0) {
    		void* stackAddr;
   			size_t stackSize;
			if (pthread_attr_getstack(&attrs, &stackAddr, &stackSize) == 0) {
				result = (char*)stackAddr + (128 * 1024) + mxASANStackMargin;
			}
		}
		pthread_attr_destroy(&attrs);
		return result;
	#elif defined(__ets__) && !ESP32
		extern cont_t g_cont;
		return 192 + (char *)g_cont.stack;
	#elif defined(__ets__) && ESP32
		TaskStatus_t info;
		vTaskGetTaskInfo(NULL, &info, pdFALSE, eReady);
		return 512 + (char *)info.pxStackBase;
	#else
		return C_NULL;
	#endif
}

#endif

void fxDeleteScript(txScript* script)
{
	if (script) {
		if (script->symbolsBuffer)
			c_free(script->symbolsBuffer);
		if (script->hostsBuffer)
			c_free(script->hostsBuffer);
		if (script->codeBuffer)
			c_free(script->codeBuffer);
		c_free(script);
	}
}


const txUTF8Sequence gxUTF8Sequences[] ICACHE_RODATA_ATTR = {
	{1, 0x80, 0x00, 0*6, 0x0000007F, 0x00000000},
	{2, 0xE0, 0xC0, 1*6, 0x000007FF, 0x00000080},
	{3, 0xF0, 0xE0, 2*6, 0x0000FFFF, 0x00000800},
	{4, 0xF8, 0xF0, 3*6, 0x001FFFFF, 0x00010000},
	{5, 0xFC, 0xF8, 4*6, 0x03FFFFFF, 0x00200000},
	{6, 0xFE, 0xFC, 5*6, 0x7FFFFFFF, 0x04000000},
	{0, 0, 0, 0, 0, 0},
};

static const char gxHexLower[] ICACHE_FLASH_ATTR = "0123456789abcdef";
static const char gxHexUpper[] ICACHE_FLASH_ATTR = "0123456789ABCDEF";
static txBoolean fxParseHex(txU1 c, txU4* value);

txBoolean fxIsIdentifierFirst(txU4 c)
{
	#define mxIdentifierFirstCount 1300
	static const txU2 gxIdentifierFirstTable[mxIdentifierFirstCount] ICACHE_RODATA_ATTR = {
		36,0,29,25,5,0,2,25,48,0,11,0,5,0,6,22,2,30,2,457,5,11,15,4,8,0,2,0,130,4,2,1,
		3,3,2,0,7,0,2,2,2,0,2,19,2,82,2,138,9,165,2,37,3,0,7,40,72,26,5,3,46,42,36,1,
		2,98,2,0,16,1,8,1,11,2,3,0,17,0,2,29,30,88,12,0,25,32,10,1,5,0,6,21,5,0,10,0,
		4,0,24,24,8,10,6,23,2,5,18,41,59,53,4,0,19,0,8,9,16,15,5,7,3,1,3,21,2,6,2,0,
		4,3,4,0,17,0,14,1,2,2,15,1,11,0,9,5,5,1,3,21,2,6,2,1,2,1,2,1,32,3,2,0,
		20,2,17,8,2,2,2,21,2,6,2,1,2,4,4,0,19,0,16,1,24,0,12,7,3,1,3,21,2,6,2,1,
		2,4,4,0,31,1,2,2,16,0,18,0,2,5,4,2,2,3,4,1,2,0,2,1,4,1,4,2,4,11,23,0,
		53,7,2,2,2,22,2,15,4,0,27,2,3,0,3,1,31,0,5,7,2,2,2,22,2,9,2,4,4,0,32,1,
		2,1,16,1,18,8,2,2,2,40,3,0,17,0,6,2,9,2,25,5,6,17,4,23,2,8,2,0,3,6,59,47,
		2,1,13,6,59,1,2,0,2,4,2,23,2,0,2,9,2,1,10,0,3,4,2,0,22,3,33,0,64,7,2,35,
		28,4,116,42,21,0,17,5,5,3,4,0,4,1,8,2,5,12,13,0,18,37,2,0,6,0,3,42,2,332,2,3,
		3,6,2,0,2,3,3,40,2,3,3,32,2,3,3,6,2,0,2,3,3,14,2,56,2,3,3,66,38,15,17,85,
		3,5,4,619,3,16,2,25,6,74,4,10,8,17,14,18,15,17,15,12,2,2,16,51,36,0,5,0,68,88,8,40,
		2,0,6,69,11,30,50,29,3,4,12,43,5,25,55,22,10,52,83,0,94,46,18,7,55,29,14,1,11,43,27,35,
		42,2,11,35,3,8,8,42,3,2,42,3,2,5,2,1,4,0,6,191,65,277,3,5,3,37,3,5,3,7,2,0,
		2,0,2,0,2,30,3,52,2,6,2,0,4,2,2,6,4,3,3,5,5,12,6,2,2,6,117,0,14,0,17,12,
		102,0,5,0,3,9,2,0,3,5,7,0,2,0,2,0,2,15,3,3,6,4,5,0,18,40,2680,228,7,3,4,1,
		13,37,2,0,6,0,3,55,8,0,17,22,10,6,2,6,2,6,2,6,2,6,2,6,2,6,2,6,551,2,26,8,
		8,4,3,4,5,85,5,4,2,89,2,3,6,42,2,93,18,31,49,15,513,6591,65,22156,68,45,3,268,4,15,11,1,
		21,46,17,30,3,79,40,8,3,102,3,63,6,1,2,0,2,4,25,15,2,2,2,3,2,22,30,51,15,49,63,5,
		4,0,2,1,12,27,11,22,26,28,8,46,29,0,17,4,2,9,11,4,2,40,24,2,2,7,21,22,4,0,4,49,
		2,0,4,1,3,4,3,0,2,0,25,2,3,10,8,2,13,5,3,5,3,5,10,6,2,6,2,42,2,13,7,114,
		30,11171,13,22,5,48,8453,365,3,105,39,6,13,4,6,0,2,9,2,12,2,4,2,0,2,1,2,1,2,107,34,362,
		19,63,3,53,41,11,117,4,2,134,37,25,7,25,12,88,4,5,3,5,3,5,3,2,36,11,2,25,2,18,2,1,
		2,14,3,13,35,122,70,52,268,28,4,48,48,31,14,29,6,37,11,29,3,35,5,7,2,4,43,157,19,35,5,35,
		5,39,9,51,13,10,2,14,2,6,2,1,2,10,2,14,2,6,2,1,68,310,10,21,11,7,25,5,2,41,2,8,
		70,5,3,0,2,43,2,1,4,0,3,22,11,22,10,30,66,18,2,1,11,21,11,25,71,55,7,1,65,0,16,3,
		2,2,2,28,43,28,4,28,36,7,2,27,28,53,11,21,11,18,14,17,111,72,56,50,14,50,14,35,349,41,7,1,
		79,28,11,0,9,21,43,17,47,20,28,22,13,52,58,1,3,0,14,44,33,24,27,35,30,0,3,0,9,34,4,0,
		13,47,15,3,22,0,2,0,36,17,2,24,85,6,2,0,2,3,2,14,2,9,8,46,39,7,3,1,3,21,2,6,
		2,1,2,4,4,0,19,0,13,4,159,52,19,3,21,2,31,47,21,1,2,0,185,46,42,3,37,47,21,0,60,42,
		14,0,72,26,38,6,186,43,117,63,32,7,3,0,3,7,2,1,2,23,16,0,2,0,95,7,3,38,17,0,2,0,
		29,0,11,39,8,0,22,0,12,45,20,0,19,72,264,8,2,36,18,0,50,29,113,6,2,1,2,37,22,0,26,5,
		2,1,2,31,15,0,328,18,190,0,80,921,103,110,18,195,2637,96,16,1070,4050,582,8634,568,8,30,18,78,18,29,19,47,
		17,3,32,20,6,18,689,63,129,74,6,0,67,12,65,1,2,0,29,6135,9,1237,43,8,8936,3,2,6,2,1,2,290,
		46,2,18,3,9,395,2309,106,6,12,4,8,8,9,5991,84,2,70,2,1,3,0,3,1,3,3,2,11,2,0,2,6,
		2,64,2,3,3,7,2,6,2,27,2,3,2,4,2,0,4,6,2,339,3,24,2,24,2,30,2,24,2,30,2,24,
		2,30,2,24,2,30,2,24,2,7,1845,30,482,44,11,6,17,0,322,29,19,43,1269,6,2,3,2,1,2,14,2,196,
		60,67,8,0,1205,3,2,26,2,1,2,0,3,0,2,9,2,3,2,0,2,0,7,0,5,0,2,0,2,0,2,2,
		2,1,2,0,3,0,2,0,2,0,2,0,2,0,2,1,2,0,3,3,2,6,2,3,2,3,2,0,2,9,2,16,
		6,2,2,4,2,16,4421,42719,33,4152,8,221,3,5761,15,7472,3104,541,1507,4938,
	};
	const txU2* p = gxIdentifierFirstTable;
	const txU2* q = p + mxIdentifierFirstCount;
	txU4 s = 0;
	while (p < q) {
		s += c_read16(p++);
		if (c < s)
			return 0;
		s += c_read16(p++);
		if (c <= s)
			return 1;
	}
	return 0;
}

txBoolean fxIsIdentifierNext(txU4 c)
{
	#define mxIdentifierNextCount 1514
	static const txU2 gxIdentifierNextTable[mxIdentifierNextCount] ICACHE_RODATA_ATTR = {
		36,0,12,9,8,25,5,0,2,25,48,0,11,0,2,0,3,0,6,22,2,30,2,457,5,11,15,4,8,0,2,0,
		18,116,2,1,3,3,2,0,7,4,2,0,2,19,2,82,2,138,2,4,3,165,2,37,3,0,7,40,9,44,2,0,
		2,1,2,1,2,0,9,26,5,3,30,10,6,73,5,101,2,7,3,9,2,18,3,0,17,58,3,100,15,53,5,0,
		3,0,3,45,19,27,5,10,6,23,2,5,10,73,2,128,3,9,2,18,2,7,3,1,3,21,2,6,2,0,4,3,
		3,8,3,1,3,3,9,0,5,1,2,4,3,11,11,0,2,0,3,2,2,5,5,1,3,21,2,6,2,1,2,1,
		2,1,3,0,2,4,5,1,3,2,4,0,8,3,2,0,8,15,12,2,2,8,2,2,2,21,2,6,2,1,2,4,
		3,9,2,2,2,2,3,0,16,3,3,9,10,6,2,2,2,7,3,1,3,21,2,6,2,1,2,4,3,8,3,1,
		3,2,8,2,5,1,2,4,3,9,2,0,17,1,2,5,4,2,2,3,4,1,2,0,2,1,4,1,4,2,4,11,
		5,4,4,2,2,3,3,0,7,0,15,9,17,12,2,2,2,22,2,15,3,8,2,2,2,3,8,1,2,2,3,0,
		3,3,3,9,17,3,2,7,2,2,2,22,2,9,2,4,3,8,2,2,2,3,8,1,7,1,2,3,3,9,2,1,
		14,12,2,2,2,50,2,2,2,4,6,3,8,4,3,9,11,5,2,2,2,17,4,23,2,8,2,0,3,6,4,0,
		5,5,2,0,2,7,7,9,3,1,14,57,6,14,2,9,40,1,2,0,2,4,2,23,2,0,2,22,3,4,2,0,
		2,5,3,9,3,3,33,0,24,1,7,9,12,0,2,0,2,0,5,9,2,35,5,19,2,17,2,35,10,0,58,73,
		7,77,3,37,2,0,6,0,3,42,2,332,2,3,3,6,2,0,2,3,3,40,2,3,3,32,2,3,3,6,2,0,
		2,3,3,14,2,56,2,3,3,66,3,2,10,8,15,15,17,85,3,5,4,619,3,16,2,25,6,74,4,10,8,21,
		10,21,12,19,13,12,2,2,2,1,13,83,4,0,5,1,3,9,34,2,2,10,7,88,8,42,6,69,11,30,2,11,
		5,11,11,39,3,4,12,43,5,25,7,10,38,27,5,62,2,28,3,10,7,9,14,0,9,13,2,15,50,76,4,9,
		18,8,13,115,13,55,9,9,4,48,3,8,8,42,3,2,17,2,2,38,6,533,3,5,3,37,3,5,3,7,2,0,
		2,0,2,0,2,30,3,52,2,6,2,0,4,2,2,6,4,3,3,5,5,12,6,2,2,6,16,1,50,1,20,0,
		29,0,14,0,17,12,52,12,5,0,4,11,18,0,5,0,3,9,2,0,3,5,7,0,2,0,2,0,2,15,3,3,
		6,4,5,0,18,40,2680,228,7,8,13,37,2,0,6,0,3,55,8,0,16,23,10,6,2,6,2,6,2,6,2,6,
		2,6,2,6,2,6,2,31,518,2,26,14,2,4,3,4,5,85,3,6,2,89,2,3,6,42,2,93,18,31,49,15,
		513,6591,65,22156,68,45,3,268,4,27,21,47,5,9,2,114,38,8,3,102,3,63,6,1,2,0,2,4,25,53,5,0,
		20,51,13,69,11,9,7,23,4,0,2,48,3,35,13,28,4,64,15,10,7,30,2,54,10,13,3,9,7,22,4,72,
		25,2,3,15,3,4,11,5,3,5,3,5,10,6,2,6,2,42,2,13,7,122,2,1,3,9,7,11171,13,22,5,48,
		8453,365,3,105,39,6,13,4,6,11,2,12,2,4,2,0,2,1,2,1,2,107,34,362,19,63,3,53,41,11,5,15,
		17,15,4,1,25,2,33,4,2,134,20,9,8,25,5,0,2,25,12,88,4,5,3,5,3,5,3,2,36,11,2,25,
		2,18,2,1,2,14,3,13,35,122,70,52,137,0,131,28,4,48,16,0,32,31,14,29,6,42,6,29,3,35,5,7,
		2,4,43,157,3,9,7,35,5,35,5,39,9,51,13,10,2,14,2,6,2,1,2,10,2,14,2,6,2,1,68,310,
		10,21,11,7,25,5,2,41,2,8,70,5,3,0,2,43,2,1,4,0,3,22,11,22,10,30,66,18,2,1,11,21,
		11,25,71,55,7,1,65,3,2,1,6,7,2,2,2,28,3,2,5,0,33,28,4,28,36,7,2,29,26,53,11,21,
		11,18,14,17,111,72,56,50,14,50,14,39,9,9,327,41,2,1,4,1,79,28,11,0,9,32,32,21,43,20,28,22,
		10,70,32,15,10,59,8,0,14,24,8,9,7,52,2,9,5,3,9,35,3,0,10,68,5,3,2,12,2,0,36,17,
		2,36,7,0,66,6,2,0,2,3,2,14,2,9,8,58,6,9,7,3,2,7,3,1,3,21,2,6,2,1,2,4,
		2,9,3,1,3,2,3,0,7,0,6,6,3,6,4,4,140,74,6,9,5,3,31,69,2,0,9,9,167,53,3,8,
		24,5,35,64,4,0,12,9,39,56,8,9,55,26,3,14,5,9,7,6,186,58,102,73,22,7,3,0,3,7,2,1,
		2,29,2,1,3,8,13,9,71,7,3,45,3,7,2,1,28,62,9,0,9,73,4,0,19,72,264,8,2,44,2,8,
		16,9,25,29,3,21,2,13,74,6,2,1,2,43,4,0,2,1,2,8,9,9,7,5,2,1,2,36,2,1,2,5,
		8,9,311,22,186,0,80,921,103,110,18,195,2637,96,16,1070,4050,582,8634,568,8,30,2,9,7,78,2,9,7,29,3,4,
		12,54,10,3,13,9,10,20,6,18,689,63,129,74,5,56,8,16,65,1,2,1,12,1,15,6135,9,1237,43,8,8936,3,
		2,6,2,1,2,290,46,2,18,3,9,395,2309,106,6,12,4,8,8,9,4,1,4706,45,3,22,543,4,4,5,9,7,
		3,6,31,3,149,2,444,84,2,70,2,1,3,0,3,1,3,3,2,11,2,0,2,6,2,64,2,3,3,7,2,6,
		2,27,2,3,2,4,2,0,4,6,2,339,3,24,2,24,2,30,2,24,2,30,2,24,2,30,2,24,2,30,2,24,
		2,7,3,49,513,54,5,49,9,0,15,0,23,4,2,14,1105,30,226,6,2,16,3,6,2,1,2,4,214,44,4,13,
		3,9,5,0,322,30,18,57,1255,6,2,3,2,1,2,14,2,196,12,6,42,75,5,9,1191,3,2,26,2,1,2,0,
		3,0,2,9,2,3,2,0,2,0,7,0,5,0,2,0,2,0,2,2,2,1,2,0,3,0,2,0,2,0,2,0,
		2,0,2,1,2,0,3,3,2,6,2,3,2,3,2,0,2,9,2,16,6,2,2,4,2,16,3381,9,1031,42719,33,4152,
		8,221,3,5761,15,7472,3104,541,1507,4938,
	};
	const txU2* p = gxIdentifierNextTable;
	const txU2* q = p + mxIdentifierNextCount;
	txU4 s = 0;
	while (p < q) {
		s += c_read16(p++);
		if (c < s)
			return 0;
		s += c_read16(p++);
		if (c <= s)
			return 1;
	}
	s += 716214;
	if (c < s)
		return 0;
	s += 239;
	if (c <= s)
		return 1;
	return 0;
}

txBoolean fxIsSpace(txInteger character)
{
	static const txU4 spaces[27] ICACHE_RODATA_ATTR = {
		0x00000009,
        0x0000000A,
        0x0000000B,
        0x0000000C,
        0x0000000D,
		0x00000020,
		0x000000A0,
		0x00001680,
		0x00002000,
		0x00002001,
		0x00002002,
		0x00002003,
		0x00002004,
		0x00002005,
		0x00002006,
		0x00002007,
		0x00002008,
		0x00002009,
		0x0000200A,
		0x00002028,
		0x00002029,
		0x0000202F,
		0x0000205F,
		0x00003000,
		0x0000FEFF,
		0xFFFFFFFF,
		0x00000000,
	};
	const txU4 *p = spaces;
	txU4 c = (txU4)character;
	txU4 s;
	while ((s = *p++)) {
		if (c < s)
			return 0;
		if (c == s)
			return 1;
	}
	return 0;
}

txBoolean fxParseHex(txU1 c, txU4* value)
{
	if (('0' <= c) && (c <= '9'))
		*value = (*value * 16) + (c - '0');
	else if (('a' <= c) && (c <= 'f'))
		*value = (*value * 16) + (10 + c - 'a');
	else if (('A' <= c) && (c <= 'F'))
		*value = (*value * 16) + (10 + c - 'A');
	else
		return 0;
	return 1;
}

txBoolean fxParseHexEscape(txString* string, txInteger* character)
{
	txU1* p = *((txU1**)string);
	txU4 value = 0;
	txU1 i, c;
	for (i = 0; i < 2; i++) {
		c = c_read8(p);
		if (!fxParseHex(c, &value))
			return 0;
		p++;
	}
	*character = (txInteger)value;
	*string = (txString)p;
	return 1;
}

txBoolean fxParseUnicodeEscape(txString* string, txInteger* character, txInteger braces, txInteger separator)
{
	txU1* p = *((txU1**)string);
	txU4 value = 0;
	txU1 c;
	txInteger i;
	
	c = c_read8(p++);
	if (braces && (c == '{')) {
		c = c_read8(p++);
		for (i = 0; value < 0x00110000; i++) {
			if (fxParseHex(c, &value)) {
				c = c_read8(p++);
			}
			else
				break;
		}
		if ((c == '}') && (i > 0) && (value < 0x00110000)) {
			*character = (txInteger)value;
			*string = (txString)p;
			return 1;
		}
		return 0;
	}
	if (!fxParseHex(c, &value)) return 0;
	c = c_read8(p++);
	if (!fxParseHex(c, &value)) return 0;
	c = c_read8(p++);
	if (!fxParseHex(c, &value)) return 0;
	c = c_read8(p++);
	if (!fxParseHex(c, &value)) return 0;
	*character = (txInteger)value;
	*string = (txString)p;
	c = c_read8(p++);
	if (c && (c == separator) && (0x0000D800 <= value) && (value <= 0x0000DBFF)) {
		c = c_read8(p++);
		if (c == 'u') {
			txU4 other = 0;
			c = c_read8(p++);
			if (!fxParseHex(c, &other)) return 1;
			c = c_read8(p++);
			if (!fxParseHex(c, &other)) return 1;
			c = c_read8(p++);
			if (!fxParseHex(c, &other)) return 1;
			c = c_read8(p++);
			if (!fxParseHex(c, &other)) return 1;
			if ((0x0000DC00 <= other) && (other <= 0x0000DFFF)) {
				*character = (txInteger)(0x00010000 + ((value & 0x03FF) << 10) + (other & 0x03FF));
				*string = (txString)p;
			}
		}
	}
	return 1;
}

txString fxSkipSpaces(txString string) 
{
	txString p = string;
	txString q = p;
	txInteger c;
	while (((q = fxUTF8Decode(q, &c))) && (c != C_EOF)) {
		if (fxIsSpace(c))
			p = q;
		else
			break;
	}
	return p;
}

txString fxStringifyHexEscape(txString string, txInteger character)
{
	const char* digits = gxHexUpper;
	txU1* p = (txU1*)string;
	*p++ = c_read8(digits + ((character & 0x000000F0) >> 4));
	*p++ = c_read8(digits + (character & 0x0000000F));
	return (txString)p;
}

txString fxStringifyUnicodeEscape(txString string, txInteger character, txInteger separator)
{
	const char* digits = (separator == '%') ? gxHexUpper : gxHexLower;
	txU1* p = (txU1*)string;
	txInteger surrogate;
	if (character > 0xFFFF) {
		character -= 0x10000;
		surrogate = 0xDC00 | (character & 0x3FF);
		character = 0xD800 | (character >> 10);
	}
	else
		surrogate = 0;
	*p++ = c_read8(digits + ((character & 0x0000F000) >> 12));
	*p++ = c_read8(digits + ((character & 0x00000F00) >> 8));
	*p++ = c_read8(digits + ((character & 0x000000F0) >> 4));
	*p++ = c_read8(digits + (character & 0x0000000F));
	if (surrogate) {
		*p++ = separator;
		*p++ = 'u';
		*p++ = c_read8(digits + ((surrogate & 0x0000F000) >> 12));
		*p++ = c_read8(digits + ((surrogate & 0x00000F00) >> 8));
		*p++ = c_read8(digits + ((surrogate & 0x000000F0) >> 4));
		*p++ = c_read8(digits + (surrogate & 0x0000000F));
	}
	return (txString)p;
}

int fxUTF8Compare(txString p1, txString p2)
{
	register const unsigned char *s1 = (const unsigned char *) p1;
	register const unsigned char *s2 = (const unsigned char *) p2;
	unsigned char c1, c2;
	do {
		c1 = (unsigned char) *s1++;
		c2 = (unsigned char) *s2++;
		if (c1 == '\0')
			return c1 - c2;
	}
	while (c1 == c2);
	if (c2 == '\0')
		return c1 - c2;
	if ((c1 == 0xC0) && (*s1 == 0x80))
		return 0 - c2;
	if ((c2 == 0xC0) && (*s2 == 0x80))
		return c1 - 0;
	return c1 - c2;
}

txString fxUTF8Decode(txString string, txInteger* character)
{
	txU1* p = (txU1*)string;
	txU4 c = c_read8(p++);
	if (c) {
		if (c & 0x80) {
			const txUTF8Sequence *sequence;
			txS4 size;
			for (sequence = gxUTF8Sequences; sequence->size; sequence++) {
				if ((c & sequence->cmask) == sequence->cval)
					break;
			}
			size = sequence->size - 1;
			while (size > 0) {
				size--;
				c = (c << 6) | (c_read8(p++) & 0x3F);
			}
			c &= sequence->lmask;
		}
		*character = (txInteger)c;
		return (txString)p;
	}
	*character = C_EOF;
	return (txString)p;
}

txString fxUTF8Encode(txString string, txInteger character)
{
	txU1* p = (txU1*)string;
	if (character < 0) {
	}
	else if (character == 0) {
		*p++ = 0xC0;
		*p++ = 0x80;
	}
	else if (character < 0x80) {
		*p++ = (txU1)character;
	}
	else if (character < 0x800) {
		*p++ = (txU1)(0xC0 | (((txU4)character) >> 6));
		*p++ = (txU1)(0x80 | (((txU4)character) & 0x3F));
	}
	else if (character < 0x10000) {
		*p++ = (txU1)(0xE0 | (((txU4)character) >> 12));
		*p++ = (txU1)(0x80 | ((((txU4)character) >> 6) & 0x3F));
		*p++ = (txU1)(0x80 | (((txU4)character) & 0x3F));
	}
	else if (character < 0x110000) {
		*p++ = (txU1)(0xF0 | (((txU4)character) >> 18));
		*p++ = (txU1)(0x80 | ((((txU4)character) >> 12) & 0x3F));
		*p++ = (txU1)(0x80 | ((((txU4)character) >> 6) & 0x3F));
		*p++ = (txU1)(0x80 | (((txU4)character) & 0x3F));
	}
	return (txString)p;
}

txSize fxUTF8Length(txInteger character)
{
	txSize length;
	if (character < 0)
		length = 0;
	else if (character == 0)
		length = 2;
	else if (character < 0x80)
		length = 1;
	else if (character < 0x800)
		length = 2;
	else if (character < 0x10000)
		length = 3;
	else if (character < 0x110000)
		length = 4;
	else
		length = 0;
	return length;
}

#if mxCESU8
txString fxCESU8Decode(txString string, txInteger* character)
{
	txInteger result;
	string = fxUTF8Decode(string, &result);
	if ((0x0000D800 <= result) && (result <= 0x0000DBFF)) {
		txString former = string;
		txInteger surrogate;
		string = fxUTF8Decode(former, &surrogate);
		if ((0x0000DC00 <= surrogate) && (surrogate <= 0x0000DFFF))
			result = 0x00010000 + ((result & 0x000003FF) << 10) + (surrogate & 0x000003FF);
		else
			string = former;
	}
	*character = result;
	return string;
}

txString fxCESU8Encode(txString string, txInteger character)
{
	txU1* p = (txU1*)string;
	if (character < 0) {
	}
	else if (character == 0) {
		*p++ = 0xC0;
		*p++ = 0x80;
	}
	else if (character < 0x80) {
		*p++ = (txU1)character;
	}
	else if (character < 0x800) {
		*p++ = (txU1)(0xC0 | (((txU4)character) >> 6));
		*p++ = (txU1)(0x80 | (((txU4)character) & 0x3F));
	}
	else if (character < 0x10000) {
		*p++ = (txU1)(0xE0 | (((txU4)character) >> 12));
		*p++ = (txU1)(0x80 | ((((txU4)character) >> 6) & 0x3F));
		*p++ = (txU1)(0x80 | (((txU4)character) & 0x3F));
	}
	else if (character < 0x110000) {
		txInteger surrogate;
		character -= 0x00010000;
		surrogate = 0xDC00 | (character & 0x3FF);
		character = 0xD800 | ((character >> 10) & 0x3FF);
		*p++ = (txU1)(0xE0 | (((txU4)character) >> 12));
		*p++ = (txU1)(0x80 | ((((txU4)character) >> 6) & 0x3F));
		*p++ = (txU1)(0x80 | (((txU4)character) & 0x3F));
		*p++ = (txU1)(0xE0 | (((txU4)surrogate) >> 12));
		*p++ = (txU1)(0x80 | ((((txU4)surrogate) >> 6) & 0x3F));
		*p++ = (txU1)(0x80 | (((txU4)surrogate) & 0x3F));
	}
	return (txString)p;
}

txSize fxCESU8Length(txInteger character)
{
	txSize length;
	if (character < 0)
		length = 0;
	else if (character == 0)
		length = 2;
	else if (character < 0x80)
		length = 1;
	else if (character < 0x800)
		length = 2;
	else if (character < 0x10000)
		length = 3;
	else if (character < 0x110000)
		length = 6;
	else
		length = 0;
	return length;
}
#endif

txSize fxUTF8ToUnicodeOffset(txString theString, txSize theOffset)
{
	txU1* p = (txU1*)theString;
	txU1 c;
	txSize unicodeOffset = 0;
	txSize utf8Offset = 0;
	
	while ((c = c_read8(p++))) {
		if ((c & 0xC0) != 0x80) {
			if (utf8Offset == theOffset)
				return unicodeOffset;
			unicodeOffset++;
		}
		utf8Offset++;
	}
	if (utf8Offset == theOffset)
		return unicodeOffset;
	else
		return -1;
}

#if 0
txSize fxUnicodeLength(txString theString)
{
	txU1* p = (txU1*)theString;
	txU1 c;
	txSize anIndex = 0;
	
	while ((c = c_read8(p++))) {
		if ((c & 0xC0) != 0x80)
			anIndex++;
	}
	return anIndex;
}
#else
 // http://www.daemonology.net/blog/2008-06-05-faster-utf8-strlen.html

#define ONEMASK ((size_t)(-1) / 0xFF)

#if defined(__has_feature)
	#if __has_feature(address_sanitizer)
		__attribute__((no_sanitize("address"))) 
	#endif
#endif
txSize fxUnicodeLength(txString _s)
{
	const char * s;
	size_t count = 0;
	size_t u;
	unsigned char b;

	/* Handle any initial misaligned bytes. */
	for (s = _s; (uintptr_t)(s) & (sizeof(size_t) - 1); s++) {
		b = c_read8(s);

		/* Exit if we hit a zero byte. */
		if (b == '\0')
			goto done;

		/* Is this byte NOT the first byte of a character? */
		count += (b >> 7) & ((~b) >> 6);
	}

	/* Handle complete blocks. */
	for (; ; s += sizeof(size_t)) {
		/* Grab 4 or 8 bytes of UTF-8 data. */
		u = *(size_t *)(s);

		/* Exit the loop if there are any zero bytes. */
		if ((u - ONEMASK) & (~u) & (ONEMASK * 0x80))
			break;

		/* Count bytes which are NOT the first byte of a character. */
		u = ((u & (ONEMASK * 0x80)) >> 7) & ((~u) >> 6);
		count += (u * ONEMASK) >> ((sizeof(size_t) - 1) * 8);
	}

	/* Take care of any left-over bytes. */
	for (; ; s++) {
		b = c_read8(s);

		/* Exit if we hit a zero byte. */
		if (b == '\0')
			break;

		/* Is this byte NOT the first byte of a character? */
		count += (b >> 7) & ((~b) >> 6);
	}

done:
	return (txSize)((s - _s) - count);
}
#endif

txSize fxUnicodeToUTF8Offset(txString theString, txSize theOffset)
{
	txU1* p = (txU1*)theString;
	txU1 c;
	txSize unicodeOffset = 0;
	txSize utf8Offset = 0;
	
    while ((c = c_read8(p++))) {
		if ((c & 0xC0) != 0x80) {
			if (unicodeOffset == theOffset)
				return utf8Offset;
			unicodeOffset++;
		}
        utf8Offset++;
	}
	if (unicodeOffset == theOffset)
		return utf8Offset;
	else
		return -1;
}

txFlag fxIntegerToIndex(void* dtoa, txInteger theInteger, txIndex* theIndex)
{
	if (0 <= theInteger) {
		*theIndex = (txIndex)theInteger;
		return 1;
	}
	return 0;
}

txFlag fxNumberToIndex(void* dtoa, txNumber number, txIndex* theIndex)
{
	txIndex integer = (txIndex)number;
	txNumber check = integer;
	if ((number == check) && (integer < 4294967295u)) {
		*theIndex = integer;
		return 1;
	}
	return 0;
}

txFlag fxStringToIndex(void* dtoa, txString theString, txIndex* theIndex)
{
	char buffer[256], c;
	txNumber number;
	txIndex integer;
	txNumber check;

	c = c_read8(theString);
	if (('+' != c) && ('-' != c) && ('.' != c) && !(('0' <= c) && ('9' >= c)))
		return 0;
	number = fxStringToNumber(dtoa, theString, 1);
	integer = (txIndex)number;
	check = integer;
	if ((number == check) && (integer < 4294967295u)) {
		fxNumberToString(dtoa, number, buffer, sizeof(buffer), 0, 0);
		if (!c_strcmp(theString, buffer)) {
			*theIndex = integer;
			return 1;
		}
	}
	return 0;
}

const txString gxIDStrings[XS_ID_COUNT] = {
	"@",
	"Symbol.asyncIterator",
	"Symbol.hasInstance",
	"Symbol.isConcatSpreadable",
	"Symbol.iterator",
	"Symbol.match",
	"Symbol.matchAll",
	"Symbol.replace",
	"Symbol.search",
	"Symbol.species",
	"Symbol.split",
	"Symbol.toPrimitive",
	"Symbol.toStringTag",
	"Symbol.unscopables",
	"AggregateError",
	"Array",
	"ArrayBuffer",
	"Atomics",
	"BigInt",
	"BigInt64Array",
	"BigUint64Array",
	"Boolean",
	"DataView",
	"Date",
	"Error",
	"EvalError",
	"FinalizationRegistry",
	"Float32Array",
	"Float64Array",
	"Int16Array",
	"Int32Array",
	"Int8Array",
	"JSON",
	"Map",
	"Math",
	"ModuleSource",
	"Number",
	"Object",
	"Promise",
	"Proxy",
	"RangeError",
	"ReferenceError",
	"Reflect",
	"RegExp",
	"Set",
	"SharedArrayBuffer",
	"String",
	"Symbol",
	"SyntaxError",
	"TypeError",
	"TypedArray",
	"URIError",
	"Uint16Array",
	"Uint32Array",
	"Uint8Array",
	"Uint8ClampedArray",
	"WeakMap",
	"WeakRef",
	"WeakSet",
	"decodeURI",
	"decodeURIComponent",
	"encodeURI",
	"encodeURIComponent",
	"escape",
	"isFinite",
	"isNaN",
	"parseFloat",
	"parseInt",
	"trace",
	"unescape",
	"Infinity",
	"NaN",
	"undefined",
	"Compartment",
	"Function",
	"eval",
	"AsyncFunction",
	"AsyncGeneratorFunction",
	"BYTES_PER_ELEMENT",
	"E",
	"EPSILON",
	"Generator",
	"GeneratorFunction",
	"LN10",
	"LN2",
	"LOG10E",
	"LOG2E",
	"MAX_SAFE_INTEGER",
	"MAX_VALUE",
	"MIN_SAFE_INTEGER",
	"MIN_VALUE",
	"NEGATIVE_INFINITY",
	"PI",
	"POSITIVE_INFINITY",
	"SQRT1_2",
	"SQRT2",
	"UTC",
	"__defineGetter__",
	"__defineSetter__",
	"__lookupGetter__",
	"__lookupSetter__",
	"__proto__",
	"abs",
	"acos",
	"acosh",
	"add",
	"aliases",
	"all",
	"allSettled",
	"and",
	"any",
	"append",
	"apply",
	"arguments",
	"as",
	"asIntN",
	"asUintN",
	"asin",
	"asinh",
	"assign",
	"asyncIterator",
	"at",
	"atan",
	"atanh",
	"atan2",
	"bind",
	"bindings",
	"bitLength",
	"boundArguments",
	"boundFunction",
	"boundThis",
	"buffer",
	"busy",
	"byteLength",
	"byteOffset",
	"cache",
	"call",
	"callee",
	"caller",
	"catch",
	"cause",
	"cbrt",
	"ceil",
	"center",
	"charAt",
	"charCodeAt",
	"chunk",
	"chunkify",
	"cleanupSome",
	"clear",
	"closure",
	"clz32",
	"codePointAt",
	"compare",
	"compareExchange",
	"compile",
	"concat",
	"configurable",
	"console",
	"construct",
	"constructor",
	"copyWithin",
	"cos",
	"cosh",
	"count",
	"create",
	"default",
	"defineProperties",
	"defineProperty",
	"delete",
	"deleteProperty",
	"deref",
	"description",
	"done",
	"dotAll",
	"eachDown",
	"eachUp",
	"endsWith",
	"entries",
	"enumerable",
	"enumerate",
	"errors",
	"evaluate",
	"every",
	"exchange",
	"exec",
	"exp",
	"expm1",
	"export",
	"exports",
	"fill",
	"filter",
	"finally",
	"find",
	"findIndex",
	"findLast",
	"findLastIndex",
	"flags",
	"flat",
	"flatMap",
	"floor",
	"for",
	"forEach",
	"free",
	"freeze",
	"from",
	"fromArrayBuffer",
	"fromBigInt",
	"fromCharCode",
	"fromCodePoint",
	"fromEntries",
	"fromString",
	"fround",
	"function",
	"get",
	"getBigInt64",
	"getBigUint64",
	"getDate",
	"getDay",
	"getFloat32",
	"getFloat64",
	"getFullYear",
	"getHours",
	"getInt16",
	"getInt32",
	"getInt8",
	"getMilliseconds",
	"getMinutes",
	"getMonth",
	"getOwnPropertyDescriptor",
	"getOwnPropertyDescriptors",
	"getOwnPropertyNames",
	"getOwnPropertySymbols",
	"getPrototypeOf",
	"getSeconds",
	"getTime",
	"getTimezoneOffset",
	"getUTCDate",
	"getUTCDay",
	"getUTCFullYear",
	"getUTCHours",
	"getUTCMilliseconds",
	"getUTCMinutes",
	"getUTCMonth",
	"getUTCSeconds",
	"getUint16",
	"getUint32",
	"getUint8",
	"getYear",
	"global",
	"globalThis",
	"groups",
	"grow",
	"growable",
	"has",
	"hasIndices",
	"hasInstance",
	"hasOwn",
	"hasOwnProperty",
	"hypot",
	"id",
	"idiv",
	"idivmod",
	"ignoreCase",
	"imod",
	"import",
	"importNow",
	"imports",
	"imul",
	"imuldiv",
	"includes",
	"index",
	"indexOf",
	"indices",
	"input",
	"irem",
	"is",
	"isArray",
	"isConcatSpreadable",
	"isExtensible",
	"isFrozen",
	"isInteger",
	"isLockFree",
	"isPrototypeOf",
	"isSafeInteger",
	"isSealed",
	"isView",
	"iterable",
	"iterator",
	"join",
	"keyFor",
	"keys",
	"lastIndex",
	"lastIndexOf",
	"left",
	"length",
	"line",
	"load",
	"local",
	"localeCompare",
	"log",
	"log1p",
	"log10",
	"log2",
	"map",
	"match",
	"matchAll",
	"max",
	"maxByteLength",
	"message",
	"min",
	"mod",
	"module",
	"multiline",
	"name",
	"needsImport",
	"needsImportMeta",
	"new.target",
	"next",
	"normalize",
	"notify",
	"now",
	"of",
	"or",
	"ownKeys",
	"padEnd",
	"padStart",
	"parse",
	"path",
	"peek",
	"poke",
	"pop",
	"pow",
	"preventExtensions",
	"propertyIsEnumerable",
	"prototype",
	"proxy",
	"push",
	"race",
	"random",
	"raw",
	"reason",
	"reduce",
	"reduceRight",
	"reexports",
	"register",
	"reject",
	"repeat",
	"replace",
	"replaceAll",
	"resizable",
	"resize",
	"resolve",
	"result",
	"return",
	"reverse",
	"revocable",
	"revoke",
	"right",
	"round",
	"seal",
	"search",
	"set",
	"setBigInt64",
	"setBigUint64",
	"setDate",
	"setFloat32",
	"setFloat64",
	"setFullYear",
	"setHours",
	"setInt16",
	"setInt32",
	"setInt8",
	"setMilliseconds",
	"setMinutes",
	"setMonth",
	"setPrototypeOf",
	"setSeconds",
	"setTime",
	"setUTCDate",
	"setUTCFullYear",
	"setUTCHours",
	"setUTCMilliseconds",
	"setUTCMinutes",
	"setUTCMonth",
	"setUTCSeconds",
	"setUint16",
	"setUint32",
	"setUint8",
	"setYear",
	"shift",
	"sign",
	"sin",
	"sinh",
	"size",
	"slice",
	"some",
	"sort",
	"source",
	"species",
	"splice",
	"split",
	"sqrt",
	"stack",
	"startsWith",
	"status",
	"sticky",
	"store",
	"stringify",
	"sub",
	"subarray",
	"substr",
	"substring",
	"tan",
	"tanh",
	"test",
	"then",
	"this",
	"throw",
	"toDateString",
	"toExponential",
	"toFixed",
	"toGMTString",
	"toISOString",
	"toJSON",
	"toLocaleDateString",
	"toLocaleLowerCase",
	"toLocaleString",
	"toLocaleTimeString",
	"toLocaleUpperCase",
	"toLowerCase",
	"toPrecision",
	"toPrimitive",
	"toString",
	"toStringTag",
	"toTimeString",
	"toUTCString",
	"toUpperCase",
	"transfer",
	"transfers",
	"trim",
	"trimEnd",
	"trimLeft",
	"trimRight",
	"trimStart",
	"trunc",
	"unicode",
	"unregister",
	"unscopables",
	"unshift",
	"uri",
	"value",
	"valueOf",
	"values",
	"wait",
	"wake",
	"weak",
	"writable",
	"xor",
	"",
	"<xsbug:script>"
};
