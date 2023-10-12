use elsa::FrozenMap;

/// Stores some parsed AST representation of the file
#[derive(Debug)]
pub struct FluentResource<'mgr>(&'mgr str);

impl<'mgr> FluentResource<'mgr> {
    pub fn new(s: &'mgr str) -> Self {
        // very simple parse step
        FluentResource(&s[0..1])
    }
}

/// Stores loaded files and parsed ASTs
///
/// Parsed ASTs are zero-copy and
/// contain references to the files
pub struct ResourceManager<'mgr> {
    strings: FrozenMap<String, String>,
    resources: FrozenMap<String, Box<FluentResource<'mgr>>>,
}

impl<'mgr> ResourceManager<'mgr> {
    pub fn new() -> Self {
        ResourceManager {
            strings: FrozenMap::new(),
            resources: FrozenMap::new(),
        }
    }

    pub fn get_resource(&'mgr self, path: &str) -> &'mgr FluentResource<'mgr> {
        let strings = &self.strings;

        if strings.get(path).is_some() {
            return self.resources.get(path).unwrap();
        } else {
            // pretend to load a file
            let string = format!("file for {}", path);
            let val = self.strings.insert(path.to_string(), string);
            let res = FluentResource::new(val);
            self.resources.insert(path.to_string(), Box::new(res))
        }
    }
}

fn main() {
    let manager = ResourceManager::new();
    let resource = manager.get_resource("somefile.ftl");
    println!("{:?}", resource);
}
