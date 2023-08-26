//! A module for working with different kinds of text encoding formats.
//!
//! Any Text within the entire application can be in one of the following 4 formats which is
//! represented by [`Encoding`].
//! - [`Line 21`](Encoding::Line21) - Used in 608 captions.
//! - [`Latin-1`](Encoding::Latin1) - ISO/IEC 8859-1.
//! - [`Ucs2`](Encoding::Ucs2) - UCS-2 code points.
//! - [`UTF-8`](Encoding::Utf8)
//!
//! To represent a string in any one of the above encoding, use the following respectively.
//! - [`Line21String`]
//! - [`Latin1String`]
//! - [`Ucs2String`]
//! - [`String`] (same as from rust std)
//!
//! Each of these 4 types can be converted to any other type using [`From::from`] and [`Into::into`].
//!
//! The above types can be used when the encoding is known at compile-time. If the exact encoding
//! is only known at runtime then [`EncodedString`] can be used. Each of the above 4 types can be
//! converted to [`EncodedString`] using [`From::from`] and [`Into::into`]. An [`EncodedString`] can
//! be converted to any of the 4 types by `to_*` methods. Conversions where the target encoding is
//! only known at runtime can be done using [`EncodedString::encode_to`].

/// Represents the different kinds of encoding that [`EncodedString`] can take.
#[derive(Copy, Clone, Debug, Eq, PartialEq)]
pub enum Encoding {
    Line21,
    Latin1,
    Ucs2,
    Utf8,
}

/// Represents a character in Line 21 encoding.
pub type Line21Char = u8;

/// Represents a character in Latin-1 encoding.
pub type Latin1Char = u8;

/// Represents a character in UCS-2 encoding.
pub type Ucs2Char = u16;

/// A String-like type containing a sequence of Line 21 encoded characters.
#[derive(Clone, Debug, Eq, PartialEq)]
pub struct Line21String(Vec<Line21Char>);

/// A String-like type containing a sequence of Latin-1 encoded characters.
#[derive(Clone, Debug, Eq, PartialEq)]
pub struct Latin1String(Vec<Latin1Char>);

/// A String-like type containing a sequence of UCS-2 code points.
#[derive(Clone, Debug, Eq, PartialEq)]
pub struct Ucs2String(Vec<Ucs2Char>);

/// A String-like type that stores its characters in one of the [`Encoding`] formats.
#[derive(Clone, Debug, Eq, PartialEq)]
pub enum EncodedString {
    Line21(Line21String),
    Latin1(Latin1String),
    Ucs2(Ucs2String),
    Utf8(String),
}

/// A placeholder for missing characters.
///
/// It is used for interconverting between [`Encoding`] formats if the target
/// format does not support a character in the source format.
pub const UNAVAILABLE_CHAR: u8 = b'?';

impl Line21String {
    /// Creates a new empty [`Line21String`].
    pub fn new() -> Line21String {
        Line21String(Vec::new())
    }

    /// Creates a new [`Line21String`] from the contents of given [`Vec`].
    pub fn from_vec(v: Vec<Line21Char>) -> Line21String {
        Line21String(v)
    }

    /// Returns a reference to the internal [`Vec`].
    pub fn as_vec(&self) -> &Vec<Line21Char> {
        &self.0
    }

    /// Returns a mutable reference to the internal [`Vec`].
    pub fn as_mut_vec(&mut self) -> &mut Vec<Line21Char> {
        &mut self.0
    }

    /// Returns the internal [`Vec`], consuming this [`Line21String`].
    pub fn into_vec(self) -> Vec<Line21Char> {
        self.0
    }

    /// Converts this [`Line21String`] to a format provided by `encoding`, returning a new [`EncodedString`].
    pub fn encode_to(&self, encoding: Encoding) -> EncodedString {
        match encoding {
            Encoding::Line21 => self.clone().into(),
            Encoding::Latin1 => EncodedString::Latin1(self.into()),
            Encoding::Ucs2 => EncodedString::Ucs2(self.into()),
            Encoding::Utf8 => EncodedString::Utf8(self.into()),
        }
    }

