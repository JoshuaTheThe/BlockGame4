#version 330 core
in vec2 vUV;
in vec3 vNormal;
out vec4 fragColor;
void main() {
  float linearDepth = (2.0 * 0.1 * 100.0) / (100.0 + 0.1 - gl_FragCoord.z * (100.0 - 0.1));
  vec3 n = normalize(vNormal);
  vec3 color = n * 0.5 + 0.5;
  float depthFade = 1.0 - smoothstep(1.0, 100.0, linearDepth);
  fragColor = vec4(color * depthFade, 1.0);
}
