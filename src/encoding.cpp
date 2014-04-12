#include <ctype.h>

void get_char_in_latin_1 (unsigned char *buffer, unsigned char c)
{
    unsigned char c1='?';
    if (c<0x80) 
    {	
        // Regular line-21 character set, mostly ASCII except these exceptions
        switch (c)
        {
            case 0x2a: // lowercase a, acute accent
                c1=0xe1;
                break;
            case 0x5c: // lowercase e, acute accent
                c1=0xe9;
                break;
            case 0x5e: // lowercase i, acute accent
                c1=0xed;
                break;			
            case 0x5f: // lowercase o, acute accent
                c1=0xf3;
                break;
            case 0x60: // lowercase u, acute accent
                c1=0xfa;
                break;
            case 0x7b: // lowercase c with cedilla
                c1=0xe7;
                break;
            case 0x7c: // division symbol
                c1=0xf7;
                break;
            case 0x7d: // uppercase N tilde
                c1=0xd1;
                break;
            case 0x7e: // lowercase n tilde
                c1=0xf1;
                break;
            default:
                c1=c;
                break;
        }
        *buffer=c1;
        return;
    }
    switch (c)
    {
        // THIS BLOCK INCLUDES THE 16 EXTENDED (TWO-BYTE) LINE 21 CHARACTERS
        // THAT COME FROM HI BYTE=0x11 AND LOW BETWEEN 0x30 AND 0x3F		
        case 0x80: // Registered symbol (R)
            c1=0xae;
            break;			
        case 0x81: // degree sign
            c1=0xb0;
            break;
        case 0x82: // 1/2 symbol			
            c1=0xbd;
            break;
        case 0x83: // Inverted (open) question mark			
            c1=0xbf;
            break;
        case 0x84: // Trademark symbol (TM) - Does not exist in Latin 1
            break;			
        case 0x85: // Cents symbol			
            c1=0xa2;
            break;
        case 0x86: // Pounds sterling			
            c1=0xa3;
            break;
        case 0x87: // Music note - Not in latin 1, so we use 'pilcrow'
            c1=0xb6;
            break;
        case 0x88: // lowercase a, grave accent
            c1=0xe0;
            break;
        case 0x89: // transparent space, we make it regular
            c1=0x20;			
            break;
        case 0x8a: // lowercase e, grave accent
            c1=0xe8;
            break;
        case 0x8b: // lowercase a, circumflex accent
            c1=0xe2;
            break;
        case 0x8c: // lowercase e, circumflex accent
            c1=0xea;			
            break;
        case 0x8d: // lowercase i, circumflex accent
            c1=0xee;
            break;
        case 0x8e: // lowercase o, circumflex accent
            c1=0xf4;
            break;
        case 0x8f: // lowercase u, circumflex accent
            c1=0xfb;
            break;
        // THIS BLOCK INCLUDES THE 32 EXTENDED (TWO-BYTE) LINE 21 CHARACTERS
        // THAT COME FROM HI BYTE=0x12 AND LOW BETWEEN 0x20 AND 0x3F
        case 0x90: // capital letter A with acute
            c1=0xc1;
            break;
        case 0x91: // capital letter E with acute
            c1=0xc9;
            break;
        case 0x92: // capital letter O with acute
            c1=0xd3;
            break;
        case 0x93: // capital letter U with acute
            c1=0xda;
            break;
        case 0x94: // capital letter U with diaresis
            c1=0xdc;
            break;
        case 0x95: // lowercase letter U with diaeresis
            c1=0xfc;
            break;
        case 0x96: // apostrophe
            c1=0x27;			
            break;
        case 0x97: // inverted exclamation mark			
            c1=0xa1;
            break;
        case 0x98: // asterisk
            c1=0x2a;			
            break;
        case 0x99: // apostrophe (yes, duped). See CCADI source code.
            c1=0x27;			
            break;
        case 0x9a: // em dash
            c1=0x2d;			
            break;
        case 0x9b: // copyright sign
            c1=0xa9;
            break;
        case 0x9c: // Service Mark - not available in latin 1
            break;
        case 0x9d: // Full stop (.)
            c1=0x2e;
            break;
        case 0x9e: // Quoatation mark
            c1=0x22;			
            break;
        case 0x9f: // Quoatation mark
            c1=0x22;			
            break;
        case 0xa0: // uppercase A, grave accent
            c1=0xc0;
            break;
        case 0xa1: // uppercase A, circumflex
            c1=0xc2;
            break;			
        case 0xa2: // uppercase C with cedilla
            c1=0xc7;
            break;
        case 0xa3: // uppercase E, grave accent
            c1=0xc8;
            break;
        case 0xa4: // uppercase E, circumflex
            c1=0xca;
            break;
        case 0xa5: // capital letter E with diaresis
            c1=0xcb;
            break;
        case 0xa6: // lowercase letter e with diaresis
            c1=0xeb;
            break;
        case 0xa7: // uppercase I, circumflex
            c1=0xce;
            break;
        case 0xa8: // uppercase I, with diaresis
            c1=0xcf;
            break;
        case 0xa9: // lowercase i, with diaresis
            c1=0xef;
            break;
        case 0xaa: // uppercase O, circumflex
            c1=0xd4;
            break;
        case 0xab: // uppercase U, grave accent
            c1=0xd9;
            break;
        case 0xac: // lowercase u, grave accent
            c1=0xf9;
            break;
        case 0xad: // uppercase U, circumflex
            c1=0xdb;
            break;
        case 0xae: // LEFT-POINTING DOUBLE ANGLE QUOTATION MARK
            c1=0xab;
            break;
        case 0xaf: // RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
            c1=0xbb;
            break;
        // THIS BLOCK INCLUDES THE 32 EXTENDED (TWO-BYTE) LINE 21 CHARACTERS
        // THAT COME FROM HI BYTE=0x13 AND LOW BETWEEN 0x20 AND 0x3F
        case 0xb0: // Uppercase A, tilde
            c1=0xc3;
            break;
        case 0xb1: // Lowercase a, tilde
            c1=0xe3;
            break;
        case 0xb2: // Uppercase I, acute accent
            c1=0xcd;
            break;
        case 0xb3: // Uppercase I, grave accent
            c1=0xcc;
            break;
        case 0xb4: // Lowercase i, grave accent
            c1=0xec;
            break;
        case 0xb5: // Uppercase O, grave accent
            c1=0xd2;
            break;
        case 0xb6: // Lowercase o, grave accent
            c1=0xf2;
            break;
        case 0xb7: // Uppercase O, tilde
            c1=0xd5;
            break;
        case 0xb8: // Lowercase o, tilde
            c1=0xf5;
            break;
        case 0xb9: // Open curly brace
            c1=0x7b;
            break;
        case 0xba: // Closing curly brace
            c1=0x7d;            
            break;
        case 0xbb: // Backslash
            c1=0x5c;
            break;
        case 0xbc: // Caret
            c1=0x5e;
            break;
        case 0xbd: // Underscore
            c1=0x5f;
            break;
        case 0xbe: // Pipe (broken bar)            
            c1=0xa6;
            break;
        case 0xbf: // Tilde
            c1=0x7e; 
            break;
        case 0xc0: // Uppercase A, umlaut            
            c1=0xc4;
            break;
        case 0xc1: // Lowercase A, umlaut
            c1=0xe3; 
            break;
        case 0xc2: // Uppercase O, umlaut
            c1=0xd6;
            break;
        case 0xc3: // Lowercase o, umlaut
            c1=0xf6;
            break;
        case 0xc4: // Esszett (sharp S)
            c1=0xdf;
            break;
        case 0xc5: // Yen symbol
            c1=0xa5;
            break;
        case 0xc6: // Currency symbol
            c1=0xa4;
            break;            
        case 0xc7: // Vertical bar
            c1=0x7c;
            break;            
        case 0xc8: // Uppercase A, ring
            c1=0xc5;
            break;
        case 0xc9: // Lowercase A, ring
            c1=0xe5;
            break;
        case 0xca: // Uppercase O, slash
            c1=0xd8;
            break;
        case 0xcb: // Lowercase o, slash
            c1=0xf8;
            break;
        case 0xcc: // Upper left corner
        case 0xcd: // Upper right corner
        case 0xce: // Lower left corner
        case 0xcf: // Lower right corner
        default: // For those that don't have representation
            *buffer='?'; // I'll do it eventually, I promise
            break; // This are weird chars anyway
    }
    *buffer=c1;	
}