    /// Converts the [`Line21String`] to lowercase, returning a new [`Line21String`].
    pub fn to_lowercase(&self) -> Line21String {
        Line21String::from_vec(
            self.as_vec()
                .iter()
                .map(|&c| line21_to_lowercase(c))
                .collect(),
        )
    }

    /// Converts the [`Line21String`] to uppercase, returning a new [`Line21String`].
    pub fn to_uppercase(&self) -> Line21String {
        Line21String::from_vec(
            self.as_vec()
                .iter()
                .map(|&c| line21_to_uppercase(c))
                .collect(),
        )
    }
}

impl Latin1String {
    /// Creates a new empty [`Latin1String`].
    pub fn new() -> Latin1String {
        Latin1String(Vec::new())
    }

    /// Creates a new [`Latin1String`] from the contents of given [`Vec`].
    pub fn from_vec(v: Vec<Latin1Char>) -> Latin1String {
        Latin1String(v)
    }

    /// Returns a reference to the internal [`Vec`].
    pub fn as_vec(&self) -> &Vec<Latin1Char> {
        &self.0
    }

    /// Returns a mutable reference to the internal [`Vec`].
    pub fn as_mut_vec(&mut self) -> &mut Vec<Latin1Char> {
        &mut self.0
    }

    /// Returns the internal [`Vec`], consuming this [`Latin1String`].
    pub fn into_vec(self) -> Vec<Latin1Char> {
        self.0
    }

    /// Converts this [`Latin1String`] to a format provided by `encoding`, returning a new [`EncodedString`].
    pub fn encode_to(&self, encoding: Encoding) -> EncodedString {
        match encoding {
            Encoding::Line21 => todo!(),
            Encoding::Latin1 => self.clone().into(),
            Encoding::Ucs2 => EncodedString::Ucs2(self.into()),
            Encoding::Utf8 => EncodedString::Utf8(self.into()),
        }
    }
}

impl Ucs2String {
    /// Creates a new empty [`Ucs2String`].
    pub fn new() -> Ucs2String {
        Ucs2String(Vec::new())
    }

    /// Creates a new [`Ucs2String`] from the contents of given [`Vec`].
    pub fn from_vec(v: Vec<Ucs2Char>) -> Ucs2String {
        Ucs2String(v)
    }

    /// Returns a reference to the internal [`Vec`].
    pub fn as_vec(&self) -> &Vec<Ucs2Char> {
        &self.0
    }

    /// Returns a mutable reference to the internal [`Vec`].
    pub fn as_mut_vec(&mut self) -> &mut Vec<Ucs2Char> {
        &mut self.0
    }

    /// Returns the internal [`Vec`], consuming this [`Ucs2String`].
    pub fn into_vec(self) -> Vec<Ucs2Char> {
        self.0
    }

    /// Converts this [`Ucs2String`] to a format provided by `encoding`, returning a new [`EncodedString`].
    pub fn encode_to(&self, encoding: Encoding) -> EncodedString {
        match encoding {
            Encoding::Line21 => EncodedString::Line21(self.into()),
            Encoding::Latin1 => EncodedString::Latin1(self.into()),
            Encoding::Ucs2 => self.clone().into(),
            Encoding::Utf8 => EncodedString::Utf8(self.into()),
        }
    }
}

impl From<&Ucs2String> for Line21String {
    fn from(value: &Ucs2String) -> Line21String {
        Line21String::from_vec(value.as_vec().iter().map(|&c| ucs2_to_line21(c)).collect())
    }
}

impl From<&str> for Line21String {
    fn from(value: &str) -> Line21String {
        Line21String::from_vec(
            value
                .chars()
                .map(char_to_ucs2)
                .map(ucs2_to_line21)
                .collect(),
        )
    }
}

