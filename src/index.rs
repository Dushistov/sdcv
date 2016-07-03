use std::str::from_utf8;
use std::cmp::Ordering;
use std::io::Cursor;
use byteorder::{NetworkEndian, ReadBytesExt};

pub trait DictionaryIndex {
    fn get_key(&self, idx: usize) -> (&str, u64, usize);
    fn count(&self) -> usize;
    fn find(&self, key: &str) -> Result<usize, usize>;
}

pub struct MemoryDictionaryIndex {
    idx_content: Vec<u8>,
    wordlist: Vec<usize>,
}

fn g_ascii_strcasecmp(s1: &str, s2: &str) -> isize {
    fn is_upper(c: u8) -> bool { c >= b'A' && c <= b'Z' }
    fn to_lower(c: u8) -> u8 { if is_upper(c) { c - b'A' + b'a' } else { c } }

    let mut it1 = s1.bytes();
    let mut it2 = s2.bytes();
    loop {
        match (it1.next(), it2.next()) {
            (None, None) => return 0,
            (Some(b1), None) => return b1 as isize,
            (None, Some(b2)) => return -(b2 as isize),
            (Some(b1), Some(b2)) => {
                let c1 = to_lower(b1) as isize;
                let c2 = to_lower(b2) as isize;
                if c1 != c2 {
                    return c1 - c2;
                }
            },
        }
    }
    0_isize
}

fn stardict_strcmp(s1: &str, s2: &str) -> isize {
    /*
    #define ISUPPER(c)              ((c) >= 'A' && (c) <= 'Z')
    #define TOLOWER(c)              (ISUPPER (c) ? (c) - 'A' + 'a' : (c))
    gint
    g_ascii_strcasecmp (const gchar *s1,
    const gchar *s2)
    {
    gint c1, c2;

    g_return_val_if_fail (s1 != NULL, 0);
    g_return_val_if_fail (s2 != NULL, 0);

    while (*s1 && *s2)
    {
    c1 = (gint)(guchar) TOLOWER (*s1);
    c2 = (gint)(guchar) TOLOWER (*s2);
    if (c1 != c2)
    return (c1 - c2);
    s1++; s2++;
}

    return (((gint)(guchar) *s1) - ((gint)(guchar) *s2));
}

    const gint a = g_ascii_strcasecmp(s1, s2);
    if (a == 0)
    return strcmp(s1, s2);
    else
    return a;
     */
    let res = g_ascii_strcasecmp(s1, s2);
    if res == 0 {
        assert_eq!(s1.len(), s2.len());
        for it in s1.bytes().zip(s2.bytes()) {
            let (b1, b2) = it;
            if b1 != b2 {
                return (b1 as isize) - (b2 as isize);
            }
        }
        0_isize
    } else {
        res
    }
}

fn stardict_str_order(s1: &str, s2: &str) -> Ordering {
    let cmp_res = stardict_strcmp(s1, s2);
    if cmp_res < 0 {
        Ordering::Less
    } else if cmp_res > 0 {
        Ordering::Greater
    } else {
        Ordering::Equal
    }
}

fn get_key_by_offset(content: &Vec<u8>, key_pos: usize) -> (&str, u64, usize) {
    let key = &content[key_pos..];
    let end_of_key = key.iter().position(|&x| x == b'\0').unwrap() + key_pos;
    let mut reader = Cursor::new(&content[end_of_key + 1..]);    
    let offset = reader.read_u32::<NetworkEndian>().unwrap() as u64;
    let length = reader.read_u32::<NetworkEndian>().unwrap() as usize;
    (from_utf8(&content[key_pos..end_of_key]).unwrap(), offset, length)
}

impl  DictionaryIndex for  MemoryDictionaryIndex  {
    fn get_key(&self, idx: usize) -> (&str, u64, usize) {
        let key_pos = self.wordlist[idx];
        get_key_by_offset(&self.idx_content, key_pos)
        /*
        let key = &self.idx_content[key_pos..];
        let end_of_key = key.iter().position(|&x| x == b'\0').unwrap();
        let mut reader = Cursor::new(&self.idx_content[end_of_key + 1..]);
        let offset = reader.read_u32::<NetworkEndian>().unwrap() as u64;
        let length = reader.read_u32::<NetworkEndian>().unwrap() as usize;
        (from_utf8(&self.idx_content[key_pos..(key_pos + end_of_key)]).unwrap(), offset, length)*/
    }
    fn count(&self) -> usize {
        self.wordlist.len()
    }
    fn find(&self, key: &str) -> Result<usize, usize> {
        self.wordlist.binary_search_by(
            |probe| stardict_str_order(get_key_by_offset(&self.idx_content, *probe).0, key)
        )
    }
}

