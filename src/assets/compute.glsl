#version 430 core

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

layout(std430, binding = 0) writeonly buffer Out {
  uint values[];
};

void main() {
  uint i = gl_GlobalInvocationID.x;
  if (i >= 256u) return;       // bounds check: 4 groups * 64 = 256, exact here
  values[i] = i;
}
