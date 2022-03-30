
import Foundation;

@objc
public class FooWrapper : NSObject {
  @objc
  public func hello(name: String) -> String {
    return Foo(name: "Foo").hello(name: name);
  }
}