impl  MemoryDictionaryIndex  {
    pub fn new(expected_wordcount : usize, idx_content : Vec<u8>) -> Result<MemoryDictionaryIndex, String> {
        const PADDING_LENGTH: usize = 8;
        let mut wordlist = vec![];
        wordlist.reserve(expected_wordcount);
        {
            let mut slice = &idx_content[0..];
            let mut pos : usize = 0;
            while let Some(idx) = slice.iter().position(|&x| x == b'\0') {
                let (head, tail) = slice.split_at(idx);
                try!(from_utf8(head).map_err(|err| "invalid utf-8 in key".to_string()));
                wordlist.push(pos);
                pos += head.len() + PADDING_LENGTH + 1;
                // +1 to skip over the NUL
                if tail.len() > PADDING_LENGTH + 1 {
                    slice = &tail[PADDING_LENGTH + 1..];
                } else {
                    break;
                }
            }
        }
        if wordlist.len() != expected_wordcount {
            Result::Err(format!("Expect words in index {}, got {} words", expected_wordcount, wordlist.len()))
        } else {
            Result::Ok(MemoryDictionaryIndex {idx_content: idx_content, wordlist : wordlist})
        }
    }
}

#[cfg(test)]
mod test {
    use	super::*;
    use super::stardict_str_order;
    use super::stardict_strcmp;
    use std::path::Path;
    use std::fs::File;
    use std::io::Read;
    use std::io::BufReader;
    use std::io::BufRead;
    use std::collections::HashSet;

    #[test]
    fn index_memory_open() {
        let idx_file_name = Path::new("tests/stardict-test_dict-2.4.2/test_dict.idx");
        let mut file = File::open(idx_file_name).unwrap();
        let mut idx_content = Vec::<u8>::new();
        file.read_to_end(&mut idx_content).unwrap();
        let mut index = MemoryDictionaryIndex::new(1, idx_content).unwrap();
        assert_eq!(1_usize, index.count());
        assert_eq!("test", index.get_key(0).0);

        let idx_file_name = Path::new("tests/words_dic/stardict-words-2.4.2/words.idx");
        let mut file = File::open(idx_file_name).unwrap();
        let mut idx_content = Vec::<u8>::new();
        file.read_to_end(&mut idx_content).unwrap();
        let index_size = 1671704_usize;
        let mut index = MemoryDictionaryIndex::new(index_size, idx_content).unwrap();
        assert_eq!(index_size, index.count());
        {
            let mut counter: usize = 0;
            let f = File::open("tests/words_dic/words.dummy").unwrap();
            let mut file = BufReader::new(&f);
            let mut dic_words = HashSet::new();
            for line in file.lines() {
                let l = line.unwrap();
                dic_words.insert(l);
            }
            for i in 0..index.count() {
                let (key, _, _) = index.get_key(i);
                if !dic_words.contains(key) {
                    panic!("no '{}'(len {}) idx {} in dic_words", key, key.len(), i);
                }
                match index.find(key) {
                    Err(idx) => panic!("we search '{}', and not found err({})", key, idx),
                    Ok(idx) => assert_eq!(idx, i),
                }
            }
        }
    }

    #[test]
    fn index_stardict_strcmp_small() {
        let arr_s = ["a", "b", "c", "d", "z"];
        let seek = "c";
        assert_eq!(arr_s.binary_search_by(|probe| stardict_str_order(probe, seek)), Ok(2));
        let seek = "e";
        assert_eq!(arr_s.binary_search_by(|probe| stardict_str_order(probe, seek)), Err(4));
    }

    #[test]
    fn index_stardict_strcmp_big() {
        let mut exp_res_list = vec![];
        {            
            let f = File::open("tests/stardict_strcmp_test_data_cmp_exp.txt").unwrap();
            let mut file = BufReader::new(&f);
            for line in file.lines() {
                let line = line.unwrap();
                let exp_res = line.parse::<isize>().unwrap();
                exp_res_list.push(exp_res);
            }
        }
        
        let f = File::open("tests/stardict_strcmp_test_data.txt").unwrap();
        let mut file = BufReader::new(&f);
        let mut prev_line : String = "".to_string();
        let mut counter : u8 = 0;
        let mut exp_it = exp_res_list.iter();
        for line in file.lines() {
            if counter == 1 {
                let cur_line = line.unwrap();
                let res = stardict_strcmp(&prev_line, &cur_line);
                let exp_res = exp_it.next().unwrap();
                if res != *exp_res {
                    panic!("we expect {}, got {} for '{}' vs '{}'", exp_res, res, prev_line, cur_line);
                }
            } else {
                prev_line = line.unwrap();
            }
            counter = (counter + 1) % 2;
        }
    }
}
