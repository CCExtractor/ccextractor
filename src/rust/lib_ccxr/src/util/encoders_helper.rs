// Some basic English words, so user-defined doesn't have to
// include the common stuff

pub const CAPITALIZED_BUILTIN: [&str; 29] = [
    "I",
    "I'd",
    "I've",
    "I'd",
    "I'll",
    "January",
    "February",
    "March",
    "April", // May skipped intentionally
    "June",
    "July",
    "August",
    "September",
    "October",
    "November",
    "December",
    "Monday",
    "Tuesday",
    "Wednesday",
    "Thursday",
    "Friday",
    "Saturday",
    "Sunday",
    "Halloween",
    "United States",
    "Spain",
    "France",
    "Italy",
    "England",
];

pub const PROFANE_BUILTIN: [&str; 25] = [
    "arse",
    "ass",
    "asshole",
    "bastard",
    "bitch",
    "bollocks",
    "child-fucker",
    "crap",
    "cunt",
    "damn",
    "frigger",
    "fuck",
    "goddamn",
    "godsdamn",
    "hell",
    "holy",
    "horseshit",
    "motherfucker",
    "nigga",
    "nigger",
    "prick",
    "shit",
    "shitass",
    "slut",
    "twat",
];

pub fn capitalize_word(index: usize, word: &mut String, capitalization_list: &[String]) {
    word.replace_range(
        index..capitalization_list[index].len(),
        capitalization_list[index].as_str(),
    );
}

pub fn censor_word(index: usize, word: &mut String, profane: &[String]) {
    word.replace_range(
        index..profane[index].len(),
        format!("{}", (0x98 as char)).as_str(),
    ); // 0x98 is the asterisk in EIA-608
}

pub fn telx_correct_case(sub_line: &mut String, capitalization_list: &[String]) {
    let delim: [char; 35] = [
        ' ',
        '\n',
        '\r',
        0x89 as char,
        0x99 as char,
        '!',
        '"',
        '#',
        '%',
        '&',
        '\'',
        '(',
        ')',
        ';',
        '<',
        '=',
        '>',
        '?',
        '[',
        '\\',
        ']',
        '*',
        '+',
        ',',
        '-',
        '.',
        '/',
        ':',
        '^',
        '_',
        '{',
        '|',
        '}',
        '~',
        '\0',
    ];

    let line = sub_line.clone();

    let mut start = 0;
    let splitted: Vec<(usize, &str)> = line
        .split(|c| delim.contains(&c))
        .map(|part| {
            let end = start + part.len();
            let result = (start, part);
            start = end + 1;
            result
        })
        .collect();

    for (index, c) in splitted {
        // check if c is in capitalization_list
        if capitalization_list.contains(&(c as &str).to_string()) {
            // get the correct_c from capitalization_list
            let correct_c: &String = capitalization_list
                .iter()
                .find(|&x| x == &c.to_string())
                .unwrap();

            // get the length of correct_c
            let len = correct_c.len();
            // replace c with correct_c in sub_line
            sub_line.replace_range(index..index + len, correct_c);
        }
    }
}

pub fn add_builtin_capitalization(list: &mut Vec<String>) {
    for word in CAPITALIZED_BUILTIN.iter() {
        list.push(word.to_string());
    }
}

pub fn add_builtin_profane(list: &mut Vec<String>) {
    for word in PROFANE_BUILTIN.iter() {
        list.push(word.to_string());
    }
}