impl From<&Line21String> for Latin1String {
    fn from(value: &Line21String) -> Latin1String {
        Latin1String::from_vec(
            value
                .as_vec()
                .iter()
                .map(|&x| line21_to_latin1(x))
                .collect(),
        )
    }
}

impl From<&Ucs2String> for Latin1String {
    fn from(value: &Ucs2String) -> Latin1String {
        Latin1String::from_vec(value.as_vec().iter().map(|&c| ucs2_to_latin1(c)).collect())
    }
}

impl From<&str> for Latin1String {
    fn from(value: &str) -> Latin1String {
        Latin1String::from_vec(
            value
                .chars()
                .map(char_to_ucs2)
                .map(ucs2_to_latin1)
                .collect(),
        )
    }
}

impl From<&Line21String> for Ucs2String {
    fn from(value: &Line21String) -> Ucs2String {
        Ucs2String::from_vec(value.as_vec().iter().map(|&x| line21_to_ucs2(x)).collect())
    }
}

impl From<&Latin1String> for Ucs2String {
    fn from(value: &Latin1String) -> Ucs2String {
        Ucs2String::from_vec(value.as_vec().iter().map(|&x| x.into()).collect())
    }
}

impl From<&str> for Ucs2String {
    fn from(value: &str) -> Ucs2String {
        Ucs2String::from_vec(value.chars().map(char_to_ucs2).collect())
    }
}

impl From<&Line21String> for String {
    fn from(value: &Line21String) -> String {
        value
            .as_vec()
            .iter()
            .map(|&x| line21_to_ucs2(x))
            .map(ucs2_to_char)
            .collect()
    }
}

impl From<&Latin1String> for String {
    fn from(value: &Latin1String) -> String {
        value
            .as_vec()
            .iter()
            .map(|&x| Into::<char>::into(x))
            .collect()
    }
}

impl From<&Ucs2String> for String {
    fn from(value: &Ucs2String) -> String {
        value.as_vec().iter().map(|&x| ucs2_to_char(x)).collect()
    }
}

impl Default for Line21String {
    fn default() -> Self {
        Self::new()
    }
}

impl Default for Latin1String {
    fn default() -> Self {
        Self::new()
    }
}

impl Default for Ucs2String {
    fn default() -> Self {
        Self::new()
    }
}

impl EncodedString {
    /// Creates an [`EncodedString`] with the given `encoding` from string slice.
    ///
    /// # Examples
    /// ```rust
    /// # use lib_ccxr::util::encoding::*;
    /// let s = EncodedString::from_str("è", Encoding::Latin1);
    /// assert_eq!(s, Latin1String::from_vec(vec![0xe8]).into())
    /// ```
    pub fn from_str(string: &str, encoding: Encoding) -> EncodedString {
        match encoding {
            Encoding::Line21 => EncodedString::Line21(string.into()),
            Encoding::Latin1 => EncodedString::Latin1(string.into()),
            Encoding::Ucs2 => EncodedString::Ucs2(string.into()),
            Encoding::Utf8 => EncodedString::Utf8(string.to_string()),
        }
    }

    /// Returns the [`Encoding`] format of this [`EncodedString`].
    ///
    /// # Examples
    /// ```rust
    /// # use lib_ccxr::util::encoding::*;
    /// let s: EncodedString = Line21String::from_vec(vec![b'a', b'b']).into();
    /// assert_eq!(s.encoding(), Encoding::Line21);
    /// ```
    pub fn encoding(&self) -> Encoding {
        match self {
            EncodedString::Line21(_) => Encoding::Line21,
            EncodedString::Latin1(_) => Encoding::Latin1,
            EncodedString::Ucs2(_) => Encoding::Ucs2,
            EncodedString::Utf8(_) => Encoding::Utf8,
        }
    }

