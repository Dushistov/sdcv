use ini::Ini;
use std::fs::File;
use std::io::Read;
use std::path::Path;
use std::path::PathBuf;
use std::error::Error;
use index::DictionaryIndex;
use index::MemoryDictionaryIndex;
use index::DiskDictionaryIndex;
use data::DictionaryData;
use data::SimpleDictionaryData;

pub struct Dictionary {
    ifo_file_name: PathBuf,
    wordcount : usize,
    bookname: String,
    sametypesequence: String,
    index: Box<DictionaryIndex>,
    data: Box<DictionaryData>
}

impl Dictionary {
    pub fn new(ifo_file_name: &Path) -> Result<Dictionary, String> {
        let basename = try!(try!(ifo_file_name.file_stem().ok_or("bad ifo file name")).to_str().ok_or("can not convert file name to unicode"));
        let dict_dir = try!(ifo_file_name.parent().ok_or("bad ifo directory name"));

        let mut file = try!(File::open(ifo_file_name).map_err(|err| err.to_string()));
        let mut ifo_content = String::new();
        match file.read_to_string(&mut ifo_content) {
            Ok(read_bytes) => { if read_bytes < 3 { return Result::Err("ifo file too small".to_string()) } },
            Result::Err(err) => return Result::Err(err.to_string())
        };

        let content =
            if ifo_content.starts_with("\u{feff}") {
                &ifo_content[1..]
            } else {
                &ifo_content[0..]
            };

        static DICT_MAGIC_DATA: &'static str = "StarDict's dict ifo file";
        if !content.starts_with(DICT_MAGIC_DATA) {
            return Result::Err("ifo magic not match".to_string());
        }

        let ifo_cfg = try!(Ini::load_from_str(content).map_err(|err| err.msg));
        let section = try!(ifo_cfg.section(None::<String>).ok_or("ifo: parse none section error".to_string()));

        let wordcount = try!(section.get("wordcount").ok_or("no wordcount".to_string()));
        let wordcount = try!(wordcount.parse::<usize>().map_err(|err| err.description().to_string()));
        let idxfilesize = try!(section.get("idxfilesize").ok_or("no idxfilesize".to_string()));
        let bookname = try!(section.get("bookname").ok_or("no bookname".to_string()));
        let sametypesequence = try!(section.get("sametypesequence").ok_or("no sametypesequence".to_string()));

        let idx_path = dict_dir.join(basename.to_string() + ".idx");
        let idx_path_gz = dict_dir.join(basename.to_string() + ".idx.gz");
        let mut index: Box<DictionaryIndex>;
        if idx_path.exists() && idx_path.is_file() {
            let disk_dict = try!(DiskDictionaryIndex::new(wordcount, &idx_path));
            index = Box::new(disk_dict);
        } else if idx_path_gz.exists() && idx_path_gz.is_file() {
            let mem_dict = try!(MemoryDictionaryIndex::new_from_gzip(wordcount, &idx_path_gz));
            index = Box::new(mem_dict);
        } else {
            return Result::Err(format!("no index file for {}", ifo_file_name.to_str().unwrap()));
        }

        let data_path = dict_dir.join(basename.to_string() + ".dict");
        let data_path_dz = dict_dir.join(basename.to_string() + ".dict.dz");
        let mut data: Box<DictionaryData>;
        if data_path.exists() && data_path.is_file() {
            let simple_data = try!(SimpleDictionaryData::new(data_path.as_path()));
            data = Box::new(simple_data);
        } else if data_path_dz.exists() && data_path_dz.is_file() {
            return Result::Err("reading dict.dz not implemented".to_string());
        } else {
            return Result::Err(format!("no data file for {}", ifo_file_name.to_str().unwrap()));
        }

        Result::Ok(Dictionary{ifo_file_name : ifo_file_name.to_path_buf(), wordcount : wordcount,
                              bookname : bookname.clone(), sametypesequence: sametypesequence.clone(),
                              index : index, data: data})
    }

    pub fn find(&mut self, key: &str) -> Option<String> {
        match self.index.find(key) {
            Err(idx) => None,
            Ok(idx) => {
                let key_pos = self.index.get_key(idx);
                Some(self.data.get_data(key_pos.1, key_pos.2).unwrap())
            }
        }
    }
}

#[cfg(test)]
mod test {
    use	super::*;
    use std::path::Path;

    #[test]
    fn open_dict() {
        let dic = Dictionary::new(Path::new("tests/stardict-test_dict-2.4.2/test_dict.ifo")).unwrap();
        assert_eq!(dic.wordcount, 1_usize);
        assert_eq!(dic.bookname, "test_dict");
        assert_eq!(dic.sametypesequence, "x");
    }

    #[test]
    fn find_in_dict() {
        let mut dic = Dictionary::new(Path::new("tests/stardict-test_dict-2.4.2/test_dict.ifo")).unwrap();
        assert_eq!(dic.find("test").unwrap(), "<k>test</k>\ntest passed");


    }
}
