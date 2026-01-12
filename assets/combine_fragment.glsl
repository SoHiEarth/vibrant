#version 330 core
in vec2 TexCoord;
out vec4 FragColor;
uniform sampler2D deferred_buffer;
void main() {
    FragColor = texture(deferred_buffer, TexCoord);
}