    /// Converts the [`EncodedString`] to Line 21 format, returning a new [`Line21String`].
    ///
    /// # Examples
    /// ```rust
    /// # use lib_ccxr::util::encoding::*;
    /// let s = EncodedString::from_str("Hi 😀", Encoding::Utf8);
    /// assert_eq!(
    ///     s.to_line21(),
    ///     Line21String::from_vec(
    ///         vec![0x48, 0x69, 0x20, 0x3f] // "Hi ?"
    ///     )
    /// )
    /// ```
    pub fn to_line21(&self) -> Line21String {
        match self {
            EncodedString::Line21(l) => l.clone(),
            EncodedString::Latin1(_) => todo!(),
            EncodedString::Ucs2(u) => u.into(),
            EncodedString::Utf8(s) => s.as_str().into(),
        }
    }

    /// Converts the [`EncodedString`] to Latin-1 format, returning a new [`Latin1String`].
    ///
    /// # Examples
    /// ```rust
    /// # use lib_ccxr::util::encoding::*;
    /// let s = EncodedString::from_str("résumé", Encoding::Utf8);
    /// assert_eq!(
    ///     s.to_latin1(),
    ///     Latin1String::from_vec(
    ///         vec![0x72, 0xe9, 0x73, 0x75, 0x6d, 0xe9]
    ///     )
    /// )
    /// ```
    pub fn to_latin1(&self) -> Latin1String {
        match self {
            EncodedString::Line21(l) => l.into(),
            EncodedString::Latin1(l) => l.clone(),
            EncodedString::Ucs2(u) => u.into(),
            EncodedString::Utf8(s) => s.as_str().into(),
        }
    }

    /// Converts the [`EncodedString`] to UCS-2 format, returing a new [`Ucs2String`].
    ///
    /// # Examples
    /// ```rust
    /// # use lib_ccxr::util::encoding::*;
    /// let v = vec![0x72, 0x5c, 0x73, 0x75, 0x6d, 0x5c]; // résumé in Line 21 encoding
    /// let s: EncodedString = Line21String::from_vec(v).into();
    /// assert_eq!(
    ///     s.to_ucs2(),
    ///     Ucs2String::from_vec(
    ///         vec![0x72, 0xe9, 0x73, 0x75, 0x6d, 0xe9]
    ///     )
    /// )
    /// ```
    pub fn to_ucs2(&self) -> Ucs2String {
        match self {
            EncodedString::Line21(l) => l.into(),
            EncodedString::Latin1(l) => l.into(),
            EncodedString::Ucs2(u) => u.clone(),
            EncodedString::Utf8(s) => s.as_str().into(),
        }
    }

    /// Converts the [`EncodedString`] to UTF-8 format, returning a new [`String`].
    ///
    /// # Examples
    /// ```rust
    /// # use lib_ccxr::util::encoding::*;
    /// let v = vec![0x72, 0x5c, 0x73, 0x75, 0x6d, 0x5c]; // résumé in Line 21 encoding
    /// let s: EncodedString = Line21String::from_vec(v).into();
    /// assert_eq!(s.to_utf8(), "résumé".to_string())
    /// ```
    pub fn to_utf8(&self) -> String {
        match self {
            EncodedString::Line21(l) => l.into(),
            EncodedString::Latin1(l) => l.into(),
            EncodedString::Ucs2(u) => u.into(),
            EncodedString::Utf8(s) => s.clone(),
        }
    }

    /// Converts this [`EncodedString`] to a format provided by `encoding`, returning a new [`EncodedString`].
    ///    
    /// # Examples
    /// ```rust
    /// # use lib_ccxr::util::encoding::*;
    /// let v = vec![0x72, 0x5c, 0x73, 0x75, 0x6d, 0x5c]; // résumé in Line 21 encoding
    /// let s: EncodedString = Line21String::from_vec(v).into();
    /// assert_eq!(s.encode_to(Encoding::Utf8), "résumé".to_string().into())
    /// ```
    pub fn encode_to(&self, encoding: Encoding) -> EncodedString {
        match encoding {
            Encoding::Line21 => EncodedString::Line21(self.to_line21()),
            Encoding::Latin1 => EncodedString::Latin1(self.to_latin1()),
            Encoding::Ucs2 => EncodedString::Ucs2(self.to_ucs2()),
            Encoding::Utf8 => EncodedString::Utf8(self.to_utf8()),
        }
    }

