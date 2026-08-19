// generated from gentext.pl Messages.xlsx
#ifndef __TJS_ERROR_INC_H__
#define __TJS_ERROR_INC_H__
TJS_MSG_DECL(TJSInternalError, TJS_N("Internal error"))
TJS_MSG_DECL(TJSWarning, TJS_N("Warning: "))
TJS_MSG_DECL(TJSWarnEvalOperator,
             TJS_N("Non-global post-! operator is used (note that the post-! operator behavior is "
                   "changed on TJS2 version 2.4.1)"))
TJS_MSG_DECL(TJSWideToNarrowConversionError,
             TJS_N("Cannot convert given wide string to narrow string"))
TJS_MSG_DECL(TJSNarrowToWideConversionError,
             TJS_N("Cannot convert given narrow string to wide string"))
TJS_MSG_DECL(TJSVariantConvertError, TJS_N("Cannot convert the variable type (%1 to %2)"))
TJS_MSG_DECL(TJSVariantConvertErrorToObject,
             TJS_N("Cannot convert the variable type (%1 to Object)"))
TJS_MSG_DECL(TJSIDExpected, TJS_N("Specify an ID"))
TJS_MSG_DECL(TJSSubstitutionInBooleanContext,
             TJS_N("Substitution in boolean context (use form of '(A=B)!=0' to compare to zero)"));
TJS_MSG_DECL(TJSCannotModifyLHS, TJS_N("This expression cannot be used as a lvalue"))
TJS_MSG_DECL(TJSInsufficientMem, TJS_N("Insufficient memory"))
TJS_MSG_DECL(TJSCannotGetResult, TJS_N("Cannot get the value of this expression"))
TJS_MSG_DECL(TJSNullAccess, TJS_N("Accessing to null object"))
TJS_MSG_DECL(TJSMemberNotFound, TJS_N("Member \"%1\" does not exist"))
TJS_MSG_DECL(TJSMemberNotFoundNoNameGiven, TJS_N("Member does not exist"))
TJS_MSG_DECL(TJSNotImplemented, TJS_N("Called method is not implemented"))
TJS_MSG_DECL(TJSInvalidParam, TJS_N("Invalid argument"))
TJS_MSG_DECL(TJSBadParamCount, TJS_N("Invalid argument count"))
TJS_MSG_DECL(TJSInvalidType, TJS_N("Not a function or invalid method/property type"))
TJS_MSG_DECL(TJSSpecifyDicOrArray, TJS_N("Specify a Dictionary object or an Array object"));
TJS_MSG_DECL(TJSSpecifyArray, TJS_N("Specify an Array object"));
TJS_MSG_DECL(TJSStringDeallocError, TJS_N("Cannot free the string memory block"))
TJS_MSG_DECL(TJSStringAllocError, TJS_N("Cannot allocate the string memory block"))
TJS_MSG_DECL(TJSMisplacedBreakContinue, TJS_N("Cannot place \"break\" or \"continue\" here"))
TJS_MSG_DECL(TJSMisplacedCase, TJS_N("Cannot place \"case\" here"))
TJS_MSG_DECL(TJSMisplacedReturn, TJS_N("Cannot place \"return\" here"))
TJS_MSG_DECL(TJSStringParseError, TJS_N("Un-terminated string/regexp/octet literal"))
TJS_MSG_DECL(TJSNumberError, TJS_N("Cannot be parsed as a number"))
TJS_MSG_DECL(TJSUnclosedComment, TJS_N("Un-terminated comment"))
TJS_MSG_DECL(TJSInvalidChar, TJS_N("Invalid character \'%1\'"))
TJS_MSG_DECL(TJSExpected, TJS_N("Expected %1"))
TJS_MSG_DECL(TJSSyntaxError, TJS_N("Syntax error (%1)"))
TJS_MSG_DECL(TJSPPError, TJS_N("Error in conditional compiling expression"))
TJS_MSG_DECL(TJSCannotGetSuper,
             TJS_N("Super class does not exist or cannot specify the super class"))
