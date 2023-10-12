use libffi::middle::Type;

fn main() {
    Type::structure(vec![Type::u16(), Type::u16()].into_iter());
}