    /// Converts the [`EncodedString`] to lowercase, returning a new [`EncodedString`].
    ///
    /// # Examples
    /// ```rust
    /// # use lib_ccxr::util::encoding::*;
    /// let a = vec![0x72, 0x5c, 0x73, 0x75, 0x6d, 0x5c]; // résumé in Line 21 encoding
    /// let b = vec![0x72, 0x91, 0x73, 0x75, 0x6d, 0x91]; // RÉSUMÉ in Line 21 encoding
    /// let sa: EncodedString = Line21String::from_vec(a).into();
    /// let sb: EncodedString = Line21String::from_vec(b).into();
    /// assert_eq!(sb.to_lowercase(), sa)
    /// ```
    pub fn to_lowercase(&self) -> EncodedString {
        match self {
            EncodedString::Line21(l) => l.to_lowercase().into(),
            EncodedString::Latin1(_) => todo!(),
            EncodedString::Ucs2(_) => todo!(),
            EncodedString::Utf8(s) => s.to_lowercase().into(),
        }
    }

    /// Converts the [`EncodedString`] to uppercase, returning a new [`EncodedString`].
    ///
    /// # Examples
    /// ```rust
    /// # use lib_ccxr::util::encoding::*;
    /// let a = vec![0x72, 0x5c, 0x73, 0x75, 0x6d, 0x5c]; // résumé in Line 21 encoding
    /// let b = vec![0x52, 0x91, 0x53, 0x55, 0x4d, 0x91]; // RÉSUMÉ in Line 21 encoding
    /// let sa: EncodedString = Line21String::from_vec(a).into();
    /// let sb: EncodedString = Line21String::from_vec(b).into();
    /// assert_eq!(sa.to_uppercase(), sb)
    /// ```
    pub fn to_uppercase(&self) -> EncodedString {
        match self {
            EncodedString::Line21(l) => l.to_uppercase().into(),
            EncodedString::Latin1(_) => todo!(),
            EncodedString::Ucs2(_) => todo!(),
            EncodedString::Utf8(s) => s.to_uppercase().into(),
        }
    }
}

impl From<Line21String> for EncodedString {
    fn from(value: Line21String) -> Self {
        EncodedString::Line21(value)
    }
}

impl From<Latin1String> for EncodedString {
    fn from(value: Latin1String) -> Self {
        EncodedString::Latin1(value)
    }
}

impl From<Ucs2String> for EncodedString {
    fn from(value: Ucs2String) -> Self {
        EncodedString::Ucs2(value)
    }
}

impl From<String> for EncodedString {
    fn from(value: String) -> Self {
        EncodedString::Utf8(value)
    }
}

