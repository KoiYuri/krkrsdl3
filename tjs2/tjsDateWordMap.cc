/*---------------------------------------------------------------------------*/
/*
        TJS2 Script Engine
        Copyright (C) 2000-2009 W.Dee <dee@kikyou.info> and contributors

        See details of license at "license.txt"
*/
/*---------------------------------------------------------------------------*/
/*
        Date/time string parser lexical analyzer word cutter.

        This file is always generated from syntax/dp_wordtable.txt by
        syntax/create_word_map.pl. Modification by hand will be lost.

*/

switch (InputPointer[0])
{
    case TJS_N('a'):
    case TJS_N('A'):
        switch (InputPointer[1])
        {
            case TJS_N('c'):
            case TJS_N('C'):
                switch (InputPointer[2])
                {
                    case TJS_N('s'):
                    case TJS_N('S'):
                        switch (InputPointer[3])
                        {
                            case TJS_N('s'):
                            case TJS_N('S'):
                                switch (InputPointer[4])
                                {
                                    case TJS_N('t'):
                                    case TJS_N('T'):
                                        if (!TJS_iswalpha(&InputPointer[5]))
                                        {
                                            InputPointer += 5;
                                            yylex->val = 1030;
                                            return DP_TZ;
                                        }
                                        break;
                                }
                                break;
                            case TJS_N('t'):
                            case TJS_N('T'):
                                if (!TJS_iswalpha(&InputPointer[4]))
                                {
                                    InputPointer += 4;
                                    yylex->val = 930;
                                    return DP_TZ;
                                }
                                break;
                        }
                        break;
                }
                break;
            case TJS_N('d'):
            case TJS_N('D'):
                switch (InputPointer[2])
                {
                    case TJS_N('t'):
                    case TJS_N('T'):
                        if (!TJS_iswalpha(&InputPointer[3]))
                        {
                            InputPointer += 3;
                            yylex->val = -300;
                            return DP_TZ;
                        }
                        break;
                }
                break;
            case TJS_N('e'):
            case TJS_N('E'):
                switch (InputPointer[2])
                {
                    case TJS_N('s'):
                    case TJS_N('S'):
                        switch (InputPointer[3])
                        {
                            case TJS_N('s'):
                            case TJS_N('S'):
                                switch (InputPointer[4])
                                {
                                    case TJS_N('t'):
                                    case TJS_N('T'):
                                        if (!TJS_iswalpha(&InputPointer[5]))
                                        {
                                            InputPointer += 5;
                                            yylex->val = 1100;
                                            return DP_TZ;
                                        }
                                        break;
                                }
                                break;
                            case TJS_N('t'):
                            case TJS_N('T'):
                                if (!TJS_iswalpha(&InputPointer[4]))
                                {
                                    InputPointer += 4;
                                    yylex->val = 1000;
                                    return DP_TZ;
                                }
                                break;
                        }
                        break;
                }
                break;
            case TJS_N('h'):
            case TJS_N('H'):
                switch (InputPointer[2])
                {
                    case TJS_N('s'):
                    case TJS_N('S'):
                        switch (InputPointer[3])
                        {
                            case TJS_N('t'):
                            case TJS_N('T'):
                                if (!TJS_iswalpha(&InputPointer[4]))
                                {
                                    InputPointer += 4;
                                    yylex->val = -1000;
                                    return DP_TZ;
                                }
                                break;
                        }
                        break;
                }
                break;
            case TJS_N('m'):
            case TJS_N('M'):
                if (!TJS_iswalpha(&InputPointer[2]))
                {
                    InputPointer += 2;
                    yylex->val = 0;
                    return DP_AM;
                }
                break;
            case TJS_N('p'):
            case TJS_N('P'):
                switch (InputPointer[2])
                {
                    case TJS_N('r'):
                    case TJS_N('R'):
                        switch (InputPointer[3])
                        {
                            case TJS_N('.'):
                                if (!TJS_iswalpha(&InputPointer[4]))
                                {
                                    InputPointer += 4;
                                    yylex->val = 3;
                                    return DP_MONTH;
                                }
                                break;
                            case TJS_N('i'):
                            case TJS_N('I'):
                                switch (InputPointer[4])
                                {
                                    case TJS_N('l'):
                                    case TJS_N('L'):
                                        if (!TJS_iswalpha(&InputPointer[5]))
                                        {
                                            InputPointer += 5;
                                            yylex->val = 3;
                                            return DP_MONTH;
                                        }
                                        break;
                                }
                                break;
                            default:
                                if (!TJS_iswalpha(&InputPointer[3]))
                                {
                                    InputPointer += 3;
                                    yylex->val = 3;
                                    return DP_MONTH;
                                }
                        }
                        break;
                }
                break;
            case TJS_N('s'):
            case TJS_N('S'):
                switch (InputPointer[2])
                {
                    case TJS_N('t'):
                    case TJS_N('T'):
                        if (!TJS_iswalpha(&InputPointer[3]))
                        {
                            InputPointer += 3;
                            yylex->val = -400;
                            return DP_TZ;
                        }
                        break;
                }
                break;
            case TJS_N('u'):
            case TJS_N('U'):
                switch (InputPointer[2])
                {
                    case TJS_N('g'):
                    case TJS_N('G'):
                        switch (InputPointer[3])
                        {
                            case TJS_N('.'):
                                if (!TJS_iswalpha(&InputPointer[4]))
                                {
                                    InputPointer += 4;
                                    yylex->val = 7;
                                    return DP_MONTH;
                                }
                                break;
                            case TJS_N('u'):
                            case TJS_N('U'):
                                switch (InputPointer[4])
                                {
                                    case TJS_N('s'):
                                    case TJS_N('S'):
                                        switch (InputPointer[5])
                                        {
                                            case TJS_N('t'):
                                            case TJS_N('T'):
                                                if (!TJS_iswalpha(&InputPointer[6]))
                                                {
                                                    InputPointer += 6;
                                                    yylex->val = 7;
                                                    return DP_MONTH;
                                                }
                                                break;
                                        }
                                        break;
                                }
                                break;
                            default:
                                if (!TJS_iswalpha(&InputPointer[3]))
                                {
                                    InputPointer += 3;
                                    yylex->val = 7;
                                    return DP_MONTH;
                                }
                        }
                        break;
                }
                break;
            case TJS_N('w'):
            case TJS_N('W'):
                switch (InputPointer[2])
                {
                    case TJS_N('s'):
                    case TJS_N('S'):
                        switch (InputPointer[3])
                        {
                            case TJS_N('s'):
                            case TJS_N('S'):
                                switch (InputPointer[4])
                                {
                                    case TJS_N('t'):
                                    case TJS_N('T'):
                                        if (!TJS_iswalpha(&InputPointer[5]))
                                        {
                                            InputPointer += 5;
                                            yylex->val = 900;
                                            return DP_TZ;
                                        }
                                        break;
                                }
                                break;
                            case TJS_N('t'):
                            case TJS_N('T'):
                                if (!TJS_iswalpha(&InputPointer[4]))
                                {
                                    InputPointer += 4;
                                    yylex->val = 800;
                                    return DP_TZ;
                                }
                                break;
                        }
                        break;
                }
                break;
            default:
                if (!TJS_iswalpha(&InputPointer[1]))
                {
                    InputPointer += 1;
                    yylex->val = -100;
                    return DP_TZ;
                }
        }
        break;
    case TJS_N('b'):
    case TJS_N('B'):
        switch (InputPointer[1])
        {
            case TJS_N('s'):
            case TJS_N('S'):
                switch (InputPointer[2])
                {
                    case TJS_N('t'):
                    case TJS_N('T'):
                        if (!TJS_iswalpha(&InputPointer[3]))
                        {
                            InputPointer += 3;
                            yylex->val = 100;
                            return DP_TZ;
                        }
                        break;
                }
                break;
            case TJS_N('t'):
            case TJS_N('T'):
                if (!TJS_iswalpha(&InputPointer[2]))
                {
                    InputPointer += 2;
                    yylex->val = 300;
                    return DP_TZ;
                }
                break;
        }
        break;
    case TJS_N('c'):
    case TJS_N('C'):
        switch (InputPointer[1])
        {
            case TJS_N('a'):
            case TJS_N('A'):
                switch (InputPointer[2])
                {
                    case TJS_N('d'):
                    case TJS_N('D'):
                        switch (InputPointer[3])
                        {
                            case TJS_N('t'):
                            case TJS_N('T'):
                                if (!TJS_iswalpha(&InputPointer[4]))
                                {
                                    InputPointer += 4;
                                    yylex->val = 1030;
                                    return DP_TZ;
                                }
                                break;
                        }
                        break;
                    case TJS_N('s'):
                    case TJS_N('S'):
                        switch (InputPointer[3])
                        {
                            case TJS_N('t'):
                            case TJS_N('T'):
                                if (!TJS_iswalpha(&InputPointer[4]))
                                {
                                    InputPointer += 4;
                                    yylex->val = 930;
                                    return DP_TZ;
                                }
                                break;
                        }
                        break;
                    case TJS_N('t'):
                    case TJS_N('T'):
                        if (!TJS_iswalpha(&InputPointer[3]))
                        {
                            InputPointer += 3;
                            yylex->val = -1000;
                            return DP_TZ;
                        }
                        break;
                }
                break;
            case TJS_N('c'):
            case TJS_N('C'):
                switch (InputPointer[2])
                {
                    case TJS_N('t'):
                    case TJS_N('T'):
                        if (!TJS_iswalpha(&InputPointer[3]))
                        {
                            InputPointer += 3;
                            yylex->val = 800;
                            return DP_TZ;
                        }
                        break;
                }
                break;
            case TJS_N('d'):
            case TJS_N('D'):
                switch (InputPointer[2])
                {
                    case TJS_N('t'):
                    case TJS_N('T'):
                        if (!TJS_iswalpha(&InputPointer[3]))
                        {
                            InputPointer += 3;
                            yylex->val = -500;
                            return DP_TZ;
                        }
                        break;
                }
                break;
            case TJS_N('e'):
            case TJS_N('E'):
                switch (InputPointer[2])
                {
                    case TJS_N('t'):
                    case TJS_N('T'):
                        switch (InputPointer[3])
                        {
                            case TJS_N('d'):
                            case TJS_N('D'):
                                switch (InputPointer[4])
                                {
                                    case TJS_N('s'):
                                    case TJS_N('S'):
                                        switch (InputPointer[5])
                                        {
                                            case TJS_N('t'):
                                            case TJS_N('T'):
                                                if (!TJS_iswalpha(&InputPointer[6]))
                                                {
                                                    InputPointer += 6;
                                                    yylex->val = 200;
                                                    return DP_TZ;
                                                }
                                                break;
                                        }
                                        break;
                                }
                                break;
                            default:
                                if (!TJS_iswalpha(&InputPointer[3]))
                                {
                                    InputPointer += 3;
                                    yylex->val = 100;
                                    return DP_TZ;
                                }
                        }
                        break;
                }
                break;
            case TJS_N('s'):
            case TJS_N('S'):
                switch (InputPointer[2])
                {
                    case TJS_N('t'):
                    case TJS_N('T'):
                        if (!TJS_iswalpha(&InputPointer[3]))
                        {
                            InputPointer += 3;
                            yylex->val = -600;
                            return DP_TZ;
                        }
                        break;
                }
                break;
        }
        break;
    case TJS_N('d'):
    case TJS_N('D'):
        switch (InputPointer[1])
        {
            case TJS_N('e'):
            case TJS_N('E'):
                switch (InputPointer[2])
                {
                    case TJS_N('c'):
                    case TJS_N('C'):
                        switch (InputPointer[3])
                        {
                            case TJS_N('.'):
                                if (!TJS_iswalpha(&InputPointer[4]))
                                {
                                    InputPointer += 4;
                                    yylex->val = 11;
                                    return DP_MONTH;
                                }
                                break;
                            case TJS_N('e'):
                            case TJS_N('E'):
                                switch (InputPointer[4])
                                {
                                    case TJS_N('m'):
                                    case TJS_N('M'):
                                        switch (InputPointer[5])
                                        {
                                            case TJS_N('b'):
                                            case TJS_N('B'):
                                                switch (InputPointer[6])
                                                {
                                                    case TJS_N('e'):
                                                    case TJS_N('E'):
                                                        switch (InputPointer[7])
                                                        {
                                                            case TJS_N('r'):
                                                            case TJS_N('R'):
                                                                if (!TJS_iswalpha(&InputPointer[8]))
                                                                {
                                                                    InputPointer += 8;
                                                                    yylex->val = 11;
                                                                    return DP_MONTH;
                                                                }
                                                                break;
                                                        }
                                                        break;
                                                }
                                                break;
                                        }
                                        break;
                                }
                                break;
                            default:
                                if (!TJS_iswalpha(&InputPointer[3]))
                                {
                                    InputPointer += 3;
                                    yylex->val = 11;
                                    return DP_MONTH;
                                }
                        }
                        break;
                }
                break;
            case TJS_N('n'):
            case TJS_N('N'):
                switch (InputPointer[2])
                {
                    case TJS_N('t'):
                    case TJS_N('T'):
                        if (!TJS_iswalpha(&InputPointer[3]))
                        {
                            InputPointer += 3;
                            yylex->val = 100;
                            return DP_TZ;
                        }
                        break;
                }
                break;
        }
        break;
    case TJS_N('e'):
    case TJS_N('E'):
        switch (InputPointer[1])
        {
            case TJS_N('a'):
            case TJS_N('A'):
                switch (InputPointer[2])
                {
                    case TJS_N('s'):
                    case TJS_N('S'):
                        switch (InputPointer[3])
                        {
                            case TJS_N('t'):
                            case TJS_N('T'):
                                if (!TJS_iswalpha(&InputPointer[4]))
                                {
                                    InputPointer += 4;
                                    yylex->val = 1000;
                                    return DP_TZ;
                                }
                                break;
                        }
                        break;
                }
                break;
            case TJS_N('d'):
            case TJS_N('D'):
                switch (InputPointer[2])
                {
                    case TJS_N('t'):
                    case TJS_N('T'):
                        if (!TJS_iswalpha(&InputPointer[3]))
                        {
                            InputPointer += 3;
                            yylex->val = -400;
                            return DP_TZ;
                        }
                        break;
                }
                break;
            case TJS_N('e'):
            case TJS_N('E'):
                switch (InputPointer[2])
                {
                    case TJS_N('t'):
                    case TJS_N('T'):
                        switch (InputPointer[3])
                        {
                            case TJS_N('d'):
                            case TJS_N('D'):
                                switch (InputPointer[4])
                                {
                                    case TJS_N('s'):
                                    case TJS_N('S'):
                                        switch (InputPointer[5])
                                        {
                                            case TJS_N('t'):
                                            case TJS_N('T'):
                                                if (!TJS_iswalpha(&InputPointer[6]))
                                                {
                                                    InputPointer += 6;
                                                    yylex->val = 300;
                                                    return DP_TZ;
                                                }
                                                break;
                                        }
                                        break;
                                }
                                break;
                            default:
                                if (!TJS_iswalpha(&InputPointer[3]))
                                {
                                    InputPointer += 3;
                                    yylex->val = 200;
                                    return DP_TZ;
                                }
                        }
                        break;
                }
                break;
            case TJS_N('s'):
            case TJS_N('S'):
                switch (InputPointer[2])
                {
                    case TJS_N('t'):
                    case TJS_N('T'):
                        if (!TJS_iswalpha(&InputPointer[3]))
                        {
                            InputPointer += 3;
                            yylex->val = -500;
                            return DP_TZ;
                        }
                        break;
                }
                break;
        }
        break;
    case TJS_N('f'):
    case TJS_N('F'):
        switch (InputPointer[1])
        {
            case TJS_N('e'):
            case TJS_N('E'):
                switch (InputPointer[2])
                {
                    case TJS_N('b'):
                    case TJS_N('B'):
                        switch (InputPointer[3])
                        {
                            case TJS_N('.'):
                                if (!TJS_iswalpha(&InputPointer[4]))
                                {
                                    InputPointer += 4;
                                    yylex->val = 1;
                                    return DP_MONTH;
                                }
                                break;
                            case TJS_N('r'):
                            case TJS_N('R'):
                                switch (InputPointer[4])
                                {
                                    case TJS_N('u'):
                                    case TJS_N('U'):
                                        switch (InputPointer[5])
                                        {
                                            case TJS_N('a'):
                                            case TJS_N('A'):
                                                switch (InputPointer[6])
                                                {
                                                    case TJS_N('r'):
                                                    case TJS_N('R'):
                                                        switch (InputPointer[7])
                                                        {
                                                            case TJS_N('y'):
                                                            case TJS_N('Y'):
                                                                if (!TJS_iswalpha(&InputPointer[8]))
                                                                {
                                                                    InputPointer += 8;
                                                                    yylex->val = 1;
                                                                    return DP_MONTH;
                                                                }
                                                                break;
                                                        }
                                                        break;
                                                }
                                                break;
                                        }
                                        break;
                                }
                                break;
                            default:
                                if (!TJS_iswalpha(&InputPointer[3]))
                                {
                                    InputPointer += 3;
                                    yylex->val = 1;
                                    return DP_MONTH;
                                }
                        }
                        break;
                }
                break;
            case TJS_N('r'):
            case TJS_N('R'):
                switch (InputPointer[2])
                {
                    case TJS_N('i'):
                    case TJS_N('I'):
                        switch (InputPointer[3])
                        {
                            case TJS_N('.'):
                                if (!TJS_iswalpha(&InputPointer[4]))
                                {
                                    InputPointer += 4;
                                    yylex->val = 5;
                                    return DP_WDAY;
                                }
                                break;
                            case TJS_N('d'):
                            case TJS_N('D'):
                                switch (InputPointer[4])
                                {
                                    case TJS_N('a'):
                                    case TJS_N('A'):
                                        switch (InputPointer[5])
                                        {
                                            case TJS_N('y'):
                                            case TJS_N('Y'):
                                                if (!TJS_iswalpha(&InputPointer[6]))
                                                {
                                                    InputPointer += 6;
                                                    yylex->val = 5;
                                                    return DP_WDAY;
                                                }
                                                break;
                                        }
                                        break;
                                }
                                break;
                            default:
                                if (!TJS_iswalpha(&InputPointer[3]))
                                {
                                    InputPointer += 3;
                                    yylex->val = 5;
                                    return DP_WDAY;
                                }
                        }
                        break;
                }
                break;
            case TJS_N('s'):
            case TJS_N('S'):
                switch (InputPointer[2])
                {
                    case TJS_N('t'):
                    case TJS_N('T'):
                        if (!TJS_iswalpha(&InputPointer[3]))
                        {
                            InputPointer += 3;
                            yylex->val = 100;
                            return DP_TZ;
                        }
                        break;
                }
                break;
            case TJS_N('w'):
            case TJS_N('W'):
                switch (InputPointer[2])
                {
                    case TJS_N('t'):
                    case TJS_N('T'):
                        if (!TJS_iswalpha(&InputPointer[3]))
                        {
                            InputPointer += 3;
                            yylex->val = 200;
                            return DP_TZ;
                        }
                        break;
                }
                break;
        }
        break;
    case TJS_N('g'):
    case TJS_N('G'):
        switch (InputPointer[1])
        {
            case TJS_N('m'):
            case TJS_N('M'):
                switch (InputPointer[2])
                {
                    case TJS_N('t'):
                    case TJS_N('T'):
                        if (!TJS_iswalpha(&InputPointer[3]))
                        {
                            InputPointer += 3;
                            yylex->val = 0;
                            return DP_TZ;
                        }
                        break;
                }
                break;
            case TJS_N('s'):
            case TJS_N('S'):
                switch (InputPointer[2])
                {
                    case TJS_N('t'):
                    case TJS_N('T'):
                        if (!TJS_iswalpha(&InputPointer[3]))
                        {
                            InputPointer += 3;
                            yylex->val = 1000;
                            return DP_TZ;
                        }
                        break;
                }
                break;
        }
        break;
    case TJS_N('h'):
    case TJS_N('H'):
        switch (InputPointer[1])
        {
            case TJS_N('d'):
            case TJS_N('D'):
                switch (InputPointer[2])
                {
                    case TJS_N('t'):
                    case TJS_N('T'):
                        if (!TJS_iswalpha(&InputPointer[3]))
                        {
                            InputPointer += 3;
                            yylex->val = -900;
                            return DP_TZ;
                        }
                        break;
                }
                break;
        }
        break;
    case TJS_N('i'):
    case TJS_N('I'):
        switch (InputPointer[1])
        {
            case TJS_N('d'):
            case TJS_N('D'):
                switch (InputPointer[2])
                {
                    case TJS_N('l'):
                    case TJS_N('L'):
                        switch (InputPointer[3])
                        {
                            case TJS_N('e'):
                            case TJS_N('E'):
                                if (!TJS_iswalpha(&InputPointer[4]))
                                {
                                    InputPointer += 4;
                                    yylex->val = 1200;
                                    return DP_TZ;
                                }
                                break;
                            case TJS_N('w'):
                            case TJS_N('W'):
                                if (!TJS_iswalpha(&InputPointer[4]))
                                {
                                    InputPointer += 4;
                                    yylex->val = -1200;
                                    return DP_TZ;
                                }
                                break;
                        }
                        break;
                }
                break;
            case TJS_N('s'):
            case TJS_N('S'):
                switch (InputPointer[2])
                {
                    case TJS_N('t'):
                    case TJS_N('T'):
                        if (!TJS_iswalpha(&InputPointer[3]))
                        {
                            InputPointer += 3;
                            yylex->val = 200;
                            return DP_TZ;
                        }
                        break;
                }
                break;
            case TJS_N('t'):
            case TJS_N('T'):
                if (!TJS_iswalpha(&InputPointer[2]))
                {
                    InputPointer += 2;
                    yylex->val = 330;
                    return DP_TZ;
                }
                break;
        }
        break;
    case TJS_N('j'):
    case TJS_N('J'):
        switch (InputPointer[1])
        {
            case TJS_N('a'):
            case TJS_N('A'):
                switch (InputPointer[2])
                {
                    case TJS_N('n'):
                    case TJS_N('N'):
                        switch (InputPointer[3])
                        {
                            case TJS_N('.'):
                                if (!TJS_iswalpha(&InputPointer[4]))
                                {
                                    InputPointer += 4;
                                    yylex->val = 0;
                                    return DP_MONTH;
                                }
                                break;
                            case TJS_N('u'):
                            case TJS_N('U'):
                                switch (InputPointer[4])
                                {
                                    case TJS_N('a'):
                                    case TJS_N('A'):
                                        switch (InputPointer[5])
                                        {
                                            case TJS_N('r'):
                                            case TJS_N('R'):
                                                switch (InputPointer[6])
                                                {
                                                    case TJS_N('y'):
                                                    case TJS_N('Y'):
                                                        if (!TJS_iswalpha(&InputPointer[7]))
                                                        {
                                                            InputPointer += 7;
                                                            yylex->val = 0;
                                                            return DP_MONTH;
                                                        }
                                                        break;
                                                }
                                                break;
                                        }
                                        break;
                                }
                                break;
                            default:
                                if (!TJS_iswalpha(&InputPointer[3]))
                                {
                                    InputPointer += 3;
                                    yylex->val = 0;
                                    return DP_MONTH;
                                }
                        }
                        break;
                }
                break;
            case TJS_N('s'):
            case TJS_N('S'):
                switch (InputPointer[2])
                {
                    case TJS_N('t'):
                    case TJS_N('T'):
                        if (!TJS_iswalpha(&InputPointer[3]))
                        {
                            InputPointer += 3;
                            yylex->val = 900;
                            return DP_TZ;
                        }
                        break;
                }
                break;
            case TJS_N('t'):
            case TJS_N('T'):
                if (!TJS_iswalpha(&InputPointer[2]))
                {
                    InputPointer += 2;
                    yylex->val = 730;
                    return DP_TZ;
                }
                break;
            case TJS_N('u'):
            case TJS_N('U'):
                switch (InputPointer[2])
                {
                    case TJS_N('.'):
                        if (!TJS_iswalpha(&InputPointer[3]))
                        {
                            InputPointer += 3;
                            yylex->val = 5;
                            return DP_MONTH;
                        }
                        break;
                    case TJS_N('l'):
                    case TJS_N('L'):
                        switch (InputPointer[3])
                        {
                            case TJS_N('.'):
                                if (!TJS_iswalpha(&InputPointer[4]))
                                {
                                    InputPointer += 4;
                                    yylex->val = 6;
                                    return DP_MONTH;
                                }
                                break;
                            case TJS_N('y'):
                            case TJS_N('Y'):
                                if (!TJS_iswalpha(&InputPointer[4]))
                                {
                                    InputPointer += 4;
                                    yylex->val = 6;
                                    return DP_MONTH;
                                }
                                break;
                            default:
                                if (!TJS_iswalpha(&InputPointer[3]))
                                {
                                    InputPointer += 3;
                                    yylex->val = 6;
                                    return DP_MONTH;
                                }
                        }
                        break;
                    case TJS_N('n'):
                    case TJS_N('N'):
                        switch (InputPointer[3])
                        {
                            case TJS_N('.'):
                                if (!TJS_iswalpha(&InputPointer[4]))
                                {
                                    InputPointer += 4;
                                    yylex->val = 5;
                                    return DP_MONTH;
                                }
                                break;
                            case TJS_N('e'):
                            case TJS_N('E'):
                                if (!TJS_iswalpha(&InputPointer[4]))
                                {
                                    InputPointer += 4;
                                    yylex->val = 5;
                                    return DP_MONTH;
                                }
                                break;
                            default:
                                if (!TJS_iswalpha(&InputPointer[3]))
                                {
                                    InputPointer += 3;
                                    yylex->val = 5;
                                    return DP_MONTH;
                                }
                        }
                        break;
                    default:
                        if (!TJS_iswalpha(&InputPointer[2]))
                        {
                            InputPointer += 2;
                            yylex->val = 5;
                            return DP_MONTH;
                        }
                }
                break;
        }
        break;
    case TJS_N('k'):
    case TJS_N('K'):
        switch (InputPointer[1])
        {
            case TJS_N('s'):
            case TJS_N('S'):
                switch (InputPointer[2])
                {
                    case TJS_N('t'):
                    case TJS_N('T'):
                        if (!TJS_iswalpha(&InputPointer[3]))
                        {
                            InputPointer += 3;
                            yylex->val = 900;
                            return DP_TZ;
                        }
                        break;
                }
                break;
        }
        break;
    case TJS_N('l'):
    case TJS_N('L'):
        switch (InputPointer[1])
        {
            case TJS_N('i'):
            case TJS_N('I'):
                switch (InputPointer[2])
                {
                    case TJS_N('g'):
                    case TJS_N('G'):
                        switch (InputPointer[3])
                        {
                            case TJS_N('t'):
                            case TJS_N('T'):
                                if (!TJS_iswalpha(&InputPointer[4]))
                                {
                                    InputPointer += 4;
                                    yylex->val = 1000;
                                    return DP_TZ;
                                }
                                break;
                        }
                        break;
                }
                break;
        }
        break;
    case TJS_N('m'):
    case TJS_N('M'):
        switch (InputPointer[1])
        {
            case TJS_N('a'):
            case TJS_N('A'):
                switch (InputPointer[2])
                {
                    case TJS_N('r'):
                    case TJS_N('R'):
                        switch (InputPointer[3])
                        {
                            case TJS_N('.'):
                                if (!TJS_iswalpha(&InputPointer[4]))
                                {
                                    InputPointer += 4;
                                    yylex->val = 2;
                                    return DP_MONTH;
                                }
                                break;
                            case TJS_N('c'):
                            case TJS_N('C'):
                                switch (InputPointer[4])
                                {
                                    case TJS_N('h'):
                                    case TJS_N('H'):
                                        if (!TJS_iswalpha(&InputPointer[5]))
                                        {
                                            InputPointer += 5;
                                            yylex->val = 2;
                                            return DP_MONTH;
                                        }
                                        break;
                                }
                                break;
                            default:
                                if (!TJS_iswalpha(&InputPointer[3]))
                                {
                                    InputPointer += 3;
                                    yylex->val = 2;
                                    return DP_MONTH;
                                }
                        }
                        break;
                    case TJS_N('y'):
                    case TJS_N('Y'):
                        if (!TJS_iswalpha(&InputPointer[3]))
                        {
                            InputPointer += 3;
                            yylex->val = 4;
                            return DP_MONTH;
                        }
                        break;
                }
                break;
            case TJS_N('d'):
            case TJS_N('D'):
                switch (InputPointer[2])
                {
                    case TJS_N('t'):
                    case TJS_N('T'):
                        if (!TJS_iswalpha(&InputPointer[3]))
                        {
                            InputPointer += 3;
                            yylex->val = -600;
                            return DP_TZ;
                        }
                        break;
                }
                break;
            case TJS_N('e'):
            case TJS_N('E'):
                switch (InputPointer[2])
                {
                    case TJS_N('s'):
                    case TJS_N('S'):
                        switch (InputPointer[3])
                        {
                            case TJS_N('t'):
                            case TJS_N('T'):
                                if (!TJS_iswalpha(&InputPointer[4]))
                                {
                                    InputPointer += 4;
                                    yylex->val = 200;
                                    return DP_TZ;
                                }
                                break;
                        }
                        break;
                    case TJS_N('t'):
                    case TJS_N('T'):
                        switch (InputPointer[3])
                        {
                            case TJS_N('d'):
                            case TJS_N('D'):
                                switch (InputPointer[4])
                                {
                                    case TJS_N('s'):
                                    case TJS_N('S'):
                                        switch (InputPointer[5])
                                        {
                                            case TJS_N('t'):
                                            case TJS_N('T'):
                                                if (!TJS_iswalpha(&InputPointer[6]))
                                                {
                                                    InputPointer += 6;
                                                    yylex->val = 200;
                                                    return DP_TZ;
                                                }
                                                break;
                                        }
                                        break;
                                }
                                break;
                            default:
                                if (!TJS_iswalpha(&InputPointer[3]))
                                {
                                    InputPointer += 3;
                                    yylex->val = 100;
                                    return DP_TZ;
                                }
                        }
                        break;
                    case TJS_N('w'):
                    case TJS_N('W'):
                        switch (InputPointer[3])
                        {
                            case TJS_N('t'):
                            case TJS_N('T'):
                                if (!TJS_iswalpha(&InputPointer[4]))
                                {
                                    InputPointer += 4;
                                    yylex->val = 100;
                                    return DP_TZ;
                                }
                                break;
                        }
                        break;
                    case TJS_N('z'):
                    case TJS_N('Z'):
                        if (!TJS_iswalpha(&InputPointer[3]))
                        {
                            InputPointer += 3;
                            yylex->val = 100;
                            return DP_TZ;
                        }
                        break;
                }
                break;
            case TJS_N('o'):
            case TJS_N('O'):
                switch (InputPointer[2])
                {
                    case TJS_N('n'):
                    case TJS_N('N'):
                        switch (InputPointer[3])
                        {
                            case TJS_N('.'):
                                if (!TJS_iswalpha(&InputPointer[4]))
                                {
                                    InputPointer += 4;
                                    yylex->val = 1;
                                    return DP_WDAY;
                                }
                                break;
                            case TJS_N('d'):
                            case TJS_N('D'):
                                switch (InputPointer[4])
                                {
                                    case TJS_N('a'):
                                    case TJS_N('A'):
                                        switch (InputPointer[5])
                                        {
                                            case TJS_N('y'):
                                            case TJS_N('Y'):
                                                if (!TJS_iswalpha(&InputPointer[6]))
                                                {
                                                    InputPointer += 6;
                                                    yylex->val = 1;
                                                    return DP_WDAY;
                                                }
                                                break;
                                        }
                                        break;
                                }
                                break;
                            default:
                                if (!TJS_iswalpha(&InputPointer[3]))
                                {
                                    InputPointer += 3;
                                    yylex->val = 1;
                                    return DP_WDAY;
                                }
                        }
                        break;
                }
                break;
            case TJS_N('s'):
            case TJS_N('S'):
                switch (InputPointer[2])
                {
                    case TJS_N('t'):
                    case TJS_N('T'):
                        if (!TJS_iswalpha(&InputPointer[3]))
                        {
                            InputPointer += 3;
                            yylex->val = -700;
                            return DP_TZ;
                        }
                        break;
                }
                break;
            case TJS_N('t'):
            case TJS_N('T'):
                if (!TJS_iswalpha(&InputPointer[2]))
                {
                    InputPointer += 2;
                    yylex->val = 830;
                    return DP_TZ;
                }
                break;
            default:
                if (!TJS_iswalpha(&InputPointer[1]))
                {
                    InputPointer += 1;
                    yylex->val = -1200;
                    return DP_TZ;
                }
        }
        break;
    case TJS_N('n'):
    case TJS_N('N'):
        switch (InputPointer[1])
        {
            case TJS_N('d'):
            case TJS_N('D'):
                switch (InputPointer[2])
                {
                    case TJS_N('t'):
                    case TJS_N('T'):
                        if (!TJS_iswalpha(&InputPointer[3]))
                        {
                            InputPointer += 3;
                            yylex->val = -230;
                            return DP_TZ;
                        }
                        break;
                }
                break;
            case TJS_N('f'):
            case TJS_N('F'):
                switch (InputPointer[2])
                {
                    case TJS_N('t'):
                    case TJS_N('T'):
                        if (!TJS_iswalpha(&InputPointer[3]))
                        {
                            InputPointer += 3;
                            yylex->val = -330;
                            return DP_TZ;
                        }
                        break;
                }
                break;
            case TJS_N('o'):
            case TJS_N('O'):
                switch (InputPointer[2])
                {
                    case TJS_N('r'):
                    case TJS_N('R'):
                        if (!TJS_iswalpha(&InputPointer[3]))
                        {
                            InputPointer += 3;
                            yylex->val = 100;
                            return DP_TZ;
                        }
                        break;
                    case TJS_N('v'):
                    case TJS_N('V'):
                        switch (InputPointer[3])
                        {
                            case TJS_N('.'):
                                if (!TJS_iswalpha(&InputPointer[4]))
                                {
                                    InputPointer += 4;
                                    yylex->val = 10;
                                    return DP_MONTH;
                                }
                                break;
                            case TJS_N('e'):
                            case TJS_N('E'):
                                switch (InputPointer[4])
                                {
                                    case TJS_N('m'):
                                    case TJS_N('M'):
                                        switch (InputPointer[5])
                                        {
                                            case TJS_N('b'):
                                            case TJS_N('B'):
                                                switch (InputPointer[6])
                                                {
                                                    case TJS_N('e'):
                                                    case TJS_N('E'):
                                                        switch (InputPointer[7])
                                                        {
                                                            case TJS_N('r'):
                                                            case TJS_N('R'):
                                                                if (!TJS_iswalpha(&InputPointer[8]))
                                                                {
                                                                    InputPointer += 8;
                                                                    yylex->val = 10;
                                                                    return DP_MONTH;
                                                                }
                                                                break;
                                                        }
                                                        break;
                                                }
                                                break;
                                        }
                                        break;
                                }
                                break;
                            default:
                                if (!TJS_iswalpha(&InputPointer[3]))
                                {
                                    InputPointer += 3;
                                    yylex->val = 10;
                                    return DP_MONTH;
                                }
                        }
                        break;
                }
                break;
            case TJS_N('s'):
            case TJS_N('S'):
                switch (InputPointer[2])
                {
                    case TJS_N('t'):
                    case TJS_N('T'):
                        if (!TJS_iswalpha(&InputPointer[3]))
                        {
                            InputPointer += 3;
                            yylex->val = -330;
                            return DP_TZ;
                        }
                        break;
                }
                break;
            case TJS_N('t'):
            case TJS_N('T'):
                if (!TJS_iswalpha(&InputPointer[2]))
                {
                    InputPointer += 2;
                    yylex->val = -1100;
                    return DP_TZ;
                }
                break;
            case TJS_N('z'):
            case TJS_N('Z'):
                switch (InputPointer[2])
                {
                    case TJS_N('d'):
                    case TJS_N('D'):
                        switch (InputPointer[3])
                        {
                            case TJS_N('t'):
                            case TJS_N('T'):
                                if (!TJS_iswalpha(&InputPointer[4]))
                                {
                                    InputPointer += 4;
                                    yylex->val = 1300;
                                    return DP_TZ;
                                }
                                break;
                        }
                        break;
                    case TJS_N('s'):
                    case TJS_N('S'):
                        switch (InputPointer[3])
                        {
                            case TJS_N('t'):
                            case TJS_N('T'):
                                if (!TJS_iswalpha(&InputPointer[4]))
                                {
                                    InputPointer += 4;
                                    yylex->val = 1200;
                                    return DP_TZ;
                                }
                                break;
                        }
                        break;
                    case TJS_N('t'):
                    case TJS_N('T'):
                        if (!TJS_iswalpha(&InputPointer[3]))
                        {
                            InputPointer += 3;
                            yylex->val = 1200;
                            return DP_TZ;
                        }
                        break;
                }
                break;
            default:
                if (!TJS_iswalpha(&InputPointer[1]))
                {
                    InputPointer += 1;
                    yylex->val = 100;
                    return DP_TZ;
                }
        }
        break;
    case TJS_N('o'):
    case TJS_N('O'):
        switch (InputPointer[1])
        {
            case TJS_N('c'):
            case TJS_N('C'):
                switch (InputPointer[2])
                {
                    case TJS_N('t'):
                    case TJS_N('T'):
                        switch (InputPointer[3])
                        {
                            case TJS_N('.'):
                                if (!TJS_iswalpha(&InputPointer[4]))
                                {
                                    InputPointer += 4;
                                    yylex->val = 9;
                                    return DP_MONTH;
                                }
                                break;
                            case TJS_N('o'):
                            case TJS_N('O'):
                                switch (InputPointer[4])
                                {
                                    case TJS_N('b'):
                                    case TJS_N('B'):
                                        switch (InputPointer[5])
                                        {
                                            case TJS_N('e'):
                                            case TJS_N('E'):
                                                switch (InputPointer[6])
                                                {
                                                    case TJS_N('r'):
                                                    case TJS_N('R'):
                                                        if (!TJS_iswalpha(&InputPointer[7]))
                                                        {
                                                            InputPointer += 7;
                                                            yylex->val = 9;
                                                            return DP_MONTH;
                                                        }
                                                        break;
                                                }
                                                break;
                                        }
                                        break;
                                }
                                break;
                            default:
                                if (!TJS_iswalpha(&InputPointer[3]))
                                {
                                    InputPointer += 3;
                                    yylex->val = 9;
                                    return DP_MONTH;
                                }
                        }
                        break;
                }
                break;
        }
        break;
    case TJS_N('p'):
    case TJS_N('P'):
        switch (InputPointer[1])
        {
            case TJS_N('d'):
            case TJS_N('D'):
                switch (InputPointer[2])
                {
                    case TJS_N('t'):
                    case TJS_N('T'):
                        if (!TJS_iswalpha(&InputPointer[3]))
                        {
                            InputPointer += 3;
                            yylex->val = -700;
                            return DP_TZ;
                        }
                        break;
                }
                break;
            case TJS_N('m'):
            case TJS_N('M'):
                if (!TJS_iswalpha(&InputPointer[2]))
                {
                    InputPointer += 2;
                    yylex->val = 0;
                    return DP_PM;
                }
                break;
            case TJS_N('s'):
            case TJS_N('S'):
                switch (InputPointer[2])
                {
                    case TJS_N('t'):
                    case TJS_N('T'):
                        if (!TJS_iswalpha(&InputPointer[3]))
                        {
                            InputPointer += 3;
                            yylex->val = -800;
                            return DP_TZ;
                        }
                        break;
                }
                break;
        }
        break;
    case TJS_N('s'):
    case TJS_N('S'):
        switch (InputPointer[1])
        {
            case TJS_N('a'):
            case TJS_N('A'):
                switch (InputPointer[2])
                {
                    case TJS_N('d'):
                    case TJS_N('D'):
                        switch (InputPointer[3])
                        {
                            case TJS_N('t'):
                            case TJS_N('T'):
                                if (!TJS_iswalpha(&InputPointer[4]))
                                {
                                    InputPointer += 4;
                                    yylex->val = 1030;
                                    return DP_TZ;
                                }
                                break;
                        }
                        break;
                    case TJS_N('s'):
                    case TJS_N('S'):
                        switch (InputPointer[3])
                        {
                            case TJS_N('t'):
                            case TJS_N('T'):
                                if (!TJS_iswalpha(&InputPointer[4]))
                                {
                                    InputPointer += 4;
                                    yylex->val = 930;
                                    return DP_TZ;
                                }
                                break;
                        }
                        break;
                    case TJS_N('t'):
                    case TJS_N('T'):
                        switch (InputPointer[3])
                        {
                            case TJS_N('.'):
                                if (!TJS_iswalpha(&InputPointer[4]))
                                {
                                    InputPointer += 4;
                                    yylex->val = 6;
                                    return DP_WDAY;
                                }
                                break;
                            case TJS_N('u'):
                            case TJS_N('U'):
                                switch (InputPointer[4])
                                {
                                    case TJS_N('r'):
                                    case TJS_N('R'):
                                        switch (InputPointer[5])
                                        {
                                            case TJS_N('d'):
                                            case TJS_N('D'):
                                                switch (InputPointer[6])
                                                {
                                                    case TJS_N('a'):
                                                    case TJS_N('A'):
                                                        switch (InputPointer[7])
                                                        {
                                                            case TJS_N('y'):
                                                            case TJS_N('Y'):
                                                                if (!TJS_iswalpha(&InputPointer[8]))
                                                                {
                                                                    InputPointer += 8;
                                                                    yylex->val = 6;
                                                                    return DP_WDAY;
                                                                }
                                                                break;
                                                        }
                                                        break;
                                                }
                                                break;
                                        }
                                        break;
                                }
                                break;
                            default:
                                if (!TJS_iswalpha(&InputPointer[3]))
                                {
                                    InputPointer += 3;
                                    yylex->val = 6;
                                    return DP_WDAY;
                                }
                        }
                        break;
                }
                break;
            case TJS_N('e'):
            case TJS_N('E'):
                switch (InputPointer[2])
                {
                    case TJS_N('p'):
                    case TJS_N('P'):
                        switch (InputPointer[3])
                        {
                            case TJS_N('.'):
                                if (!TJS_iswalpha(&InputPointer[4]))
                                {
                                    InputPointer += 4;
                                    yylex->val = 8;
                                    return DP_MONTH;
                                }
                                break;
                            case TJS_N('t'):
                            case TJS_N('T'):
                                switch (InputPointer[4])
                                {
                                    case TJS_N('.'):
                                        if (!TJS_iswalpha(&InputPointer[5]))
                                        {
                                            InputPointer += 5;
                                            yylex->val = 8;
                                            return DP_MONTH;
                                        }
                                        break;
                                    case TJS_N('e'):
                                    case TJS_N('E'):
                                        switch (InputPointer[5])
                                        {
                                            case TJS_N('m'):
                                            case TJS_N('M'):
                                                switch (InputPointer[6])
                                                {
                                                    case TJS_N('b'):
                                                    case TJS_N('B'):
                                                        switch (InputPointer[7])
                                                        {
                                                            case TJS_N('e'):
                                                            case TJS_N('E'):
                                                                switch (InputPointer[8])
                                                                {
                                                                    case TJS_N('r'):
                                                                    case TJS_N('R'):
                                                                        if (!TJS_iswalpha(
                                                                                &InputPointer[9]))
                                                                        {
                                                                            InputPointer += 9;
                                                                            yylex->val = 8;
                                                                            return DP_MONTH;
                                                                        }
                                                                        break;
                                                                }
                                                                break;
                                                        }
                                                        break;
                                                }
                                                break;
                                        }
                                        break;
                                    default:
                                        if (!TJS_iswalpha(&InputPointer[4]))
                                        {
                                            InputPointer += 4;
                                            yylex->val = 8;
                                            return DP_MONTH;
                                        }
                                }
                                break;
                            default:
                                if (!TJS_iswalpha(&InputPointer[3]))
                                {
                                    InputPointer += 3;
                                    yylex->val = 8;
                                    return DP_MONTH;
                                }
                        }
                        break;
                    case TJS_N('t'):
                    case TJS_N('T'):
                        if (!TJS_iswalpha(&InputPointer[3]))
                        {
                            InputPointer += 3;
                            yylex->val = 100;
                            return DP_TZ;
                        }
                        break;
                }
                break;
            case TJS_N('s'):
            case TJS_N('S'):
                switch (InputPointer[2])
                {
                    case TJS_N('t'):
                    case TJS_N('T'):
                        if (!TJS_iswalpha(&InputPointer[3]))
                        {
                            InputPointer += 3;
                            yylex->val = 200;
                            return DP_TZ;
                        }
                        break;
                }
                break;
            case TJS_N('u'):
            case TJS_N('U'):
                switch (InputPointer[2])
                {
                    case TJS_N('n'):
                    case TJS_N('N'):
                        switch (InputPointer[3])
                        {
                            case TJS_N('.'):
                                if (!TJS_iswalpha(&InputPointer[4]))
                                {
                                    InputPointer += 4;
                                    yylex->val = 0;
                                    return DP_WDAY;
                                }
                                break;
                            case TJS_N('d'):
                            case TJS_N('D'):
                                switch (InputPointer[4])
                                {
                                    case TJS_N('a'):
                                    case TJS_N('A'):
                                        switch (InputPointer[5])
                                        {
                                            case TJS_N('y'):
                                            case TJS_N('Y'):
                                                if (!TJS_iswalpha(&InputPointer[6]))
                                                {
                                                    InputPointer += 6;
                                                    yylex->val = 0;
                                                    return DP_WDAY;
                                                }
                                                break;
                                        }
                                        break;
                                }
                                break;
                            default:
                                if (!TJS_iswalpha(&InputPointer[3]))
                                {
                                    InputPointer += 3;
                                    yylex->val = 0;
                                    return DP_WDAY;
                                }
                        }
                        break;
                }
                break;
            case TJS_N('w'):
            case TJS_N('W'):
                switch (InputPointer[2])
                {
                    case TJS_N('t'):
                    case TJS_N('T'):
                        if (!TJS_iswalpha(&InputPointer[3]))
                        {
                            InputPointer += 3;
                            yylex->val = 100;
                            return DP_TZ;
                        }
                        break;
                }
                break;
        }
        break;
    case TJS_N('t'):
    case TJS_N('T'):
        switch (InputPointer[1])
        {
            case TJS_N('h'):
            case TJS_N('H'):
                switch (InputPointer[2])
                {
                    case TJS_N('u'):
                    case TJS_N('U'):
                        switch (InputPointer[3])
                        {
                            case TJS_N('.'):
                                if (!TJS_iswalpha(&InputPointer[4]))
                                {
                                    InputPointer += 4;
                                    yylex->val = 4;
                                    return DP_WDAY;
                                }
                                break;
                            case TJS_N('r'):
                            case TJS_N('R'):
                                switch (InputPointer[4])
                                {
                                    case TJS_N('s'):
                                    case TJS_N('S'):
                                        switch (InputPointer[5])
                                        {
                                            case TJS_N('.'):
                                                if (!TJS_iswalpha(&InputPointer[6]))
                                                {
                                                    InputPointer += 6;
                                                    yylex->val = 4;
                                                    return DP_WDAY;
                                                }
                                                break;
                                            case TJS_N('d'):
                                            case TJS_N('D'):
                                                switch (InputPointer[6])
                                                {
                                                    case TJS_N('a'):
                                                    case TJS_N('A'):
                                                        switch (InputPointer[7])
                                                        {
                                                            case TJS_N('y'):
                                                            case TJS_N('Y'):
                                                                if (!TJS_iswalpha(&InputPointer[8]))
                                                                {
                                                                    InputPointer += 8;
                                                                    yylex->val = 4;
                                                                    return DP_WDAY;
                                                                }
                                                                break;
                                                        }
                                                        break;
                                                }
                                                break;
                                            default:
                                                if (!TJS_iswalpha(&InputPointer[5]))
                                                {
                                                    InputPointer += 5;
                                                    yylex->val = 4;
                                                    return DP_WDAY;
                                                }
                                        }
                                        break;
                                }
                                break;
                            default:
                                if (!TJS_iswalpha(&InputPointer[3]))
                                {
                                    InputPointer += 3;
                                    yylex->val = 4;
                                    return DP_WDAY;
                                }
                        }
                        break;
                }
                break;
            case TJS_N('u'):
            case TJS_N('U'):
                switch (InputPointer[2])
                {
                    case TJS_N('e'):
                    case TJS_N('E'):
                        switch (InputPointer[3])
                        {
                            case TJS_N('.'):
                                if (!TJS_iswalpha(&InputPointer[4]))
                                {
                                    InputPointer += 4;
                                    yylex->val = 2;
                                    return DP_WDAY;
                                }
                                break;
                            case TJS_N('s'):
                            case TJS_N('S'):
                                switch (InputPointer[4])
                                {
                                    case TJS_N('.'):
                                        if (!TJS_iswalpha(&InputPointer[5]))
                                        {
                                            InputPointer += 5;
                                            yylex->val = 2;
                                            return DP_WDAY;
                                        }
                                        break;
                                    case TJS_N('d'):
                                    case TJS_N('D'):
                                        switch (InputPointer[5])
                                        {
                                            case TJS_N('a'):
                                            case TJS_N('A'):
                                                switch (InputPointer[6])
                                                {
                                                    case TJS_N('y'):
                                                    case TJS_N('Y'):
                                                        if (!TJS_iswalpha(&InputPointer[7]))
                                                        {
                                                            InputPointer += 7;
                                                            yylex->val = 2;
                                                            return DP_WDAY;
                                                        }
                                                        break;
                                                }
                                                break;
                                        }
                                        break;
                                    default:
                                        if (!TJS_iswalpha(&InputPointer[4]))
                                        {
                                            InputPointer += 4;
                                            yylex->val = 2;
                                            return DP_WDAY;
                                        }
                                }
                                break;
                            default:
                                if (!TJS_iswalpha(&InputPointer[3]))
                                {
                                    InputPointer += 3;
                                    yylex->val = 2;
                                    return DP_WDAY;
                                }
                        }
                        break;
                }
                break;
        }
        break;
    case TJS_N('u'):
    case TJS_N('U'):
        switch (InputPointer[1])
        {
            case TJS_N('t'):
            case TJS_N('T'):
                switch (InputPointer[2])
                {
                    case TJS_N('c'):
                    case TJS_N('C'):
                        if (!TJS_iswalpha(&InputPointer[3]))
                        {
                            InputPointer += 3;
                            yylex->val = 0;
                            return DP_TZ;
                        }
                        break;
                    default:
                        if (!TJS_iswalpha(&InputPointer[2]))
                        {
                            InputPointer += 2;
                            yylex->val = 0;
                            return DP_TZ;
                        }
                }
                break;
        }
        break;
    case TJS_N('w'):
    case TJS_N('W'):
        switch (InputPointer[1])
        {
            case TJS_N('a'):
            case TJS_N('A'):
                switch (InputPointer[2])
                {
                    case TJS_N('d'):
                    case TJS_N('D'):
                        switch (InputPointer[3])
                        {
                            case TJS_N('t'):
                            case TJS_N('T'):
                                if (!TJS_iswalpha(&InputPointer[4]))
                                {
                                    InputPointer += 4;
                                    yylex->val = 800;
                                    return DP_TZ;
                                }
                                break;
                        }
                        break;
                    case TJS_N('s'):
                    case TJS_N('S'):
                        switch (InputPointer[3])
                        {
                            case TJS_N('t'):
                            case TJS_N('T'):
                                if (!TJS_iswalpha(&InputPointer[4]))
                                {
                                    InputPointer += 4;
                                    yylex->val = 700;
                                    return DP_TZ;
                                }
                                break;
                        }
                        break;
                    case TJS_N('t'):
                    case TJS_N('T'):
                        if (!TJS_iswalpha(&InputPointer[3]))
                        {
                            InputPointer += 3;
                            yylex->val = -100;
                            return DP_TZ;
                        }
                        break;
                }
                break;
            case TJS_N('d'):
            case TJS_N('D'):
                switch (InputPointer[2])
                {
                    case TJS_N('t'):
                    case TJS_N('T'):
                        if (!TJS_iswalpha(&InputPointer[3]))
                        {
                            InputPointer += 3;
                            yylex->val = 900;
                            return DP_TZ;
                        }
                        break;
                }
                break;
            case TJS_N('e'):
            case TJS_N('E'):
                switch (InputPointer[2])
                {
                    case TJS_N('d'):
                    case TJS_N('D'):
                        switch (InputPointer[3])
                        {
                            case TJS_N('.'):
                                if (!TJS_iswalpha(&InputPointer[4]))
                                {
                                    InputPointer += 4;
                                    yylex->val = 3;
                                    return DP_WDAY;
                                }
                                break;
                            case TJS_N('n'):
                            case TJS_N('N'):
                                switch (InputPointer[4])
                                {
                                    case TJS_N('e'):
                                    case TJS_N('E'):
                                        switch (InputPointer[5])
                                        {
                                            case TJS_N('s'):
                                            case TJS_N('S'):
                                                switch (InputPointer[6])
                                                {
                                                    case TJS_N('d'):
                                                    case TJS_N('D'):
                                                        switch (InputPointer[7])
                                                        {
                                                            case TJS_N('a'):
                                                            case TJS_N('A'):
                                                                switch (InputPointer[8])
                                                                {
                                                                    case TJS_N('y'):
                                                                    case TJS_N('Y'):
                                                                        if (!TJS_iswalpha(
                                                                                &InputPointer[9]))
                                                                        {
                                                                            InputPointer += 9;
                                                                            yylex->val = 3;
                                                                            return DP_WDAY;
                                                                        }
                                                                        break;
                                                                }
                                                                break;
                                                        }
                                                        break;
                                                }
                                                break;
                                        }
                                        break;
                                }
                                break;
                            default:
                                if (!TJS_iswalpha(&InputPointer[3]))
                                {
                                    InputPointer += 3;
                                    yylex->val = 3;
                                    return DP_WDAY;
                                }
                        }
                        break;
                    case TJS_N('t'):
                    case TJS_N('T'):
                        switch (InputPointer[3])
                        {
                            case TJS_N('d'):
                            case TJS_N('D'):
                                switch (InputPointer[4])
                                {
                                    case TJS_N('s'):
                                    case TJS_N('S'):
                                        switch (InputPointer[5])
                                        {
                                            case TJS_N('t'):
                                            case TJS_N('T'):
                                                if (!TJS_iswalpha(&InputPointer[6]))
                                                {
                                                    InputPointer += 6;
                                                    yylex->val = 100;
                                                    return DP_TZ;
                                                }
                                                break;
                                        }
                                        break;
                                }
                                break;
                            default:
                                if (!TJS_iswalpha(&InputPointer[3]))
                                {
                                    InputPointer += 3;
                                    yylex->val = 0;
                                    return DP_TZ;
                                }
                        }
                        break;
                }
                break;
            case TJS_N('s'):
            case TJS_N('S'):
                switch (InputPointer[2])
                {
                    case TJS_N('t'):
                    case TJS_N('T'):
                        if (!TJS_iswalpha(&InputPointer[3]))
                        {
                            InputPointer += 3;
                            yylex->val = 800;
                            return DP_TZ;
                        }
                        break;
                }
                break;
        }
        break;
    case TJS_N('y'):
    case TJS_N('Y'):
        switch (InputPointer[1])
        {
            case TJS_N('d'):
            case TJS_N('D'):
                switch (InputPointer[2])
                {
                    case TJS_N('t'):
                    case TJS_N('T'):
                        if (!TJS_iswalpha(&InputPointer[3]))
                        {
                            InputPointer += 3;
                            yylex->val = -800;
                            return DP_TZ;
                        }
                        break;
                }
                break;
            default:
                if (!TJS_iswalpha(&InputPointer[1]))
                {
                    InputPointer += 1;
                    yylex->val = 1200;
                    return DP_TZ;
                }
        }
        break;
    case TJS_N('z'):
    case TJS_N('Z'):
        if (!TJS_iswalpha(&InputPointer[1]))
        {
            InputPointer += 1;
            yylex->val = 0;
            return DP_TZ;
        }
        break;
}