TJS_MSG_DECL(TJSInvalidOpecode, TJS_N("Invalid VM code"))
TJS_MSG_DECL(TJSRangeError, TJS_N("The value is out of the range"))
TJS_MSG_DECL(TJSAccessDenyed, TJS_N("Invalid operation for Read-only or Write-only property"))
TJS_MSG_DECL(TJSNativeClassCrash, TJS_N("Invalid object context"))
TJS_MSG_DECL(TJSInvalidObject, TJS_N("The object is already invalidated"))
TJS_MSG_DECL(TJSDuplicatedPropHandler, TJS_N("Duplicated \"setter\" or \"getter\""))
TJS_MSG_DECL(TJSCannotOmit, TJS_N("\"...\" is used out of functions"))
TJS_MSG_DECL(TJSCannotParseDate, TJS_N("Invalid date format"))
TJS_MSG_DECL(TJSInvalidValueForTimestamp, TJS_N("Invalid value for date/time"))
TJS_MSG_DECL(TJSExceptionNotFound,
             TJS_N("Cannot convert exception because \"Exception\" does not exist"))
TJS_MSG_DECL(TJSInvalidFormatString, TJS_N("Invalid format string"))
TJS_MSG_DECL(TJSDivideByZero, TJS_N("Division by zero"))
TJS_MSG_DECL(TJSNotReconstructiveRandomizeData, TJS_N("Cannot reconstruct random seeds"))
TJS_MSG_DECL(TJSSymbol, TJS_N("ID"))
TJS_MSG_DECL(TJSCallHistoryIsFromOutOfTJS2Script, TJS_N("[out of TJS2 script]"))
TJS_MSG_DECL(TJSNObjectsWasNotFreed, TJS_N("Total %1 Object(s) was not freed"))
TJS_MSG_DECL(TJSObjectCreationHistoryDelimiter, TJS_N(" <-- "));
TJS_MSG_DECL(TJSObjectWasNotFreed,
             TJS_N("Object %1 [%2] wes not freed / The object was created at : %2"))
TJS_MSG_DECL(TJSGroupByObjectTypeAndHistory,
             TJS_N("Group by object type and location where the object was created"))
TJS_MSG_DECL(TJSGroupByObjectType, TJS_N("Group by object type"))
TJS_MSG_DECL(TJSObjectCountingMessageGroupByObjectTypeAndHistory, TJS_N("%1 time(s) : [%2] %3"))
TJS_MSG_DECL(TJSObjectCountingMessageTJSGroupByObjectType, TJS_N("%1 time(s) : [%2]"))
TJS_MSG_DECL(
    TJSWarnRunningCodeOnDeletingObject,
    TJS_N(
        "%4: Running code on deleting-in-progress object %1[%2] / The object was created at : %3"))
TJS_MSG_DECL(TJSWriteError, TJS_N("Write error"))
TJS_MSG_DECL(TJSReadError, TJS_N("Read error"))
TJS_MSG_DECL(TJSSeekError, TJS_N("Seek error"))
TJS_MSG_DECL(TJSByteCodeBroken,
             TJS_N("Bytecode read error. File is broken or it's not bytecode file."))
TJS_MSG_DECL(TJSObjectHashMapLogVersionMismatch, TJS_N("Object Hash Map log version mismatch"))
TJS_MSG_DECL(TJSCurruptedObjectHashMapLog, TJS_N("Currupted Object Hash Map log"))
TJS_MSG_DECL(TJSUnknownFailure, TJS_N("Unknown failure : %08X"))
TJS_MSG_DECL(TJSUnknownPackUnpackTemplateCharcter, TJS_N("Unknown pack/unpack TEMPLATE charcter"))
TJS_MSG_DECL(TJSUnknownBitStringCharacter, TJS_N("Unknown bit string charcter"))
TJS_MSG_DECL(TJSUnknownHexStringCharacter, TJS_N("Unknown Hex string charcter"))
TJS_MSG_DECL(TJSNotSupportedUuencode, TJS_N("Not supported uuencode"))
TJS_MSG_DECL(TJSNotSupportedBER, TJS_N("Not supported BER"))
TJS_MSG_DECL(TJSNotSupportedUnpackLP, TJS_N("Not supported unpack 'lp'"))
TJS_MSG_DECL(TJSNotSupportedUnpackP, TJS_N("Not supported unpack 'p'"))
#endif
