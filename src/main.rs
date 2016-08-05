extern crate getopts;
extern crate gettext;
extern crate ini;
extern crate byteorder;
extern crate libc;
extern crate flate2;

mod core;
mod index;
mod data;
mod mem_mapped_file;
use core::Dictionary;

use std::env;
use std::result::Result;
use getopts::Options;
use gettext::Catalog;
use std::fs::File;
use std::path::Path;

struct Library {
    dicts : Vec<Dictionary>
}

impl Library {
    fn new(dict_dirs: &[&Path], dict_order_names: &[String], dict_disable_names: &[String]) -> Result<Library, String> {
        let dicts = vec![Dictionary::new(Path::new("/home/evgeniy/.stardict/dic/stardict-Mueller7accentGPL-2.4.2/Mueller7accentGPL.ifo")).unwrap()];
        Ok(Library{dicts: dicts})
    }

    pub fn simple_lookup(&mut self, phrase: &str) /*-> Option<String>*/ {
        for dict in self.dicts.iter_mut() {
            if let Some(translation) = dict.find(phrase) {
                println!("{}", translation);
            }
        }
    }
}

fn print_usage(program: &str, opts: Options) {
    let brief = format!("Usage: {} [options] [list of words]", program);
    print!("{}", opts.usage(&brief));
}

fn main() {
    let translation_f = File::open("/usr/share/locale/ru/LC_MESSAGES/sdcv.mo").expect("could not open the catalog");
    let catalog = Catalog::parse(translation_f).expect("could not parse the catalog");

    let args: Vec<String> = env::args().collect();
    let program = args[0].clone();
    let mut opts = Options::new();
    opts.optflag("v", "version", catalog.gettext("display version information and exit"));
    opts.optflag("h", "help", catalog.gettext("Show help options"));
    let matches = match opts.parse(&args[1..]) {
        Ok(m) => { m }
        Err(f) => { panic!(f.to_string()) }
    };
    if matches.opt_present("h") {
        print_usage(&program, opts);
        return;
    }
    let dict_dirs = vec![Path::new("/tmp")];
    let dict_order_names: Vec<String> = vec![];
    let dict_disable_names: Vec<String> = vec![];
    let mut library = Library::new(&dict_dirs, &dict_order_names, &dict_disable_names).unwrap();
    library.simple_lookup("man");
}