void get_char_in_unicode (unsigned char *buffer, unsigned char c)
{
    unsigned char c1,c2;
    switch (c)
    {
        case 0x84: // Trademark symbol (TM) 
            c2=0x21;
            c1=0x22;
            break;
        case 0x87: // Music note
            c2=0x26;
            c1=0x6a;
            break;
        case 0x9c: // Service Mark
            c2=0x21;
            c1=0x20;
            break;
        case 0xcc: // Upper left corner
            c2=0x23;
            c1=0x1c;
            break;			
        case 0xcd: // Upper right corner
            c2=0x23;
            c1=0x1d;
            break;
        case 0xce: // Lower left corner
            c2=0x23;
            c1=0x1e;
            break;
        case 0xcf: // Lower right corner
            c2=0x23;
            c1=0x1f;
            break;
        default: // Everything else, same as latin-1 followed by 00			
            get_char_in_latin_1 (&c1,c);
            c2=0;
            break;
    }
    *buffer=c1;
    *(buffer+1)=c2;
}

int get_char_in_utf_8 (unsigned char *buffer, unsigned char c) // Returns number of bytes used
{
    if (c<0x80) // Regular line-21 character set, mostly ASCII except these exceptions
    {
        switch (c)
        {
        case 0x2a: // lowercase a, acute accent
            *buffer=0xc3;
            *(buffer+1)=0xa1;
            return 2;
        case 0x5c: // lowercase e, acute accent
            *buffer=0xc3;
            *(buffer+1)=0xa9;
            return 2;
        case 0x5e: // lowercase i, acute accent
            *buffer=0xc3;
            *(buffer+1)=0xad;
            return 2;
        case 0x5f: // lowercase o, acute accent
            *buffer=0xc3;
            *(buffer+1)=0xb3;
            return 2;
        case 0x60: // lowercase u, acute accent
            *buffer=0xc3;
            *(buffer+1)=0xba;
            return 2;
        case 0x7b: // lowercase c with cedilla
            *buffer=0xc3;
            *(buffer+1)=0xa7;
            return 2;
        case 0x7c: // division symbol
            *buffer=0xc3;
            *(buffer+1)=0xb7;
            return 2;
        case 0x7d: // uppercase N tilde
            *buffer=0xc3;
            *(buffer+1)=0x91;
            return 2;
        case 0x7e: // lowercase n tilde
            *buffer=0xc3;
            *(buffer+1)=0xb1;
            return 2;
        default:
            *buffer=c;
            return 1;
        }
    }
    switch (c)
    {
        // THIS BLOCK INCLUDES THE 16 EXTENDED (TWO-BYTE) LINE 21 CHARACTERS
        // THAT COME FROM HI BYTE=0x11 AND LOW BETWEEN 0x30 AND 0x3F		
        case 0x80: // Registered symbol (R)
            *buffer=0xc2;
            *(buffer+1)=0xae;			
            return 2;
        case 0x81: // degree sign
            *buffer=0xc2;
            *(buffer+1)=0xb0;
            return 2;
        case 0x82: // 1/2 symbol
            *buffer=0xc2;
            *(buffer+1)=0xbd;
            return 2;
        case 0x83: // Inverted (open) question mark
            *buffer=0xc2;
            *(buffer+1)=0xbf;
            return 2;
        case 0x84: // Trademark symbol (TM)
            *buffer=0xe2;
            *(buffer+1)=0x84;
            *(buffer+2)=0xa2;
            return 3;
        case 0x85: // Cents symbol
            *buffer=0xc2;
            *(buffer+1)=0xa2;
            return 2;
        case 0x86: // Pounds sterling
            *buffer=0xc2;
            *(buffer+1)=0xa3;
            return 2;
        case 0x87: // Music note			
            *buffer=0xe2;
            *(buffer+1)=0x99;
            *(buffer+2)=0xaa;
            return 3;
        case 0x88: // lowercase a, grave accent
            *buffer=0xc3;
            *(buffer+1)=0xa0;
            return 2;
        case 0x89: // transparent space, we make it regular
            *buffer=0x20;			
            return 1;
        case 0x8a: // lowercase e, grave accent
            *buffer=0xc3;
            *(buffer+1)=0xa8;
            return 2;
        case 0x8b: // lowercase a, circumflex accent
            *buffer=0xc3;
            *(buffer+1)=0xa2;
            return 2;
        case 0x8c: // lowercase e, circumflex accent
            *buffer=0xc3;
            *(buffer+1)=0xaa;
            return 2;
        case 0x8d: // lowercase i, circumflex accent
            *buffer=0xc3;
            *(buffer+1)=0xae;
            return 2;
        case 0x8e: // lowercase o, circumflex accent
            *buffer=0xc3;
            *(buffer+1)=0xb4;
            return 2;
        case 0x8f: // lowercase u, circumflex accent
            *buffer=0xc3;
            *(buffer+1)=0xbb;
            return 2;
        // THIS BLOCK INCLUDES THE 32 EXTENDED (TWO-BYTE) LINE 21 CHARACTERS
        // THAT COME FROM HI BYTE=0x12 AND LOW BETWEEN 0x20 AND 0x3F
        case 0x90: // capital letter A with acute
            *buffer=0xc3;
            *(buffer+1)=0x81;
            return 2;
        case 0x91: // capital letter E with acute
            *buffer=0xc3;
            *(buffer+1)=0x89;
            return 2;
        case 0x92: // capital letter O with acute
            *buffer=0xc3;
            *(buffer+1)=0x93;
            return 2;
        case 0x93: // capital letter U with acute
            *buffer=0xc3;
            *(buffer+1)=0x9a;
            return 2;
        case 0x94: // capital letter U with diaresis
            *buffer=0xc3;
            *(buffer+1)=0x9c;
            return 2;
        case 0x95: // lowercase letter U with diaeresis
            *buffer=0xc3;
            *(buffer+1)=0xbc;
            return 2;
        case 0x96: // apostrophe
            *buffer=0x27;			
            return 1;
        case 0x97: // inverted exclamation mark
            *buffer=0xc2;
            *(buffer+1)=0xa1;
            return 2;
        case 0x98: // asterisk
            *buffer=0x2a;			
            return 1;
        case 0x99: // Plain single quote
            *buffer=0x27;			
            return 1;
        case 0x9a: // em dash
            *buffer=0xe2;			
            *(buffer+1)=0x80;
            *(buffer+2)=0x94;
            return 3;
        case 0x9b: // copyright sign
            *buffer=0xc2;
            *(buffer+1)=0xa9;
            return 2;
        case 0x9c: // Service mark 
            *buffer=0xe2;			
            *(buffer+1)=0x84;
            *(buffer+2)=0xa0;
            return 3;
        case 0x9d: // Round bullet
            *buffer=0xe2;			
            *(buffer+1)=0x80;
            *(buffer+2)=0xa2;
            return 3;
        case 0x9e: // Opening double quotes
            *buffer=0xe2;			
            *(buffer+1)=0x80;
            *(buffer+2)=0x9c;
            return 3;
        case 0x9f: // Closing double quotes
            *buffer=0xe2;			
            *(buffer+1)=0x80;
            *(buffer+2)=0x9d;
            return 3;
        case 0xa0: // uppercase A, grave accent
            *buffer=0xc3;
            *(buffer+1)=0x80;
            return 2;
        case 0xa1: // uppercase A, circumflex
            *buffer=0xc3;
            *(buffer+1)=0x82;
            return 2;
        case 0xa2: // uppercase C with cedilla
            *buffer=0xc3;
            *(buffer+1)=0x87;
            return 2;
        case 0xa3: // uppercase E, grave accent
            *buffer=0xc3;
            *(buffer+1)=0x88;
            return 2;
        case 0xa4: // uppercase E, circumflex
            *buffer=0xc3;
            *(buffer+1)=0x8a;
            return 2;
        case 0xa5: // capital letter E with diaresis
            *buffer=0xc3;
            *(buffer+1)=0x8b;
            return 2;
        case 0xa6: // lowercase letter e with diaresis
            *buffer=0xc3;
            *(buffer+1)=0xab;
            return 2;
        case 0xa7: // uppercase I, circumflex
            *buffer=0xc3;
            *(buffer+1)=0x8e;
            return 2;
        case 0xa8: // uppercase I, with diaresis
            *buffer=0xc3;
            *(buffer+1)=0x8f;
            return 2;
        case 0xa9: // lowercase i, with diaresis
            *buffer=0xc3;
            *(buffer+1)=0xaf;
            return 2;
        case 0xaa: // uppercase O, circumflex
            *buffer=0xc3;
            *(buffer+1)=0x94;
            return 2;
        case 0xab: // uppercase U, grave accent
            *buffer=0xc3;
            *(buffer+1)=0x99;
            return 2;
        case 0xac: // lowercase u, grave accent
            *buffer=0xc3;
            *(buffer+1)=0xb9;
            return 2;
        case 0xad: // uppercase U, circumflex
            *buffer=0xc3;
            *(buffer+1)=0x9b;
            return 2;
        case 0xae: // LEFT-POINTING DOUBLE ANGLE QUOTATION MARK
            *buffer=0xc2;
            *(buffer+1)=0xab;
            return 2;
        case 0xaf: // RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
            *buffer=0xc2;
            *(buffer+1)=0xbb;
            return 2;
        // THIS BLOCK INCLUDES THE 32 EXTENDED (TWO-BYTE) LINE 21 CHARACTERS
        // THAT COME FROM HI BYTE=0x13 AND LOW BETWEEN 0x20 AND 0x3F
        case 0xb0: // Uppercase A, tilde
            *buffer=0xc3;
            *(buffer+1)=0x83;
            return 2;
        case 0xb1: // Lowercase a, tilde
            *buffer=0xc3;
            *(buffer+1)=0xa3;
            return 2;
        case 0xb2: // Uppercase I, acute accent
            *buffer=0xc3;
            *(buffer+1)=0x8d;
            return 2;
        case 0xb3: // Uppercase I, grave accent
            *buffer=0xc3;
            *(buffer+1)=0x8c;
            return 2;
        case 0xb4: // Lowercase i, grave accent
            *buffer=0xc3;
            *(buffer+1)=0xac;
            return 2;
        case 0xb5: // Uppercase O, grave accent
            *buffer=0xc3;
            *(buffer+1)=0x92;
            return 2;
        case 0xb6: // Lowercase o, grave accent
            *buffer=0xc3;
            *(buffer+1)=0xb2;
            return 2;
        case 0xb7: // Uppercase O, tilde
            *buffer=0xc3;
            *(buffer+1)=0x95;
            return 2;
        case 0xb8: // Lowercase o, tilde
            *buffer=0xc3;
            *(buffer+1)=0xb5;
            return 2;
        case 0xb9: // Open curly brace
            *buffer=0x7b;
            return 1;
        case 0xba: // Closing curly brace
            *buffer=0x7d;            
            return 1;
        case 0xbb: // Backslash
            *buffer=0x5c;
            return 1;
        case 0xbc: // Caret
            *buffer=0x5e;
            return 1;
        case 0xbd: // Underscore
            *buffer=0x5f;
            return 1;
        case 0xbe: // Pipe (broken bar)
            *buffer=0xc2; 
            *(buffer+1)=0xa6;
            return 2;
        case 0xbf: // Tilde
            *buffer=0x7e; // Not sure
            return 1;
        case 0xc0: // Uppercase A, umlaut
            *buffer=0xc3; 
            *(buffer+1)=0x84;
            return 2;
        case 0xc1: // Lowercase A, umlaut
            *buffer=0xc3; 
            *(buffer+1)=0xa4;
            return 2;
        case 0xc2: // Uppercase O, umlaut
            *buffer=0xc3; 
            *(buffer+1)=0x96;
            return 2;
        case 0xc3: // Lowercase o, umlaut
            *buffer=0xc3; 
            *(buffer+1)=0xb6;
            return 2;
        case 0xc4: // Esszett (sharp S)
            *buffer=0xc3;
            *(buffer+1)=0x9f;
            return 2;
        case 0xc5: // Yen symbol
            *buffer=0xc2;
            *(buffer+1)=0xa5;
            return 2;
        case 0xc6: // Currency symbol
            *buffer=0xc2;
            *(buffer+1)=0xa4;
            return 2;
        case 0xc7: // Vertical bar
            *buffer=0x7c; 
            return 1;
        case 0xc8: // Uppercase A, ring
            *buffer=0xc3;
            *(buffer+1)=0x85;
            return 2;
        case 0xc9: // Lowercase A, ring
            *buffer=0xc3;
            *(buffer+1)=0xa5;
            return 2;
        case 0xca: // Uppercase O, slash
            *buffer=0xc3;
            *(buffer+1)=0x98;
            return 2;
        case 0xcb: // Lowercase o, slash
            *buffer=0xc3;
            *(buffer+1)=0xb8;
            return 2;
        case 0xcc: // Top left corner
            *buffer=0xe2;
            *(buffer+1)=0x8c;
            *(buffer+2)=0x9c;
            return 3;
        case 0xcd: // Top right corner
            *buffer=0xe2;
            *(buffer+1)=0x8c;
            *(buffer+2)=0x9d;
            return 3;
        case 0xce: // Bottom left corner
            *buffer=0xe2;
            *(buffer+1)=0x8c;
            *(buffer+2)=0x9e;
            return 3;
        case 0xcf: // Bottom right corner
            *buffer=0xe2;
            *(buffer+1)=0x8c;
            *(buffer+2)=0x9f;
            return 3;
        default: // 
            *buffer='?'; // I'll do it eventually, I promise
            return 1; // This are weird chars anyway
    }
}

