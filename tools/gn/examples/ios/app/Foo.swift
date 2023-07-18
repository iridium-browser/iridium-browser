
import Bar

class Foo {
  var name: String;
  public init(name: String) {
    self.name = name;
  }
  public func hello(name: String) -> String {
    return Greeter.greet(greeting: "Hello", name: name, from: self.name);
  }
}
