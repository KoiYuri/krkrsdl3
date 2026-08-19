//---------------------------------------------------------------------------
/*
        TJS2 Script Engine
        Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

        See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Japanese localized messages
//---------------------------------------------------------------------------

TJS_MSG_DECL(TJSInternalError, TJS_N("内部エラーが発生しました"))
TJS_MSG_DECL(TJSWarning, TJS_N("警告: "))
TJS_MSG_DECL(TJSWarnEvalOperator,
             TJS_N("グローバルでない場所で後置 ! 演算子が使われています(この演算子の挙動はTJS2 "
                   "version 2.4.1 で変わりましたのでご注意ください)"))
TJS_MSG_DECL(
    TJSNarrowToWideConversionError,
    TJS_N("ANSI 文字列を UNICODE "
          "文字列に変換できません。現在のコードページで解釈できない文字が含まれてます。正しいデータ"
          "が指定されているかを確認してください。データが破損している可能性もあります"))
TJS_MSG_DECL(TJSVariantConvertError, TJS_N("%1 から %2 へ型を変換できません"))
TJS_MSG_DECL(TJSVariantConvertErrorToObject,
             TJS_N("%1 から Object へ型を変換できません。Object 型が要求される文脈で Object "
                   "型以外の値が渡されるとこのエラーが発生します"))
TJS_MSG_DECL(TJSIDExpected, TJS_N("識別子を指定してください"))
TJS_MSG_DECL(TJSSubstitutionInBooleanContext,
             TJS_N("論理値が求められている場所で = 演算子が使用されています(== "
                   "演算子の間違いですか？代入した上でゼロと値を比較したい場合は、(A=B) != 0 "
                   "の形式を使うことをお勧めします)"));
TJS_MSG_DECL(TJSCannotModifyLHS, TJS_N("不正な代入か不正な式の操作です"))
TJS_MSG_DECL(TJSInsufficientMem, TJS_N("メモリが足りません"))
TJS_MSG_DECL(TJSCannotGetResult, TJS_N("この式からは値を得ることができません"))
TJS_MSG_DECL(TJSNullAccess, TJS_N("null オブジェクトにアクセスしようとしました"))
TJS_MSG_DECL(TJSMemberNotFound, TJS_N("メンバ \"%1\" が見つかりません"))
TJS_MSG_DECL(TJSMemberNotFoundNoNameGiven, TJS_N("メンバが見つかりません"))
TJS_MSG_DECL(TJSNotImplemented, TJS_N("呼び出そうとした機能は未実装です"))
TJS_MSG_DECL(TJSInvalidParam, TJS_N("不正な引数です"))
TJS_MSG_DECL(TJSBadParamCount, TJS_N("引数の数が不正です"))
TJS_MSG_DECL(TJSInvalidType, TJS_N("関数ではないかプロパティの種類が違います"))
TJS_MSG_DECL(TJSSpecifyDicOrArray,
             TJS_N("Dictionary または Array クラスのオブジェクトを指定してください"))
TJS_MSG_DECL(TJSSpecifyArray, TJS_N("Array クラスのオブジェクトを指定してください"))
TJS_MSG_DECL(TJSStringDeallocError, TJS_N("文字列メモリブロックを解放できません"))
TJS_MSG_DECL(TJSStringAllocError, TJS_N("文字列メモリブロックを確保できません"))
TJS_MSG_DECL(TJSMisplacedBreakContinue,
             TJS_N("\"break\" または \"continue\" はここに書くことはできません"))
TJS_MSG_DECL(TJSMisplacedCase, TJS_N("\"case\" はここに書くことはできません"))
TJS_MSG_DECL(TJSMisplacedReturn, TJS_N("\"return\" はここに書くことはできません"))
TJS_MSG_DECL(
    TJSStringParseError,
    TJS_N("文字列定数/正規表現/オクテット即値が終わらないままスクリプトの終端に達しました"))
TJS_MSG_DECL(TJSNumberError, TJS_N("数値として解釈できません"))
TJS_MSG_DECL(TJSUnclosedComment, TJS_N("コメントが終わらないままスクリプトの終端に達しました"))
TJS_MSG_DECL(TJSInvalidChar, TJS_N("不正な文字です : \'%1\'"))
TJS_MSG_DECL(TJSExpected, TJS_N("%1 がありません"))
TJS_MSG_DECL(TJSSyntaxError, TJS_N("文法エラーです(%1)"))
TJS_MSG_DECL(TJSPPError, TJS_N("条件コンパイル式にエラーがあります"))
TJS_MSG_DECL(TJSCannotGetSuper, TJS_N("スーパークラスが存在しないかスーパークラスを特定できません"))
TJS_MSG_DECL(TJSInvalidOpecode, TJS_N("不正な VM コードです"))
TJS_MSG_DECL(TJSRangeError, TJS_N("値が範囲外です"))
TJS_MSG_DECL(
    TJSAccessDenyed,
    TJS_N("読み込み専用あるいは書き込み専用プロパティに対して行えない操作をしようとしました"))
TJS_MSG_DECL(TJSNativeClassCrash, TJS_N("実行コンテキストが違います"))
TJS_MSG_DECL(TJSInvalidObject, TJS_N("オブジェクトはすでに無効化されています"))
TJS_MSG_DECL(TJSCannotOmit, TJS_N("\"...\" は関数外では使えません"))
TJS_MSG_DECL(TJSCannotParseDate, TJS_N("不正な日付文字列の形式です"))
TJS_MSG_DECL(TJSInvalidValueForTimestamp, TJS_N("不正な日付・時刻です"))
TJS_MSG_DECL(TJSExceptionNotFound,
             TJS_N("\"Exception\" が存在しないため例外オブジェクトを作成できません"))
TJS_MSG_DECL(TJSInvalidFormatString, TJS_N("不正な書式文字列です"))
TJS_MSG_DECL(TJSDivideByZero, TJS_N("0 で除算をしようとしました"))
TJS_MSG_DECL(TJSNotReconstructiveRandomizeData,
             TJS_N("乱数系列を初期化できません(おそらく不正なデータが渡されました)"))
TJS_MSG_DECL(TJSSymbol, TJS_N("識別子"))
TJS_MSG_DECL(TJSCallHistoryIsFromOutOfTJS2Script, TJS_N("[TJSスクリプト管理外]"))
TJS_MSG_DECL(TJSNObjectsWasNotFreed, TJS_N("合計 %1 個のオブジェクトが解放されていません"))
TJS_MSG_DECL(TJSObjectCreationHistoryDelimiter, TJS_N("\n                     "))
TJS_MSG_DECL(TJSObjectWasNotFreed,
             TJS_N("オブジェクト %1 [%2] "
                   "が解放されていません。オブジェクト作成時の呼び出し履歴は以下の通りです:\n      "
                   "               %3"))
TJS_MSG_DECL(TJSGroupByObjectTypeAndHistory,
             TJS_N("オブジェクトのタイプとオブジェクト作成時の履歴による分類"))
TJS_MSG_DECL(TJSGroupByObjectType, TJS_N("オブジェクトのタイプによる分類"))
TJS_MSG_DECL(TJSObjectCountingMessageGroupByObjectTypeAndHistory,
             TJS_N("%1 個 : [%2]\n                     %3"))
TJS_MSG_DECL(TJSObjectCountingMessageTJSGroupByObjectType, TJS_N("%1 個 : [%2]"))
TJS_MSG_DECL(TJSWarnRunningCodeOnDeletingObject,
             TJS_N("%4: 削除中のオブジェクト %1[%2] "
                   "上でコードが実行されています。このオブジェクトの作成時の呼び出し履歴は以下の通"
                   "りです:\n                     %3"))
TJS_MSG_DECL(TJSWriteError, TJS_N("書き込みエラーが発生しました"))
TJS_MSG_DECL(TJSReadError,
             TJS_N("読み込みエラーが発生しました。ファイルが破損している可能性や、デバイスからの読"
                   "み込みに失敗した可能性があります"))
TJS_MSG_DECL(TJSSeekError,
             TJS_N("シークエラーが発生しました。ファイルが破損している可能性や、デバイスからの読み"
                   "込みに失敗した可能性があります"))

TJS_MSG_DECL(TJSByteCodeBroken,
             TJS_N("バイトコードファイル読み込みエラー。ファイルが壊れているかバイトコードとは異な"
                   "るファイルです"))
//---------------------------------------------------------------------------