unsigned char cctolower (unsigned char c)
{
    if (c>='A' && c<='Z')
        return tolower(c);
    switch (c)
    {
        case 0x7d: // uppercase N tilde
            return 0x7e;
        case 0x90: // capital letter A with acute
            return 0x2a;
        case 0x91: // capital letter E with acute
            return 0x5c; 
        case 0x92: // capital letter O with acute
            return 0x5f; 
        case 0x93: // capital letter U with acute
            return 0x60; 
        case 0xa2: // uppercase C with cedilla
            return 0x7b; 
        case 0xa0: // uppercase A, grave accent
            return 0x88; 
        case 0xa3: // uppercase E, grave accent
            return 0x8a; 
        case 0xa1: // uppercase A, circumflex
            return 0x8b; 
        case 0xa4: // uppercase E, circumflex
            return 0x8c; 
        case 0xa7: // uppercase I, circumflex
            return 0x8d; 
        case 0xaa: // uppercase O, circumflex
            return 0x8e; 
        case 0xad: // uppercase U, circumflex
            return 0x8f; 
        case 0x94: // capital letter U with diaresis
            return 0x95; 
        case 0xa5: // capital letter E with diaresis
            return 0xa6; 
        case 0xa8: // uppercase I, with diaresis
            return 0xa9; 
        case 0xab: // uppercase U, grave accent
            return 0xac; 
        case 0xb0: // Uppercase A, tilde
            return 0xb1;
        case 0xb2: // Uppercase I, acute accent
            return 0x5e;
        case 0xb3: // Uppercase I, grave accent
            return 0xb4;
        case 0xb5: // Uppercase O, grave accent
            return 0xb6;
        case 0xb7: // Uppercase O, tilde
            return 0xb8;
        case 0xc0: // Uppercase A, umlaut
            return 0xc1;
        case 0xc2: // Uppercase O, umlaut
            return 0xc3;
        case 0xc8: // Uppercase A, ring
            return 0xc9;
        case 0xca: // Uppercase O, slash
            return 0xcb;
    }
    return c;
}

