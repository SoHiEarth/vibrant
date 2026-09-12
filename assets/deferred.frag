#version 330 core

struct Light {
  int type; // 0: Global, 1: Point
  vec3 position;
  float intensity;
  vec3 color;
  float falloff;
  float volumetric_intensity;
};

const int MAX_LIGHTS = 64;
in vec2 TexCoord;
out vec4 FragColor;
uniform sampler2D color_buffer;
uniform sampler2D normal_buffer;
uniform int light_count;
uniform Light lights[MAX_LIGHTS];

void main() {
  vec4 sample = texture(color_buffer, TexCoord);
  if (sample.a == 0.0) {
    discard;
  }
  vec3 albedo = sample.rgb;
  vec3 normal = texture(normal_buffer, TexCoord).rgb * 2.0 - 1.0;
  normal = normalize(normal);
  vec3 total_lighting = vec3(0.0);
  for (int i = 0; i < light_count && i < MAX_LIGHTS; i++) {
    if (lights[i].type == 0) {
      total_lighting += albedo * lights[i].color * lights[i].intensity;
    }
    else if (lights[i].type == 1) {
      vec2 light_offset = lights[i].position.xy - TexCoord;
      vec3 light_dir = normalize(vec3(light_offset, lights[i].position.z));
      float distance = length(light_offset);
      float attenuation = lights[i].intensity / (1.0 + lights[i].falloff * distance * distance);
      attenuation = max(attenuation, 0.0);
      float diff = max(dot(normal, light_dir), 0.0);
      total_lighting += albedo * lights[i].color * diff * attenuation;
      float volumetric = lights[i].volumetric_intensity / (1.0 + distance * distance);
      total_lighting += lights[i].color * volumetric * attenuation;
    }
  }
  
  FragColor = vec4(total_lighting, 1.0);
}
