//! MKV language filtering support.
//!
//! Matroska files support two language code formats:
//! - ISO 639-2 (3-letter bibliographic codes): "eng", "fre", "chi"
//! - BCP 47 / IETF language tags: "en-US", "fr-CA", "zh-Hans"
//!
//! This module provides [`MkvLangFilter`] for parsing and matching language codes.

use std::fmt;
use std::str::FromStr;

/// A filter for matching MKV track languages.
///
/// Supports comma-separated lists of language codes in either:
/// - ISO 639-2 format (3-letter codes like "eng", "fre")
/// - BCP 47 format (tags like "en-US", "fr-CA", "zh-Hans")
///
/// # Examples
///
/// ```
/// use lib_ccxr::common::MkvLangFilter;
///
/// // Single language
/// let filter: MkvLangFilter = "eng".parse().unwrap();
/// assert!(filter.matches("eng", None));
///
/// // Multiple languages
/// let filter: MkvLangFilter = "eng,fre,chi".parse().unwrap();
/// assert!(filter.matches("fre", None));
///
/// // BCP 47 matching
/// let filter: MkvLangFilter = "en-US,fr-CA".parse().unwrap();
/// assert!(filter.matches("eng", Some("en-US")));
/// ```
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct MkvLangFilter {
    /// The original input string (used for C FFI)
    raw: String,
    /// Parsed and validated language codes
    codes: Vec<LanguageCode>,
}

/// A single language code, either ISO 639-2 or BCP 47.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct LanguageCode {
    /// The normalized (lowercase) code
    code: String,
}

/// Error type for invalid language codes.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct InvalidLanguageCode {
    /// The invalid code
    pub code: String,
    /// Description of what's wrong
    pub reason: &'static str,
}

impl fmt::Display for InvalidLanguageCode {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "invalid language code '{}': {}", self.code, self.reason)
    }
}

impl std::error::Error for InvalidLanguageCode {}

impl LanguageCode {
    /// Validates and creates a new language code.
    ///
    /// Accepts:
    /// - ISO 639-2 codes: 3 ASCII letters (e.g., "eng", "fre")
    /// - BCP 47 tags: primary language with optional subtags separated by hyphens
    ///   (e.g., "en-US", "fr-CA", "zh-Hans-CN")
    ///
    /// # BCP 47 Structure
    /// - Primary language: 2-3 letters
    /// - Script (optional): 4 letters (e.g., "Hans", "Latn")
    /// - Region (optional): 2 letters or 3 digits (e.g., "US", "419")
    /// - Variant (optional): 5-8 alphanumeric characters
    pub fn new(code: &str) -> Result<Self, InvalidLanguageCode> {
        let code = code.trim();

        if code.is_empty() {
            return Err(InvalidLanguageCode {
                code: code.to_string(),
                reason: "empty language code",
            });
        }

        // Check for valid characters (alphanumeric and hyphens only)
        if !code.chars().all(|c| c.is_ascii_alphanumeric() || c == '-') {
            return Err(InvalidLanguageCode {
                code: code.to_string(),
                reason: "must contain only ASCII letters, digits, and hyphens",
            });
        }

        // Cannot start or end with hyphen
        if code.starts_with('-') || code.ends_with('-') {
            return Err(InvalidLanguageCode {
                code: code.to_string(),
                reason: "cannot start or end with hyphen",
            });
        }

        // Cannot have consecutive hyphens
        if code.contains("--") {
            return Err(InvalidLanguageCode {
                code: code.to_string(),
                reason: "cannot have consecutive hyphens",
            });
        }

        // Validate subtag structure
        let subtags: Vec<&str> = code.split('-').collect();

        // First subtag must be the primary language (2-3 letters)
        let primary = subtags[0];
        if primary.len() < 2 || primary.len() > 3 {
            return Err(InvalidLanguageCode {
                code: code.to_string(),
                reason: "primary language subtag must be 2-3 letters",
            });
        }
        if !primary.chars().all(|c| c.is_ascii_alphabetic()) {
            return Err(InvalidLanguageCode {
                code: code.to_string(),
                reason: "primary language subtag must contain only letters",
            });
        }

        // Validate subsequent subtags
        for subtag in subtags.iter().skip(1) {
            if subtag.is_empty() {
                return Err(InvalidLanguageCode {
                    code: code.to_string(),
                    reason: "empty subtag",
                });
            }

            let len = subtag.len();
            let all_alpha = subtag.chars().all(|c| c.is_ascii_alphabetic());
            let all_digit = subtag.chars().all(|c| c.is_ascii_digit());
            let all_alnum = subtag.chars().all(|c| c.is_ascii_alphanumeric());

            // Valid subtag types:
            // - Script: 4 letters (e.g., "Hans")
            // - Region: 2 letters or 3 digits (e.g., "US", "419")
            // - Variant: 5-8 alphanumeric, or 4 starting with digit
            // - Extension: single letter followed by more subtags
            // - Private use: 'x' followed by 1-8 char subtags
            let valid = match len {
                1 => subtag.chars().all(|c| c.is_ascii_alphanumeric()), // Extension singleton
                2 => all_alpha,                                         // Region (2 letters)
                3 => all_alpha || all_digit,                            // 3 letters or 3 digits
                4 => all_alpha || (subtag.chars().next().unwrap().is_ascii_digit() && all_alnum), // Script or variant starting with digit
                5..=8 => all_alnum,                                     // Variant
                _ => false,
            };

            if !valid {
                return Err(InvalidLanguageCode {
                    code: code.to_string(),
                    reason: "invalid subtag format",
                });
            }
        }

        Ok(Self {
            code: code.to_lowercase(),
        })
    }