fn line21_to_latin1(c: Line21Char) -> Latin1Char {
    if c < 0x80 {
        // Regular line-21 character set, mostly ASCII except these exceptions
        match c {
            0x2a => 0xe1,             // lowercase a, acute accent
            0x5c => 0xe9,             // lowercase e, acute accent
            0x5e => 0xed,             // lowercase i, acute accent
            0x5f => 0xf3,             // lowercase o, acute accent
            0x60 => 0xfa,             // lowercase u, acute accent
            0x7b => 0xe7,             // lowercase c with cedilla
            0x7c => 0xf7,             // division symbol
            0x7d => 0xd1,             // uppercase N tilde
            0x7e => 0xf1,             // lowercase n tilde
            0x7f => UNAVAILABLE_CHAR, // Solid block - Does not exist in Latin 1
            _ => c,
        }
    } else {
        match c {
            // THIS BLOCK INCLUDES THE 16 EXTENDED (TWO-BYTE) LINE 21 CHARACTERS
            // THAT COME FROM HI BYTE=0x11 AND LOW BETWEEN 0x30 AND 0x3F
            0x80 => 0xae,             // Registered symbol (R)
            0x81 => 0xb0,             // degree sign
            0x82 => 0xbd,             // 1/2 symbol
            0x83 => 0xbf,             // Inverted (open) question mark
            0x84 => UNAVAILABLE_CHAR, // Trademark symbol (TM) - Does not exist in Latin 1
            0x85 => 0xa2,             // Cents symbol
            0x86 => 0xa3,             // Pounds sterling
            0x87 => 0xb6,             // Music note - Not in latin 1, so we use 'pilcrow'
            0x88 => 0xe0,             // lowercase a, grave accent
            0x89 => 0x20,             // transparent space, we make it regular
            0x8a => 0xe8,             // lowercase e, grave accent
            0x8b => 0xe2,             // lowercase a, circumflex accent
            0x8c => 0xea,             // lowercase e, circumflex accent
            0x8d => 0xee,             // lowercase i, circumflex accent
            0x8e => 0xf4,             // lowercase o, circumflex accent
            0x8f => 0xfb,             // lowercase u, circumflex accent
            // THIS BLOCK INCLUDES THE 32 EXTENDED (TWO-BYTE) LINE 21 CHARACTERS
            // THAT COME FROM HI BYTE=0x12 AND LOW BETWEEN 0x20 AND 0x3F
            0x90 => 0xc1,             // capital letter A with acute
            0x91 => 0xc9,             // capital letter E with acute
            0x92 => 0xd3,             // capital letter O with acute
            0x93 => 0xda,             // capital letter U with acute
            0x94 => 0xdc,             // capital letter U with diaeresis
            0x95 => 0xfc,             // lowercase letter U with diaeresis
            0x96 => 0x27,             // apostrophe
            0x97 => 0xa1,             // inverted exclamation mark
            0x98 => 0x2a,             // asterisk
            0x99 => 0x27,             // apostrophe (yes, duped). See CCADI source code.
            0x9a => 0x2d,             // em dash
            0x9b => 0xa9,             // copyright sign
            0x9c => UNAVAILABLE_CHAR, // Service Mark - not available in latin 1
            0x9d => 0x2e,             // Full stop (.)
            0x9e => 0x22,             // Quotation mark
            0x9f => 0x22,             // Quotation mark
            0xa0 => 0xc0,             // uppercase A, grave accent
            0xa1 => 0xc2,             // uppercase A, circumflex
            0xa2 => 0xc7,             // uppercase C with cedilla
            0xa3 => 0xc8,             // uppercase E, grave accent
            0xa4 => 0xca,             // uppercase E, circumflex
            0xa5 => 0xcb,             // capital letter E with diaeresis
            0xa6 => 0xeb,             // lowercase letter e with diaeresis
            0xa7 => 0xce,             // uppercase I, circumflex
            0xa8 => 0xcf,             // uppercase I, with diaeresis
            0xa9 => 0xef,             // lowercase i, with diaeresis
            0xaa => 0xd4,             // uppercase O, circumflex
            0xab => 0xd9,             // uppercase U, grave accent
            0xac => 0xf9,             // lowercase u, grave accent
            0xad => 0xdb,             // uppercase U, circumflex
            0xae => 0xab,             // LEFT-POINTING DOUBLE ANGLE QUOTATION MARK
            0xaf => 0xbb,             // RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
            // THIS BLOCK INCLUDES THE 32 EXTENDED (TWO-BYTE) LINE 21 CHARACTERS
            // THAT COME FROM HI BYTE=0x13 AND LOW BETWEEN 0x20 AND 0x3F
            0xb0 => 0xc3,             // Uppercase A, tilde
            0xb1 => 0xe3,             // Lowercase a, tilde
            0xb2 => 0xcd,             // Uppercase I, acute accent
            0xb3 => 0xcc,             // Uppercase I, grave accent
            0xb4 => 0xec,             // Lowercase i, grave accent
            0xb5 => 0xd2,             // Uppercase O, grave accent
            0xb6 => 0xf2,             // Lowercase o, grave accent
            0xb7 => 0xd5,             // Uppercase O, tilde
            0xb8 => 0xf5,             // Lowercase o, tilde
            0xb9 => 0x7b,             // Open curly brace
            0xba => 0x7d,             // Closing curly brace
            0xbb => 0x5c,             // Backslash
            0xbc => 0x5e,             // Caret
            0xbd => 0x5f,             // Underscore
            0xbe => 0xa6,             // Pipe (broken bar)
            0xbf => 0x7e,             // Tilde
            0xc0 => 0xc4,             // Uppercase A, umlaut
            0xc1 => 0xe3,             // Lowercase A, umlaut
            0xc2 => 0xd6,             // Uppercase O, umlaut
            0xc3 => 0xf6,             // Lowercase o, umlaut
            0xc4 => 0xdf,             // Eszett (sharp S)
            0xc5 => 0xa5,             // Yen symbol
            0xc6 => 0xa4,             // Currency symbol
            0xc7 => 0x7c,             // Vertical bar
            0xc8 => 0xc5,             // Uppercase A, ring
            0xc9 => 0xe5,             // Lowercase A, ring
            0xca => 0xd8,             // Uppercase O, slash
            0xcb => 0xf8,             // Lowercase o, slash
            0xcc => UNAVAILABLE_CHAR, // Upper left corner
            0xcd => UNAVAILABLE_CHAR, // Upper right corner
            0xce => UNAVAILABLE_CHAR, // Lower left corner
            0xcf => UNAVAILABLE_CHAR, // Lower right corner
            _ => UNAVAILABLE_CHAR,    // For those that don't have representation
                                       // I'll do it eventually, I promise
                                       // This are weird chars anyway
        }
    }
}

