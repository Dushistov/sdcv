use std::fs::File;
use std::io::Read;
use std::io::Seek;
use std::path::Path;
use std::io::SeekFrom;
use std::str::from_utf8;

pub trait DictionaryData {
    fn get_data(& mut self, offset: u64, length: usize) -> Result<String, String>;
}

pub struct SimpleDictionaryData {
    data_file: File,
}

impl DictionaryData for SimpleDictionaryData {
    fn get_data(& mut self, offset: u64, length: usize) -> Result<String, String> {
        try!(self.data_file.seek(SeekFrom::Start(offset)).map_err(|err| err.to_string()));
        let mut buffer = Vec::<u8>::with_capacity(length);
        buffer.resize(length, 0_u8);
        try!(self.data_file.read_exact(& mut buffer).map_err(|err| err.to_string()));
        let utf8_s = try!(String::from_utf8(buffer).map_err(|err| "invalid utf-8 data in data".to_string()));
        Result::Ok(utf8_s)
    }
}

impl SimpleDictionaryData {
    pub fn new(data_file_name: &Path) ->Result<SimpleDictionaryData, String> {
        let file = try!(File::open(data_file_name).map_err(|err| err.to_string()));
        Result::Ok(SimpleDictionaryData{data_file : file})
    }
}