unsigned char cctoupper (unsigned char c)
{
    if (c>='a' && c<='z')
        return toupper(c);
    switch (c)
    {
        case 0x7e: // lowercase n tilde
            return 0x7d;
        case 0x2a: // lowercase a, acute accent
            return 0x90;
        case 0x5c: // lowercase e, acute accent
            return 0x91;
        case 0x5e: // lowercase i, acute accent
            return 0xb2;
        case 0x5f: // lowercase o, acute accent
            return 0x92;
        case 0x60: // lowercase u, acute accent
            return 0x93;
        case 0x7b: // lowercase c with cedilla
            return 0xa2;
        case 0x88: // lowercase a, grave accent
            return 0xa0;
        case 0x8a: // lowercase e, grave accent
            return 0xa3;
        case 0x8b: // lowercase a, circumflex accent
            return 0xa1;
        case 0x8c: // lowercase e, circumflex accent
            return 0xa4;
        case 0x8d: // lowercase i, circumflex accent
            return 0xa7;
        case 0x8e: // lowercase o, circumflex accent
            return 0xaa;
        case 0x8f: // lowercase u, circumflex accent
            return 0xad;
        case 0x95: // lowercase letter U with diaeresis
            return 0x94;
        case 0xa6: // lowercase letter e with diaresis
            return 0xa5;
        case 0xa9: // lowercase i, with diaresis
            return 0xa8;
        case 0xac: // lowercase u, grave accent
            return 0xab;
        case 0xb1: // Lowercase a, tilde
            return 0xb0; 
        case 0xb4: // Lowercase i, grave accent
            return 0xb3;
        case 0xb6: // Lowercase o, grave accent
            return 0xb5; 
        case 0xb8: // Lowercase o, tilde	
            return 0xb7;
        case 0xc1: // Lowercase A, umlaut	
            return 0xc0; 
        case 0xc3: // Lowercase o, umlaut
            return 0xc2;
        case 0xc9: // Lowercase A, ring
            return 0xc8; 
        case 0xcb: // Lowercase o, slash
            return 0xca; 
    }
    return c;
}