fn line21_to_ucs2(c: Line21Char) -> Ucs2Char {
    match c {
        0x7f => 0x25A0,                  // Solid block
        0x84 => 0x2122,                  // Trademark symbol (TM)
        0x87 => 0x266a,                  // Music note
        0x9c => 0x2120,                  // Service Mark
        0xcc => 0x231c,                  // Upper left corner
        0xcd => 0x231d,                  // Upper right corner
        0xce => 0x231e,                  // Lower left corner
        0xcf => 0x231f,                  // Lower right corner
        _ => line21_to_latin1(c).into(), // Everything else, same as latin-1 followed by 00
    }
}

fn ucs2_to_line21(c: Ucs2Char) -> Line21Char {
    if c < 0x80 {
        c as u8
    } else {
        UNAVAILABLE_CHAR
    }
}

fn ucs2_to_latin1(c: Ucs2Char) -> Latin1Char {
    // Code points 0 to U+00FF are the same in both.
    if c < 0xff {
        c as u8
    } else {
        match c {
            0x0152 => 188, // U+0152 = 0xBC: OE ligature
            0x0153 => 189, // U+0153 = 0xBD: oe ligature
            0x0160 => 166, // U+0160 = 0xA6: S with caron
            0x0161 => 168, // U+0161 = 0xA8: s with caron
            0x0178 => 190, // U+0178 = 0xBE: Y with diaresis
            0x017D => 180, // U+017D = 0xB4: Z with caron
            0x017E => 184, // U+017E = 0xB8: z with caron
            0x20AC => 164, // U+20AC = 0xA4: Euro
            _ => UNAVAILABLE_CHAR,
        }
    }
}