    /// Returns the normalized (lowercase) code.
    pub fn as_str(&self) -> &str {
        &self.code
    }

    /// Checks if this code matches a track's language.
    ///
    /// Matching rules:
    /// 1. Exact match (case-insensitive)
    /// 2. Prefix match for BCP 47 (e.g., "en" matches "en-US")
    pub fn matches(&self, iso639: &str, bcp47: Option<&str>) -> bool {
        let iso639_lower = iso639.to_lowercase();
        let bcp47_lower = bcp47.map(|s| s.to_lowercase());

        // Exact match on ISO 639-2
        if self.code == iso639_lower {
            return true;
        }

        // Exact match on BCP 47
        if let Some(ref bcp) = bcp47_lower {
            if self.code == *bcp {
                return true;
            }
        }

        // Prefix match: "en" matches "en-US", "eng" matches track with bcp47 "en-US"
        // The filter code could be a prefix of the track's BCP 47 tag
        if let Some(ref bcp) = bcp47_lower {
            if bcp.starts_with(&self.code) && bcp[self.code.len()..].starts_with('-') {
                return true;
            }
            // Or the track's BCP 47 could be a prefix of the filter
            if self.code.starts_with(bcp.as_str())
                && self.code[bcp.len()..].starts_with('-')
            {
                return true;
            }
        }

        false
    }
}

impl FromStr for LanguageCode {
    type Err = InvalidLanguageCode;

    fn from_str(s: &str) -> Result<Self, Self::Err> {
        Self::new(s)
    }
}

impl fmt::Display for LanguageCode {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}", self.code)
    }
}

impl MkvLangFilter {
    /// Creates a new filter from a comma-separated list of language codes.
    pub fn new(input: &str) -> Result<Self, InvalidLanguageCode> {
        let input = input.trim();
        if input.is_empty() {
            return Err(InvalidLanguageCode {
                code: String::new(),
                reason: "empty language filter",
            });
        }

        let codes: Result<Vec<LanguageCode>, _> =
            input.split(',').map(LanguageCode::new).collect();

        Ok(Self {
            raw: input.to_string(),
            codes: codes?,
        })
    }

    /// Returns the raw input string (for C FFI compatibility).
    pub fn as_raw_str(&self) -> &str {
        &self.raw
    }

    /// Returns the parsed language codes.
    pub fn codes(&self) -> &[LanguageCode] {
        &self.codes
    }

    /// Checks if any of the filter's codes match a track's language.
    ///
    /// # Arguments
    /// - `iso639`: The track's ISO 639-2 language code (e.g., "eng")
    /// - `bcp47`: The track's BCP 47 language tag, if available (e.g., "en-US")
    pub fn matches(&self, iso639: &str, bcp47: Option<&str>) -> bool {
        self.codes.iter().any(|code| code.matches(iso639, bcp47))
    }
}

