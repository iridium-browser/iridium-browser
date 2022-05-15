// v-0035: The access decoration is required for 'particles'.

struct Particles {
  particles : array<f32, 4>
};

@group(0) @binding(1) var<storage> particles : Particles;

@stage(vertex)
fn main() -> @builtin(position) vec4<f32> {
  return vec4<f32>();
}