fn line21_to_lowercase(c: Line21Char) -> Line21Char {
    if c.is_ascii_uppercase() {
        c - b'A' + b'a'
    } else {
        match c {
            0x7d => 0x7e, // uppercase N tilde
            0x90 => 0x2a, // capital letter A with acute
            0x91 => 0x5c, // capital letter E with acute
            0x92 => 0x5f, // capital letter O with acute
            0x93 => 0x60, // capital letter U with acute
            0xa2 => 0x7b, // uppercase C with cedilla
            0xa0 => 0x88, // uppercase A, grave accent
            0xa3 => 0x8a, // uppercase E, grave accent
            0xa1 => 0x8b, // uppercase A, circumflex
            0xa4 => 0x8c, // uppercase E, circumflex
            0xa7 => 0x8d, // uppercase I, circumflex
            0xaa => 0x8e, // uppercase O, circumflex
            0xad => 0x8f, // uppercase U, circumflex
            0x94 => 0x95, // capital letter U with diaeresis
            0xa5 => 0xa6, // capital letter E with diaeresis
            0xa8 => 0xa9, // uppercase I, with diaeresis
            0xab => 0xac, // uppercase U, grave accent
            0xb0 => 0xb1, // Uppercase A, tilde
            0xb2 => 0x5e, // Uppercase I, acute accent
            0xb3 => 0xb4, // Uppercase I, grave accent
            0xb5 => 0xb6, // Uppercase O, grave accent
            0xb7 => 0xb8, // Uppercase O, tilde
            0xc0 => 0xc1, // Uppercase A, umlaut
            0xc2 => 0xc3, // Uppercase O, umlaut
            0xc8 => 0xc9, // Uppercase A, ring
            0xca => 0xcb, // Uppercase O, slash
            x => x,
        }
    }
}

fn line21_to_uppercase(c: Line21Char) -> Line21Char {
    if c.is_ascii_lowercase() {
        c - b'a' + b'A'
    } else {
        match c {
            0x7e => 0x7d, // lowercase n tilde
            0x2a => 0x90, // lowercase a, acute accent
            0x5c => 0x91, // lowercase e, acute accent
            0x5e => 0xb2, // lowercase i, acute accent
            0x5f => 0x92, // lowercase o, acute accent
            0x60 => 0x93, // lowercase u, acute accent
            0x7b => 0xa2, // lowercase c with cedilla
            0x88 => 0xa0, // lowercase a, grave accent
            0x8a => 0xa3, // lowercase e, grave accent
            0x8b => 0xa1, // lowercase a, circumflex accent
            0x8c => 0xa4, // lowercase e, circumflex accent
            0x8d => 0xa7, // lowercase i, circumflex accent
            0x8e => 0xaa, // lowercase o, circumflex accent
            0x8f => 0xad, // lowercase u, circumflex accent
            0x95 => 0x94, // lowercase letter U with diaeresis
            0xa6 => 0xa5, // lowercase letter e with diaeresis
            0xa9 => 0xa8, // lowercase i, with diaeresis
            0xac => 0xab, // lowercase u, grave accent
            0xb1 => 0xb0, // Lowercase a, tilde
            0xb4 => 0xb3, // Lowercase i, grave accent
            0xb6 => 0xb5, // Lowercase o, grave accent
            0xb8 => 0xb7, // Lowercase o, tilde
            0xc1 => 0xc0, // Lowercase A, umlaut
            0xc3 => 0xc2, // Lowercase o, umlaut
            0xc9 => 0xc8, // Lowercase A, ring
            0xcb => 0xca, // Lowercase o, slash
            x => x,
        }
    }
}

fn ucs2_to_char(c: Ucs2Char) -> char {
    let x: u32 = c.into();
    char::from_u32(x).unwrap_or(UNAVAILABLE_CHAR.into())
}

fn char_to_ucs2(c: char) -> Ucs2Char {
    (c as u32).try_into().unwrap_or(UNAVAILABLE_CHAR.into())
}