impl FromStr for MkvLangFilter {
    type Err = InvalidLanguageCode;

    fn from_str(s: &str) -> Result<Self, Self::Err> {
        Self::new(s)
    }
}

impl fmt::Display for MkvLangFilter {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}", self.raw)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_iso639_codes() {
        // Valid 3-letter codes
        assert!(LanguageCode::new("eng").is_ok());
        assert!(LanguageCode::new("fre").is_ok());
        assert!(LanguageCode::new("chi").is_ok());
        assert!(LanguageCode::new("ENG").is_ok()); // Case insensitive

        // 2-letter codes (ISO 639-1 style, valid in BCP 47)
        assert!(LanguageCode::new("en").is_ok());
        assert!(LanguageCode::new("fr").is_ok());
    }

    #[test]
    fn test_bcp47_codes() {
        // Language + region
        assert!(LanguageCode::new("en-US").is_ok());
        assert!(LanguageCode::new("fr-CA").is_ok());
        assert!(LanguageCode::new("pt-BR").is_ok());

        // Language + script
        assert!(LanguageCode::new("zh-Hans").is_ok());
        assert!(LanguageCode::new("zh-Hant").is_ok());
        assert!(LanguageCode::new("sr-Latn").is_ok());

        // Language + script + region
        assert!(LanguageCode::new("zh-Hans-CN").is_ok());
        assert!(LanguageCode::new("zh-Hant-TW").is_ok());

        // UN M.49 numeric region codes
        assert!(LanguageCode::new("es-419").is_ok()); // Latin America
    }

    #[test]
    fn test_invalid_codes() {
        // Too short
        assert!(LanguageCode::new("a").is_err());

        // Invalid characters
        assert!(LanguageCode::new("en_US").is_err()); // Underscore not allowed
        assert!(LanguageCode::new("en US").is_err()); // Space not allowed
        assert!(LanguageCode::new("ça").is_err());    // Non-ASCII

        // Invalid structure
        assert!(LanguageCode::new("-en").is_err());   // Leading hyphen
        assert!(LanguageCode::new("en-").is_err());   // Trailing hyphen
        assert!(LanguageCode::new("en--US").is_err()); // Double hyphen

        // Empty
        assert!(LanguageCode::new("").is_err());
    }

    #[test]
    fn test_filter_multiple_codes() {
        let filter = MkvLangFilter::new("eng,fre,chi").unwrap();
        assert_eq!(filter.codes().len(), 3);
        assert!(filter.matches("eng", None));
        assert!(filter.matches("fre", None));
        assert!(filter.matches("chi", None));
        assert!(!filter.matches("spa", None));
    }

    #[test]
    fn test_filter_bcp47_matching() {
        let filter = MkvLangFilter::new("en-US,fr-CA").unwrap();

        // Exact BCP 47 match
        assert!(filter.matches("eng", Some("en-US")));
        assert!(filter.matches("fre", Some("fr-CA")));

        // No match
        assert!(!filter.matches("eng", Some("en-GB")));
        assert!(!filter.matches("eng", None));
    }

    #[test]
    fn test_filter_mixed_formats() {
        let filter = MkvLangFilter::new("eng,fr-CA,zh-Hans").unwrap();

        assert!(filter.matches("eng", None));
        assert!(filter.matches("fre", Some("fr-CA")));
        assert!(filter.matches("chi", Some("zh-Hans")));
    }

    #[test]
    fn test_case_insensitivity() {
        let filter = MkvLangFilter::new("ENG,FR-CA").unwrap();
        assert!(filter.matches("eng", None));
        assert!(filter.matches("ENG", None));
        assert!(filter.matches("fre", Some("fr-ca")));
        assert!(filter.matches("FRE", Some("FR-CA")));
    }

    #[test]
    fn test_raw_string_preserved() {
        let filter = MkvLangFilter::new("eng,fre").unwrap();
        assert_eq!(filter.as_raw_str(), "eng,fre");
    }
}
